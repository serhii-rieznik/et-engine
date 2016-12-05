/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/base/material.h>
#include <et/rendering/base/rendering.h>
#include <et/rendering/base/shadersource.h>
#include <et/core/json.h>

namespace et
{

const std::string kCode = "code";
const std::string kInputLayout = "input-layout";
const std::string kOptions = "options";
extern const std::string shaderDefaultHeader;

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
	if (entry.object != tex)
	{
		entry.object = tex;
		entry.index = static_cast<uint32_t>(t);
		entry.binding = t;

		for (auto& i : _instances)
			i->invalidateTextureSet();
	}
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

Program::Pointer Material::program(RenderPassClass pt) const
{
	auto i = _programs.find(pt);
	return (i == _programs.end()) ? Program::Pointer() : i->second;
}

Texture::Pointer Material::texture(MaterialTexture t)
{
	return textures[static_cast<uint32_t>(t)].object;
}

Sampler::Pointer Material::sampler(MaterialTexture t)
{
	return samplers[static_cast<uint32_t>(t)].object;
}

void Material::setProgram(const Program::Pointer& prog, RenderPassClass pt)
{
	_programs[pt] = prog;
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

	Dictionary codeContainer;
	{
		VariantBase::Pointer codeObj = obj.objectForKey(kCode);
		if (codeObj->variantClass() == VariantClass::String)
			codeContainer.setObjectForKey("forward", codeObj);
		else if (codeObj->variantClass() == VariantClass::Dictionary)
			codeContainer = codeObj;
		else 
			ET_FAIL("Invalid type for `code` parameter in material. Should be string or dictionary");
	}
	loadCode(codeContainer, baseFolder, obj.dictionaryForKey(kOptions));
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
	else if (_renderer->api() == RenderingAPI::DirectX12)
	{
		layout.append("struct VSInput {\n");
		for (const auto& element : _inputLayout.elements())
		{
			char buffer[256] = {};
			sprintf(buffer, "%s %s : %s; \n", 
				dataTypeToString(element.type(), _renderer->api()).c_str(), 
				vertexAttributeUsageToString(element.usage()).c_str());
			layout.append(buffer);
		}
		layout.append("};\n");
	}
	else
	{
		ET_FAIL("Not implemented");
	}
	return layout;
}

void Material::loadCode(const Dictionary& codes, const std::string& baseFolder, Dictionary defines)
{
	for (const auto& kv : codes->content)
	{
		ET_ASSERT(kv.second->variantClass() == VariantClass::String);
		RenderPassClass cls = stringToRenderPassClass(kv.first);
		loadCode(StringValue(kv.second)->content, cls, baseFolder, defines);
	}
}

void Material::loadCode(const std::string& codeString, RenderPassClass passCls, const std::string& baseFolder, Dictionary defines)
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
	else if (_renderer->api() == RenderingAPI::DirectX12)
	{
		codeFileName = application().resolveFileName(codeString + ".hlsl");
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
			code.insert(positionInCode, shaderDefaultHeader);
		}
		else if (what != ParseDirective::StageDefine)
		{
			log::warning("Unknown directive in source code");
		}
	}, {ParseDirective::StageDefine});

	setProgram(_renderer->createProgram(programSource), passCls);
}

MaterialInstancePointer Material::instance()
{
	flushInstances();

	retain();
	MaterialInstance::Pointer result = MaterialInstance::Pointer::create(Material::Pointer(this));
	release();

	result->textures = textures;
	result->samplers = samplers;
	result->properties = properties;
	result->_programs = _programs;
	result->setCullMode(cullMode());
	result->setDepthState(depthState());
	result->setBlendState(blendState());
	_instances.emplace_back(result);
	return result;
}

const MaterialInstanceCollection& Material::instances() const
{
	return _instances;
}

