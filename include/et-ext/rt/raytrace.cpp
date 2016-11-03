/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <thread>
#include <mutex>
#include <et-ext/rt/raytrace.h>
#include <et-ext/rt/raytraceobjects.h>
#include <et-ext/rt/bsdf.h>
#include <et/app/application.h>
#include <et/camera/camera.h>

namespace et
{

class RaytracePrivate
{
public:
	RaytracePrivate(Raytrace* owner);
	~RaytracePrivate();

	void backwardPathTraceThreadFunction(uint32_t index);
	void forwardPathTraceThreadFunction(uint32_t index);
	void visualizeDistributionThreadFunction(uint32_t index);

	void emitWorkerThreads();
	void stopWorkerThreads();

	void buildMaterialAndTriangles(s3d::Scene::Pointer);

	uint32_t materialIndexWithName(const std::string&);

	void buildRegions(const vec2i& size);
	void estimateRegionsOrder();

	vec4 raytracePixel(const vec2i&, uint32_t samples, uint32_t& bounces);
	rt::float4 gatherAtPixel(const vec2&, uint32_t&);

	rt::Region getNextRegion();

	void renderSpacePartitioning();
	void renderKDTreeRecursive(uint32_t nodeIndex, uint32_t index);
	void renderBoundingBox(const rt::BoundingBox&, const vec4& color);
	void renderLine(const vec2& from, const vec2& to, const vec4& color);
	void renderPixel(const vec2&, const vec4& color);
	vec2 projectPoint(const rt::float4&);

	void fillRegionWithColor(const rt::Region&, const vec4& color);
	void renderTriangle(const rt::Triangle&);

	void flushToForwardTraceBuffer(const Vector<rt::float4>&);

public:
	Raytrace* owner = nullptr;
	Raytrace::Options options;
	rt::KDTree kdTree;
	rt::EnvironmentSampler::Pointer sampler;
	rt::Integrator::Pointer integrator;
	rt::Material::Collection materials;
	rt::TriangleList lightTriangles;
	Map<uint32_t, rt::index> lightTriangleToIndex;
	Camera camera;
	vec2i viewportSize;
	vec2i regionSize;

	Vector<std::thread> workerThreads;
	std::atomic<uint32_t> threadCounter;
	std::atomic<uint32_t> sampledRegions;

	Vector<rt::Region> regions;
	std::mutex regionsLock;

	std::atomic<bool> running;
	std::atomic<uint64_t> startTime;
	std::atomic<uint64_t> elapsedTime;

	Vector<rt::float4> forwardTraceBuffer;
	std::mutex forwardTraceBufferMutex;
	uint32_t flushCounter = 0;

