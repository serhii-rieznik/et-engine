/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/indexbufferdata.h>

namespace et
{
	typedef IntrusivePtr<IndexBufferData> IndexBuffer;
	typedef std::vector<IndexBuffer> IndexBufferList;
}
