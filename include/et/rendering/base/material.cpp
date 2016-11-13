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
const std::string kOptions = "options";
extern const std::string shaderDefaultHeaders[static_cast<uint32_t>(RenderingAPI::Count)];

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
	mtl::OptionalObject<Texture::Pointer>& entry = textures[static_cast<uint32_t>(t)];
	entry.object = tex;
	entry.index = static_cast<uint32_t>(t);
	entry.binding = t;

	for (auto& i : _instances)
		i->invalidateTextureSet();
}

void Material::setSampler(MaterialTexture t, Sampler::Pointer smp)
{
	mtl::OptionalObject<Sampler::Pointer>& entry = samplers[static_cast<uint32_t>(t)];
	entry.object = smp;
	entry.index = static_cast<uint32_t>(t);
	entry.binding = t;

	for (auto& i : _instances)
		i->invalidateTextureSet();
}

void Material::setVector(MaterialParameter p, const vec4& v)
{
	properties[static_cast<uint32_t>(p)] = v;
	properties[static_cast<uint32_t>(p)].binding = p;

	for (auto& i : _instances)
		i->invalidateConstantBuffer();
}

void Material::setFloat(MaterialParameter p, float f)
{
	properties[static_cast<uint32_t>(p)] = f;
	properties[static_cast<uint32_t>(p)].binding = p;

	for (auto& i : _instances)
		i->invalidateConstantBuffer();
}

vec4 Material::getVector(MaterialParameter p) const
{
	return getParameter<vec4>(p);
}

float Material::getFloat(MaterialParameter p) const
{
	return getParameter<float>(p);
}

Texture::Pointer Material::texture(MaterialTexture t)
{
	return textures[static_cast<uint32_t>(t)].object;
}

Sampler::Pointer Material::sampler(MaterialTexture t)
{
	return samplers[static_cast<uint32_t>(t)].object;
}

void Material::setProgram(Program::Pointer p)
{
	_program = p;
}

void Material::setDepthState(const DepthState& ds)
{
	_depthState = ds;
}

void Material::setBlendState(const BlendState& bs)
{
	_blendState = bs;
}

