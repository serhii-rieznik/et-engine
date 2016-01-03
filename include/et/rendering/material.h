/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/program.h>

namespace et
{
	class RenderState;
	class MaterialFactory;
	class Material : public et::Shared
	{
	public:
		ET_DECLARE_POINTER(Material)
		
	public:
		Material(MaterialFactory*);
		
		void loadFromJson(const std::string&, const std::string& baseFolder, ObjectsCache& cache);
		
		void enableInRenderState(RenderState&);
		
		et::Program::Pointer program() const
			{ return _program; }
		
		const DepthState& depthState() const
			{ return _depth; }
		
		const BlendState& blendState() const
			{ return _blend; }
		
		CullMode cullMode() const
			{ return _cullMode; }

	private:
		struct Property
		{
			std::string name;
			uint32_t offset = 0;
			uint32_t length = 0;
		};
		
		void addProperty(const std::string& name, uint32_t off, uint32_t len);
		
	public:
		MaterialFactory* _factory = nullptr;
		Program::Pointer _program;
		DepthState _depth;
		BlendState _blend;
		CullMode _cullMode;
	};
}
