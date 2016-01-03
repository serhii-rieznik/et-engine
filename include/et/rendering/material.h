/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <unordered_map>
#include <et/rendering/program.h>

namespace et
{
	class Material : public et::Shared
	{
	public:
		ET_DECLARE_POINTER(Material)
		
	public:
		Material(RenderContext*);
		
		void loadFromJson(const std::string&, ObjectsCache& cache);
		
		et::Program::Pointer program() const
			{ return _program; }
		
		const DepthState& depthState() const
			{ return _depth; }
		
		const BlendState& blendState() const
			{ return _blend; }
		
	public:
		RenderContext* _rc = nullptr;
		Program::Pointer _program;
		DepthState _depth;
		BlendState _blend;
	};
}
