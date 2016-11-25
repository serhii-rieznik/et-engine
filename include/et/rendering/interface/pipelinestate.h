/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/rendering.h>
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

	static const String& kWorldTransform()
	{
		static String value("worldTransform"); return value;
	}
	static const String& kWorldRotationTransform()
	{
		static String value("worldRotationTransform"); return value;
	}
	static const String& kObjectVariables()
	{
		static String value("ObjectVariables"); return value;
	}
	static const String& kMaterialVariables()
	{
		static String value("MaterialVariables"); return value;
	}
	static const String& kPassVariables()
	{
		static String value("PassVariables"); return value;
	}

public:
	virtual ~PipelineState() = default;

	virtual void build() = 0;

	const RenderPass::Pointer renderPass() const
	{
		return _renderPass;
	}

	void setRenderPass(RenderPass::Pointer pass)
	{
		_renderPass = pass;
	}

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

private:
	VertexDeclaration _decl;
	RenderPass::Pointer _renderPass;
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

	PipelineState::Pointer find(const VertexDeclaration&, const Program::Pointer&, const RenderPass::Pointer&, 
        const DepthState&, const BlendState&, CullMode, PrimitiveType);

	void addToCache(const PipelineState::Pointer&);
	void clear();

private:
	ET_DECLARE_PIMPL(PipelineStateCache, 256);
};
}
