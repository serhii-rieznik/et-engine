/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/program.h>
#include <et/apiobjects/apiobjectfactory.h>

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
			const std::string& defines = std::string());

		Program::Pointer loadProgram(const std::string& file, ObjectsCache&,
			const StringList& defines);

		Program ::Pointer genProgram(const std::string& name, const std::string&, const std::string&,
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
		ProgramFactoryPrivate* _private;
	};
}
