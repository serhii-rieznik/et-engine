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

	union Variable
	{
		struct
		{
			uint32_t offset : 12;
			uint32_t sizeInBytes : 12;
			uint32_t arraySize : 7;
			uint32_t enabled : 1;
		};
		uint32_t data = 0;
	};

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
	virtual void build(uint32_t stages, const std::string& source) = 0;

	const Reflection& reflection() const
		{ return _reflection; }

	void printReflection() const;

protected:
	Reflection _reflection;
};

}
