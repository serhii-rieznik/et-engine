/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/material.h>

namespace et
{
/*
	class MaterialFactory : public Shared
	{
	public:
		ET_DECLARE_POINTER(MaterialFactory);
		
	public:
		MaterialFactory();

		Program::Pointer loadProgram(const std::string& file, ObjectsCache&,
			const std::string& defines = emptyString);

		Program::Pointer loadProgram(const std::string& file, ObjectsCache&,
			const StringList& defines);

		Program ::Pointer genProgram(const std::string& name, const std::string&, const std::string&,
			const StringList& defines = StringList(), const std::string& workFolder = ".");
		
		Program::Pointer genProgramAsIs(const std::string& name, const std::string&, const std::string&,
			const StringList& defines = StringList(), const std::string& workFolder = ".");

		Program ::Pointer genProgram(const std::string& name, const std::string& vs, const std::string& gs,
			const std::string& fs, const StringList& defines = StringList(), const std::string& workFolder = ".");
		
	private:
		enum class ShaderType : uint32_t
		{
			Vertex,
			Geometry,
			Fragment,
		};
		
	private:
		ET_DENY_COPY(MaterialFactory)
		friend class MaterialFactoryPrivate;
		
		void parseSourceCode(ShaderType type, std::string& code,
			const StringList& defines, const std::string& workFolder);
		
		StringList loadProgramSources(const std::string&, std::string&, std::string&, std::string&,
			const StringList& defines = StringList());
		
	private:
		ObjectLoader::Pointer _objectLoader;
	};
*/ 
}
