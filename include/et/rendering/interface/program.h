/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/rendering.h>
#include <et/rendering/base/variableset.h>
#include <et/rendering/base/vertexdeclaration.h>
#include <et/rendering/interface/textureset.h>

namespace et
{
class Program : public LoadableObject
{
public:
	ET_DECLARE_POINTER(Program);

	struct Variable
	{
		uint32_t offset = 0;
		uint32_t sizeInBytes = 0;
		uint32_t arraySize = 0;
		uint32_t enabled = 0;
	};
	using VariableMap = UnorderedMap<String, Variable>;

	struct Reflection
	{
		VertexDeclaration inputLayout;

		uint32_t objectVariablesBufferSize = 0;
		Variable objectVariables[ObjectVariable_max];

		uint32_t materialVariablesBufferSize = 0;
		Variable materialVariables[MaterialVariable_max];

		TextureSet::Reflection textures;

		void serialize(std::ostream&) const;
		bool deserialize(std::istream&);
	};

public:
	virtual void build(const std::string& source) = 0;

	const Reflection& reflection() const
		{ return _reflection; }

	void printReflection() const;

protected:
	Reflection _reflection;
};

}