	ray3d centerRay;
	float focalDistance = 1.0f;
};

Raytrace::Raytrace()
{
	ET_PIMPL_INIT(Raytrace, this);
	setOutputMethod([](const vec2i&, const vec4&){ });
}

Raytrace::~Raytrace()
{
	ET_PIMPL_FINALIZE(Raytrace)
}

void Raytrace::perform(s3d::Scene::Pointer scene, const Camera& cam, const vec2i& dimension)
{
	_private->camera.getValuesFromCamera(cam);

	_private->viewportSize = dimension;
	_private->buildMaterialAndTriangles(scene);

	ET_ASSERT(_private->viewportSize.x > 0);
	ET_ASSERT(_private->viewportSize.y > 0);

	_private->buildRegions(vec2i(static_cast<int>(_private->options.renderRegionSize)));

	if (_private->options.method == Raytrace::Method::PathTracing)
	{
		Invocation([this]() {
			_private->estimateRegionsOrder();
		}).invokeInBackground();
	}
	else
	{
		_private->emitWorkerThreads();
	}
}

vec4 Raytrace::performAtPoint(s3d::Scene::Pointer scene, const Camera& cam, const vec2i& dimension, const vec2i& pixel)
{
	_private->stopWorkerThreads();

	_private->camera.getValuesFromCamera(cam);
	_private->viewportSize = dimension;
	_private->buildMaterialAndTriangles(scene);

	uint32_t bounces = 0;
	vec4 color = _private->raytracePixel(vec2i(pixel.x, dimension.y - pixel.y), _private->options.raysPerPixel, bounces);
	log::info("Sampled color:\n\tsRGB: %.4f, %.4f, %.4f\n\tRGB: %.4f, %.4f, %.4f",
		color.x, color.y, color.z, std::pow(color.x, 2.2f), std::pow(color.y, 2.2f), std::pow(color.z, 2.2f));
	return color;
}

void Raytrace::stop()
{
	_private->stopWorkerThreads();
}

void Raytrace::setOptions(const Raytrace::Options& options)
{
	_private->options = options;
}

void Raytrace::renderSpacePartitioning()
{
	_private->renderSpacePartitioning();
}

void Raytrace::output(const vec2i& pos, const vec4& color)
{
	_outputMethod(pos, color);
}

void Raytrace::setEnvironmentSampler(rt::EnvironmentSampler::Pointer sampler)
{
	_private->sampler = sampler;
}

void Raytrace::setIntegrator(rt::Integrator::Pointer integrator)
{
	_private->integrator = integrator;
}

/*
 * Private implementation
 */
RaytracePrivate::RaytracePrivate(Raytrace* o) :
owner(o), running(false)
{
}

RaytracePrivate::~RaytracePrivate()
{
	stopWorkerThreads();
}

void RaytracePrivate::emitWorkerThreads()
{
	srand(static_cast<unsigned int>(time(nullptr)));
	startTime = queryContiniousTimeInMilliSeconds();
	elapsedTime = 0;

	forwardTraceBuffer.clear();
	forwardTraceBuffer.resize(viewportSize.square());
	std::fill(forwardTraceBuffer.begin(), forwardTraceBuffer.end(), rt::float4(0.0f, 0.0f, 0.0f, 0.0f));

	uint64_t totalRays = static_cast<uint64_t>(viewportSize.square()) * options.raysPerPixel;
	log::info("Rendering started: %d x %d, %llu rpp, %llu total rays",
			  viewportSize.x, viewportSize.y, static_cast<uint64_t>(options.raysPerPixel), totalRays);

	running = true;

	if (options.threads == 0)
	{
		options.threads = std::thread::hardware_concurrency();
	}

	threadCounter.store(options.threads);
	for (uint32_t i = 0; i < options.threads; ++i)
	{
#   if (ET_RT_EVALUATE_DISTRIBUTION)
		workerThreads.emplace_back(&RaytracePrivate::visualizeDistributionThreadFunction, this, i);
#	else
		if (options.method == Raytrace::Method::LightTracing)
		{
			workerThreads.emplace_back(&RaytracePrivate::forwardPathTraceThreadFunction, this, i);
		}
		else
		{
			workerThreads.emplace_back(&RaytracePrivate::backwardPathTraceThreadFunction, this, i);
		}
#   endif
	}
}

void RaytracePrivate::stopWorkerThreads()
{
	running = false;
	for (auto& t : workerThreads)
		t.join();
	workerThreads.clear();
}

void RaytracePrivate::buildMaterialAndTriangles(s3d::Scene::Pointer scene)
{
	materials.clear();
	lightTriangles.clear();

	rt::TriangleList triangles;
	triangles.reserve(0xffff);

	lightTriangles.reserve(0xffff);

	auto meshes = scene->childrenOfType(s3d::ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		mesh->prepareRenderBatches();
		for (const auto& rb : mesh->renderBatches())
		{
			bool isEmitter = false;
			Material::Pointer batchMaterial = rb->material();

			auto materialIndex = materialIndexWithName(batchMaterial->name());
			if (materialIndex == InvalidIndex)
			{
				float alpha = clamp(batchMaterial->getFloat(MaterialParameter::Roughness), 0.0f, 1.0f);
				float eta = batchMaterial->getFloat(MaterialParameter::IndexOfRefraction);

				rt::Material::Class cls = rt::Material::Class::Diffuse;
				if (alpha < 1.0f)
				{
					if (eta == 0.0f)
					{
						log::info("Adding new conductor material: %s", batchMaterial->name().c_str());
						cls = rt::Material::Class::Conductor;
					}
					else
					{
						log::info("Adding new dielectric material: %s", batchMaterial->name().c_str());
						cls = rt::Material::Class::Dielectric;
					};
				}
				else
				{
					log::info("Adding new diffuse material: %s", batchMaterial->name().c_str());
				}

				materialIndex = static_cast<uint32_t>(materials.size());
				materials.emplace_back(cls);
				auto& mat = materials.back();

				mat.name = batchMaterial->name();
				mat.diffuse = rt::float4(batchMaterial->getVector(MaterialParameter::AlbedoColor));
				mat.specular = rt::float4(batchMaterial->getVector(MaterialParameter::ReflectanceColor));
				mat.emissive = rt::float4(batchMaterial->getVector(MaterialParameter::EmissiveColor));
				mat.roughness = std::pow(alpha, 2.0f);
				mat.ior = eta;

				isEmitter = mat.emissive.length() > 0.0f;
			}

			VertexStorage::Pointer vs = rb->vertexStorage();
			ET_ASSERT(vs.valid());

			IndexArray::Pointer ia = rb->indexArray();
			ET_ASSERT(ia.valid());

			const mat4& t = rb->transformation();

			triangles.reserve(triangles.size() + rb->numIndexes());

			const auto pos = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
			const auto nrm = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Normal, 0);
			bool hasUV = vs->hasAttribute(VertexAttributeUsage::TexCoord0);
			VertexDataAccessor<DataType::Vec2> uv0;
			if (hasUV)
			{
				uv0 = vs->accessData<DataType::Vec2>(VertexAttributeUsage::TexCoord0, 0);
			}

			for (uint32_t i = 0; i < rb->numIndexes(); i += 3)
			{
				uint32_t i0 = ia->getIndex(rb->firstIndex() + i + 0);
				uint32_t i1 = ia->getIndex(rb->firstIndex() + i + 1);
				uint32_t i2 = ia->getIndex(rb->firstIndex() + i + 2);

				triangles.emplace_back();
				auto& tri = triangles.back();
				tri.v[0] = rt::float4(t * pos[i0], 1.0f);
				tri.v[1] = rt::float4(t * pos[i1], 1.0f);
				tri.v[2] = rt::float4(t * pos[i2], 1.0f);
				tri.n[0] = rt::float4(t.rotationMultiply(nrm[i0]).normalized(), 0.0f);
				tri.n[1] = rt::float4(t.rotationMultiply(nrm[i1]).normalized(), 0.0f);
				tri.n[2] = rt::float4(t.rotationMultiply(nrm[i2]).normalized(), 0.0f);
				if (hasUV)
				{
					tri.t[0] = rt::float4(uv0[i0].x, uv0[i0].y, 0.0f, 0.0f);
					tri.t[1] = rt::float4(uv0[i1].x, uv0[i1].y, 0.0f, 0.0f);
					tri.t[2] = rt::float4(uv0[i2].x, uv0[i2].y, 0.0f, 0.0f);
				}
				else
				{
					tri.t[0] = tri.t[1] = tri.t[2] = rt::float4(0.0f);
				}
				tri.materialIndex = static_cast<rt::index>(materialIndex);
				tri.computeSupportData();

				if (isEmitter)
				{
					uint32_t lIndex = static_cast<uint32_t>(lightTriangles.size());
					lightTriangleToIndex[lIndex] = static_cast<rt::index>(triangles.size() - 1);
					lightTriangles.push_back(tri);
				}
			}
		}
	}

