/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/program.h>
#include <et/rendering/apiobjectfactory.h>

namespace et
{
	class ProgramFactoryPrivate;
	class ProgramFactory : public APIObjectFactory
	{
	public:
		ET_DECLARE_POINTER(ProgramFactory)
		
	public:
		ProgramFactory(RenderContext* rc);
		~ProgramFactory();

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
		enum ShaderType
		{
			ShaderType_Vertex,
			ShaderType_Geometry,
			ShaderType_Fragment,
		};
		
	private:
		ET_DENY_COPY(ProgramFactory)
		friend class ProgramFactoryPrivate;
		
		void parseSourceCode(ShaderType type, std::string& code,
			const StringList& defines, const std::string& workFolder);
		
		void reloadObject(LoadableObject::Pointer, ObjectsCache&);
		
		StringList loadProgramSources(const std::string&, std::string&, std::string&, std::string&,
			const StringList& defines = StringList());
		
	private:
		ObjectLoader::Pointer _objectLoader;
		std::string _commonHeader;
		std::string _fragShaderHeader;
		std::string _vertShaderHeader;
		
		ET_DECLARE_PIMPL(ProgramFactory, 32)
	};
}
