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

namespace et {

const std::string kCode = "code";
const std::string kInputLayout = "input-layout";
const std::string kOptions = "options";
const std::string kDefault = "default";

std::string Material::_shaderDefaultHeader;
/*
 * Material
 */
Material::Material(RenderInterface* ren)
	: _renderer(ren) {
	_activeInstances.reserve(8);
	_instancesPool.reserve(8);
	initDefaultHeader();
}

uint64_t Material::sortingKey() const {
	return 0;
}

void Material::setTexture(const std::string& t, const Texture::Pointer& tex, const ResourceRange& range) {
	if (validateTextureName(t) == false)
		return;

	OptionalTextureObject& entry = textures[t];
	if ((entry.object != tex) || (entry.range != range))
	{
		entry.object = tex;
		entry.range = range;
		entry.binding = t;
		invalidateTextureBindingsSet();
	}
}

void Material::setSampler(const std::string& s, const Sampler::Pointer& smp) {
	if (validateSamplerName(s) == false)
		return;

	OptionalSamplerObject& entry = samplers[s];
	if (entry.object != smp)
	{
		entry.object = smp;
		entry.binding = s;
		invalidateTextureBindingsSet();
	}
}

void Material::setImage(const std::string& s, const Texture::Pointer& tex) {
	if (validateImageName(s) == false)
		return;

	OptionalImageObject& entry = images[s];
	if (entry.object != tex)
	{
		entry.object = tex;
		entry.binding = s;
		invalidateTextureBindingsSet();
	}
}

void Material::setVector(MaterialVariable p, const vec4& v) {
	uint32_t ip = static_cast<uint32_t>(p);
	auto& prop = properties[ip];
	prop.set(v);
	prop.binding = ip;
	invalidateConstantBuffer();
}

void Material::setFloat(MaterialVariable p, float f) {
	uint32_t ip = static_cast<uint32_t>(p);
	auto& prop = properties[ip];
	prop.set(f);
	prop.binding = ip;
	invalidateConstantBuffer();
}

vec4 Material::getVector(MaterialVariable p) const {
	return getParameter<vec4>(p);
}

float Material::getFloat(MaterialVariable p) const {
	return getParameter<float>(p);
}

const Texture::Pointer& Material::texture(const std::string& t) {
	return textures[t].object;
}

const Sampler::Pointer& Material::sampler(const std::string& t) {
	return samplers[t].object;
}

const Texture::Pointer& Material::image(const std::string& t) {
	return images[t].object;
}

void Material::setProgram(const Program::Pointer& prog, const std::string& pt) {
	_configurations[pt].program = prog;
}

void Material::setDepthState(const DepthState& ds, const std::string& pt) {
	_configurations[pt].depthState = ds;
}

void Material::setBlendState(const BlendState& bs, const std::string& pt) {
	_configurations[pt].blendState = bs;
}

void Material::setCullMode(CullMode cm, const std::string& pt) {
	_configurations[pt].cullMode = cm;
}

void Material::loadFromJson(const std::string& source, const std::string& baseFolder) {
	VariantClass cls = VariantClass::Invalid;
	Dictionary obj = json::deserialize(source, cls);
	if (cls != VariantClass::Dictionary)
	{
		log::error("Unable to load material from JSON");
		return;
	}

	for (const auto& subObj : obj->content)
	{
		if (subObj.first == kName)
		{
			setName(StringValue(subObj.second)->content);
		}
		else if (isValidRenderPassName(subObj.first))
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
		else
		{
			log::warning("Entry `%s` in material `%s` description is not recognized", subObj.first.c_str(), name().c_str());
		}
	}

	for (const auto& config : _configurations)
	{
		for (const auto& ts : config.second.program->reflection().textures)
		{
			for (const auto& t : ts.samplers)
				_usedSamplers.emplace(t.first);
			for (const auto& t : ts.textures)
				_usedTextures.emplace(t.first);
			for (const auto& t : ts.images)
				_usedImages.emplace(t.first);
		}
	}

	invalidateConstantBuffer();
	invalidateTextureBindingsSet();
}

const Material::Configuration& Material::configuration(const std::string& cls) const {
	static const Material::Configuration emptyConfiguration;

	auto i = _configurations.find(cls);
	if (i == _configurations.end())
	{
		i = _configurations.find(kDefault);
		ET_ASSERT(i != _configurations.end());
	}
	return (i == _configurations.end()) ? emptyConfiguration : i->second;
}

void Material::loadRenderPass(const std::string& cls, const Dictionary& obj, const std::string& baseFolder) {
	bool isGraphicsPipeline = (obj.stringForKey(kClass)->content != kCompute);
	_pipelineClass = isGraphicsPipeline ? PipelineClass::Graphics : PipelineClass::Compute;

	if (isGraphicsPipeline)
	{
		CullMode cullMode = CullMode::Disabled;
		if (obj.hasKey(kCullMode) && !stringToCullMode(obj.stringForKey(kCullMode)->content, cullMode))
			log::error("Invalid cull mode specified in material: %s", obj.stringForKey(kCullMode)->content.c_str());

		_configurations[cls].inputLayout = loadInputLayout(obj.dictionaryForKey(kInputLayout));
		setDepthState(deserializeDepthState(obj.dictionaryForKey(kDepthState)), cls);
		setBlendState(deserializeBlendState(obj.objectForKey(kBlendState)), cls);
		setCullMode(cullMode, cls);
	}

	_configurations[cls].program = loadCode(obj.stringForKey(kCode)->content, baseFolder, obj.dictionaryForKey(kOptions),
		_configurations[cls].inputLayout, _configurations[cls].usedFiles);
}

VertexDeclaration Material::loadInputLayout(Dictionary layout) {
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

std::string Material::generateInputLayout(const VertexDeclaration& decl) {
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
	const Dictionary& defines, const VertexDeclaration& decl, StringList& fileNames) {
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
			sprintf(buffer, "#define %s %llu\n", kv.first.c_str(), IntegerValue(kv.second)->content);
		}
		else if (kv.second->variantClass() == VariantClass::Float)
		{
			sprintf(buffer, "#define %s %0.7f\n", kv.first.c_str(), FloatValue(kv.second)->content);
		}
		else if (kv.second->variantClass() == VariantClass::String)
		{
			sprintf(buffer, "#define %s %s\n", kv.first.c_str(), StringValue(kv.second)->content.c_str());
		}
		else
		{
			ET_FAIL("Unsupported type in defines");
		}
		allDefines.emplace_back(buffer);
	}

