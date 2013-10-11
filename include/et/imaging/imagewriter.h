/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/geometry/geometry.h>

namespace et
{
	enum ImageFormat 
	{
		ImageFormat_PNG,
	};

	class ImageWriter
	{
	public:
		static std::string extensionForImageFormat(ImageFormat fmt);

		static bool writeImageToFile(const std::string& fileName, const BinaryDataStorage& data,
									 const vec2i& size, int components, int bitsPerComponent, ImageFormat fmt, bool flip);
		
		static bool writeImageToBuffer(BinaryDataStorage& buffer, const BinaryDataStorage& data,
									 const vec2i& size, int components, int bitsPerComponent, ImageFormat fmt, bool flip);
	};
}