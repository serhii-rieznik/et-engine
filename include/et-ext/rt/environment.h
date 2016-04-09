/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/imaging/texturedescription.h>
#include <et-ext/rt/raytraceobjects.h>

namespace et
{
namespace rt
{
	class EnvironmentSampler : public Shared
	{
	public:
		ET_DECLARE_POINTER(EnvironmentSampler);
		
	public:
		virtual ~EnvironmentSampler() { }
		virtual float4 sampleInDirection(const float4&) = 0;
	};
	
	class EnvironmentColorSampler : public EnvironmentSampler
	{
	public:
		ET_DECLARE_POINTER(EnvironmentColorSampler);
		
	public:
		EnvironmentColorSampler(const float4& color) :
			_color(color) { }
		
		float4 sampleInDirection(const rt::float4&)
			{ return _color; }
		
	private:
		float4 _color;
	};

	class DirectionalLightSampler : public EnvironmentSampler
	{
	public:
		ET_DECLARE_POINTER(DirectionalLightSampler);

	public:
		DirectionalLightSampler(const float4& color, const float4& direction);
		float4 sampleInDirection(const float4&);

	private:
		float4 _color;
		float4 _direction;
	};
	
	class EnvironmentEquirectangularMapSampler : public EnvironmentSampler
	{
	public:
		ET_DECLARE_POINTER(EnvironmentEquirectangularMapSampler);
		
	public:
		EnvironmentEquirectangularMapSampler(TextureDescription::Pointer, const float4& scale);
		
		float4 sampleInDirection(const float4&);
		
	private:
		float4 sampleTexture(vec2i);
		
	private:
		TextureDescription::Pointer _data;
		float4 _scale = float4(1.0f);
        vec2i _textureSize;
        vec4* _rawData = nullptr;
	};
}
}