	kdTree.build(triangles, options.maxKDTreeDepth);

	centerRay = camera.castRay(vec2(0.0f));
	rt::KDTree::TraverseResult centerHit = kdTree.traverse(centerRay);
	if (centerHit.triangleIndex != InvalidIndex)
	{
		focalDistance = (centerHit.intersectionPoint - rt::float4(centerRay.origin)).length();
	}
	focalDistance += options.focalDistanceCorrection;

	auto stats = kdTree.nodesStatistics();
	log::info("KD-Tree statistics:\n\t%llu nodes\n\t%llu leaf nodes\n\t%llu empty leaf nodes"
			  "\n\t%llu max depth\n\t%llu min triangles per node\n\t%llu max triangles per node"
			  "\n\t%llu total triangles\n\t%llu distributed triangles\n\t%llu light triangles"
			  "\n\t%.2f focal distance"
			  "\n\t%.2f aperture size",
			  uint64_t(stats.totalNodes), uint64_t(stats.leafNodes), uint64_t(stats.emptyLeafNodes),
			  uint64_t(stats.maxDepth), uint64_t(stats.minTrianglesPerNode), uint64_t(stats.maxTrianglesPerNode),
			  uint64_t(stats.totalTriangles), uint64_t(stats.distributedTriangles), uint64_t(lightTriangles.size()),
			  focalDistance, options.apertureSize);

	if (options.renderKDTree)
	{
		kdTree.printStructure();
	}
}

uint32_t RaytracePrivate::materialIndexWithName(const std::string& n)
{
	for (size_t i = 0, e = materials.size(); i < e; ++i)
	{
		if (materials.at(i).name == n)
			return static_cast<uint32_t>(i);
	}
	return InvalidIndex;
}

