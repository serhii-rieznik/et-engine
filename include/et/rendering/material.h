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
	class MaterialFactory;
	class Material : public et::Shared
	{
	public:
		ET_DECLARE_POINTER(Material)
		
	public:
		Material(MaterialFactory*);
		
		void loadFromJson(const std::string&, const std::string& baseFolder, ObjectsCache& cache);
		
		et::Program::Pointer program() const
			{ return _program; }
		
		const DepthState& depthState() const
			{ return _depth; }
		
		const BlendState& blendState() const
			{ return _blend; }
		
		CullMode cullMode() const
			{ return _cullMode; }
		
	public:
		MaterialFactory* _factory = nullptr;
		Program::Pointer _program;
		DepthState _depth;
		BlendState _blend;
		CullMode _cullMode;
	};
}