void Material::setCullMode(CullMode cm)
{
	_cullMode = cm;
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

	setDepthState(deserializeDepthState(obj.dictionaryForKey(kDepthState)));

	setBlendState(deserializeBlendState(obj.dictionaryForKey(kBlendState)));

	if (obj.hasKey(kCullMode) && !stringToCullMode(obj.stringForKey(kCullMode)->content, _cullMode))
	{
		log::error("Invalid cull mode specified in material: %s", obj.stringForKey(kCullMode)->content.c_str());
	}

	loadInputLayout(obj.dictionaryForKey(kInputLayout));
	
	loadCode(obj.stringForKey(kCode)->content, baseFolder, obj.dictionaryForKey(kOptions));
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

std::string Material::generateInputLayout()
{
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
	else if (_renderer->api() == RenderingAPI::Vulkan)
	{
		for (const auto& element : _inputLayout.elements())
		{
			char buffer[256] = {};
			sprintf(buffer, "layout (location = %u) in %s %s; \n", 
				static_cast<uint32_t>(element.usage()),
				dataTypeToString(element.type(), _renderer->api()).c_str(), 
				vertexAttributeUsageToString(element.usage()).c_str());
			layout.append(buffer);
		}
	}
	else
	{
		ET_FAIL("Not implemented");
	}
	return layout;
}

void Material::loadCode(const std::string& codeString, const std::string& baseFolder, Dictionary defines)
{
	std::string codeFileName;

	application().pushSearchPath(baseFolder);
	if (_renderer->api() == RenderingAPI::Metal)
	{
		codeFileName = application().resolveFileName(codeString + ".metal");
	}
	else if (_renderer->api() == RenderingAPI::Vulkan)
	{
		codeFileName = application().resolveFileName(codeString + ".glsl");
	}
	else
	{
		debug::debugBreak();
		ET_FAIL("Not implemented");
	}
	application().popSearchPaths();

	ET_ASSERT(fileExists(codeFileName));

	StringList allDefines;
	for (const auto& kv : defines->content)
	{
		char buffer[1024] = {};
		if (kv.second->variantClass() == VariantClass::Integer)
		{
			sprintf(buffer, "#define %s %llu", kv.first.c_str(), IntegerValue(kv.second)->content);
		}
		else if (kv.second->variantClass() == VariantClass::Float)
		{
			sprintf(buffer, "#define %s %0.7f", kv.first.c_str(), FloatValue(kv.second)->content);
		}
		else if (kv.second->variantClass() == VariantClass::String)
		{
			sprintf(buffer, "#define %s %s", kv.first.c_str(), StringValue(kv.second)->content.c_str());
		}
		else
		{
			ET_FAIL("Unsupported type in defines");
		}
		allDefines.emplace_back(buffer);
	}

	std::string programSource = loadTextFile(codeFileName);
	parseShaderSource(programSource, getFilePath(codeFileName), allDefines,
		[this](ParseDirective what, std::string& code, uint32_t positionInCode)
	{
		if (what == ParseDirective::InputLayout)
		{
			std::string layout = generateInputLayout();
			code.insert(positionInCode, layout);
		}
		else if (what == ParseDirective::DefaultHeader)
		{
			code.insert(positionInCode, shaderDefaultHeaders[static_cast<uint32_t>(_renderer->api())]);
		}
		else if (what != ParseDirective::StageDefine)
		{
			log::warning("Unknown directive in source code");
		}
	}, {ParseDirective::StageDefine});

	setProgram(_renderer->createProgram(programSource));
}

MaterialInstancePointer Material::instance()
{
	MaterialInstance::Pointer result = MaterialInstance::Pointer::create(Material::Pointer(this));
	result->textures = textures;
	result->samplers = samplers;
	result->properties = properties;
	result->setCullMode(cullMode());
	result->setDepthState(depthState());
	result->setBlendState(blendState());
	result->setProgram(program());
	_instances.emplace_back(result);
	return result;
}

const MaterialInstanceCollection& Material::instances() const
{
	return _instances;
}

void Material::releaseInstances()
{
	_instances.clear();
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

void MaterialInstance::buildTextureSet()
{
	_textureSetValid = true;
}

void MaterialInstance::buildConstantBuffer()
{
	ET_ASSERT(program().valid());

	_constBufferOffset = 0;
	
	if (_constBufferData != nullptr)
	{
		_renderer->sharedConstantBuffer().free(_constBufferData);
		_constBufferData = nullptr;
	}
	
	const Program::Reflection& reflection = program()->reflection();
	if (reflection.materialVariablesBufferSize == 0)
	{
		_constantBufferValid = true;
		return;
	}

	_constBufferData = _renderer->sharedConstantBuffer().staticAllocate(reflection.materialVariablesBufferSize, _constBufferOffset);

	auto setFunc = [&, this](const mtl::OptionalValue& p) 
	{
		if (p.isSet())
		{
			const String& name = mtl::materialParameterToString(p.binding);
			auto var = reflection.materialVariables.find(name);
			if (var != reflection.materialVariables.end())
			{
				memcpy(_constBufferData + var->second.offset, p.data, p.size);
			}
		}
	};

	for (const auto& p : base()->properties)
		setFunc(p);

	for (const auto& p : properties)
		setFunc(p);

	_constantBufferValid = true;
}

TextureSet::Pointer MaterialInstance::textureSet()
{
	if (!_textureSetValid)
		buildTextureSet();

	return _textureSet;
}


uint32_t MaterialInstance::sharedConstantBufferOffset()
{
	if (!_constantBufferValid)
		buildConstantBuffer();

	return _constBufferOffset;
}

void MaterialInstance::invalidateTextureSet()
{
	_textureSetValid = false;
}

void MaterialInstance::invalidateConstantBuffer()
{
	_constantBufferValid = false;
}

/*
 * Service
 */
const String& mtl::materialParameterToString(MaterialParameter p)
{
	ET_ASSERT(p < MaterialParameter::Count);
	static const Map<MaterialParameter, String> names =
	{
		{ MaterialParameter::AlbedoColor, "albedoColor" },
		{ MaterialParameter::ReflectanceColor, "reflectanceColor" },
		{ MaterialParameter::EmissiveColor, "emissiveColor" },
		{ MaterialParameter::Roughness, "roughness" },
		{ MaterialParameter::Metallness, "metallness" },
		{ MaterialParameter::Opacity, "opacity" },
		{ MaterialParameter::NormalScale, "normalScale" },
		{ MaterialParameter::IndexOfRefraction, "indexOfRefraction" },
		{ MaterialParameter::SpecularExponent, "specularExponent" },
	};
	return names.at(p);
}

const Map<MaterialTexture, String>& materialTextureNames()
{ 
	static const Map<MaterialTexture, String> localMap =
	{
		{ MaterialTexture::Albedo, "albedoTexture" },
		{ MaterialTexture::Reflectance, "reflectanceTexture" },
		{ MaterialTexture::Roughness, "roughnessTexture" },
		{ MaterialTexture::Emissive, "emissiveTexture" },
		{ MaterialTexture::Opacity, "opacityTexture" },
		{ MaterialTexture::Normal, "normalTexture" },
	};
	return localMap;
}

const String& mtl::materialTextureToString(MaterialTexture t)
{
	ET_ASSERT(t < MaterialTexture::Count);
	return materialTextureNames().at(t);
}

MaterialTexture mtl::stringToMaterialTexture(const String& name)
{
	for (const auto& ts : materialTextureNames())
	{
		if (ts.second == name)
			return ts.first;
	}
	log::error("Unknown texture name %s", name.c_str());
	return MaterialTexture::Count;
}

const String& mtl::materialSamplerToString(MaterialTexture t)
{
	ET_ASSERT(t < MaterialTexture::Count);
	static const Map<MaterialTexture, String> names =
	{
		{ MaterialTexture::Albedo, "albedoSampler" },
		{ MaterialTexture::Reflectance, "reflectanceSampler" },
		{ MaterialTexture::Roughness, "roughnessSampler" },
		{ MaterialTexture::Emissive, "emissiveSampler" },
		{ MaterialTexture::Opacity, "opacitySampler" },
		{ MaterialTexture::Normal, "normalSampler" },
	};
	return names.at(t);
}

const std::string shaderDefaultHeaders[static_cast<uint32_t>(RenderingAPI::Count)] = 
{
R"(
#define VertexStreamBufferIndex         0
#define ObjectVariablesBufferIndex      4
#define MaterialVariablesBufferIndex    5
#define PassVariablesBufferIndex        6
#define PI                              3.1415926536
#define HALF_PI                         1.5707963268
#define INV_PI                          0.3183098862

using namespace metal;

struct PassVariables
{
	float4x4 viewProjection;
	float4x4 projection;
	float4x4 view;
	float4 cameraPosition;
	float4 cameraDirection;
	float4 cameraUp;
	float4 lightPosition;
};
)",

R"(
#version 450
#define VertexStreamBufferIndex         0
#define ObjectVariablesBufferIndex      4
#define MaterialVariablesBufferIndex    5
#define PassVariablesBufferIndex        6
#define PI                              3.1415926536
#define HALF_PI                         1.5707963268
#define INV_PI                          0.3183098862

#define PassVariables PassVariables { \
	mat4 viewProjection; \
	mat4 projection; \
	mat4 view; \
	vec4 cameraPosition; \
	vec4 cameraDirection; \
	vec4 cameraUp; \
	vec4 lightPosition; \
}
)",

R"(
DX12 not implemented yet
)"
};

}