void RaytracePrivate::buildRegions(const vec2i& aSize)
{
	std::unique_lock<std::mutex> lock(regionsLock);
	regions.clear();
	sampledRegions.store(0);

	regionSize = aSize;

	if (regionSize.x > viewportSize.x)
		regionSize.x = viewportSize.x;
	if (regionSize.y > viewportSize.y)
		regionSize.y = viewportSize.y;

	ET_ASSERT(regionSize.x > 0);
	ET_ASSERT(regionSize.y > 0);

	int xr = viewportSize.x / regionSize.x;
	int yr = viewportSize.y / regionSize.y;

	for (int y = 0; y < yr; ++y)
	{
		for (int x = 0; x < xr; ++x)
		{
			regions.emplace_back();
			regions.back().origin = vec2i(x, y) * regionSize;
			regions.back().size = regionSize;
		}

		if (xr * regionSize.x < viewportSize.x)
		{
			int w = viewportSize.x - xr * regionSize.x;
			regions.emplace_back();
			regions.back().origin = vec2i(xr, y) * regionSize;
			regions.back().size = vec2i(w, regionSize.y);
		}
	}

	if (yr * regionSize.y < viewportSize.y)
	{
		int h = viewportSize.y - yr * regionSize.y;
		for (int x = 0; x < xr; ++x)
		{
			regions.emplace_back();
			regions.back().origin = vec2i(x, yr) * regionSize;
			regions.back().size = vec2i(regionSize.x, h);
		}

		if (xr * regionSize.x < viewportSize.x)
		{
			int w = viewportSize.x - xr * regionSize.x;
			regions.emplace_back();
			regions.back().origin = vec2i(xr, yr) * regionSize;
			regions.back().size = vec2i(w, h);
		}
	}
}

rt::Region RaytracePrivate::getNextRegion()
{
	std::unique_lock<std::mutex> lock(regionsLock);
	for (auto& rgn : regions)
	{
		if (rgn.sampled == false)
		{
			++sampledRegions;
			rgn.sampled = true;
			return rgn;
		}
	}
	return rt::Region();
}

/*
 * Raytrace functions
 */
void RaytracePrivate::visualizeDistributionThreadFunction(uint32_t index)
{
	const uint32_t sampleTestCount = 100000000;
	const uint32_t renderTestCount = 1000000;

	static rt::float4 testDirection;

	if (index == 0)
	{
		testDirection = rt::float4(2.0f * rt::fastRandomFloat() - 1.0f, 2.0f * rt::fastRandomFloat() - 1.0f, 2.0f * rt::fastRandomFloat() - 1.0f, 0.0f);
		testDirection.normalize();
	}

	auto distribution = rt::ggxDistribution;
	float alpha = 0.1f;

	if (index > 0)
	{
		float l = camera.position().length() / 10.0f;
		for (uint32_t i = 0; running && (i < renderTestCount); ++i)
		{
			auto n = rt::randomVectorOnHemisphere(testDirection, distribution, alpha);
			vec2 e = projectPoint(n * l);
			renderPixel(e, vec4(1.0f, 0.01f));
		}
		return;
	}

	const uint32_t sampleCount = 1000;
	Vector<uint32_t> prob(sampleCount, 0);
	for (uint32_t i = 0; running && (i < sampleTestCount); ++i)
	{
		auto v = rt::randomVectorOnHemisphere(testDirection, distribution, alpha).dot(testDirection);
		uint32_t VdotN = static_cast<uint32_t>(clamp(v, 0.0f, 1.0f) * static_cast<float>(sampleCount));
		prob[VdotN] += 1;
	}

	uint32_t maxValue = prob.front();
	for (auto i : prob)
	{
		maxValue = std::max(maxValue, i);
	}
	float vScale = static_cast<float>(sampleTestCount) / static_cast<float>(maxValue);

	const float off = 10.0f;
	vec2 p00(off);
	vec2 p01(viewportSize.x - off, off);
	vec2 p10(off, viewportSize.y - off);
	renderLine(p00, p01, vec4(1.0f));
	renderLine(p00, p10, vec4(1.0f));

	float ds = 1.0f / static_cast<float>(sampleCount - 1);
	float lastHeight = vScale * static_cast<float>(prob.front()) / static_cast<float>(sampleTestCount);
	for (uint32_t i = 1; running && (i < sampleCount); ++i)
	{
		float delta = static_cast<float>(i) * ds;
		float x0 = off + (delta - ds)* (viewportSize.x - 2.0f * off);
		float x1 = off + delta * (viewportSize.x - 2.0f * off);
		float newHeight = vScale * static_cast<float>(prob[i]) / static_cast<float>(sampleTestCount);

		vec2 ph1(x0, off + (viewportSize.y - 2.0f * off) * lastHeight);
		vec2 ph2(x1, off + (viewportSize.y - 2.0f * off) * newHeight);
		renderLine(ph1, ph2, vec4(1.0f, 1.0f, 0.0f, 1.0f));

		ph1.y = off + (viewportSize.y - 2.0f * off) * distribution(delta - ds, alpha);
		ph2.y = off + (viewportSize.y - 2.0f * off) * distribution(delta, alpha);
		renderLine(ph1, ph2, vec4(1.0f, 0.0f, 0.0f, 1.0f));

		lastHeight = newHeight;
	}
}

