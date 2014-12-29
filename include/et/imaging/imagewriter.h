/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/containers.h>

namespace et
{
	enum ImageFormat 
	{
		ImageFormat_PNG,
		ImageFormat_max
	};
	
	std::string extensionForImageFormat(ImageFormat);
	void setCompressionLevelForImageFormat(ImageFormat, float);

	bool writeImageToFile(const std::string& fileName, const BinaryDataStorage& data,
		const vec2i& size, int components, int bitsPerComponent, ImageFormat fmt, bool flip);
		
	bool writeImageToBuffer(BinaryDataStorage& buffer, const BinaryDataStorage& data,
		const vec2i& size, int components, int bitsPerComponent, ImageFormat fmt, bool flip);
}