	std::string programSource = loadTextFile(codeFileName);
	fileNames = parseShaderSource(programSource, getFilePath(codeFileName), allDefines,
		[this, &decl](ParseDirective what, std::string& code, uint32_t positionInCode) {
		if (what == ParseDirective::InputLayout)
		{
			code.insert(positionInCode, generateInputLayout(decl));
		}
		else if (what == ParseDirective::DefaultHeader)
		{
			code.insert(positionInCode, _shaderDefaultHeader);
		}
		else if (what == ParseDirective::Options)
		{
			code.insert(positionInCode, _renderer->options().optionsHeader());
		}
		else if (what != ParseDirective::StageDefine)
		{
			log::warning("Unknown directive in source code");
		}
	}, { ParseDirective::StageDefine });

	fileNames.emplace_back(codeFileName);

	uint32_t stagesMask = (_pipelineClass == PipelineClass::Graphics) ?
		programStagesMask(ProgramStage::Vertex, ProgramStage::Fragment) : programStagesMask(ProgramStage::Compute);

	return _renderer->createProgram(stagesMask, programSource);
}

MaterialInstancePointer Material::instance() {
	flushInstances();

	MaterialInstance::Pointer result;
	if (_instancesPool.empty())
	{
		InstusivePointerScope<Material> pointerScope(this);
		result = MaterialInstance::Pointer::create(Material::Pointer(this));
	}
	else
	{
		result = _instancesPool.back();
		_instancesPool.pop_back();
	}

	result->setName(name() + "-Instance" + intToStr(_instancesCounter++));
	result->textures = textures;
	result->samplers = samplers;
	result->properties = properties;

	_activeInstances.emplace_back(result);
	return result;
}

