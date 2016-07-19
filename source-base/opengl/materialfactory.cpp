/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/MaterialFactory.h>

using namespace et;

class et::MaterialFactoryPrivate 
{
public:
	struct Loader : public ObjectLoader
	{
		MaterialFactory* owner = nullptr;

		Loader(MaterialFactory* aOwner) :
			owner(aOwner) { }

		void reloadObject(LoadableObject::Pointer o, ObjectsCache& c)
			{ owner->reloadObject(o, c); }
	};

	IntrusivePtr<Loader> loader;

	MaterialFactoryPrivate(MaterialFactory* owner) : 
		loader(etCreateObject<Loader>(owner)) { }
};

StringList parseDefinesString(std::string defines, std::string separators = ",; \t");

extern const std::string openGl2VertexHeader;
extern const std::string openGl3VertexHeader;

extern const std::string openGl2FragmentHeader;
extern const std::string openGl3FragmentHeader;

MaterialFactory::MaterialFactory(RenderContext* rc) : APIObjectFactory(rc)
{
	ET_PIMPL_INIT(MaterialFactory, this)

#if (ET_OPENGLES)	
	_commonHeader =
		"#define etLowp		lowp\n"
		"#define etMediump		mediump\n"
		"#define etHighp		highp\n"
		"#define ET_OPENGL_ES	1\n";
	
	if (OpenGLCapabilities::instance().version() >= OpenGLVersion::Version_3x)
	{
		_commonHeader = "#version " + OpenGLCapabilities::instance().glslVersionShortString() + " es\n" +
			_commonHeader + "#define ET_OPENGL_ES_3\n";
	}
	else
	{
		_commonHeader += "#define ET_OPENGL_ES_2\n";
	}
	
#else
	_commonHeader = 
		"#version " + OpenGLCapabilities::instance().glslVersionShortString() + "\n";

	if (OpenGLCapabilities::instance().glslVersionShortString() < "130")
		_commonHeader += "#extension GL_EXT_gpu_shader4 : enable\n";

	_commonHeader +=
		"#define etLowp\n"
		"#define etMediump\n"
		"#define etHighp\n";
#endif

	if (OpenGLCapabilities::instance().version() == OpenGLVersion::Version_2x)
	{
		_vertShaderHeader = openGl2VertexHeader;
		_fragShaderHeader = openGl2FragmentHeader;
	}
	else
	{
		_vertShaderHeader = openGl3VertexHeader;
		_fragShaderHeader = openGl3FragmentHeader;
	}
}

MaterialFactory::~MaterialFactory()
{
	ET_PIMPL_FINALIZE(MaterialFactory)
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
				StringList aDefines = parseDefinesString(s.substr(s.find_first_of(':') + 1));
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
		return Program::Pointer::create(renderContext());
	
	std::string workFolder = getFilePath(file);
	
	parseSourceCode(ShaderType::Vertex, vertex_shader, defines, workFolder);
	parseSourceCode(ShaderType::Geometry, geom_shader, defines, workFolder);
	parseSourceCode(ShaderType::Fragment, frag_shader, defines, workFolder);
	
	Program::Pointer program = Program::Pointer::create(renderContext(), vertex_shader, geom_shader,
		frag_shader, getFileName(file), file, defines);
	
	for (auto& s : sourceFiles)
		program->addOrigin(s);
	
	cache.manage(program, _private->loader);
	return program;
}

Program::Pointer MaterialFactory::loadProgram(const std::string& file, ObjectsCache& cache, const std::string& defines)
{
	return loadProgram(file, cache, parseDefinesString(defines));
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
	
	return Program::Pointer::create(renderContext(), vs, gs, fs, name, name, defines);
}

Program ::Pointer MaterialFactory::genProgram(const std::string& name, const std::string& vertexshader,
	const std::string& fragmentshader, const StringList& defines, const std::string& workFolder)
{
	std::string vs = vertexshader;
	std::string fs = fragmentshader;
	
	parseSourceCode(ShaderType::Vertex, vs, defines, workFolder);
	parseSourceCode(ShaderType::Fragment, fs, defines, workFolder);
	
	return Program::Pointer::create(renderContext(), vs, emptyString, fs, name, name, defines);
}

Program::Pointer MaterialFactory::genProgramAsIs(const std::string& name, const std::string& vs, const std::string& fs,
	const StringList& defines, const std::string&)
{
	return Program::Pointer::create(renderContext(), vs, emptyString, fs, name, name, defines);
}

void MaterialFactory::parseSourceCode(ShaderType type, std::string& source, const StringList& defines,
	const std::string& workFolder)
{
	if (source.empty()) return;
	
	std::string header = _commonHeader;
	
	if (type == ShaderType::Vertex)
		header += _vertShaderHeader;
	else if (type == ShaderType::Fragment)
		header += _fragShaderHeader;

	for (const auto i : defines)
		header += "\n" + i;
	
	source = header + "\n" + source;

	std::string::size_type ip = source.find("#include");

	bool hasIncludes = ip != std::string::npos;
	while (hasIncludes)
	{
		while (ip != std::string::npos)
		{
			std::string before = source.substr(0, ip);
			
			source.erase(0, before.size());
			std::string ifname = source.substr(0, source.find_first_of(char(10)));
			source.erase(0, ifname.size());
			std::string after = source.substr();
			
			if (ifname.find_first_of('"') != std::string::npos)
			{
				ifname.erase(0, ifname.find_first_of('"') + 1);
				ifname.erase(ifname.find_last_of('"'));
			}
			else 
			{
				ifname.erase(0, ifname.find_first_of('<') + 1);
				ifname.erase(ifname.find_last_of('>'));
			}
			
			std::string include = "";
			
			std::string baseName = removeUpDir(workFolder + ifname);
			while (baseName.find("..") != std::string::npos)
				baseName = removeUpDir(baseName);
			
			if (fileExists(baseName))
			{
				include = loadTextFile(baseName);
			}
			else
			{
				log::error("failed to include %s, starting from folder %s", ifname.c_str(), workFolder.c_str());
			}
			
			source = before + include + after;
			ip = source.find("#include");
		}
		
		hasIncludes = ip != std::string::npos;
	}
}

