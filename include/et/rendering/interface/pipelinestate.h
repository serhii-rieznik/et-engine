/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/rendering.h>
#include <et/rendering/vertexarrayobject.h>
#include <et/rendering/interface/program.h>
#include <et/rendering/base/vertexdeclaration.h>

namespace et
{
	class PipelineState : public Shared
	{
	public:
		ET_DECLARE_POINTER(PipelineState);

	public:
		virtual ~PipelineState() = default;

		virtual void build() = 0;

		const VertexDeclaration& inputLayout() const
			{ return _decl; }

		void setInputLayout(VertexDeclaration decl)
			{ _decl = decl; }

		VertexArrayObject::Pointer vertexStream() const
			{ return _vertexStream; }

		void setVertexStream(VertexArrayObject::Pointer vs)
			{ _vertexStream = vs; }

		const BlendState& blendState() const
			{ return _blend; }

		void setBlendState(const BlendState& bs)
			{ _blend = bs; }

		const DepthState& depthState() const
			{ return _depth; }

		void setDepthState(const DepthState& ds)
			{ _depth = ds; }

		CullMode cullMode() const
			{ return _cull; }

		void setCullMode(CullMode cm)
			{ _cull = cm; }

		Program::Pointer program() const
			{ return _program; }

		void setProgram(Program::Pointer prog)
			{ _program = prog; }

		TextureFormat renderTargetFormat() const
			{ return _renderTargetFormat; }
		
		void setRenderTargetFormat(TextureFormat fmt)
			{ _renderTargetFormat = fmt; }

	private:
		VertexDeclaration _decl;
		VertexArrayObject::Pointer _vertexStream;
		Program::Pointer _program;
		BlendState _blend;
		DepthState _depth;
		CullMode _cull = CullMode::Disabled;
		TextureFormat _renderTargetFormat = TextureFormat::RGBA8;
	};
}
