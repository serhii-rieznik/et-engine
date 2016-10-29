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
#include <et/rendering/base/vertexstream.h>
#include <et/rendering/base/vertexdeclaration.h>

namespace et
{
	class PipelineState : public Shared
	{
	public:
		ET_DECLARE_POINTER(PipelineState);

		static const String& kWorldTransform()
			{ static String value("worldTransform"); return value; }

		static const String& kWorldRotationTransform()
			{ static String value("worldRotationTransform"); return value; }

		struct Variable
		{
			uint32_t offset = 0;
			uint32_t size = 0;
		};
		using VariableMap = UnorderedMap<String, Variable>;

		struct Reflection
		{
			UnorderedMap<String, uint32_t> vertexTextures;
			UnorderedMap<String, uint32_t> vertexSamplers;
			UnorderedMap<String, uint32_t> fragmentTextures;
			UnorderedMap<String, uint32_t> fragmentSamplers;
			VariableMap passVariables;
			VariableMap materialVariables;
			VariableMap objectVariables;
		};

	public:
		virtual ~PipelineState() = default;

		virtual void build() = 0;

		const RenderPass::Pointer renderPass() const 
			{ return _renderPass; }

		void setRenderPass(RenderPass::Pointer pass) 
			{ _renderPass = pass;  }

		const VertexDeclaration& inputLayout() const
			{ return _decl; }

		void setInputLayout(const VertexDeclaration& decl)
			{ _decl = decl; }

		VertexStream::Pointer vertexStream() const
			{ return _vertexStream; }

		void setVertexStream(VertexStream::Pointer vs)
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

	protected:
		Reflection reflection;
		void printReflection();

	private:
		VertexDeclaration _decl;
		RenderPass::Pointer _renderPass;
		VertexStream::Pointer _vertexStream;
		Program::Pointer _program;
		BlendState _blend;
		DepthState _depth;
		CullMode _cull = CullMode::Disabled;
		TextureFormat _renderTargetFormat = TextureFormat::RGBA8;
	};

	class PipelineStateCachePrivate;
	class PipelineStateCache
	{
	public:
		PipelineStateCache();
		~PipelineStateCache();
		
		PipelineState::Pointer find(const VertexDeclaration&, VertexStream::Pointer, Program::Pointer,
									const DepthState&, const BlendState&, CullMode, TextureFormat);

		void addToCache(PipelineState::Pointer);

	private:
		ET_DECLARE_PIMPL(PipelineStateCache, 256);
	};
}
