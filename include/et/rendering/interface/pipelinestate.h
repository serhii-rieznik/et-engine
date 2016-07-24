/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/rendering.h>
#include <et/rendering/vertexdeclaration.h>
#include <et/rendering/vertexarrayobject.h>

namespace et
{
	class PipelineState : public Shared
	{
	public:
		ET_DECLARE_POINTER(PipelineState);

		struct ConstructInfo
		{
			VertexDeclaration vertexInput;
			VertexArrayObject::Pointer vertexStream;
			Map<uint32_t, Texture::Pointer> textureBinding;
			Program::Pointer program;
			BlendState blend;
			DepthState depth;
			CullMode cull = CullMode::Disabled;
		};

	public:
		PipelineState(const ConstructInfo& info) :
			_info(info) { }

		virtual ~PipelineState() = default;

		const ConstructInfo& info() const
		{
			return _info;
		}

		void setVertexStream(VertexArrayObject::Pointer vs)
		{
			_info.vertexStream = vs;
		}

	private:
		ConstructInfo _info;
	};
}
