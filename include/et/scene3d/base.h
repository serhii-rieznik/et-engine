/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	namespace s3d
	{
		enum class ElementType : uint32_t
		{
			Container,
			Mesh,
			Camera,
			Light,
			ParticleSystem,
			Line,
			Skeleton,

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
