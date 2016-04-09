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
#include <et/app/application.h>
#include <et/camera/camera.h>

const et::rt::float_type et::rt::Constants::epsilon = 0.00025f;
const et::rt::float_type et::rt::Constants::minusEpsilon = -epsilon;
const et::rt::float_type et::rt::Constants::onePlusEpsilon = 1.0f + epsilon;
const et::rt::float_type et::rt::Constants::epsilonSquared = epsilon * epsilon;
const et::rt::float_type et::rt::Constants::initialSplitValue = std::numeric_limits<float>::max();

class et::RaytracePrivate
{
public:
	RaytracePrivate(Raytrace* owner);
	~RaytracePrivate();

	void threadFunction();
	void emitWorkerThreads();
	void stopWorkerThreads();

	void buildMaterialAndTriangles(s3d::Scene::Pointer);

	size_t materialIndexWithName(const std::string&);

	void buildRegions(vec2i size);
	void estimateRegionsOrder();

	vec4 raytracePixel(const vec2i&, size_t samples, size_t& bounces);

	rt::Region getNextRegion();

	void renderSpacePartitioning();
	void renderKDTreeRecursive(size_t nodeIndex, size_t index);
	void renderBoundingBox(const rt::BoundingBox&, const vec4& color);
	void renderLine(const vec2& from, const vec2& to, const vec4& color);
	void renderPixel(const vec2&, const vec4& color);
	vec2 projectPoint(const rt::float4&);

	void fillRegionWithColor(const rt::Region&, const vec4& color);
	void renderTriangle(const rt::Triangle&);

public:
	Raytrace* owner = nullptr;
	Raytrace::Options options;
	rt::KDTree kdTree;
	rt::EnvironmentSampler::Pointer sampler;
	rt::Integrator::Pointer integrator;
	rt::Material::Collection materials;
	Camera camera;
	vec2i viewportSize = vec2i(0);

	Vector<std::thread> workerThreads;
	std::atomic<size_t> threadCounter;

	Vector<rt::Region> regions;
	std::mutex regionsLock;

	std::atomic<bool> running;
	uint64_t startTime = 0;
};

using namespace et;

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
	_private->camera = cam;
	_private->viewportSize = dimension;
	_private->buildMaterialAndTriangles(scene);
	_private->buildRegions(vec2i(static_cast<int>(_private->options.renderRegionSize)));
	Invocation([this]() {
		_private->estimateRegionsOrder();
	}).invokeInBackground();
}

vec4 Raytrace::performAtPoint(s3d::Scene::Pointer scene, const Camera& cam, const vec2i& dimension, const vec2i& pixel)
{
	_private->stopWorkerThreads();

	_private->camera = cam;
	_private->viewportSize = dimension;
	_private->buildMaterialAndTriangles(scene);

	size_t bounces = 0;
	return _private->raytracePixel(vec2i(pixel.x, dimension.y - pixel.y), _private->options.raysPerPixel, bounces);
}

void Raytrace::stop()
{
	_private->stopWorkerThreads();
}

