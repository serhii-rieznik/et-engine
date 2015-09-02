/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <thread>
#include <mutex>
#include <et/rt/raytrace.h>
#include <et/rt/raytraceobjects.h>
#include <et/app/application.h>

namespace et
{
	enum class DebugRenderMode
	{
		TraceSinglePoint,
		RenderTriangle
	};
	
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

		vec4simd gatherBouncesRecursive(const rt::Ray&, size_t depth, size_t& maxDepth);
		vec4simd sampleEnvironment(const vec4simd& direction);

		rt::Region getNextRegion();
		
		void renderSpacePartitioning();
		void renderKDTreeRecursive(KDTree::Node*, size_t index);
		void renderBoundingBox(const rt::BoundingBox&, const vec4& color);
		void renderLine(const vec2& from, const vec2& to, const vec4& color);
		void renderPixel(const vec2&, const vec4& color);
		vec2 projectPoint(const vec4simd&);
		
		void fillRegionWithColor(const rt::Region&, const vec4& color);
		void renderTriangle(const rt::Triangle&);
		
	public:
		Raytrace* owner = nullptr;
		Raytrace::Options options;
		KDTree kdTree;
		Camera camera;
		vec2i viewportSize = vec2i(0);
		DebugRenderMode debugMode = DebugRenderMode::TraceSinglePoint;

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

float fastRandomFloat()
{
	static const int maxValues = 1000;
	static float values[maxValues];
	static bool shouldInit = false;
	
	if (shouldInit)
	{
		for (size_t i = 0; i < maxValues; ++i)
			values[i] = randomFloat(0.0f, 1.0f);
		shouldInit = false;
	}
	
	return values[rand() % maxValues];
}

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
	std::vector<rt::Triangle> triangles;
	
	auto meshes = scene->childrenOfType(s3d::ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		auto& vs = mesh->vertexStorage();
		auto& ia = mesh->indexArray();
		if (vs.invalid() || ia.invalid()) continue;

		auto meshMaterial = mesh->material();
		size_t matIndex = materialIndexWithName(meshMaterial->name());
		if (matIndex == InvalidIndex)
		{
			matIndex = materials.size();
			materials.emplace_back();
			auto& mat = materials.back();
			mat.diffuse = vec4simd(meshMaterial->getVector(MaterialParameter_DiffuseColor));
			mat.specular = vec4simd(meshMaterial->getVector(MaterialParameter_SpecularColor));
			mat.emissive = vec4simd(meshMaterial->getVector(MaterialParameter_EmissiveColor));
			mat.roughness = meshMaterial->getFloat(MaterialParameter_Roughness);
			mat.name = meshMaterial->name();
		}

		const mat4& t = mesh->finalTransform();

		const auto pos = vs->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Position, 0);
		const auto nrm = vs->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Normal, 0);
		for (uint32_t i = 0; i < mesh->numIndexes(); i += 3)
		{
			size_t i0 = ia->getIndex(mesh->startIndex() + i + 0);
			size_t i1 = ia->getIndex(mesh->startIndex() + i + 1);
			size_t i2 = ia->getIndex(mesh->startIndex() + i + 2);

			triangles.emplace_back();
			auto& tri = triangles.back();
			tri.v[0] = vec4simd(t * pos[i0], 1.0f);
			tri.v[1] = vec4simd(t * pos[i1], 1.0f);
			tri.v[2] = vec4simd(t * pos[i2], 1.0f);
			tri.n[0] = vec4simd(t.rotationMultiply(nrm[i0]).normalized(), 0.0f);
			tri.n[1] = vec4simd(t.rotationMultiply(nrm[i1]).normalized(), 0.0f);
			tri.n[2] = vec4simd(t.rotationMultiply(nrm[i2]).normalized(), 0.0f);
			tri.materialIndex = matIndex;
			tri.computeSupportData();
		}
	}
	
	kdTree.build(triangles, options.maxKDTreeDepth, options.kdTreeSplits);
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
		// renderSpacePartitioning();
		
		auto endTime = queryContiniousTimeInMilliSeconds();
		uint64_t diff = endTime - startTime;
		
		log::info("Rendering completed: %llu, (in %llu ms, %.3g s)", endTime,
			diff, static_cast<double>(diff) / 1000.0f);
	}
}

