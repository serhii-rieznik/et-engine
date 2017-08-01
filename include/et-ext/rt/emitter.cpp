/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/emitter.h>

namespace et
{
namespace rt
{

UniformEmitter::UniformEmitter(const float4& color) : 
	Emitter(Emitter::Type::Uniform), _color(color)
{
}

float4 UniformEmitter::samplePoint(const Scene& scene) const
{
	return randomVectorOnHemisphere(float4(0.0f), rand() % 2 ? float4(0.0f, 1.0f, 0.0f, 0.0f) : float4(0.0f, -1.0f, 0.0f, 0.0f), uniformDistribution);
}

float UniformEmitter::pdf(const float4& position, const float4& direction, const float4& lightPosition, const float4& lightNormal) const
{
	return 0.0f;
}

float4 UniformEmitter::evaluate(const Scene& scene, const float4& position, const float4& direction,
	float4& nrm, float4& pos, float& pdf) const
{
	float4 result(0.0f);
	KDTree::TraverseResult hit = scene.kdTree.traverse(Ray(position, direction));
	if (hit.triangleIndex == InvalidIndex)
	{
		pdf = 1.0f;
		pos = position + direction * std::numeric_limits<float>::max();
		nrm = direction * (-1.0f);
		result = _color;
	}
	return result;
}

EnvironmentEmitter::EnvironmentEmitter(const Image::Pointer& img) :
	Emitter(Emitter::Type::Environment), _image(img)
{
}

MeshEmitter::MeshEmitter(uint32_t firstTriangle, uint32_t numTriangles, uint32_t materialIndex)
	: Emitter(Emitter::Type::Area), _firstTriangle(firstTriangle), _numTriangles(numTriangles), _materialIndex(materialIndex)
{

}

void MeshEmitter::prepare(const Scene& scene)
{
	for (uint32_t i = 0; i < _numTriangles; ++i)
	{
		_area += scene.kdTree.triangleAtIndex(_firstTriangle + i).area();
	}
}

float4 MeshEmitter::samplePoint(const Scene& scene) const
{
	const Triangle& emitterTriangle = scene.kdTree.triangleAtIndex(_firstTriangle + rand() % _numTriangles);
	float4 bc = randomBarycentric();
	return emitterTriangle.interpolatedPosition(bc) + float4(0.0f, 0.0f, 0.0f, 1.0f);
}

float4 MeshEmitter::evaluate(const Scene& scene, const float4& position, const float4& direction, 
	float4& nrm, float4& pos, float& pdfOut) const
{
	float4 result(0.0f);

	KDTree::TraverseResult hit = scene.kdTree.traverse(Ray(position, direction));
	if (containsTriangle(hit.triangleIndex))
	{
		const Triangle& hitTriangle = scene.kdTree.triangleAtIndex(hit.triangleIndex);
		nrm = hitTriangle.interpolatedNormal(hit.intersectionPointBarycentric);
		pos = hit.intersectionPoint;

		pdfOut = pdf(position, direction, pos, nrm);
		result = scene.materials[_materialIndex].emissive;
	}

	return result;
}

float MeshEmitter::pdf(const float4& position, const float4& direction, const float4& lightPosition, const float4& lightNormal) const
{
	float cosTheta = -lightNormal.dot(direction);

	return (cosTheta < Constants::epsilon) ? 0.0f :
		(position - lightPosition).dotSelf() / (_area * cosTheta); 
}

/*
EnvironmentEquirectangularMapSampler::EnvironmentEquirectangularMapSampler(
	TextureDescription::Pointer data, const float4& scale) : _data(data), _scale(scale)
{
	if (bitsPerPixelForTextureFormat(_data->format) != 128)
	{
		ET_FAIL("Only RGBA32F textures are supported at this time")
	}
    
    _rawData = reinterpret_cast<vec4*>(_data->data.binary());
    _textureSize = _data->size;
}

float4 EnvironmentEquirectangularMapSampler::sampleTexture(vec2i texCoord)
{
	{
		while (texCoord.x >= _data->size.x) texCoord.x -= _data->size.x;
		while (texCoord.y >= _data->size.y) texCoord.y -= _data->size.y;
		while (texCoord.x < 0) texCoord.x += _data->size.x;
		while (texCoord.y < 0) texCoord.y += _data->size.y;
	}
	
	return float4(_rawData[texCoord.x + texCoord.y * _textureSize.x]);
}

float4 EnvironmentEquirectangularMapSampler::sampleInDirection(const float4& r)
{
	float phi = 0.5f + std::atan2(r.cZ(), r.cX()) / DOUBLE_PI;
	float theta = 0.5f + std::asin(r.cY()) / PI;
	
	vec2 tc(phi * _data->size.x, theta * _data->size.y);
	vec2i baseTexCoord(static_cast<int>(tc.x), static_cast<int>(tc.y));
	
    float4 c00 = sampleTexture(baseTexCoord); ++baseTexCoord.x;
    float4 c01 = sampleTexture(baseTexCoord); ++baseTexCoord.y;
    float4 c11 = sampleTexture(baseTexCoord); --baseTexCoord.x;
    float4 c10 = sampleTexture(baseTexCoord);
	
    float dx = tc.x - std::floor(tc.x);
    float dy = tc.y - std::floor(tc.y);
    float4 cx1 = c00 * (1.0f - dx) + c01 * dx;
	float4 cx2 = c10 * (1.0f - dx) + c11 * dx;

    return _scale * (cx1 * (1.0f - dy) + cx2 * dy);
}

DirectionalLightSampler::DirectionalLightSampler(const float4& direction, const float4& color) :
	_color(color), _direction(direction * float4(1.0f, 1.0f, 1.0f, 0.0f))
{
	_direction.normalize();
}

float4 DirectionalLightSampler::sampleInDirection(const float4& inDirection)
{
	float dp = std::max(0.0f, inDirection.dot(_direction));
    return _color * std::pow(dp, 128.0f);
}
*/
}
}