const MaterialInstanceCollection& Material::instances() const {
	return _activeInstances;
}

void Material::flushInstances() {
	size_t index = 0;
	while (index < _activeInstances.size())
	{
		if (_activeInstances[index]->retainCount() == 1)
		{
			std::swap(_activeInstances[index], _activeInstances.back());
			_instancesPool.emplace_back(_activeInstances.back());
			_activeInstances.pop_back();
		}
		else
		{
			++index;
		}
	}
}

void Material::invalidateInstances() {
	ET_ASSERT(!isInstance());
	for (MaterialInstance::Pointer& i : _activeInstances)
	{
		i->invalidateConstantBuffer();
		i->invalidateTextureBindingsSet();
	}
}

void Material::releaseInstances() {
	ET_ASSERT(!isInstance());
	_activeInstances.clear();
	_instancesPool.clear();
}

void Material::invalidateTextureBindingsSet() {
	for (MaterialInstance::Pointer& i : _activeInstances)
		i->invalidateTextureBindingsSet();
}

void Material::invalidateConstantBuffer() {
	for (MaterialInstance::Pointer& i : _activeInstances)
		i->invalidateConstantBuffer();
}

bool Material::validateImageName(const std::string& samplerName) {
	return _usedImages.count(samplerName) > 0;
}

bool Material::validateTextureName(const std::string& textureName) {
	return _usedTextures.count(textureName) > 0;
}

bool Material::validateSamplerName(const std::string& samplerName) {
	return _usedSamplers.count(samplerName) > 0;
}

/*
 * Material Instance
 */
MaterialInstance::MaterialInstance(Material::Pointer bs)
	: Material(bs->_renderer), _base(bs) {
}

Material::Pointer& MaterialInstance::base() {
	ET_ASSERT(isInstance());
	return _base;
}

const Material::Pointer& MaterialInstance::base() const {
	ET_ASSERT(isInstance());
	return _base;
}

void MaterialInstance::buildTextureBindingsSet(const std::string& pt, Holder<TextureSet::Pointer>& holder) {
	ET_ASSERT(isInstance());

	const Program::Reflection& reflection = base()->configuration(pt).program->reflection();

	TextureSet::Description description;
	for (const TextureSet::ReflectionSet& ref : reflection.textures)
	{
		TextureSet::DescriptionSet& desc = description[ref.stage];

		for (const auto& r : ref.textures)
		{
			const Texture::Pointer& baseTexture = base()->textures[r.first].object;
			const ResourceRange& baseRange = base()->textures[r.first].range;

			const Texture::Pointer& ownTexture = textures[r.first].object;
			const ResourceRange& ownRange = textures[r.first].range;

			TextureSet::TextureBinding& descriptionTexture = desc.textures[r.second];
			descriptionTexture.image = ownTexture.valid() ? ownTexture : baseTexture;
			descriptionTexture.range = ownTexture.valid() ? ownRange : baseRange;
			if (descriptionTexture.image.invalid())
			{
				descriptionTexture.image = _renderer->checkersTexture();
				descriptionTexture.range = ResourceRange::whole;
			}
		}

		for (const auto& r : ref.samplers)
		{
			Sampler::Pointer& descriptionSampler = desc.samplers[r.second];
			const Sampler::Pointer& builtInSampler = _renderer->builtInSampler(r.first);
			if (builtInSampler.valid())
			{
				descriptionSampler = builtInSampler;
			}
			else
			{
				const Sampler::Pointer& baseSampler = base()->samplers[r.first].object;
				const Sampler::Pointer& ownSampler = samplers[r.first].object;
				descriptionSampler = ownSampler.valid() ? ownSampler : baseSampler;
				if (descriptionSampler.invalid())
					descriptionSampler = _renderer->builtInSampler(Sampler::PointClamp);
			}
		}

		for (const auto& r : ref.images)
		{
			const Texture::Pointer& baseImage = base()->images[r.first].object;
			const Texture::Pointer& ownImage = images[r.first].object;
			Texture::Pointer& descriptionImage = description[ref.stage].images[r.second];

			descriptionImage = ownImage.valid() ? ownImage : baseImage;
			if (descriptionImage.invalid())
				descriptionImage = _renderer->blackImage();
		}
	}

	holder.obj = _renderer->createTextureSet(description);
	holder.valid = true;
}