void MaterialFactory::reloadObject(LoadableObject::Pointer obj, ObjectsCache&)
{
	std::string vertex_shader;
	std::string geom_shader;
	std::string frag_shader;
	
	StringList sourceFiles = loadProgramSources(obj->origin(), vertex_shader, geom_shader, frag_shader);
	if (sourceFiles.empty()) return;
	
	// TODO: handle defines
	std::string workFolder = getFilePath(obj->origin());
	parseSourceCode(ShaderType::Vertex, vertex_shader, StringList(), workFolder);
	parseSourceCode(ShaderType::Geometry, geom_shader, StringList(), workFolder);
	parseSourceCode(ShaderType::Fragment, frag_shader, StringList(), workFolder);
	
	Program::Pointer(obj)->buildProgram(vertex_shader, geom_shader, frag_shader);
}

Material::Pointer MaterialFactory::createMaterial()
{
	return Material::Pointer::create(this);
}

Material::Pointer MaterialFactory::loadMaterial(const std::string& fileName)
{
	Material::Pointer result = createMaterial();
	result->loadFromJson(loadTextFile(fileName), getFilePath(fileName));
	return result;
}

/*
 *
 * Service stuff
 *
 */

StringList parseDefinesString(std::string defines, std::string separators)
{
	StringList result;
	
	auto addDefine = [&result](std::string& define)
	{
		if (define.empty()) return;
		
		std::transform(define.begin(), define.end(), define.begin(), [](char c)
			{ return (c == '=') ? ' ' : c; });
		result.push_back("#define " + define + "\n");
	};
	
	if (separators.length() == 0)
		separators = ",; \t";

	while (defines.length() > 0)
	{
		std::string::size_type separator_pos = std::string::npos;

		for (auto& s_i : separators)
		{
			std::string::size_type s = defines.find_first_of(s_i);
			if (s != std::string::npos)
			{
				separator_pos = s;
				break;
			}
		}

		if (separator_pos == std::string::npos)
		{
			addDefine(defines);
			break;
		}
		else
		{
			std::string define = defines.substr(0, separator_pos);
			defines.erase(0, separator_pos + 1);
			addDefine(define);
		}
	}
	
	return result;
}

const std::string openGl2VertexHeader = R"(
#define etShadow2D			shadow2D
#define etTexture2D			texture2D
#define etTextureCube		textureCube
#define etShadow2DProj		shadow2DProj
#define etTexture2DProj		texture2DProj
#define etTexture2DLod		texture2DLod
#define etTexture2DArray	texture2DArray
#define etTextureCubeLod	textureCubeLod
#define etTextureRect		texture2DRect
#define etVertexIn			attribute
#define etVertexOut			varying
)";

const std::string openGl3VertexHeader = R"(
#define etShadow2D			texture
#define etTexture2D			texture
#define etTextureCube		texture
#define etTextureRect		texture
#define etTexture2DArray	texture
#define etShadow2DProj		textureProj
#define etTexture2DProj		textureProj
#define etTexture2DLod		textureLod
#define etTextureCubeLod	textureLod
#define etVertexIn			in
#define etVertexOut			out
)";

const std::string openGl2FragmentHeader = R"(
#define etShadow2D			shadow2D
#define etTexture2D			texture2D
#define etTextureCube		textureCube
#define etShadow2DProj		shadow2DProj
#define etTexture2DProj		texture2DProj
#define etTexture2DLod		textureLod
#define etTexture2DArray	texture2DArray
#define etTextureCubeLod	textureCubeLod
#define etTextureRect		texture2DRect
#define etFragmentIn		varying
#define etFragmentOut		gl_FragColor
#define etFragmentOut0		gl_FragData[0]
#define etFragmentOut1		gl_FragData[1]
#define etFragmentOut2		gl_FragData[2]
#define etFragmentOut3		gl_FragData[3]
#define etFragmentOut4		gl_FragData[4]
#define etFragmentOut5		gl_FragData[5]
#define etFragmentOut6		gl_FragData[6]
#define etFragmentOut7		gl_FragData[7]
)";

const std::string openGl3FragmentHeader = R"(
#define etShadow2D			texture
#define etTexture2D			texture
#define etTextureCube		texture
#define etTextureRect		texture
#define etTexture2DArray	texture
#define etShadow2DProj		textureProj
#define etTexture2DProj		textureProj
#define etTexture2DLod		textureLod
#define etTextureCubeLod	textureLod
#define etFragmentIn		in
#define etFragmentOut		etFragmentOut0
layout (location = 0) out etHighp vec4 etFragmentOut0;
layout (location = 1) out etHighp vec4 etFragmentOut1;
layout (location = 2) out etHighp vec4 etFragmentOut2;
layout (location = 3) out etHighp vec4 etFragmentOut3;
layout (location = 4) out etHighp vec4 etFragmentOut4;
layout (location = 5) out etHighp vec4 etFragmentOut5;
layout (location = 6) out etHighp vec4 etFragmentOut6;
layout (location = 7) out etHighp vec4 etFragmentOut7;
)";
