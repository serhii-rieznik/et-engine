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
#include <et-ext/rt/reconstruction.h>
#include <et-ext/rt/sampler.h>
#include <et/app/application.h>
#include <et/camera/camera.h>

#include <et/pbr/pbr.h>

namespace et
{
namespace rt
{

class ET_ALIGNED(16) RaytracePrivate
{
public:
	RaytracePrivate(Raytrace* owner);
	~RaytracePrivate();

	void backwardPathTraceThreadFunction(uint32_t index);
	void forwardPathTraceThreadFunction(uint32_t index);
	void visualizeDistributionThreadFunction(uint32_t index);
	void visualizeSamplerThreadFunction(uint32_t index);

	void emitWorkerThreads();
	void waitForCompletion();
	void stopWorkerThreads();

	void buildScene(const s3d::Scene::Pointer&);	

	void buildRegions(const vec2i& size);
	void estimateRegionsOrder();

	vec4 raytracePixel(const vec2i&, uint32_t samples, uint32_t& bounces);

	Region getNextRegion();

	void renderSpacePartitioning();
	void renderKDTreeRecursive(uint32_t nodeIndex, uint32_t index);
	void renderBoundingBox(const BoundingBox&, const vec4& color);
	void renderLine(const vec2& from, const vec2& to, const vec4& color);
	void renderPixel(const vec2&, const vec4& color);
	vec2 projectPoint(const float4&);

	void fillRegionWithColor(const Region&, const vec4& color);
	void renderTriangle(const Triangle&);

	void flushToForwardTraceBuffer(const Vector<float4>&);

public:
	Scene scene;

	Raytrace* owner = nullptr;
	EvaluateFunction evaluateFunction = nullptr;
	Camera camera;

	Vector<std::thread> workerThreads;

	Map<uint32_t, uint32_t> lightTriangleToIndex;
	Vector<Region> regions;
	Vector<float4> forwardTraceBuffer;
	TriangleList lightTriangles;

	std::mutex regionsLock;
	std::mutex forwardTraceBufferMutex;
	std::atomic<bool> running{false};
	std::atomic<uint32_t> threadCounter{0};
	std::atomic<uint32_t> processedRegions{0};
	std::atomic<uint64_t> startTime{0};
	std::atomic<uint64_t> minTimePerRegion{0};
	std::atomic<uint64_t> maxTimePerRegion{0};
	std::atomic<uint64_t> totalTimePerRegions{0};

