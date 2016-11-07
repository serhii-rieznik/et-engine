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
		static const String& kObjectVariables()
			{ static String value("ObjectVariables"); return value; }
		static const String& kMaterialVariables()
			{ static String value("MaterialVariables"); return value; }
		static const String& kPassVariables()
			{ static String value("PassVariables"); return value; }

		struct Variable
		{
			uint32_t offset = 0;
			uint32_t size = 0;
		};
		using VariableMap = UnorderedMap<String, Variable>;

		struct Reflection
		{
			VertexDeclaration inputLayout;
			
			VariableMap passVariables;
			uint32_t passVariablesBufferSize = 0;

			VariableMap materialVariables;
			uint32_t materialVariablesBufferSize = 0;

			VariableMap objectVariables;
			uint32_t objectVariablesBufferSize = 0;

			UnorderedMap<String, uint32_t> vertexTextures;
			UnorderedMap<String, uint32_t> vertexSamplers;
			UnorderedMap<String, uint32_t> fragmentTextures;
			UnorderedMap<String, uint32_t> fragmentSamplers;
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

		PrimitiveType primitiveType() const
			{ return _primitiveType; }

		void setPrimitiveType(PrimitiveType pt)
			{ _primitiveType = pt; }

		template <typename T>
		void setObjectVariable(const String& name, const T& t)
			{ uploadObjectVariable(name, &t, sizeof(T)); }

	protected:
		Reflection reflection;
		BinaryDataStorage objectVariablesBuffer;
		void buildBuffers();
		void printReflection();

		void uploadObjectVariable(const String& name, const void* ptr, uint32_t size);

	private:
		VertexDeclaration _decl;
		RenderPass::Pointer _renderPass;
		Program::Pointer _program;
		BlendState _blend;
		DepthState _depth;
		CullMode _cull = CullMode::Disabled;
		TextureFormat _renderTargetFormat = TextureFormat::RGBA8;
		PrimitiveType _primitiveType = PrimitiveType::Triangles;
	};

	class PipelineStateCachePrivate;
	class PipelineStateCache
	{
	public:
		PipelineStateCache();
		~PipelineStateCache();
		
		PipelineState::Pointer find(const VertexDeclaration&, Program::Pointer, const DepthState&,
			const BlendState&, CullMode, TextureFormat, PrimitiveType);

		void addToCache(PipelineState::Pointer);

	private:
		ET_DECLARE_PIMPL(PipelineStateCache, 256);
	};
}