void MaterialInstance::buildConstantBuffer(const std::string& pt, Holder<ConstantBufferEntry::Pointer>& holder) {
	ET_ASSERT(isInstance());

	const Program::Reflection& reflection = base()->configuration(pt).program->reflection();
	if (reflection.materialVariablesBufferSize == 0)
	{
		holder.obj.reset(nullptr);
		holder.valid = true;
	}
	else
	{
		holder.obj = _renderer->sharedConstantBuffer().allocate(reflection.materialVariablesBufferSize, ConstantBufferStaticAllocation);

		auto setFunc = [&, this](const OptionalValue& p) {
			if (p.isSet() && reflection.materialVariables[p.binding].enabled)
				memcpy(holder.obj->data() + reflection.materialVariables[p.binding].offset, p.data, p.dataSize);
		};

		for (const auto& p : base()->properties)
			setFunc(p.second);

		for (const auto& p : properties)
			setFunc(p.second);

		holder.valid = true;
	}
}

const TextureSet::Pointer& MaterialInstance::textureBindingsSet(const std::string& pt) {
	ET_ASSERT(isInstance());

	auto& holder = _textureBindingsSets[pt];
	if (!holder.valid)
		buildTextureBindingsSet(pt, holder);

	return holder.obj;
}

const ConstantBufferEntry::Pointer& MaterialInstance::constantBufferData(const std::string& pt) {
	ET_ASSERT(isInstance());

	auto& holder = _constBuffers[pt];
	if (!holder.valid)
		buildConstantBuffer(pt, holder);

	return holder.obj;
}

void MaterialInstance::invalidateTextureBindingsSet() {
	ET_ASSERT(isInstance());

	for (auto& hld : _textureBindingsSets)
		hld.second.valid = false;
}

void MaterialInstance::invalidateConstantBuffer() {
	ET_ASSERT(isInstance());

	for (auto& hld : _constBuffers)
		hld.second.valid = false;
}

void MaterialInstance::serialize(std::ostream&) const {

}

void MaterialInstance::deserialize(std::istream&) {

}

bool MaterialInstance::validateImageName(const std::string& nm) {
	return _base->validateImageName(nm);
}

bool MaterialInstance::validateTextureName(const std::string& nm) {
	return _base->validateTextureName(nm);
}

bool MaterialInstance::validateSamplerName(const std::string& nm) {
	return _base->validateSamplerName(nm);

}

void Material::initDefaultHeader() {
	if (_shaderDefaultHeader.empty())
	{
		_shaderDefaultHeader = R"(
#define PI					(3.1415926536)
#define HALF_PI				(1.5707963268)
#define DOUBLE_PI			(6.2831853072)
#define INV_PI				(0.3183098862)
#define LUMINANCE_SCALE		(25600.0)

#define DECL_REGISTER_IMPL(type, index, space)	register(type##index, space)
#define DECL_REGISTER_PROXY(type, index, space) DECL_REGISTER_IMPL(type, index, space)
#define DECL_REGISTER(type, space)				DECL_REGISTER_PROXY(type, __LINE__, space)

#define DECL_OBJECT_BUFFER		register(c0, space0)
#define DECL_MATERIAL_BUFFER	register(c1, space0)

#define DECLARE_BUFFER		DECL_REGISTER(c, space0)
#define DECLARE_TEXTURE		DECL_REGISTER(t, space1) 
#define DECLARE_STORAGE		DECL_REGISTER(u, space3)

#define DECLARE_EXPLICIT_SAMPLER		DECL_REGISTER(s, space2) 
#define DECLARE_SAMPLER(filter, wrap)	SamplerState filter##wrap : DECLARE_EXPLICIT_SAMPLER

DECLARE_SAMPLER(Anisotropic, Wrap);
DECLARE_SAMPLER(Linear, Wrap);
DECLARE_SAMPLER(Linear, Clamp);
DECLARE_SAMPLER(Point, Wrap);
DECLARE_SAMPLER(Point, Clamp);

)";
	}
}

}