	uint32_t totalRegions = 0;
	uint32_t flushCounter = 0;
	vec2i viewportSize;
	vec2i regionSize;
};

Raytrace::Raytrace()
{
	ET_PIMPL_INIT(Raytrace, this);
	setOutputMethod([](const vec2i&, const vec4&) {});
}

Raytrace::~Raytrace()
{
	ET_PIMPL_FINALIZE(Raytrace);
}

void Raytrace::perform(s3d::Scene::Pointer scene, const vec2i& dimension)
{
	_private->camera.getValuesFromCamera(scene->renderCamera().reference());
	_private->scene.sampler.setSamplesCount(_private->scene.options.raysPerPixel);
	_private->viewportSize = dimension;
	_private->buildScene(scene);

	ET_ASSERT(_private->viewportSize.x > 0);
	ET_ASSERT(_private->viewportSize.y > 0);

	_private->buildRegions(vec2i(static_cast<int>(_private->scene.options.renderRegionSize)));
	_private->emitWorkerThreads();
}

void Raytrace::waitForCompletion()
{
	_private->waitForCompletion();
}

vec4 Raytrace::performAtPoint(const vec2i& pixel)
{
	uint32_t bounces = 0;
	vec4 color = _private->raytracePixel(vec2i(pixel.x, pixel.y), _private->scene.options.raysPerPixel, bounces);

	log::info("Sampled color:\n\tsRGB: %.4f, %.4f, %.4f\n\tRGB: %.4f, %.4f, %.4f, (%u bounces)",
		color.x, color.y, color.z, std::pow(color.x, 2.2f), std::pow(color.y, 2.2f), std::pow(color.z, 2.2f),
		bounces);
	
	_outputMethod(pixel, color);

	return color;
}

void Raytrace::stop()
{
	_private->stopWorkerThreads();
}

void Raytrace::setOptions(const Options& options)
{
	_private->scene.options = options;
}

void Raytrace::renderSpacePartitioning()
{
	_private->renderSpacePartitioning();
}

void Raytrace::output(const vec2i& pos, const vec4& color)
{
	_outputMethod(pos, color);
}

void Raytrace::setIntegrator(EvaluateFunction eval)
{
	_private->evaluateFunction = eval;
}

bool Raytrace::running() const
{
	return _private->running;
}

void Raytrace::reportProgress()
{
	uint32_t processedRegions = _private->processedRegions.load();
	if (processedRegions == 0)
		return;

	uint64_t elapsedTime = queryContiniousTimeInMilliSeconds() - _private->startTime;
	uint64_t minTime = _private->minTimePerRegion.load();
	uint64_t maxTime = _private->maxTimePerRegion.load();
	uint64_t avgTime = elapsedTime / processedRegions;
	uint64_t remTime = (_private->totalRegions - processedRegions) * avgTime;

	log::info("[%s] region %3u / %3u, min: %llu.%03llu, max: %llu.%03llu, avg: %llu.%03llu, remaining: %llu.%03llu",
		floatToTimeStr(static_cast<float>(elapsedTime) / 1000.0f, false).c_str(),
		processedRegions, _private->totalRegions,
		minTime / 1000, minTime % 1000, maxTime / 1000, maxTime % 1000,
		avgTime / 1000, avgTime % 1000, remTime / 1000, remTime % 1000);
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
	minTimePerRegion.store(std::numeric_limits<uint64_t>::max());
	maxTimePerRegion.store(0);

	forwardTraceBuffer.clear();
	forwardTraceBuffer.resize(viewportSize.square());
	std::fill(forwardTraceBuffer.begin(), forwardTraceBuffer.end(), float4(0.0f, 0.0f, 0.0f, 0.0f));

	uint64_t totalRays = static_cast<uint64_t>(viewportSize.square()) * scene.options.raysPerPixel;
	log::info("Rendering started: %d x %d, %llu rpp, %llu total rays",
		viewportSize.x, viewportSize.y, static_cast<uint64_t>(scene.options.raysPerPixel), totalRays);

	running = true;

	if (scene.options.threads == 0)
	{
		scene.options.threads = std::thread::hardware_concurrency();
	}

	threadCounter.store(scene.options.threads);
	for (uint32_t i = 0; i < scene.options.threads; ++i)
	{
#   if (ET_RT_EVALUATE_DISTRIBUTION)
		workerThreads.emplace_back(&RaytracePrivate::visualizeDistributionThreadFunction, this, i);
#	elif (ET_RT_EVALUATE_SAMPLER)
		workerThreads.emplace_back(&RaytracePrivate::visualizeSamplerThreadFunction, this, i);
#	else
		if (scene.options.method == RaytraceMethod::ForwardLightTracing)
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

void RaytracePrivate::waitForCompletion()
{
	for (auto& t : workerThreads)
		t.join();
	workerThreads.clear();
}

void RaytracePrivate::stopWorkerThreads()
{
	running = false;
	waitForCompletion();
}

void RaytracePrivate::buildScene(const s3d::Scene::Pointer& input)
{
	Vector<Scene::SceneEntry> geometry;
	geometry.reserve(256);

	s3d::BaseElement::List objects = input->childrenOfType(s3d::ElementType::DontCare);
	for (s3d::BaseElement::Pointer obj : objects)
	{
		if (obj->type() == s3d::ElementType::Mesh)
		{
			s3d::Mesh::Pointer mesh = obj;
			for (const RenderBatch::Pointer& rb : mesh->renderBatches())
				geometry.emplace_back(rb, mesh->transform());
		}
		else if (obj->type() == s3d::ElementType::Light)
		{
			s3d::LightElement::Pointer light = obj;
			geometry.emplace_back(light->light());
		}
	}
	scene.build(geometry, input->renderCamera());
}

void RaytracePrivate::buildRegions(const vec2i& aSize)
{
	std::unique_lock<std::mutex> lock(regionsLock);
	regions.clear();

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

	totalRegions = static_cast<uint32_t>(regions.size());
	processedRegions.store(0);
}

Region RaytracePrivate::getNextRegion()
{
	std::unique_lock<std::mutex> lock(regionsLock);
	for (auto& rgn : regions)
	{
		if (rgn.sampled == false)
		{
			rgn.sampled = true;
			return rgn;
		}
	}
	return Region();
}

/*
 * Raytrace functions
 */
void RaytracePrivate::visualizeSamplerThreadFunction(uint32_t index)
{
	if (index >= 4)
		return;

	const float gap = 50.0f;
	const float yOffset = 5.0f + gap;

	if (index == 0)
	{
		const uint32_t randomBufferSize = 2000;
		const uint32_t randomBufferSamples = randomBufferSize * randomBufferSize;
		Vector<float> occurences;
		occurences.resize(randomBufferSize + 1);

		log::info("Calculating random sampler distribution...");
		float upper = 0.0f;
		float lower = std::numeric_limits<float>::max();
		for (uint32_t i = 0; i < randomBufferSamples; ++i)
		{
			float Xi = fastRandomFloat();
			uint32_t bin = static_cast<uint32_t>(static_cast<float>(randomBufferSize) * Xi);
			occurences[bin] += 1.0f;
			lower = std::min(lower, Xi);
			upper = std::max(upper, Xi);
		}
		log::info("Bounds: [%f, %f]", lower, upper);

		upper = 0.0f;
		lower = std::numeric_limits<float>::max();
		for (float occ : occurences)
		{
			lower = std::min(lower, occ);
			upper = std::max(upper, occ);
		}
		log::info("Distribution: [%f, %f]", lower, upper);

		float x = 10.0f;
		float dx = static_cast<float>(viewportSize.x - 20) / static_cast<float>(randomBufferSize);
		for (uint32_t i = 0; i + 1 < randomBufferSize; ++i, x += dx)
		{
			float y0 = 5.0f + gap * (1.0f - occurences[i+0] / (upper - lower));
			float y1 = 5.0f + gap * (1.0f - occurences[i+1] / (upper - lower));
			renderLine(vec2(x, y0), vec2(x + dx, y1), vec4(1.0f, 0.5f, 0.25f, 1.0f));
		}
	}

	vec2 vp = vector2ToFloat(viewportSize);
	float gridSize = (std::min(vp.x, vp.y - yOffset) - 3.0f * gap) / 2.0f;
	vec2 infoSize = vec2(2.0f * gridSize + 3.0f * gap);
	vec2 offset = 0.5f * (vec2(vp.x, vp.y + yOffset) - infoSize) + vec2(yOffset + gap, gap);

	vec2 tl = offset + vec2((gridSize + gap) * float(index % 2), (gridSize + gap) * float(index / 2));
	vec2 br = tl + vec2(gridSize);
	vec2 bl(tl.x, br.y);
	vec2 tr(br.x, tl.y);

	const vec4 gridColor(0.25f, 1.0f);
	const vec4 pixelColor(1.0f, 0.25f, 0.0f, 1.0f);

	renderLine(tl, tr, gridColor);
	renderLine(tl, bl, gridColor);
	renderLine(bl, br, gridColor);
	renderLine(tr, br, gridColor);

	uint32_t subdivs = 50;
	Sampler::Pointer samplers[4] =
	{
		RandomSampler::Pointer::create(subdivs * subdivs),
		UniformSampler::Pointer::create(subdivs * subdivs),
		StratifiedSampler::Pointer::create(subdivs * subdivs, 0.0f, 1.0f),
		StratifiedSampler::Pointer::create(subdivs * subdivs, 0.1f, 0.8f),
	};

	float dp = gridSize / static_cast<float>(subdivs);
	for (uint32_t r = 0; r < subdivs; ++r)
	{
		float fr = static_cast<float>(r);
		renderLine(vec2(tl.x, tl.y + fr * dp), vec2(tr.x, tl.y + fr * dp), gridColor);
	}
	for (uint32_t c = 0; c < subdivs; ++c)
	{
		float fc = static_cast<float>(c);
		renderLine(vec2(tl.x + fc * dp, tl.y), vec2(tl.x + fc * dp, bl.y), gridColor);
	}

	vec2 sample;
	while (samplers[index]->next(sample))
	{
		renderPixel(tl + sample * gridSize, pixelColor);
		renderPixel(tl + sample * gridSize + vec2(-1.0f, 0.0f), pixelColor);
		renderPixel(tl + sample * gridSize + vec2(+1.0f, 0.0f), pixelColor);
		renderPixel(tl + sample * gridSize + vec2(0.0f, +1.0f), pixelColor);
		renderPixel(tl + sample * gridSize + vec2(0.0f, -1.0f), pixelColor);
	}
}

void RaytracePrivate::visualizeDistributionThreadFunction(uint32_t index)
{
	const uint32_t sampleTestCount = 100000000;
	const uint32_t renderTestCount = 1000000;

	static float4 testDirection;

	if (index == 0)
	{
		testDirection = float4(2.0f * fastRandomFloat() - 1.0f, 2.0f * fastRandomFloat() - 1.0f, 2.0f * fastRandomFloat() - 1.0f, 0.0f);
		testDirection.normalize();
	}

	auto distribution = cosineDistribution;
	float alpha = 0.1f;

	if (index > 0)
	{
		float l = camera.position().length() / 10.0f;
		for (uint32_t i = 0; running && (i < renderTestCount); ++i)
		{
			float4 rnd = scene.sampler.sample(i, renderTestCount);
			auto n = randomVectorOnHemisphere(rnd, testDirection, distribution, alpha);
			vec2 e = projectPoint(n * l);
			renderPixel(e, vec4(1.0f, 0.01f));
		}
		return;
	}

	const uint32_t sampleCount = 1000;
	Vector<uint32_t> prob(sampleCount, 0);
	for (uint32_t i = 0; running && (i < sampleTestCount); ++i)
	{
		float4 rnd = scene.sampler.sample(i, sampleTestCount);
		auto v = randomVectorOnHemisphere(rnd, testDirection, distribution, alpha).dot(testDirection);
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

	const uint32_t raysPerIteration = static_cast<uint32_t>(viewportSize.square() / 4);

	float imagePlaneDistanceSq = 2.0f * sqr(static_cast<float>(viewportSize.x) / std::tan(camera.fieldOfView()));

	Vector<float4> localBuffer(viewportSize.square(), float4(0.0f));

	float4 cameraPos(camera.position(), 0.0f);
	float4 cameraDir(-camera.direction(), 0.0f);
	vec3 viewport = vector3ToFloat(vec3i(viewportSize, 0));

	auto projectToCamera = [&](const Ray& inRay, const KDTree::TraverseResult& hit,
		const float4& color, const float4& nrm)
	{
		float4 toCamera = cameraPos - hit.intersectionPoint;
		toCamera.normalize();

		const auto& tri = scene.kdTree.triangleAtIndex(hit.triangleIndex);
		const auto& mat = scene.materials[tri.materialIndex];
		float4 uv0 = tri.interpolatedTexCoord0(hit.intersectionPointBarycentric);
		BSDFSample sample(inRay.direction, toCamera, nrm, mat, uv0, BSDFSample::Direction::Forward);

		if (sample.OdotN <= 0.0f)
			return;

		auto projected = camera.project(hit.intersectionPoint.xyz());
		if ((projected.x * projected.x > 1.0f) || (projected.y * projected.y > 1.0f) || (projected.z * projected.z > 1.0f))
			return;

		auto backHit = scene.kdTree.traverse(Ray(cameraPos, sample.Wo * (-1.0f)));
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
		for (uint32_t ir = 0; running && (ir < raysPerIteration); ++ir)
		{
			uint32_t emitterIndex = rand() % lightTriangles.size();
			const auto& emitterTriangle = lightTriangles[emitterIndex];

			KDTree::TraverseResult source;
			source.intersectionPointBarycentric = randomBarycentric();
			source.intersectionPoint = emitterTriangle.interpolatedPosition(source.intersectionPointBarycentric);
			source.triangleIndex = lightTriangleToIndex[emitterIndex];

			float4 triangleNormal = emitterTriangle.interpolatedNormal(source.intersectionPointBarycentric);
			float4 rnd = scene.sampler.sample(ir, raysPerIteration);
			float4 sourceDir = randomVectorOnHemisphere(rnd, triangleNormal, cosineDistribution);

			float pickProb = 1.0f / static_cast<float>(lightTriangles.size());
			float area = emitterTriangle.area();

			float4 color = scene.materials[emitterTriangle.materialIndex].emissive * (area / pickProb);
			Ray currentRay(source.intersectionPoint + sourceDir * Constants::epsilon, sourceDir);

			projectToCamera(currentRay, source, color, triangleNormal);

			for (uint32_t pathLength = 0; pathLength < scene.options.maxPathLength; ++pathLength)
			{
				auto hit = scene.kdTree.traverse(currentRay);
				if (hit.triangleIndex == InvalidIndex)
				{
					break;
				}

				const auto& tri = scene.kdTree.triangleAtIndex(hit.triangleIndex);
				const auto& mat = scene.materials[tri.materialIndex];

				if (mat.emissive.dotSelf() > 0.0f)
				{
					break;
				}

				float4 nrm = tri.interpolatedNormal(hit.intersectionPointBarycentric);
				float4 uv0 = tri.interpolatedTexCoord0(hit.intersectionPointBarycentric);
				BSDFSample sample(currentRay.direction, nrm, mat, uv0, BSDFSample::Direction::Forward);

#			if (ET_RT_VISUALIZE_BRDF)
				projectToCamera(currentRay, hit, float4(sample.bsdf()), nrm);
				break;
#			else
				color *= sample.evaluate();
				projectToCamera(currentRay, hit, color, nrm);
#			endif

				currentRay.direction = sample.Wo;
				currentRay.origin = hit.intersectionPoint + currentRay.direction * Constants::epsilon;
			}
		}

		log::info("Iteration finished");
		flushToForwardTraceBuffer(localBuffer);
		std::fill(localBuffer.begin(), localBuffer.end(), float4(0.0f));
	}
	log::info("Thread finished");
}

void RaytracePrivate::backwardPathTraceThreadFunction(uint32_t threadId)
{
	DataStorage<vec4> localData(sqr(scene.options.renderRegionSize), 0);

	while (running)
	{
		auto region = getNextRegion();
		if (region.sampled == false)
			break;

		uint64_t runTime = queryContiniousTimeInMilliSeconds();

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
				float s = static_cast<float>(pixel.x) / static_cast<float>(viewportSize.x);

				float temperature = 750.0f + s * 12000.0f;
				pbr::DefaultSpectrumSamples smp;
				float* samples = smp.mutableSamples();

				pbr::SpectrumBase::blackBodyRadiation(pbr::SpectrumBase::defaultWavelengths, samples,
					pbr::SpectrumBase::WavelengthSamples, temperature);

				smp.toRGB(localData[k].xyz().data());
				localData[k] /= std::max(localData[k].x, std::max(localData[k].y, localData[k].z));

				const float keyPoints[] = { 2700.0f, 4000.0f, 6500.0f };

				for (const float p : keyPoints)
				{
					if (std::abs(temperature - p) < 5.0f)
						localData[k] = vec4(0.0f);
				}

				uint32_t bounces = 0;
				// localData[k] = raytracePixel(pixel, scene.options.raysPerPixel, bounces);
				++k;
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

		uint64_t regionTime = queryContiniousTimeInMilliSeconds() - runTime;
		minTimePerRegion = std::min(minTimePerRegion.load(), regionTime);
		maxTimePerRegion = std::max(maxTimePerRegion.load(), regionTime);
		totalTimePerRegions += regionTime;
		++processedRegions;
	}

	--threadCounter;

	if (threadCounter.load() == 0)
	{
		running = false;

		if (scene.options.renderKDTree)
			renderSpacePartitioning();

		owner->reportProgress();
		owner->renderFinished.invokeInMainRunLoop();
	}
}

vec4 RaytracePrivate::raytracePixel(const vec2i& intCoord, uint32_t samples, uint32_t& bounces)
{
	if (evaluateFunction == nullptr)
	{
		ET_FAIL("Integrator is not set");
		return vec4(0.0f);
	}

	float4 result(0.0f);
	float weight = 0.0f;
	vec2 pixelSize = vec2(1.0f) / vector2ToFloat(viewportSize);
	vec2 baseCoordinate = vector2ToFloat(intCoord);

	uint32_t rndOffset = static_cast<uint32_t>(intCoord.x + intCoord.x * intCoord.y);

	Evaluate eval;
	eval.totalRayCount = samples;
	for (eval.rayIndex = 0; eval.rayIndex < eval.totalRayCount; ++eval.rayIndex)
	{
		vec2 normalizedCoordinate = 2.0f * (baseCoordinate) * pixelSize - vec2(1.0f);
		ray3d baseRay = camera.castRay(normalizedCoordinate);
		float distanceToFocalPlane = scene.focalDistance / baseRay.direction.dot(scene.centerRay.direction);
		vec3 focalPoint = camera.position() + distanceToFocalPlane * baseRay.direction;

		float phi = fastRandomFloat() * DOUBLE_PI;
		float r = std::sqrt(fastRandomFloat());
		float uScale = std::sin(phi) * scene.options.apertureSize * r;
		float vScale = std::cos(phi) * scene.options.apertureSize * r;
		vec3 uOffset = perpendicularVector(baseRay.direction);
		vec3 vOffset = cross(uOffset, baseRay.direction);

		vec3 shiftedOrigin = camera.position() + uOffset * uScale + vOffset * vScale;
		vec3 shiftedDirection = (focalPoint - shiftedOrigin).normalize();

		float w = 1.0f;
		eval.rayIndex += rndOffset;
		result += evaluateFunction(scene, ray3d(shiftedOrigin, shiftedDirection), eval) * w;
		eval.rayIndex -= rndOffset;
		weight += w;
	}
	return vec4(result.xyz() / weight, 1.0f);
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

	std::sort(regions.begin(), regions.end(), [](const Region& l, const Region& r)
	{
		return l.estimatedBounces > r.estimatedBounces;
	});

	emitWorkerThreads();
}

void RaytracePrivate::renderSpacePartitioning()
{
	renderBoundingBox(scene.kdTree.bboxAt(0), vec4(1.0f, 0.0f, 1.0f, 1.0f));
	renderKDTreeRecursive(0, 0);
}

void RaytracePrivate::renderKDTreeRecursive(uint32_t nodeIndex, uint32_t index)
{
	const vec4 colorOdd(1.0f, 1.0f, 0.0f, 1.0f);
	const vec4 colorEven(0.0f, 1.0f, 1.0f, 1.0f);

	const auto& node = scene.kdTree.nodeAt(nodeIndex);

	if (node.axis <= MaxAxisIndex)
	{
		renderKDTreeRecursive(node.children[0], index + 1);
		renderKDTreeRecursive(node.children[1], index + 1);
	}
	else
	{
		renderBoundingBox(scene.kdTree.bboxAt(nodeIndex), (index % 2) ? colorOdd : colorEven);
	}
}

void RaytracePrivate::renderBoundingBox(const BoundingBox& box, const vec4& color)
{
	vec2 c0 = projectPoint(box.center + box.halfSize * float4(-1.0f, -1.0f, -1.0f, 0.0f));
	vec2 c1 = projectPoint(box.center + box.halfSize * float4(1.0f, -1.0f, -1.0f, 0.0f));
	vec2 c2 = projectPoint(box.center + box.halfSize * float4(-1.0f, 1.0f, -1.0f, 0.0f));
	vec2 c3 = projectPoint(box.center + box.halfSize * float4(1.0f, 1.0f, -1.0f, 0.0f));
	vec2 c4 = projectPoint(box.center + box.halfSize * float4(-1.0f, -1.0f, 1.0f, 0.0f));
	vec2 c5 = projectPoint(box.center + box.halfSize * float4(1.0f, -1.0f, 1.0f, 0.0f));
	vec2 c6 = projectPoint(box.center + box.halfSize * float4(-1.0f, 1.0f, 1.0f, 0.0f));
	vec2 c7 = projectPoint(box.center + box.halfSize * float4(1.0f, 1.0f, 1.0f, 0.0f));

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

vec2 RaytracePrivate::projectPoint(const float4& p)
{
	return vector2ToFloat(viewportSize) *
		(vec2(0.5f, 0.5f) + vec2(0.5f, 0.5f) * camera.project(p.xyz()).xy());
}

void RaytracePrivate::fillRegionWithColor(const Region& region, const vec4& color)
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

void RaytracePrivate::renderTriangle(const Triangle& tri)
{
	const vec4 lineColor(5.0f, 0.9f, 0.8f, 1.0f);

	vec2 c0 = projectPoint(tri.v[0]);
	vec2 c1 = projectPoint(tri.v[1]);
	vec2 c2 = projectPoint(tri.v[2]);
	renderLine(c0, c1, lineColor);
	renderLine(c1, c2, lineColor);
	renderLine(c2, c0, lineColor);
}

void RaytracePrivate::flushToForwardTraceBuffer(const Vector<float4>& localBuffer)
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

		vec2i px(static_cast<int>(i % viewportSize.x), static_cast<int>(i / viewportSize.x));
		owner->output(px, output);
	}
}

}
}