void Material::flushInstances()
{
	auto i = std::remove_if(_instances.begin(), _instances.end(), [](const MaterialInstance::Pointer& inst) {
		return inst->retainCount() == 1;
	});
	_instances.erase(i, _instances.end());
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

void MaterialInstance::buildTextureSet(RenderPassClass pt)
{
	ET_ASSERT(program(pt).valid());
	
	Program::Pointer prog = program(pt);
	const Program::Reflection& reflection = prog->reflection();

	TextureSet::Description description;
	for (const auto& i : prog->reflection().textures.fragmentTextures)
	{
		if (i.second < static_cast<uint32_t>(MaterialTexture::FirstSharedTexture))
		{
			description.fragmentTextures[i.second] = base()->textures[i.second].object;
			if (textures[i.second].object.valid())
				description.fragmentTextures[i.second] = textures[i.second].object;
		}
	}
	for (const auto& i : prog->reflection().textures.vertexTextures)
	{
		if (i.second < static_cast<uint32_t>(MaterialTexture::FirstSharedTexture))
		{
			description.vertexTextures[i.second] = base()->textures[i.second].object;
			if (textures[i.second].object.valid())
				description.vertexTextures[i.second] = textures[i.second].object;
		}
	}
	for (const auto& i : prog->reflection().textures.fragmentSamplers)
	{
		if (i.second < static_cast<uint32_t>(MaterialTexture::FirstSharedTexture))
		{
			description.fragmentSamplers[i.second] = base()->samplers[i.second].object;
			if (samplers[i.second].object.valid())
				description.fragmentSamplers[i.second] = samplers[i.second].object;
		}
	}
	for (const auto& i : prog->reflection().textures.vertexSamplers)
	{
		if (i.second < static_cast<uint32_t>(MaterialTexture::FirstSharedTexture))
		{
			description.vertexSamplers[i.second] = base()->samplers[i.second].object;
			if (samplers[i.second].object.valid())
				description.vertexSamplers[i.second] = samplers[i.second].object;
		}
	}

	for (auto& i : description.fragmentTextures)
	{
		if (i.second.invalid())
			i.second = _renderer->defaultTexture();
	}
	for (auto& i : description.vertexTextures)
	{
		if (i.second.invalid())
			i.second = _renderer->defaultTexture();
	}
	for (auto& i : description.fragmentSamplers)
	{
		if (i.second.invalid())
			i.second = _renderer->defaultSampler();
	}
	for (auto& i : description.vertexSamplers)
	{
		if (i.second.invalid())
			i.second = _renderer->defaultSampler();
	}
	
	_textureSets[pt].obj = _renderer->createTextureSet(description);
	_textureSets[pt].valid = true;
}

void MaterialInstance::buildConstantBuffer(RenderPassClass pt)
{
	Program::Pointer prog = program(pt);
	ET_ASSERT(prog.valid());

	ConstantBufferEntry entry = _constBuffers[pt].obj;
	if (entry.valid())
		_renderer->sharedConstantBuffer().free(entry);
	
	const Program::Reflection& reflection = prog->reflection();
	if (reflection.materialVariablesBufferSize == 0)
	{
		_constBuffers[pt].obj = { };
		_constBuffers[pt].valid = true;
		return;
	}

	entry = _renderer->sharedConstantBuffer().staticAllocate(reflection.materialVariablesBufferSize);

	auto setFunc = [&, this](const mtl::OptionalValue& p) 
	{
		if (p.isSet())
		{
			const String& name = mtl::materialParameterToString(p.binding);
			auto var = reflection.materialVariables.find(name);
			if (var != reflection.materialVariables.end())
			{
				memcpy(entry.data() + var->second.offset, p.data, p.size);
			}
		}
	};

	for (const auto& p : base()->properties)
		setFunc(p);

	for (const auto& p : properties)
		setFunc(p);

	_constBuffers[pt].obj = entry;
	_constBuffers[pt].valid = true;
}

TextureSet::Pointer MaterialInstance::textureSet(RenderPassClass pt)
{
	if (!_textureSets[pt].valid)
		buildTextureSet(pt);

	return _textureSets.at(pt).obj;
}

ConstantBufferEntry MaterialInstance::constantBufferData(RenderPassClass pt)
{
	if (!_constBuffers[pt].valid)
		buildConstantBuffer(pt);

	return _constBuffers.at(pt).obj;
}

void MaterialInstance::invalidateTextureSet()
{
	for (auto& hld : _textureSets)
		hld.second.valid = false;
}

void MaterialInstance::invalidateConstantBuffer()
{
	for (auto& hld : _constBuffers)
		hld.second.valid = false;
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
		{ MaterialTexture::Shadow, "shadowTexture" },
		{ MaterialTexture::AmbientOcclusion, "aoTexture" },
		{ MaterialTexture::Environment, "environmentTexture" },
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
		{ MaterialTexture::Shadow, "shadowSampler" },
		{ MaterialTexture::AmbientOcclusion, "aoSampler" },
		{ MaterialTexture::Environment, "environmentSampler" },
	};
	return names.at(t);
}

const std::string shaderDefaultHeader =
R"(
#version 450
#define VertexStreamBufferIndex         0

#define VariablesSetIndex				0
#	define ObjectVariablesBufferIndex   4
#	define MaterialVariablesBufferIndex 5
#	define PassVariablesBufferIndex     6

#define TexturesSetIndex				1
#	define AlbedoTextureBinding			0
#	define ReflectanceTextureBinding	1
#	define EmissiveTextureBinding		2
#	define RoughnessTextureBinding		3
#	define OpacityTextureBinding		4
#	define NormalTextureBinding			5

#define SharedTexturesSetIndex			2
#	define ShadowTextureBinding			6
#	define AOTextureBinding				7
#	define EnvironmentTextureBinding	8

#define PI                              3.1415926536
#define HALF_PI                         1.5707963268
#define DOUBLE_PI                       6.2831853072
#define INV_PI                          0.3183098862

#define PassVariables PassVariables { \
	mat4 viewProjection; \
	mat4 projection; \
	mat4 view; \
	vec4 cameraPosition; \
	vec4 cameraDirection; \
	vec4 cameraUp; \
	vec4 lightPosition; \
	mat4 lightProjection; \
}
)";

}
