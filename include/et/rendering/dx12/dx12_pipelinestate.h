/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/pipelinestate.h>

namespace et
{
	class DX12PipelineState : public PipelineState
	{
	public:
		ET_DECLARE_POINTER(DX12PipelineState);

	public:
		void build();
	};
}
