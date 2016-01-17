/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <thread>
#include <mutex>
#include <et/rt/raytrace.h>
#include <et/rt/raytraceobjects.h>
#include <et/app/application.h>

#define USE_ITERATIVE_GATHER 1

namespace et
{
	
const float rt::Constants::epsilon = 0.0001f;
const float rt::Constants::minusEpsilon = -epsilon;
const float rt::Constants::onePlusEpsilon = 1.0f + epsilon;
const float rt::Constants::epsilonSquared = epsilon * epsilon;
const float rt::Constants::initialSplitValue = std::numeric_limits<float>::max();

namespace
{
	enum class DebugRenderMode : size_t
	{
		RenderNormals,
		RenderDiffuse,
		RenderTriangle
	};
	
	enum class RayClass : size_t
	{
		Debug,
		Diffuse,
		Reflected,
		Refracted,
	};

	struct ET_ALIGNED(16) FastTraverseStack
	{
	public:
		enum : size_t
		{
			MaxElements = 64,
			MaxElementsPlusOne = MaxElements + 1,
		};
		
	public:
		void emplace(const rt::float4& a, const rt::float4& m)
		{
			ET_ASSERT(_size < MaxElements);
			add[_size] = a;
			mul[_size] = m;
			++_size;
		}
		
		bool empty() const
			{ return _size == 0; }
		
		bool hasSomething() const
			{ return _size > 0; }
		
		const rt::float4& topMul() const
			{ ET_ASSERT(_size < MaxElementsPlusOne); return mul[_size - 1]; }
		
		const rt::float4& topAdd() const
			{ ET_ASSERT(_size < MaxElementsPlusOne); return add[_size - 1]; }
		
		void pop()
			{ ET_ASSERT(_size > 0); --_size; }
		
		size_t size() const
			{ return _size; }
		
	private:
		rt::float4 add[MaxElements];
		rt::float4 mul[MaxElements];
		size_t _size = 0;
	};
}
	
	class RaytracePrivate
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

		rt::float4 gatherBouncesIterative(const rt::Ray&, size_t depth, size_t& maxDepth);
		
		rt::float4 sampleEnvironment(const rt::float4& direction);

		rt::Region getNextRegion();
		
		void renderSpacePartitioning();
		void renderKDTreeRecursive(size_t nodeIndex, size_t index);
		void renderBoundingBox(const rt::BoundingBox&, const vec4& color);
		void renderLine(const vec2& from, const vec2& to, const vec4& color);
		void renderPixel(const vec2&, const vec4& color);
		vec2 projectPoint(const rt::float4&);
		
		void fillRegionWithColor(const rt::Region&, const vec4& color);
		void renderTriangle(const rt::Triangle&);
		
		RayClass classifyRay(rt::float4& hitNormal, const rt::Material& hitMaterial,
			const rt::float4& inDirection, rt::float4& direction, rt::float4& output);
		
	public:
		Raytrace* owner = nullptr;
		Raytrace::Options options;
		KDTree kdTree;
		rt::EnvironmentSampler::Pointer sampler;
		Camera camera;
		vec2i viewportSize = vec2i(0);
		DebugRenderMode debugMode = DebugRenderMode::RenderNormals;

		std::vector<std::thread> workerThreads;
		std::atomic<bool> running;
		std::vector<rt::Material> materials;
		std::vector<rt::Region> regions;
		std::mutex regionsLock;
		std::atomic<size_t> threadCounter;
		uint64_t startTime = 0;
	};
}

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
	Invocation([this]()
	{
		_private->estimateRegionsOrder();
	}).invokeInBackground();
}