void RaytracePrivate::forwardPathTraceThreadFunction(uint32_t threadId)
{
	if (lightTriangles.empty())
	{
		log::error("No light sources found in scene");
		return;
	}

	const int raysPerIteration = viewportSize.square() / 4;

	float imagePlaneDistanceSq = 2.0f * sqr(static_cast<float>(viewportSize.x) / std::tan(camera.fieldOfView()));

	Vector<rt::float4> localBuffer(viewportSize.square(), rt::float4(0.0f));

	rt::float4 cameraPos(camera.position(), 0.0f);
	rt::float4 cameraDir(-camera.direction(), 0.0f);
	vec3 viewport = vector3ToFloat(vec3i(viewportSize, 0));

	auto projectToCamera = [&](const rt::Ray& inRay, const rt::KDTree::TraverseResult& hit,
							   const rt::float4& color, const rt::float4& nrm)
	{
		rt::float4 toCamera = cameraPos - hit.intersectionPoint;
		toCamera.normalize();

		const auto& tri = kdTree.triangleAtIndex(hit.triangleIndex);
		const auto& mat = materials.at(tri.materialIndex);
		rt::float4 uv0 = tri.interpolatedTexCoord0(hit.intersectionPointBarycentric);
		rt::BSDFSample sample(inRay.direction, toCamera, nrm, mat, uv0, rt::BSDFSample::Direction::Forward);

		if (sample.OdotN <= 0.0f)
			return;

		auto projected = camera.project(hit.intersectionPoint.xyz());
		if ((projected.x * projected.x > 1.0f) || (projected.y * projected.y > 1.0f) || (projected.z * projected.z > 1.0f))
			return;

		auto backHit = kdTree.traverse(rt::Ray(cameraPos, sample.Wo * (-1.0f)));
		if (backHit.triangleIndex != hit.triangleIndex)
			return;

		float pdf = sample.pdf();
		if (pdf <= 0.0f)
			return;

		float bsdf = sample.bsdf();
		if (bsdf == 0.0f)
			return;

		float VdotD = -cameraDir.dot(sample.Wo);
		float pdfW = imagePlaneDistanceSq / (VdotD * sqr(VdotD));
		float imageToSurfaceFactor = sample.OdotN / (hit.intersectionPoint - cameraPos).dotSelf();
		float scaleFactor = bsdf * pdfW * imageToSurfaceFactor / static_cast<float>(raysPerIteration);

		ET_ASSERT(scaleFactor >= 0.0f);

		projected = (0.5f * projected + vec3(0.5f)) * viewport;
		vec2i pixel(static_cast<int>(projected.x), static_cast<int>(projected.y));
		localBuffer[pixel.x + pixel.y * viewportSize.x] += color * scaleFactor;
	};

	while (running)
	{
		for (int ir = 0; running && (ir < raysPerIteration); ++ir)
		{
			uint32_t emitterIndex = rand() % lightTriangles.size();
			const auto& emitterTriangle = lightTriangles[emitterIndex];

			rt::KDTree::TraverseResult source;
			source.intersectionPointBarycentric = rt::randomBarycentric();
			source.intersectionPoint = emitterTriangle.interpolatedPosition(source.intersectionPointBarycentric);
			source.triangleIndex = lightTriangleToIndex[emitterIndex];

			rt::float4 triangleNormal = emitterTriangle.interpolatedNormal(source.intersectionPointBarycentric);
			rt::float4 sourceDir = rt::randomVectorOnHemisphere(triangleNormal, rt::cosineDistribution);

			float pickProb = 1.0f / static_cast<float>(lightTriangles.size());
			float area = emitterTriangle.area();

			rt::float4 color = materials.at(emitterTriangle.materialIndex).emissive * (area / pickProb);
			rt::Ray currentRay(source.intersectionPoint + sourceDir * rt::Constants::epsilon, sourceDir);

			projectToCamera(currentRay, source, color, triangleNormal);

			for (uint32_t pathLength = 0; pathLength < options.maxPathLength; ++pathLength)
			{
				auto hit = kdTree.traverse(currentRay);
				if (hit.triangleIndex == InvalidIndex)
				{
					break;
				}

				const auto& tri = kdTree.triangleAtIndex(hit.triangleIndex);
				const auto& mat = materials.at(tri.materialIndex);

				if (mat.emissive.dotSelf() > 0.0f)
				{
					break;
				}

				rt::float4 nrm = tri.interpolatedNormal(hit.intersectionPointBarycentric);
				rt::float4 uv0 = tri.interpolatedTexCoord0(hit.intersectionPointBarycentric);
				rt::BSDFSample sample(currentRay.direction, nrm, mat, uv0, rt::BSDFSample::Direction::Forward);

#			if (ET_RT_VISUALIZE_BRDF)
				projectToCamera(currentRay, hit, rt::float4(sample.bsdf()), nrm);
				break;
#			else
				color *= sample.evaluate();
				projectToCamera(currentRay, hit, color, nrm);
#			endif

				currentRay.direction = sample.Wo;
				currentRay.origin = hit.intersectionPoint + currentRay.direction * rt::Constants::epsilon;
			}
		}

		log::info("Iteration finished");
		flushToForwardTraceBuffer(localBuffer);
		std::fill(localBuffer.begin(), localBuffer.end(), rt::float4(0.0f));
	}
	log::info("Thread finished");
}

