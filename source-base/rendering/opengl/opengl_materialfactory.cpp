/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

/*
#include <et/app/application.h>
#include <et/rendering/opengl/opengl.h>
#include <et/rendering/opengl/opengl_caps.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/MaterialFactory.h>

using namespace et;

const std::string kDefaultSeparators = " ;,\t\n";

MaterialFactory::MaterialFactory()
{
}

StringList MaterialFactory::loadProgramSources(const std::string& file, std::string& vertex_shader,
	std::string& geom_shader, std::string& frag_shader, const StringList& defines)
{
	StringList sources;
	StringList resultDefines = defines;
	std::string vertex_source;
	std::string geometry_source;
	std::string fragment_source;
	
	bool shouldParseProgramFile = true;
	
	std::string filename = file;
	if (!fileExists(filename))
	{
		bool shouldFailAndReturn = true;
		
		filename = application().resolveFolderName(file);
		if (!fileExists(filename))
		{
			vertex_source = replaceFileExt(filename, ".vsh");
			if (fileExists(vertex_source))
			{
				fragment_source = replaceFileExt(filename, ".fsh");
				shouldFailAndReturn = !fileExists(fragment_source);
				shouldParseProgramFile = false;
			}
		}
		
		if (shouldFailAndReturn)
		{
			log::error("Unable to find file: %s", file.c_str());
			return sources;
		}
	}
	
	if (shouldParseProgramFile)
	{
		std::string s;
		InputStream progFile(filename, StreamMode_Binary);
		while (!(progFile.stream().eof() || progFile.stream().fail()))
		{
			getline(progFile.stream(), s);
			trim(s);
			
			std::string id = s.substr(0, s.find(':'));
			trim(id);
			lowercase(id);
			
			if (id == "vs")
				vertex_source = s.substr(s.find_first_of(':') + 1);
			
			if (id == "gs")
				geometry_source = s.substr(s.find_first_of(':') + 1);
			
			if (id == "fs")
				fragment_source = s.substr(s.find_first_of(':') + 1);
			
			if (id == "defines")
			{
				StringList aDefines = parseDefines(s.substr(s.find_first_of(':') + 1), kDefaultSeparators);
				resultDefines.insert(resultDefines.end(), aDefines.begin(), aDefines.end());
			}
		}
	}
	
	normalizeFilePath(trim(vertex_source));
	normalizeFilePath(trim(geometry_source));
	normalizeFilePath(trim(fragment_source));
	
	std::string programFolder = getFilePath(filename);

	if (!fileExists(vertex_source))
		vertex_source = programFolder + vertex_source;
	
	if (!fileExists(vertex_source))
		vertex_source = application().resolveFileName(vertex_source);
	
	if (!fileExists(vertex_source))
		vertex_source = application().resolveFileName(vertex_source);
	
	if (fileExists(vertex_source))
	{
		sources.push_back(vertex_source);
		vertex_shader = loadTextFile(vertex_source);
		ET_ASSERT((vertex_shader.size() > 1) && "Vertex shader source should not be empty");
	}
	
	if (!geometry_source.empty())
	{
		if (!fileExists(geometry_source))
			geometry_source = programFolder + geometry_source;
		
		if (!fileExists(geometry_source))
			geometry_source = application().resolveFileName(geometry_source);
		
		if (!fileExists(geometry_source))
			geometry_source = application().resolveFileName(geometry_source);
		
		if (fileExists(geometry_source))
		{
			geom_shader = loadTextFile(geometry_source);
			sources.push_back(geometry_source);
		}
	}
	
	if (!fileExists(fragment_source))
		fragment_source = programFolder + fragment_source;
	
	if (!fileExists(fragment_source))
		fragment_source = application().resolveFileName(fragment_source);
	
	if (!fileExists(fragment_source))
		fragment_source = application().resolveFileName(fragment_source);
	
	if (fileExists(fragment_source))
	{
		sources.push_back(fragment_source);
		frag_shader = loadTextFile(fragment_source);
		ET_ASSERT((frag_shader.size() > 1) && "Fragment shader source should not be empty");
	}
	
	return sources;
}

Program::Pointer MaterialFactory::loadProgram(const std::string& file, ObjectsCache& cache,
	const StringList& defines)
{
	auto cachedPrograms = cache.findObjects(file);
	for (Program::Pointer cached : cachedPrograms)
	{
		if (cached.valid())
		{
			if (cached->defines().size() == defines.size())
			{
				bool same = true;
				for (auto& inDefine : defines)
				{
					for (auto& cDefine : cached->defines())
					{
						if (inDefine != cDefine)
						{
							same = false;
							break;
						}
						
						if (!same)
							break;
					}
				}
				if (same)
					return cached;
			}
		}
	}
	
	std::string vertex_shader;
	std::string geom_shader;
	std::string frag_shader;

	StringList sourceFiles = loadProgramSources(file, vertex_shader, geom_shader, frag_shader);
	
	if (sourceFiles.empty())
	{
		return Program::Pointer::create();
	}

	std::string workFolder = getFilePath(file);
	
	parseSourceCode(ShaderType::Vertex, vertex_shader, defines, workFolder);
	parseSourceCode(ShaderType::Geometry, geom_shader, defines, workFolder);
	parseSourceCode(ShaderType::Fragment, frag_shader, defines, workFolder);
	
	Program::Pointer program = Program::Pointer::create(vertex_shader, geom_shader,
		frag_shader, getFileName(file), file, defines);
	
    cache.manage(program, ObjectLoader::Pointer());
	return program;
}

Program::Pointer MaterialFactory::loadProgram(const std::string& file, ObjectsCache& cache, const std::string& defines)
{
	return loadProgram(file, cache, parseDefines(defines, kDefaultSeparators));
}

Program::Pointer MaterialFactory::genProgram(const std::string& name, const std::string& vertexshader,
	const std::string& geometryshader, const std::string& fragmentshader, const StringList& defines,
	const std::string& workFolder)
{
	std::string vs = vertexshader;
	std::string gs = geometryshader;
	std::string fs = fragmentshader;
	
	parseSourceCode(ShaderType::Vertex, vs, defines, workFolder);
	parseSourceCode(ShaderType::Geometry, gs, defines, workFolder);
	parseSourceCode(ShaderType::Fragment, fs, defines, workFolder);
	
	return Program::Pointer::create(vs, gs, fs, name, name, defines);
}

Program ::Pointer MaterialFactory::genProgram(const std::string& name, const std::string& vertexshader,
	const std::string& fragmentshader, const StringList& defines, const std::string& workFolder)
{
	std::string vs = vertexshader;
	std::string fs = fragmentshader;
	
	parseSourceCode(ShaderType::Vertex, vs, defines, workFolder);
	parseSourceCode(ShaderType::Fragment, fs, defines, workFolder);
	
	return Program::Pointer::create(vs, emptyString, fs, name, name, defines);
}

Program::Pointer MaterialFactory::genProgramAsIs(const std::string& name, const std::string& vs, const std::string& fs,
	const StringList& defines, const std::string&)
{
	return Program::Pointer::create(vs, emptyString, fs, name, name, defines);
}

void MaterialFactory::parseSourceCode(ShaderType type, std::string& source, const StringList& defines,
	const std::string& workFolder)
{
	std::string header = _commonHeader;
	
	if (type == ShaderType::Vertex)
		header += _vertShaderHeader;
	else if (type == ShaderType::Fragment)
		header += _fragShaderHeader;

	for (const auto i : defines)
		header += "\n" + i;
	
	source = header + "\n" + source;
    
    parseIncludes(source, workFolder);
}
*/