vec4 Raytrace::performAtPoint(s3d::Scene::Pointer scene, const Camera& cam,
	const vec2i& dimension, const vec2i& pixel)
{
	_private->stopWorkerThreads();

	_private->camera = cam;
	_private->viewportSize = dimension;
	_private->buildMaterialAndTriangles(scene);
	
	_private->debugMode = DebugRenderMode::RenderTriangle;
	
	size_t bounces = 0;
	return _private->raytracePixel(vec2i(pixel.x, dimension.y - pixel.y),
		_private->options.raysPerPixel, bounces);
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
	
	log::info("Rendering started: %llu", startTime);
	
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
			float r = clamp(meshMaterial->getFloat(MaterialParameter::Roughness), 0.0f, 1.0f);
			auto kA = meshMaterial->getVector(MaterialParameter::AmbientColor);
			auto kD = meshMaterial->getVector(MaterialParameter::DiffuseColor);
			mat.name = meshMaterial->name();
			mat.diffuse = rt::float4(kA + kD);
			mat.specular = rt::float4(meshMaterial->getVector(MaterialParameter::SpecularColor));
			mat.emissive = rt::float4(meshMaterial->getVector(MaterialParameter::EmissiveColor));
			mat.roughness = HALF_PI * (1.0f - std::cos(HALF_PI * r));
			mat.ior = meshMaterial->getFloat(MaterialParameter::Transparency);
		}
		
		for (const auto& rb : mesh->renderBatches())
		{
			auto& vs = rb->vertexStorage();
			auto& ia = rb->indexArray();
			if (vs.invalid() || ia.invalid()) continue;
			
			const mat4& t = mesh->finalTransform();
			
			triangles.reserve(triangles.size() + rb->numIndexes());
			
			const auto pos = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
			const auto nrm = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Normal, 0);
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
				tri.materialIndex = static_cast<rt::index>(materialIndex);
				tri.computeSupportData();
			}
		}
	}
	
	kdTree.build(triangles, options.maxKDTreeDepth, options.kdTreeSplits);
	
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
	
	vec2 pixelSize = vec2(1.0f) / vector2ToFloat(viewportSize);
	vec2 pixelBase = 2.0f * (vector2ToFloat(pixel) * pixelSize) - vec2(1.0f);

#if (USE_ITERATIVE_GATHER)
	rt::float4 result = gatherBouncesIterative(camera.castRay(pixelBase), 0, bounces);
	for (size_t m = 1; m < samples; ++m)
	{
		vec2 jitter = pixelSize * vec2(2.0f * rt::fastRandomFloat() - 1.0f, 2.0f * rt::fastRandomFloat() - 1.0f);
		result += gatherBouncesIterative(camera.castRay(pixelBase + jitter), 0, bounces);
	}
#else
	rt::float4 result = gatherBouncesRecursive(camera.castRay(pixelBase), 0, bounces);
	for (int m = 1; m < samples; ++m)
	{
		vec2 jitter = pixelSize * vec2(2.0f * fastRandomFloat() - 1.0f, 2.0f * fastRandomFloat() - 1.0f);
		result += gatherBouncesRecursive(camera.castRay(pixelBase + jitter), 0, bounces);
	}
#endif

	vec4 output = (result / static_cast<float>(samples)).toVec4();
	output.w = 1.0f;
	
	return output;
}

inline void computeDiffuseVector(const rt::float4& indidence, const rt::float4& normal,
	rt::float4& direction)
{
	direction = normal;
}

inline void computeReflectionVector(const rt::float4& indidence, const rt::float4& normal,
	rt::float4& direction)
{
	direction = rt::reflect(indidence, normal);
	if (direction.dot(normal) <= rt::Constants::epsilon)
		direction = rt::reflect(direction, normal);
}

inline void computeRefractionVector(const rt::float4& incidence, const rt::float4& normal,
	float k, float eta, rt::float4& direction)
{
	direction = incidence * eta - normal * (eta * normal.dot(incidence) + std::sqrt(k));
	if (direction.dot(normal) >= rt::Constants::minusEpsilon)
		direction = rt::reflect(direction, normal);
}