vec4 RaytracePrivate::raytracePixel(const vec2i& pixel, size_t samples, size_t& bounces)
{
	ET_ASSERT(samples > 0);
	
	vec2 pixelSize = vec2(1.0f) / vector2ToFloat(viewportSize);
	vec2 pixelBase = 2.0f * (vector2ToFloat(pixel) * pixelSize) - vec2(1.0f);

	vec4simd result = gatherBouncesRecursive(camera.castRay(pixelBase), 0, bounces);
	for (int m = 1; m < samples; ++m)
	{
		vec2 jitter = vec2(randomFloat(-pixelSize.x, pixelSize.x),
			randomFloat(-pixelSize.y, pixelSize.y));
		result += gatherBouncesRecursive(camera.castRay(pixelBase + jitter), 0, bounces);
	}

	vec4 output = (result / static_cast<float>(samples)).toVec4();
	output.w = 1.0f;
	
	return output;
}

vec4simd RaytracePrivate::gatherBouncesRecursive(const rt::Ray& r, size_t depth, size_t& maxDepth)
{
	maxDepth = std::max(maxDepth, depth);
	const vec4simd epsilon(0.0001f);

	KDTree::TraverseResult traverse = kdTree.traverse(r);
	if (traverse.triangleIndex == InvalidIndex)
		return sampleEnvironment(r.direction);
		
	const auto& tri = kdTree.triangleAtIndex(traverse.triangleIndex);
	const auto& mat = materials[tri.materialIndex];
	
	vec4simd n = tri.interpolatedNormal(traverse.intersectionPointBarycentric);
	n.normalize();
	
	/* backface culling for camera
	if ((depth == 0) && (n.dot(r.direction) > 0.0f))
		return gatherBouncesRecursive(rt::Ray(traverse.intersectionPoint + n * epsilon, r.direction), 1, maxDepth);
	// */
	
	if (options.debugRendering)
	{
		if (debugMode == DebugRenderMode::TraceSinglePoint)
			return mat.emissive + mat.diffuse;

		if (debugMode == DebugRenderMode::RenderTriangle)
		{
			renderTriangle(tri);
			return vec4simd(0.0f);
		}
	}
	
	vec4simd newDirection;
	vec4simd materialColor;
	vec4simd colorScale;
	if (fastRandomFloat() >= mat.roughness)
	{
		// compute specular reflection
		vec4simd reflected = rt::reflect(r.direction, n);
		reflected.normalize();

		newDirection = rt::randomVectorOnHemisphere(reflected, HALF_PI * mat.roughness);

		if (newDirection.dot(n) < 0.0f)
			newDirection = rt::reflect(newDirection, newDirection.crossXYZ(n));

		newDirection.normalize();

		materialColor = mat.specular;
		colorScale = newDirection.dotVector(reflected);
	}
	else 
	{
		// compute diffuse reflection
		newDirection = rt::randomVectorOnHemisphere(n, HALF_PI);
		materialColor = mat.diffuse;
		colorScale = newDirection.dotVector(n);
	}

	if (depth <= options.maxRecursionDepth)
	{
		rt::Ray nextRay(traverse.intersectionPoint + n * epsilon, newDirection);
		vec4simd nextColor = gatherBouncesRecursive(nextRay, depth + 1, maxDepth);
		return mat.emissive + materialColor * nextColor * colorScale;
	}

	return mat.emissive;
}

vec4simd RaytracePrivate::sampleEnvironment(const vec4simd& direction)
{
	const vec4simd ambient(40.0f / 255.0f, 58.0f / 255.0f, 72.0f / 255.0f, 1.0f);
	const vec4simd sun(249.0f / 255.0f, 243.0f / 255.0f, 179.0f / 255.0f, 1.0f);
	const vec4simd atmosphere(173.0f / 255.0f, 181.0f / 255.0f, 185.0f / 255.0f, 1.0f);
	
	vec4simd sunDirection = vec4simd(0.0f, 1.0f, 0.0f, 0.0f);
	sunDirection.normalize();
	
	float t = etMax(0.0f, direction.dot(sunDirection));
	float s = 5.0f * pow(t, 32.0f) + 8.0f * pow(t, 256.0f);
	return (ambient + atmosphere * std::sqrt(t) + sun * s) * 2.0f;
	// */
}