void Raytrace::setOptions(const et::Raytrace::Options& options)
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

	uint64_t totalRays = static_cast<uint64_t>(viewportSize.square()) * options.raysPerPixel;
	log::info("Rendering started: %d x %d, %llu rpp, %llu total rays",
			  viewportSize.x, viewportSize.y, static_cast<uint64_t>(options.raysPerPixel), totalRays);

	running = true;
	threadCounter.store(std::thread::hardware_concurrency());
	for (unsigned i = 0, e = std::thread::hardware_concurrency(); i < e; ++i)
		workerThreads.emplace_back(&RaytracePrivate::threadFunction, this);
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
	rt::TriangleList triangles;

	auto meshes = scene->childrenOfType(s3d::ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		auto meshMaterial = mesh->material();
		auto materialIndex = materialIndexWithName(meshMaterial->name());
		if (materialIndex == InvalidIndex)
		{
			materialIndex = materials.size();
			materials.emplace_back();
			auto& mat = materials.back();
			rt::float_type r = clamp(meshMaterial->getFloat(MaterialParameter::Roughness), 0.0f, 1.0f);
			auto kA = meshMaterial->getVector(MaterialParameter::AmbientColor);
			auto kD = meshMaterial->getVector(MaterialParameter::DiffuseColor);
			mat.name = meshMaterial->name();
			mat.diffuse = rt::float4(kA + kD);
			mat.specular = rt::float4(meshMaterial->getVector(MaterialParameter::SpecularColor));
			mat.emissive = rt::float4(meshMaterial->getVector(MaterialParameter::EmissiveColor));
			mat.roughness = HALF_PI * (1.0f - std::cos(HALF_PI * r));
			mat.ior = meshMaterial->getFloat(MaterialParameter::Transparency);
			if (mat.roughness < 1.0f)
			{
				if (mat.ior == 0.0f)
				{
					mat.type = rt::MaterialType::Conductor;
				}
				else
				{
					mat.type = rt::MaterialType::Dielectric;
				}
			}
		}

		mesh->prepareRenderBatches();
		for (const auto& rb : mesh->renderBatches())
		{
			auto& vs = rb->vertexStorage();
			auto& ia = rb->indexArray();
			if (vs.invalid() || ia.invalid()) continue;

			const mat4& t = rb->transformation();

			triangles.reserve(triangles.size() + rb->numIndexes());

			const auto pos = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
			const auto nrm = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Normal, 0);
			// const auto uv0 = vs->accessData<DataType::Vec2>(VertexAttributeUsage::TexCoord0, 0);
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
				/*
				tri.t[0] = rt::float4(uv0[i0].x, uv0[i0].y, 0.0f, 0.0f);
				tri.t[1] = rt::float4(uv0[i1].x, uv0[i1].y, 0.0f, 0.0f);
				tri.t[2] = rt::float4(uv0[i2].x, uv0[i2].y, 0.0f, 0.0f);
				*/
				tri.materialIndex = static_cast<rt::index>(materialIndex);
				tri.computeSupportData();
			}
		}
	}

	kdTree.build(triangles, options.maxKDTreeDepth);

	auto stats = kdTree.nodesStatistics();
	log::info("KD-Tree statistics:\n\t%llu nodes\n\t%llu leaf nodes\n\t%llu empty leaf nodes"
			  "\n\t%llu max depth\n\t%llu min triangles per node\n\t%llu max triangles per node"
			  "\n\t%llu total triangles\n\t%llu distributed triangles", uint64_t(stats.totalNodes),
			  uint64_t(stats.leafNodes), uint64_t(stats.emptyLeafNodes), uint64_t(stats.maxDepth),
			  uint64_t(stats.minTrianglesPerNode), uint64_t(stats.maxTrianglesPerNode),
			  uint64_t(stats.totalTriangles), uint64_t(stats.distributedTriangles));

	if (options.renderKDTree)
		kdTree.printStructure();
}

size_t RaytracePrivate::materialIndexWithName(const std::string& n)
{
	for (size_t i = 0, e = materials.size(); i < e; ++i)
	{
		if (materials.at(i).name == n)
			return i;
	}
	return InvalidIndex;
}

void RaytracePrivate::buildRegions(vec2i size)
{
	std::unique_lock<std::mutex> lock(regionsLock);
	regions.clear();

	if (size.x > viewportSize.x)
		size.x = viewportSize.x;

	if (size.y > viewportSize.y)
		size.y = viewportSize.y;

	int xr = viewportSize.x / size.x;
	int yr = viewportSize.y / size.y;

	for (int y = 0; y < yr; ++y)
	{
		for (int x = 0; x < xr; ++x)
		{
			regions.emplace_back();
			regions.back().origin = vec2i(x, y) * size;
			regions.back().size = size;
		}

		if (xr * size.x < viewportSize.x)
		{
			int w = viewportSize.x - xr * size.x;
			regions.emplace_back();
			regions.back().origin = vec2i(xr, y) * size;
			regions.back().size = vec2i(w, size.y);
		}
	}

	if (yr * size.y < viewportSize.y)
	{
		int h = viewportSize.y - yr * size.y;
		for (int x = 0; x < xr; ++x)
		{
			regions.emplace_back();
			regions.back().origin = vec2i(x, yr) * size;
			regions.back().size = vec2i(size.x, h);
		}

		if (xr * size.x < viewportSize.x)
		{
			int w = viewportSize.x - xr * size.x;
			regions.emplace_back();
			regions.back().origin = vec2i(xr, yr) * size;
			regions.back().size = vec2i(w, h);
		}
	}
}