void RaytracePrivate::backwardPathTraceThreadFunction(uint32_t threadId)
{
	DataStorage<vec4> localData(sqr(options.renderRegionSize), 0);

	while (running)
	{
		auto region = getNextRegion();
		if (region.sampled == false)
			break;

		auto runTime = queryContiniousTimeInMilliSeconds();

		vec2i pixel;
		for (pixel.y = region.origin.y; pixel.y < region.origin.y + region.size.y; ++pixel.y)
		{
			owner->_outputMethod(vec2i(region.origin.x, pixel.y), vec4(1.0f, 0.0f, 0.0f, 1.0f));
			owner->_outputMethod(vec2i(region.origin.x + region.size.x - 1, pixel.y), vec4(1.0f, 0.0f, 0.0f, 1.0f));
		}
		for (pixel.x = region.origin.x; pixel.x < region.origin.x + region.size.x; ++pixel.x)
		{
			owner->_outputMethod(vec2i(pixel.x, region.origin.y), vec4(1.0f, 0.0f, 0.0f, 1.0f));
			owner->_outputMethod(vec2i(pixel.x, region.origin.y + region.size.y - 1), vec4(1.0f, 0.0f, 0.0f, 1.0f));
		}

		uint32_t k = 0;
		for (pixel.y = region.origin.y; running && (pixel.y < region.origin.y + region.size.y); ++pixel.y)
		{
			for (pixel.x = region.origin.x; running && (pixel.x < region.origin.x + region.size.x); ++pixel.x)
			{
				uint32_t bounces = 0;
				localData[k++] = raytracePixel(pixel, options.raysPerPixel, bounces);
			}
		}

		k = 0;
		for (pixel.y = region.origin.y; running && (pixel.y < region.origin.y + region.size.y); ++pixel.y)
		{
			for (pixel.x = region.origin.x; running && (pixel.x < region.origin.x + region.size.x); ++pixel.x)
			{
				owner->_outputMethod(pixel, localData[k++]);
			}
		}

		elapsedTime += queryContiniousTimeInMilliSeconds() - runTime;
		/*
		 float averageTime = static_cast<float>(elapsedTime) / static_cast<float>(1000 * sampledRegions);
		 auto actualElapsed = queryContiniousTimeInMilliSeconds() - startTime;
		 log::info("Elapsed: %s, estimated: %s",
		 floatToTimeStr(static_cast<float>(actualElapsed) / 1000.f, false).c_str(),
		 floatToTimeStr(averageTime * static_cast<float>(regions.size() - sampledRegions), false).c_str());
		 */
	}

	--threadCounter;

	if (threadCounter.load() == 0)
	{
		if (options.renderKDTree)
			renderSpacePartitioning();

		auto endTime = queryContiniousTimeInMilliSeconds();
		uint64_t diff = endTime - startTime;

		log::info("Rendering completed: %llu, (in %llu ms, %.3g s)", endTime,
				  diff, static_cast<float>(diff) / 1000.0f);

		owner->renderFinished.invokeInMainRunLoop();
	}
}

