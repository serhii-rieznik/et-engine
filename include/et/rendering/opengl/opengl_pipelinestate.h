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
	class OpenGLPipelineState : public PipelineState
	{
	public:
		ET_DECLARE_POINTER(OpenGLPipelineState);

	public:
        OpenGLPipelineState() = default;
		void build() override { }
	};
}
