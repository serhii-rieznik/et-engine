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

std::string Material::_shaderDefaultHeader;
/*
 * Material
 */
Material::Material(RenderInterface* ren)
	: _renderer(ren)
{
	initDefaultHeader();
}

uint64_t Material::sortingKey() const
{
	return 0;
}

void Material::setTexture(MaterialTexture t, Texture::Pointer tex)
{
	OptionalObject<Texture::Pointer>& entry = textures[static_cast<uint32_t>(t)];
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
	OptionalObject<Sampler::Pointer>& entry = samplers[static_cast<uint32_t>(t)];
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

void Material::setProgram(const Program::Pointer& prog, RenderPassClass pt)
{
	_passes[pt].program = prog;
}

void Material::setDepthState(const DepthState& ds, RenderPassClass pt)
{
	_passes[pt].depthState = ds;
}

void Material::setBlendState(const BlendState& bs, RenderPassClass pt)
{
	_passes[pt].blendState = bs;
}

void Material::setCullMode(CullMode cm, RenderPassClass pt)
{
	_passes[pt].cullMode = cm;
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

	setName(obj.stringForKey(kName)->content);
	for (const auto& subObj : obj->content)
	{
		if (isValidRenderPassName(subObj.first))
		{
			if (subObj.second->variantClass() == VariantClass::Dictionary)
			{
				loadRenderPass(subObj.first, Dictionary(subObj.second), baseFolder);
			}
			else
			{
				log::error("Entry `%s` in material `%s` description does not describe render pass", subObj.first.c_str(), name().c_str());
			}
		}
		else if (subObj.first != kName)
		{
			log::warning("Entry `%s` in material `%s` description is not recognized", subObj.first.c_str(), name().c_str());
		}
	}
}

const Material::Configuration& Material::configuration(RenderPassClass cls) const
{
	ET_ASSERT(_passes.count(cls) > 0);
	return _passes.at(cls);
}

void Material::loadRenderPass(const std::string& name, const Dictionary& obj, const std::string& baseFolder)
{
	RenderPassClass cls = stringToRenderPassClass(name);

	CullMode cullMode = CullMode::Disabled;
	if (obj.hasKey(kCullMode) && !stringToCullMode(obj.stringForKey(kCullMode)->content, cullMode))
		log::error("Invalid cull mode specified in material: %s", obj.stringForKey(kCullMode)->content.c_str());

	_passes[cls].inputLayout = loadInputLayout(obj.dictionaryForKey(kInputLayout));

	_passes[cls].program = loadCode(obj.stringForKey(kCode)->content, baseFolder, obj.dictionaryForKey(kOptions),
		_passes[cls].inputLayout, _passes[cls].usedFiles);

	setDepthState(deserializeDepthState(obj.dictionaryForKey(kDepthState)), cls);
	setBlendState(deserializeBlendState(obj.objectForKey(kBlendState)), cls);
	setCullMode(cullMode, cls);
}

VertexDeclaration Material::loadInputLayout(Dictionary layout)
{
	VertexDeclaration decl;

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
		decl.push_back(kv.first, static_cast<DataType>(kv.second - 1));

	return decl;
}

std::string Material::generateInputLayout(const VertexDeclaration& decl)
{
	std::string layout;
	layout.reserve(1024);

	layout.append("struct VSInput {\n");
	for (const auto& element : decl.elements())
	{
		char buffer[256] = {};
		sprintf(buffer, "%s %s : %s;\n",
			dataTypeToString(element.type(), _renderer->api()).c_str(),
			vertexAttributeUsageToString(element.usage()).c_str(),
			vertexAttributeUsageSemantics(element.usage()).c_str()
		);

		layout.append(buffer);
	}
	layout.append("};\n");

	return layout;
}

Program::Pointer Material::loadCode(const std::string& codeString, const std::string& baseFolder, 
	Dictionary defines, const VertexDeclaration& decl, StringList& fileNames)
{
	application().pushSearchPath(baseFolder);
	std::string codeFileName = application().resolveFileName(codeString + ".hlsl");
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
	fileNames = parseShaderSource(programSource, getFilePath(codeFileName), allDefines,
		[this, &decl](ParseDirective what, std::string& code, uint32_t positionInCode)
	{
		if (what == ParseDirective::InputLayout)
		{
			std::string layout = generateInputLayout(decl);
			code.insert(positionInCode, layout);
		}
		else if (what == ParseDirective::DefaultHeader)
		{
			code.insert(positionInCode, _shaderDefaultHeader);
		}
		else if (what != ParseDirective::StageDefine)
		{
			log::warning("Unknown directive in source code");
		}
	}, {ParseDirective::StageDefine});
	
	fileNames.emplace_back(codeFileName);
	
	return _renderer->createProgram(programSource);
}

MaterialInstancePointer Material::instance()
{
	flushInstances();

	retain();
	MaterialInstance::Pointer result = MaterialInstance::Pointer::create(Material::Pointer(this));
	release();

	result->setName(name() + "-Instance" + intToStr(_instances.size()));
	result->textures = textures;
	result->samplers = samplers;
	result->properties = properties;
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
	const Program::Reflection& reflection = configuration(pt).program->reflection();

	TextureSet::Description description;
	for (const auto& i : reflection.textures.fragmentTextures)
	{
		description.fragmentTextures[i.second] = base()->textures[i.second].object;
		if (textures[i.second].object.valid())
			description.fragmentTextures[i.second] = textures[i.second].object;
	}
	for (const auto& i : reflection.textures.vertexTextures)
	{
		description.vertexTextures[i.second] = base()->textures[i.second].object;
		if (textures[i.second].object.valid())
			description.vertexTextures[i.second] = textures[i.second].object;
	}
	for (const auto& i : reflection.textures.fragmentSamplers)
	{
		description.fragmentSamplers[i.second] = base()->samplers[i.second].object;
		if (samplers[i.second].object.valid())
			description.fragmentSamplers[i.second] = samplers[i.second].object;
	}
	for (const auto& i : reflection.textures.vertexSamplers)
	{
		description.vertexSamplers[i.second] = base()->samplers[i.second].object;
		if (samplers[i.second].object.valid())
			description.vertexSamplers[i.second] = samplers[i.second].object;
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
	ConstantBufferEntry entry = _constBuffers[pt].obj;
	if (entry.valid())
		_renderer->sharedConstantBuffer().free(entry);
	
	const Program::Reflection& reflection = configuration(pt).program->reflection();
	if (reflection.materialVariablesBufferSize == 0)
	{
		_constBuffers[pt].obj = { };
		_constBuffers[pt].valid = true;
		return;
	}

	entry = _renderer->sharedConstantBuffer().staticAllocate(reflection.materialVariablesBufferSize);

	auto setFunc = [&, this](const OptionalValue& p) 
	{
		if (p.isSet())
		{
			const String& name = materialParameterToString(p.binding);
			auto var = reflection.materialVariables.find(name);
			if (var != reflection.materialVariables.end())
			{
				memcpy(entry.data() + var->second.offset, p.data, p.size);
			}
		}
	};

	for (const auto& p : base()->properties)
		setFunc(p.second);

	for (const auto& p : properties)
		setFunc(p.second);

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

const Material::Configuration & MaterialInstance::configuration(RenderPassClass cls) const
{
	return _base->configuration(cls);
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
const String& materialParameterToString(MaterialParameter p)
{
	ET_ASSERT(p < MaterialParameter::Count);
	static const Map<MaterialParameter, String> names =
	{
		{ MaterialParameter::BaseColorScale, "baseColorScale" },
		{ MaterialParameter::NormalScale, "normalScale" },
		{ MaterialParameter::RoughnessScale, "roughnessScale" },
		{ MaterialParameter::MetallnessScale, "metallnessScale" },
		{ MaterialParameter::EmissiveColor, "emissiveColor" },
		{ MaterialParameter::OpacityScale, "opacityScale" },
		{ MaterialParameter::IndexOfRefraction, "indexOfRefraction" },
		{ MaterialParameter::SpecularExponent, "specularExponent" },
	};
	return names.at(p);
}

const Map<MaterialTexture, String>& materialTextureNames()
{ 
	static const Map<MaterialTexture, String> localMap =
	{
		{ MaterialTexture::BaseColor, "baseColorTexture" },
		{ MaterialTexture::Normal, "normalTexture" },
		{ MaterialTexture::Roughness, "roughnessTexture" },
		{ MaterialTexture::Metallness, "metallnessTexture" },
		{ MaterialTexture::EmissiveColor, "emissiveColorTexture" },
		{ MaterialTexture::Shadow, "shadowTexture" },
		{ MaterialTexture::AmbientOcclusion, "aoTexture" },
		{ MaterialTexture::Environment, "environmentTexture" },
	};
	return localMap;
}

const Map<MaterialTexture, String>& materialSamplerNames()
{
	static const Map<MaterialTexture, String> names =
	{
		{ MaterialTexture::BaseColor, "baseColorSampler" },
		{ MaterialTexture::Normal, "normalSampler" },
		{ MaterialTexture::Roughness, "roughnessSampler" },
		{ MaterialTexture::Metallness, "metallnessSampler" },
		{ MaterialTexture::EmissiveColor, "emissiveColorSampler" },
		{ MaterialTexture::Shadow, "shadowSampler" },
		{ MaterialTexture::AmbientOcclusion, "aoSampler" },
		{ MaterialTexture::Environment, "environmentSampler" },
	};
	return names;
}

const String& materialTextureToString(MaterialTexture t)
{
	ET_ASSERT(t < MaterialTexture::Count);
	return materialTextureNames().at(t);
}

MaterialTexture stringToMaterialTexture(const String& name)
{
	for (const auto& ts : materialTextureNames())
	{
		if (ts.second == name)
			return ts.first;
	}
	return MaterialTexture::Count;
}

const String& materialSamplerToString(MaterialTexture t)
{
	ET_ASSERT(t < MaterialTexture::Count);
	return materialSamplerNames().at(t);

}

MaterialTexture samplerToMaterialTexture(const String& name)
{
	for (const auto& ts : materialSamplerNames())
	{
		if (ts.second == name)
			return ts.first;
	}
	return MaterialTexture::Count;
}

void Material::initDefaultHeader()
{
	_shaderDefaultHeader = R"(
#define VariablesSetIndex 0
#define TexturesSetIndex 1
#define PI 3.1415926536
#define HALF_PI 1.5707963268
#define DOUBLE_PI 6.2831853072
#define INV_PI 0.3183098862
)";

	char buffer[2048] = { };
	int printPos = 0;
	for (uint32_t i = static_cast<uint32_t>(MaterialTexture::FirstMaterialTexture), e = static_cast<uint32_t>(MaterialTexture::Count); i < e; ++i)
	{
		String texName = materialTextureToString(static_cast<MaterialTexture>(i));
		String smpName = materialSamplerToString(static_cast<MaterialTexture>(i));
		texName[0] = toupper(texName[0]);
		smpName[0] = toupper(smpName[0]);
		printPos += sprintf(buffer + printPos, "#define %sBinding %u\n#define %sBinding %u\n", 
			texName.c_str(), i, smpName.c_str(), i + MaterialSamplerBindingOffset);
	}
	{
		printPos += sprintf(buffer + printPos, 
			"#define ObjectVariablesBufferIndex %u\n"
			"#define MaterialVariablesBufferIndex %u\n"
			"#define PassVariablesBufferIndex %u\n", 
			ObjectVariablesBufferIndex, MaterialVariablesBufferIndex, PassVariablesBufferIndex);
	}
	
	_shaderDefaultHeader += buffer;

	_shaderDefaultHeader += R"(
#define CONSTANT_LOCATION_IMPL(name, registerName, spaceName) register(name##registerName, space##spaceName)
#define CONSTANT_LOCATION(name, register, space)              CONSTANT_LOCATION_IMPL(name, register, space)

cbuffer PassVariables : CONSTANT_LOCATION(b, PassVariablesBufferIndex, VariablesSetIndex) 
{
	row_major float4x4 viewProjection;
	row_major float4x4 projection;
	row_major float4x4 view;
	float4 cameraPosition;
	float4 cameraDirection;
	float4 cameraUp;
	float4 lightPosition;
	row_major float4x4 lightProjection;
};
)";

}
;

}