RayClass RaytracePrivate::classifyRay(rt::float4& normal, const rt::Material& mat,
	const rt::float4& inDirection, rt::float4& direction, rt::float4& output)
{
	if (mat.ior >= rt::Constants::onePlusEpsilon)
	{
		float eta = 1.001f / mat.ior;

		if (normal.dot(inDirection) >= 0.0)
		{
			normal *= -1.0f;
			eta = 1.0f / eta;
		}

		float k = rt::computeRefractiveCoefficient(inDirection, normal, eta);
		if (k >= rt::Constants::epsilon) // refract
		{
			float fresnel = rt::computeFresnelTerm(inDirection, normal, eta);
			if (rt::fastRandomFloat() >= fresnel)
			{
				// refract
				output = mat.diffuse;
				computeRefractionVector(inDirection, normal, k, eta, direction);
				return RayClass::Refracted;
			}
			else
			{
				output = mat.specular;
				computeReflectionVector(inDirection, normal, direction);
				return RayClass::Reflected;
			}
		}
		else // reflect due to total internal reflection
		{
			output = mat.specular;
			computeReflectionVector(inDirection, normal, direction);
			return RayClass::Reflected;
		}
	}
	
	if (rt::fastRandomFloat() >= mat.roughness)
	{
		// compute specular reflection
		output = mat.specular;
		computeReflectionVector(inDirection, normal, direction);
		return RayClass::Reflected;
	}
	
	// compute diffuse reflection
	output = mat.diffuse;
	computeDiffuseVector(inDirection, normal, direction);
	return RayClass::Diffuse;
}

rt::float4 RaytracePrivate::gatherBouncesIterative(const rt::Ray& inRay, size_t depth, size_t& maxDepth)
{
	auto currentRay = inRay;
	rt::float4 materialColor;
	
	FastTraverseStack bounces;
	while (bounces.size() < FastTraverseStack::MaxElements)
	{
		KDTree::TraverseResult traverse = kdTree.traverse(currentRay);
		if (traverse.triangleIndex == InvalidIndex)
		{
			bounces.emplace(sampleEnvironment(currentRay.direction), rt::float4(0.0f));
			break;
		}
		const auto& tri = kdTree.triangleAtIndex(traverse.triangleIndex);
		const auto& mat = materials[tri.materialIndex];
		
		rt::float4 clearN = tri.interpolatedNormal(traverse.intersectionPointBarycentric);
		rt::float4 roughN = rt::randomVectorOnHemisphere(clearN, mat.roughness);
		rt::float4 directionScale = clearN.dotVector(roughN);
		classifyRay(roughN, mat, currentRay.direction, currentRay.direction, materialColor);
		bounces.emplace(mat.emissive, materialColor * directionScale);
		currentRay.origin = traverse.intersectionPoint + currentRay.direction * rt::Constants::epsilon;
	}
	maxDepth = bounces.size();

	rt::float4 result(0.0f);
	do
	{
		result *= bounces.topMul();
		result += bounces.topAdd();
		bounces.pop();
	}
	while (bounces.hasSomething());

	return result;
}


rt::float4 RaytracePrivate::sampleEnvironment(const rt::float4& direction)
{
	return sampler.valid() ? sampler->sampleInDirection(direction) : vec4simd(0.0f);
	/*
	const rt::float4 ambient(40.0f / 255.0f, 58.0f / 255.0f, 72.0f / 255.0f, 1.0f);
	const rt::float4 sun(249.0f / 255.0f, 243.0f / 255.0f, 179.0f / 255.0f, 1.0f);
	const rt::float4 atmosphere(173.0f / 255.0f, 181.0f / 255.0f, 185.0f / 255.0f, 1.0f);
	rt::float4 sunDirection = rt::float4(0.0f, 1.0f, 0.0f, 0.0f);
	sunDirection.normalize();
	float t = std::max(0.0f, direction.dot(sunDirection));
	float s = 5.0f * pow(t, 32.0f) + 8.0f * pow(t, 256.0f);
	return ambient + atmosphere * std::sqrt(t) + sun * s;
	// */
}

void RaytracePrivate::estimateRegionsOrder()
{
	const float maxPossibleBounces = float(FastTraverseStack::MaxElements);
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
		float aspect = (maxPossibleBounces - float(r.estimatedBounces)) / maxPossibleBounces;
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
	for (size_t i = 0; i < 4; ++i)
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