void RaytracePrivate::estimateRegionsOrder()
{
	const size_t maxSamples = 5;
	const size_t maxEstimatedBounces = maxSamples * options.maxRecursionDepth;
	
	std::random_shuffle(regions.begin(), regions.end());
	
	for (auto& r : regions)
	{
		r.estimatedBounces = 0;
		vec4 estimatedColor(0.0);
		for (size_t i = 0; i < maxSamples; ++i)
		{
			size_t bounces = 0;
			estimatedColor += raytracePixel(r.origin + vec2i(rand() % r.size.x, rand() % r.size.y), 1, bounces);
			r.estimatedBounces += bounces;
		}
		float aspect = float(maxEstimatedBounces - r.estimatedBounces) / float(maxEstimatedBounces);
		vec4 estimatedDensity = vec4(1.0f - 0.5f * aspect * aspect, 1.0f);
		fillRegionWithColor(r, estimatedDensity * estimatedDensity);
	}
	
	std::sort(regions.begin(), regions.end(), [](const rt::Region& l, const rt::Region& r)
		{ return l.estimatedBounces > r.estimatedBounces; });
	
	emitWorkerThreads();
}

void RaytracePrivate::renderSpacePartitioning()
{
	renderBoundingBox(kdTree.root()->boundingBox, vec4(1.0f, 0.0f, 1.0f, 1.0f));
	renderKDTreeRecursive(kdTree.root(), 0);
}

void RaytracePrivate::renderKDTreeRecursive(KDTree::Node* node, size_t index)
{
	if (node)
	{
		const vec4 colorOdd(1.0f, 1.0f, 0.0f, 1.0f);
		const vec4 colorEven(0.0f, 1.0f, 1.0f, 1.0f);
		
		if ((node->left == nullptr) && (node->right == nullptr))
			renderBoundingBox(node->boundingBox, (index % 2) ? colorOdd : colorEven);
		
		renderKDTreeRecursive(node->left, index + 1);
		renderKDTreeRecursive(node->right, index + 1);
	}
}

void RaytracePrivate::renderBoundingBox(const rt::BoundingBox& box, const vec4& color)
{
	vec2 c0 = projectPoint(box.center + box.halfSize * vec4simd(-1.0f, -1.0f, -1.0f, 0.0f));
	vec2 c1 = projectPoint(box.center + box.halfSize * vec4simd( 1.0f, -1.0f, -1.0f, 0.0f));
	vec2 c2 = projectPoint(box.center + box.halfSize * vec4simd(-1.0f,  1.0f, -1.0f, 0.0f));
	vec2 c3 = projectPoint(box.center + box.halfSize * vec4simd( 1.0f,  1.0f, -1.0f, 0.0f));
	vec2 c4 = projectPoint(box.center + box.halfSize * vec4simd(-1.0f, -1.0f,  1.0f, 0.0f));
	vec2 c5 = projectPoint(box.center + box.halfSize * vec4simd( 1.0f, -1.0f,  1.0f, 0.0f));
	vec2 c6 = projectPoint(box.center + box.halfSize * vec4simd(-1.0f,  1.0f,  1.0f, 0.0f));
	vec2 c7 = projectPoint(box.center + box.halfSize * vec4simd( 1.0f,  1.0f,  1.0f, 0.0f));
	
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

vec2 RaytracePrivate::projectPoint(const vec4simd& p)
{
	return vector2ToFloat(viewportSize) *
		(vec2(0.5f, 0.5f) + vec2(0.5f, 0.5f) * camera.project(p.xyz()).xy());
}

void RaytracePrivate::fillRegionWithColor(const rt::Region& region, const vec4& color)
{
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