vec4 RaytracePrivate::raytracePixel(const vec2i& pixel, uint32_t samples, uint32_t& bounces)
{
	ET_ASSERT(samples > 0);

	if (integrator.invalid())
	{
		ET_FAIL("Integrator is not set");
		return vec4(0.0f);
	}

	vec2 pixelSize = vec2(1.0f) / vector2ToFloat(viewportSize);
	vec2 pixelBase = 2.0f * (vector2ToFloat(pixel) * pixelSize) - vec2(1.0f);

	rt::float4 result(0.0f);
	for (uint32_t m = 0; m < samples; ++m)
	{
		vec2 jitter = pixelSize * vec2(2.0f * rt::fastRandomFloat() - 1.0f, 2.0f * rt::fastRandomFloat() - 1.0f);
		ray3d baseRay = camera.castRay(pixelBase + jitter);

		float phi = rt::fastRandomFloat() * DOUBLE_PI;
		float r = std::sqrt(rt::fastRandomFloat());

		vec3 uOffset = perpendicularVector(baseRay.direction);
		float uScale = std::sin(phi) * options.apertureSize * r;

		vec3 vOffset = cross(uOffset, baseRay.direction);
		float vScale = std::cos(phi) * options.apertureSize * r;

		vec3 shiftedOrigin = camera.position() + uOffset * uScale + vOffset * vScale;

		float distanceToFocalPlane = focalDistance / baseRay.direction.dot(centerRay.direction);
		vec3 focalPoint = camera.position() + distanceToFocalPlane * baseRay.direction;
		vec3 direction = (focalPoint - shiftedOrigin).normalize();

		result += integrator->gather(ray3d(shiftedOrigin, direction), options.maxPathLength, bounces,
		kdTree, sampler, materials);
	}

	vec4 output = result.toVec4() / static_cast<float>(samples);
	output = maxv(minv(output, vec4(1.0f)), vec4(0.0f));

#if (ET_RT_ENABLE_GAMMA_CORRECTION)
	output.x = std::pow(output.x, 1.0f / 2.2f);
	output.y = std::pow(output.y, 1.0f / 2.2f);
	output.z = std::pow(output.z, 1.0f / 2.2f);
	ET_ASSERT(!isnan(output.x));
	ET_ASSERT(!isnan(output.y));
	ET_ASSERT(!isnan(output.z));
#endif

	output.w = 1.0f;
	return output;
}

void RaytracePrivate::estimateRegionsOrder()
{
	const uint32_t maxSamples = 5;

	const vec2i sx[maxSamples] =
	{
		vec2i(1, 3), vec2i(2, 3), vec2i(1, 2), vec2i(1, 3), vec2i(2, 3),
	};

	const vec2i sy[maxSamples] =
	{
		vec2i(1, 3), vec2i(1, 3), vec2i(1, 2), vec2i(2, 3), vec2i(2, 3),
	};

	std::random_shuffle(regions.begin(), regions.end());
	for (auto& r : regions)
	{
		r.estimatedBounces = 0;
		vec4 estimatedColor(0.0);
		for (uint32_t i = 0; i < maxSamples; ++i)
		{
			uint32_t bounces = 0;
			vec2i px = r.origin + vec2i(sx[i].x * r.size.x / sx[i].y, sy[i].x * r.size.y / sy[i].y);
			estimatedColor += raytracePixel(px, 1, bounces);
			r.estimatedBounces += bounces;
		}
	}

	std::sort(regions.begin(), regions.end(), [](const rt::Region& l, const rt::Region& r)
			  { return l.estimatedBounces > r.estimatedBounces; });

	emitWorkerThreads();
}

void RaytracePrivate::renderSpacePartitioning()
{
	renderBoundingBox(kdTree.bboxAt(0), vec4(1.0f, 0.0f, 1.0f, 1.0f));
	renderKDTreeRecursive(0, 0);
}

void RaytracePrivate::renderKDTreeRecursive(uint32_t nodeIndex, uint32_t index)
{
	const vec4 colorOdd(1.0f, 1.0f, 0.0f, 1.0f);
	const vec4 colorEven(0.0f, 1.0f, 1.0f, 1.0f);

	const auto& node = kdTree.nodeAt(nodeIndex);

	if (node.axis <= rt::MaxAxisIndex)
	{
		renderKDTreeRecursive(node.children[0], index + 1);
		renderKDTreeRecursive(node.children[1], index + 1);
	}
	else
	{
		renderBoundingBox(kdTree.bboxAt(nodeIndex), (index % 2) ? colorOdd : colorEven);
	}
}

