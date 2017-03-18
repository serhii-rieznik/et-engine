/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/rendering.h>
#include <et/rendering/interface/program.h>
#include <et/rendering/interface/renderpass.h>
#include <et/rendering/interface/textureset.h>
#include <et/rendering/base/vertexstream.h>
#include <et/rendering/base/vertexdeclaration.h>

namespace et
{
class PipelineState : public Shared
{
public:
	ET_DECLARE_POINTER(PipelineState);

public:
	virtual ~PipelineState() = default;

	virtual void build(const RenderPass::Pointer&) = 0;

	const VertexDeclaration& inputLayout() const
	{
		return _decl;
	}

	void setInputLayout(const VertexDeclaration& decl)
	{
		_decl = decl;
	}

	const BlendState& blendState() const
	{
		return _blend;
	}

	void setBlendState(const BlendState& bs)
	{
		_blend = bs;
	}

	const DepthState& depthState() const
	{
		return _depth;
	}

	void setDepthState(const DepthState& ds)
	{
		_depth = ds;
	}

	CullMode cullMode() const
	{
		return _cull;
	}

	void setCullMode(CullMode cm)
	{
		_cull = cm;
	}

	Program::Pointer program() const
	{
		return _program;
	}

	void setProgram(const Program::Pointer& prog)
	{
		_program = prog;
	}

	PrimitiveType primitiveType() const
	{
		return _primitiveType;
	}

	void setPrimitiveType(PrimitiveType pt)
	{
		_primitiveType = pt;
	}

	uint64_t renderPassIdentifier() const
	{
		return _renderPassId;
	}

protected:
	uint64_t _renderPassId = 0;

private:
	VertexDeclaration _decl;
	Program::Pointer _program;
	BlendState _blend;
	DepthState _depth;
	CullMode _cull = CullMode::Disabled;
	PrimitiveType _primitiveType = PrimitiveType::Triangles;
};

class PipelineStateCachePrivate;
class PipelineStateCache
{
public:
	PipelineStateCache();
	~PipelineStateCache();

	PipelineState::Pointer find(uint64_t renderPassId, const VertexDeclaration&, const Program::Pointer&, 
		const DepthState&, const BlendState&, CullMode, PrimitiveType);

	void addToCache(const RenderPass::Pointer& pass, const PipelineState::Pointer&);
	void clear();

private:
	ET_DECLARE_PIMPL(PipelineStateCache, 256);
};
}
