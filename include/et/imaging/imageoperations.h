/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>

namespace et
{
	class PixelFilter
	{
	public:
		virtual void applyRGBA(vec4ub& pixel, void* context) = 0;
		virtual ~PixelFilter() { }
	};

	enum ImageBlendType
	{
		ImageBlendType_Default,
		ImageBlendType_Additive
	};

	enum ImageBlurType
	{
		ImageBlurType_Average,
		ImageBlurType_Linear
	};

	enum ImageFilteringType
	{
		ImageFilteringType_Nearest,
		ImageFilteringType_Linear
	};

	class ImageOperations
	{
	public:
		static mat3i matrixFilterBlur;
		static mat3i matrixFilterSharpen;
		static mat3i matrixFilterStrongBlur;

		static void transfer(const BinaryDataStorage& src, const vec2i& srcSize, int srcComponents,
			BinaryDataStorage& dst, const vec2i& dstSize, int dstComponents, const vec2i& position);

		static void draw(const BinaryDataStorage& src, const vec2i& srcSize, int srcComponents,
			BinaryDataStorage& dst, const vec2i& dstSize, int dstComponents, const recti& destRect,
			ImageBlendType blend, ImageFilteringType filter);

		static void fill(BinaryDataStorage& dst, const vec2i& dstSize, int dstComponents, const recti& r, const vec4ub& color);

		static void applyPixelFilter(BinaryDataStorage& data, const vec2i& size, int components, PixelFilter* filter, void* context);
		static void applyMatrixFilter(BinaryDataStorage& data, const vec2i& size, int components, const mat3i& m);

		static void blur(BinaryDataStorage& data, const vec2i& size, int components, vec2i direction, int radius, ImageBlurType type);
		static void median(BinaryDataStorage& data, const vec2i& size, int components, int radius);

		static void normalMapFilter(BinaryDataStorage& data, const vec2i& size, int components, const vec2& scale);

	};
}