rt::Region RaytracePrivate::getNextRegion()
{
	std::unique_lock<std::mutex> lock(regionsLock);

	size_t index = 0;
	for (auto& rgn : regions)
	{
		if (!rgn.sampled)
		{
			rgn.sampled = true;
			return rgn;
		}
		++index;
	}

	return rt::Region();
}

/*
 * Raytrace function
 */
void RaytracePrivate::threadFunction()
{
	while (running)
	{
		auto region = getNextRegion();
		if (!region.sampled)
			break;

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

		for (pixel.y = region.origin.y; pixel.y < region.origin.y + region.size.y; ++pixel.y)
		{
			for (pixel.x = region.origin.x; pixel.x < region.origin.x + region.size.x; ++pixel.x)
			{
				size_t bounces = 0;
				owner->_outputMethod(pixel, raytracePixel(pixel, options.raysPerPixel, bounces));
				if (!running)
					return;
			}
		}
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

vec4 RaytracePrivate::raytracePixel(const vec2i& pixel, size_t samples, size_t& bounces)
{
	ET_ASSERT(samples > 0);

	if (integrator.invalid())
	{
		ET_FAIL("Integrator is not set");
		return vec4(0.0f);
	}

	vec2 pixelSize = vec2(1.0f) / vector2ToFloat(viewportSize);
	vec2 pixelBase = 2.0f * (vector2ToFloat(pixel) * pixelSize) - vec2(1.0f);

	rt::float4 result = integrator->gather(camera.castRay(pixelBase), 0, bounces, kdTree, sampler, materials);
	for (size_t m = 1; m < samples; ++m)
	{
		vec2 jitter = pixelSize * vec2(2.0f * rt::fastRandomFloat() - 1.0f, 2.0f * rt::fastRandomFloat() - 1.0f);
		result += integrator->gather(camera.castRay(pixelBase + jitter), 0, bounces, kdTree, sampler, materials);
	}
	vec4 output = (result / static_cast<float>(samples)).toVec4();
	output.w = 1.0f;

	return output;
}

void RaytracePrivate::estimateRegionsOrder()
{
	const rt::float_type maxPossibleBounces = float(rt::PathTraceIntegrator::MaxTraverseDepth);
	const size_t maxSamples = 5;

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
		for (size_t i = 0; i < maxSamples; ++i)
		{
			size_t bounces = 0;
			estimatedColor += raytracePixel(r.origin + vec2i(sx[i].x * r.size.x / sx[i].y,
															 sy[i].x * r.size.y / sy[i].y), 1, bounces);
			r.estimatedBounces += bounces;
		}
		rt::float_type aspect = (maxPossibleBounces - float(r.estimatedBounces)) / maxPossibleBounces;
		vec4 estimatedDensity = vec4(1.0f - 0.5f * aspect * aspect, 1.0f);

		fillRegionWithColor(r, estimatedDensity * estimatedDensity);
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

void RaytracePrivate::renderKDTreeRecursive(size_t nodeIndex, size_t index)
{
	const vec4 colorOdd(1.0f, 1.0f, 0.0f, 1.0f);
	const vec4 colorEven(0.0f, 1.0f, 1.0f, 1.0f);

	const auto& node = kdTree.nodeAt(nodeIndex);

	if (node.axis >= 0)
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
	rt::float_type dt = 1.0f / length(to - from);

	rt::float_type t = 0.0f;
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
	for (size_t i = 0; i < 4; ++i)
	{
		rt::float_type d = length(nearPixels[i] - pixel);
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