void RaytracePrivate::renderBoundingBox(const rt::BoundingBox& box, const vec4& color)
{
	vec2 c0 = projectPoint(box.center + box.halfSize * rt::float4(-1.0f, -1.0f, -1.0f, 0.0f));
	vec2 c1 = projectPoint(box.center + box.halfSize * rt::float4( 1.0f, -1.0f, -1.0f, 0.0f));
	vec2 c2 = projectPoint(box.center + box.halfSize * rt::float4(-1.0f,  1.0f, -1.0f, 0.0f));
	vec2 c3 = projectPoint(box.center + box.halfSize * rt::float4( 1.0f,  1.0f, -1.0f, 0.0f));
	vec2 c4 = projectPoint(box.center + box.halfSize * rt::float4(-1.0f, -1.0f,  1.0f, 0.0f));
	vec2 c5 = projectPoint(box.center + box.halfSize * rt::float4( 1.0f, -1.0f,  1.0f, 0.0f));
	vec2 c6 = projectPoint(box.center + box.halfSize * rt::float4(-1.0f,  1.0f,  1.0f, 0.0f));
	vec2 c7 = projectPoint(box.center + box.halfSize * rt::float4( 1.0f,  1.0f,  1.0f, 0.0f));

	renderLine(c0, c1, color);
	renderLine(c0, c2, color);
	renderLine(c0, c4, color);
	renderLine(c1, c5, color);
	renderLine(c2, c6, color);
	renderLine(c3, c1, color);
	renderLine(c3, c2, color);
	renderLine(c3, c7, color);
	renderLine(c4, c5, color);
	renderLine(c4, c6, color);
	renderLine(c7, c5, color);
	renderLine(c7, c6, color);
}

void RaytracePrivate::renderLine(const vec2& from, const vec2& to, const vec4& color)
{
	float dt = 1.0f / length(to - from);

	float t = 0.0f;
	while (t <= 1.0f)
	{
		renderPixel(mix(from, to, t), color);
		t += dt;
	}
}

void RaytracePrivate::renderPixel(const vec2& pixel, const vec4& color)
{
	vec2 nearPixels[4];
	nearPixels[0] = floorv(pixel);
	nearPixels[1] = nearPixels[0] + vec2(1.0f, 0.0f);
	nearPixels[2] = nearPixels[0] + vec2(0.0f, 1.0f);
	nearPixels[3] = nearPixels[0] + vec2(1.0f, 1.0f);
	for (uint32_t i = 0; i < 4; ++i)
	{
		float d = length(nearPixels[i] - pixel);
		vec2i px(static_cast<int>(nearPixels[i].x), static_cast<int>(nearPixels[i].y));
		owner->_outputMethod(px, color * vec4(1.0f, 1.0f, 1.0f, 1.0f - d));
	}
}

vec2 RaytracePrivate::projectPoint(const rt::float4& p)
{
	return vector2ToFloat(viewportSize) *
	(vec2(0.5f, 0.5f) + vec2(0.5f, 0.5f) * camera.project(p.xyz()).xy());
}

void RaytracePrivate::fillRegionWithColor(const rt::Region& region, const vec4& color)
{
	ET_ASSERT(!isinf(color.x));
	ET_ASSERT(!isinf(color.y));
	ET_ASSERT(!isinf(color.z));
	ET_ASSERT(!isinf(color.w));

	vec2i pixel;
	for (pixel.y = region.origin.y; pixel.y < region.origin.y + region.size.y; ++pixel.y)
	{
		for (pixel.x = region.origin.x; pixel.x < region.origin.x + region.size.x; ++pixel.x)
			owner->_outputMethod(pixel, color);
	}
}

void RaytracePrivate::renderTriangle(const rt::Triangle& tri)
{
	const vec4 lineColor(5.0f, 0.9f, 0.8f, 1.0f);

	vec2 c0 = projectPoint(tri.v[0]);
	vec2 c1 = projectPoint(tri.v[1]);
	vec2 c2 = projectPoint(tri.v[2]);
	renderLine(c0, c1, lineColor);
	renderLine(c1, c2, lineColor);
	renderLine(c2, c0, lineColor);
}

void RaytracePrivate::flushToForwardTraceBuffer(const Vector<rt::float4>& localBuffer)
{
	std::lock_guard<std::mutex> lock(forwardTraceBufferMutex);

	flushCounter++;

	float rsScale = 1.0f / static_cast<float>(flushCounter);
	
	auto src = localBuffer.data();
	auto dst = forwardTraceBuffer.data();
	for (uint32_t i = 0; i < forwardTraceBuffer.size(); ++i, ++dst, ++src)
	{
		*dst += *src;
		
		vec4 output = dst->toVec4() * rsScale;
		output.w = 1.0f;
		
#   if (ET_RT_ENABLE_GAMMA_CORRECTION)
		output.x = std::pow(output.x, 1.0f / 2.2f);
		output.y = std::pow(output.y, 1.0f / 2.2f);
		output.z = std::pow(output.z, 1.0f / 2.2f);
		ET_ASSERT(!isnan(output.x));
		ET_ASSERT(!isnan(output.y));
		ET_ASSERT(!isnan(output.z));
#    endif
		
		vec2i px(static_cast<int>(i % viewportSize.x), static_cast<int>(i / viewportSize.x));
		owner->output(px, output);
	}
}

}
