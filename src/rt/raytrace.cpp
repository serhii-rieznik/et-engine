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

namespace et
{
	class RaytracePrivate
	{
	public:
		enum : size_t 
		{
			InvalidIndex = -1,
		};

	public:
		RaytracePrivate(Raytrace* owner);
		~RaytracePrivate();

		void threadFunction();
		void emitWorkerThreads();
		void stopWorkerThreads();

		void buildMaterialAndTriangles(s3d::Scene::Pointer);

		size_t materialIndexWithName(const std::string&);

		size_t findIntersection(const rt::Ray&, vec4simd&);

		void buildRegions(vec2i size);

		vec4 raytracePixel(const vec2i&);

		vec4simd gatherBouncesRecursive(const rt::Ray&, size_t depth);
		vec4simd sampleEnvironment(const vec4simd& direction);

		rt::Region getNextRegion();

	public:
		std::vector<std::thread> workerThreads;
		std::atomic<bool> running;
		vec2i viewportSize = vec2i(0);
		Raytrace* owner = nullptr;
		Camera camera;

		std::vector<rt::Material> materials;
		std::vector<rt::Triangle> triangles;

		std::vector<rt::Region> regions;
		std::mutex regionsLock;
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
	_private->buildRegions(vec2i(32));
	_private->emitWorkerThreads();
}

void Raytrace::stop()
{
	_private->stopWorkerThreads();
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
	running = true;
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
	triangleEx te;

	materials.clear();
	triangles.clear();

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
			mat.roughness = 0.06125f;
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

size_t RaytracePrivate::findIntersection(const rt::Ray& ray, vec4simd& intersectionPoint)
{
	size_t returnIndex = InvalidIndex;
	size_t index = 0;
	float minDistance = std::numeric_limits<float>::max();

	for (const auto& tri : triangles)
	{
		vec4simd ip;
		if (rt::rayTriangle(ray, tri, ip))
		{
			float distance = (ip - ray.origin).dotSelf();
			if (distance < minDistance)
			{
				intersectionPoint = ip;
				returnIndex = index;
				minDistance = distance;
			}
		}
		++index;
	}

	return returnIndex;
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

	std::random_shuffle(regions.begin(), regions.end());
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
				owner->_outputMethod(pixel, raytracePixel(pixel));
				if (!running)
					return;
			}
		}
	}
	log::info("RT thread completed");
}

vec4 RaytracePrivate::raytracePixel(const vec2i& pixel)
{
	const int multisample = 256;
	vec2 pixelSize = vec2(1.0f) / vector2ToFloat(viewportSize);
	vec2 pixelBase = 2.0f * (vector2ToFloat(pixel) * pixelSize) - vec2(1.0f);

	vec4simd result(0.0f);

	for (int m = 0; m < multisample; ++m)
	{
		vec2 jitter(randomFloat(-pixelSize.x, pixelSize.x), randomFloat(-pixelSize.y, pixelSize.y));
		result += gatherBouncesRecursive(camera.castRay(pixelBase + jitter), 0);
	}

	return (result / static_cast<float>(multisample)).toVec4();
}

vec4simd RaytracePrivate::gatherBouncesRecursive(const rt::Ray& r, size_t depth)
{
	const size_t maxDepth = 16;
	const vec4simd epsilon(0.0001f);

	vec4simd intersectionPoint;
	size_t i = findIntersection(r, intersectionPoint);

	if (i == InvalidIndex)
		return sampleEnvironment(r.direction);

	const auto& tri = triangles[i];
	const auto& mat = materials[tri.materialIndex];

	vec4simd barycentric = tri.barycentric(intersectionPoint);
	vec4simd n = tri.interpolatedNormal(barycentric);

	vec4simd newDirection;
	vec4simd materialColor;
	if (randomFloat(0.0f, 1.0f) > mat.roughness) 
	{
		// compute specular reflection
		newDirection = rt::randomVectorOnHemisphere(rt::reflect(r.direction, n), HALF_PI * mat.roughness);
		if (newDirection.dot(n) < 0.0f)
		{
			vec4simd p = newDirection.crossXYZ(n);
			newDirection = rt::reflect(newDirection, p);
			newDirection.normalize();
		}
		materialColor = mat.specular;
	}
	else 
	{
		// compute diffuse reflection
		newDirection = rt::randomVectorOnHemisphere(n, HALF_PI);
		materialColor = mat.diffuse;
	}

	vec4simd nextColor(0.0f);

	if (depth < maxDepth)
		nextColor = gatherBouncesRecursive(rt::Ray(intersectionPoint + n * epsilon, newDirection), depth + 1);

	return mat.emissive + materialColor * nextColor * newDirection.dotVector(n);
}

vec4simd RaytracePrivate::sampleEnvironment(const vec4simd& direction)
{
	const vec4simd color(0.85f, 0.92f, 1.0f, 0.5f);
	const vec4simd ambient(0.215f, 0.18f, 0.1f, 0.5f);

	float t = std::max(0.0f, direction.y());
	float a = std::sqrt(t) + t * t;
	return ambient + color * vec4simd(a, a, a, 1.0f);
}
