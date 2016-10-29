/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/rt/raytraceobjects.h>

namespace et
{
namespace rt
{
	struct Material;

	struct ET_ALIGNED(16) BSDFSample
	{
		enum class Class : uint32_t
		{
			Diffuse,
			Reflection,
			Transmittance
		};

		enum class Direction : uint32_t
		{
			Forward,
			Backward
		};

		BSDFSample(const rt::float4& _wi, const rt::float4& _n, const Material&, Direction _dir);
		BSDFSample(const rt::float4& _wi, const rt::float4& _wo, const rt::float4& _n, const Material&, Direction _dir);

		float pdf();
		float bsdf();
		float4 evaluate();
		float4 combinedEvaluate();

		float4 Wi = float4(0.0f);
		float4 Wo = float4(0.0f);
		float4 n = float4(0.0f);
		float4 color = float4(0.0f);
		float4 h = float4(0.0f);
		float IdotN = 0.0f;
		float OdotN = 0.0f;
		float cosTheta = 0.0f;
		float eta = 0.0f;
		float fresnel = 0.0f;
		float alpha = 0.0f;
		Class cls = Class::Diffuse;
		Direction dir = Direction::Backward;
	};

	struct ET_ALIGNED(16) Material
	{
	public:
		using Collection = Vector<Material>;

		enum class Class : uint32_t
		{
			Diffuse,
			Conductor,
			Dielectric
		};

	public:
		Material(Class cl) :
			cls(cl) { }

	public:
		float4 diffuse = float4(1.0f);
		float4 specular = float4(1.0f);
		float4 emissive = float4(0.0f);
		float_type roughness = 0.0f;
		float_type ior = 0.0f;
		Class cls = Class::Diffuse;
		std::string name;
	};
}
}
