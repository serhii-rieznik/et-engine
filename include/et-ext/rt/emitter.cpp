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
	_color(color)
{
}

EmitterInteraction UniformEmitter::sample(const Scene&, const float4& position, const float4& normal) const
{
	EmitterInteraction result;
	result.direction = normal;
	result.sample = _color;
	return result;
}

EnvironmentEmitter::EnvironmentEmitter(const Image::Pointer& img) : 
	_image(img)
{
}

EmitterInteraction EnvironmentEmitter::sample(const Scene&, const float4& position, const float4& normal) const
{
	EmitterInteraction result;
	result.direction = normal;
	result.sample = float4(0.25, 0.75, 1.0, 1.0);
	return result;
}

MeshEmitter::MeshEmitter(index firstTriangle, index numTriangles, index materialIndex) 
	: _firstTriangle(firstTriangle), _numTriangles(numTriangles), _materialIndex(materialIndex)
{

}

EmitterInteraction MeshEmitter::sample(const Scene& scene, const float4& position, const float4&) const
{
	EmitterInteraction result;

	uint32_t emitterIndex = _firstTriangle + rand() % _numTriangles;
	const Triangle& emitterTriangle = scene.kdTree.triangleAtIndex(emitterIndex);
	float4 bc = randomBarycentric();
	float4 emitterPos = emitterTriangle.interpolatedPosition(bc) + float4(0.0f, 0.0f, 0.0f, 1.0f);
	
	result.normal = emitterTriangle.interpolatedNormal(bc);
	result.direction = normalize(emitterPos - position);

	float NdotD = result.normal.dot(result.direction);
	if (NdotD > 0.0f)
		return result; // from behind

	KDTree::TraverseResult hit = scene.kdTree.traverse(Ray(position, result.direction));
	if (hit.triangleIndex != emitterIndex)
		return result; // miss

	float area = emitterTriangle.area();
	float pdf = 1.0f / area;

	result.sample = scene.materials.at(_materialIndex).emissive / pdf;
	return result;
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
	float_type phi = 0.5f + std::atan2(r.cZ(), r.cX()) / DOUBLE_PI;
	float_type theta = 0.5f + std::asin(r.cY()) / PI;
	
	vec2 tc(phi * _data->size.x, theta * _data->size.y);
	vec2i baseTexCoord(static_cast<int>(tc.x), static_cast<int>(tc.y));
	
    float4 c00 = sampleTexture(baseTexCoord); ++baseTexCoord.x;
    float4 c01 = sampleTexture(baseTexCoord); ++baseTexCoord.y;
    float4 c11 = sampleTexture(baseTexCoord); --baseTexCoord.x;
    float4 c10 = sampleTexture(baseTexCoord);
	
    float_type dx = tc.x - std::floor(tc.x);
    float_type dy = tc.y - std::floor(tc.y);
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
	float_type dp = std::max(0.0f, inDirection.dot(_direction));
    return _color * std::pow(dp, 128.0f);
}
*/
}
}
