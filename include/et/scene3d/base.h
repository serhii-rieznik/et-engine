/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	namespace s3d
	{
		enum Flags : uint32_t
		{
			Flag_Renderable = 0x0001,
			Flag_HasAnimations = 0x0002
		};

		enum class ElementType : uint32_t
		{
			Container,
			Mesh,
			SupportMesh,
			Camera,
			Light,
			ParticleSystem,

			max,
			DontCare
		};

		enum : uint32_t
		{
			ElementType_First = static_cast<uint32_t>(ElementType::Container),
			ElementType_Max = static_cast<uint32_t>(ElementType::max),
		};
	}
}
