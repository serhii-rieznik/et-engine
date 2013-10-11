/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/sound/sounddescription.h>

namespace et
{
	namespace audio
	{
		Description::Pointer loadWAVFile(const std::string&);
		Description::Pointer loadCAFFile(const std::string&);
		Description::Pointer loadAIFFile(const std::string&);

		Description::Pointer loadFile(const std::string&);
		
		size_t openALFormatFromChannelsAndBitDepth(size_t numChannels, size_t bitDepth);
	}
}
