/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/base/material.h>
#include <et/core/json.h>

namespace et
{

const std::string kCode = "code";
const std::string kInputLayout = "input-layout";
const std::string kBuiltInInput = "%built-in-input%";

/*
 * Material
 */
Material::Material(RenderInterface* ren)
	: _renderer(ren)
{
}

uint64_t Material::sortingKey() const
{
	return 0;
}

void Material::setTexture(MaterialTexture t, Texture::Pointer tex)
{
	_textures[static_cast<size_t>(t)] = tex;
}

void Material::setVector(MaterialParameter p, const vec4& v)
{
	_params[static_cast<size_t>(p)] = v;
}

vec4 Material::getVector(MaterialParameter p) const
{
	return getParameter<vec4>(p);
}

void Material::setFloat(MaterialParameter p, float f)
{
	_params[static_cast<size_t>(p)] = f;
}

float Material::getFloat(MaterialParameter p) const
{
	return getParameter<float>(p);
}

void Material::loadFromJson(const std::string& source, const std::string& baseFolder)
{
	VariantClass cls = VariantClass::Invalid;
	Dictionary obj = json::deserialize(source, cls);
	if (cls != VariantClass::Dictionary)
	{
		log::error("Unable to load material from JSON");
		return;
	}

	_depthState = deserializeDepthState(obj.dictionaryForKey(kDepthState));

	_blendState = deserializeBlendState(obj.dictionaryForKey(kBlendState));

	if (obj.hasKey(kCullMode) && !stringToCullMode(obj.stringForKey(kCullMode)->content, _cullMode))
	{
		log::error("Invalid cull mode specified in material: %s", obj.stringForKey(kCullMode)->content.c_str());
	}

	loadInputLayout(obj.dictionaryForKey(kInputLayout));
	
	loadCode(obj.stringForKey(kCode)->content, baseFolder);
}

void Material::loadInputLayout(Dictionary layout)
{
	_inputLayout.clear();

	Map<VertexAttributeUsage, uint32_t> sortedContent;
	for (const auto& kv : layout->content)
	{
		VertexAttributeUsage usage = stringToVertexAttributeUsage(kv.first);
		ET_ASSERT(usage != VertexAttributeUsage::Unknown);

		ET_ASSERT(kv.second->variantClass() == VariantClass::Integer);
		uint32_t size = static_cast<uint32_t>(IntegerValue(kv.second)->content);
		ET_ASSERT(size > 0);
		ET_ASSERT(size <= 4);

		sortedContent.emplace(usage, size);
	}

	for (const auto& kv : sortedContent)
	{
		_inputLayout.push_back(kv.first, static_cast<DataType>(kv.second - 1));
	}
}

void Material::generateInputLayout(std::string& source)
{
	uint64_t idPos = source.find(kBuiltInInput);
	ET_ASSERT(idPos != std::string::npos);

	std::string layout;
	layout.reserve(1024);


	if (_renderer->api() == RenderingAPI::Metal)
	{
		layout.append("struct VSInput {\n");
		for (const auto& element : _inputLayout.elements())
		{
			char buffer[256] = {};
			sprintf(buffer, "\t%s %s [[attribute(%u)]]; \n",
				dataTypeToString(element.type(), _renderer->api()).c_str(),
				vertexAttributeUsageToString(element.usage()).c_str(), static_cast<uint32_t>(element.usage()));
			layout.append(buffer);
		}
		layout.append("};\n");
	}
	else
	{
		ET_FAIL("Not implemented");
	}

	auto b = source.begin() + idPos;
	source.replace(b, b + kBuiltInInput.length(), layout);
}

void Material::loadCode(const std::string& codeString, const std::string& baseFolder)
{
	std::string codeFileName;

	application().pushSearchPath(baseFolder);
	if (_renderer->api() == RenderingAPI::Metal)
	{
		codeFileName = application().resolveFileName(codeString + ".metal");
	}
	else
	{
		ET_FAIL("Not implemented");
	}
	application().popSearchPaths();

	std::string programSource;
	if (fileExists(codeFileName))
	{
		StringList defines;
		programSource = loadTextFile(codeFileName);
		parseShaderSource(programSource, baseFolder, defines);
		generateInputLayout(programSource);
		_program = _renderer->createProgram(programSource, emptyString, defines, baseFolder);
	}
}

MaterialInstancePointer Material::instance()
{
	MaterialInstance::Pointer result = MaterialInstance::Pointer::create(Material::Pointer(this));
	result->_textures = _textures;
	result->_params = _params;
	return result;
}

Program::Pointer Material::program()
{
	return _program;
}

/*
 * Material Instance
 */
MaterialInstance::MaterialInstance(Material::Pointer bs)
	: Material(bs->_renderer), _base(bs)
{
}

Material::Pointer MaterialInstance::base()
{
	return _base;
}

/*
 * Service
 */
#define MTL_CASE(X) case MaterialParameter::X: return #X;
std::string materialParameterToString(MaterialParameter p)
{
	switch (p)
	{
		MTL_CASE(AmbientColor)
		MTL_CASE(DiffuseColor)
		MTL_CASE(SpecularColor)
		MTL_CASE(EmissiveColor)
		MTL_CASE(Roughness)
		MTL_CASE(Opacity)
		MTL_CASE(NormalTextureScale)
		default:
			ET_FAIL_FMT("Invalid or unknown material parameter provided: %u", static_cast<uint32_t>(p));
	}
}
#undef MTL_CASE

}
