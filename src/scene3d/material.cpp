/*
* This file is part of `et engine`
* Copyright 2009-2015 by Sergey Reznik
* Please, modify content only if you know what are you doing.
*
*/

#include <et/core/conversion.h>
#include <et/core/serialization.h>
#include <et/core/tools.h>
#include <et/core/cout.h>
#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/scene3d/material.h>

using namespace et;
using namespace et::s3d;

static const vec4 nullVector;
static const Texture::Pointer nullTexture = Texture::Pointer();

extern const std::string materialKeys[MaterialParameter_max];
size_t stringToMaterialParameter(const std::string&);

Material::Material() :
	LoadableObject("default-material")
{
	setVector(MaterialParameter_DiffuseColor, vec4(1.0f));
}

Material::~Material()
{

}

Material* Material::duplicate() const
{
	Material* m = etCreateObject<Material>();
	
	m->tag = tag;
	m->setName(name());
	m->setOrigin(origin());
	m->_defaultIntParameters = _defaultIntParameters;
	m->_defaultFloatParameters = _defaultFloatParameters;
	m->_defaultVectorParameters = _defaultVectorParameters;
	m->_defaultTextureParameters = _defaultTextureParameters;
	m->_defaultStringParameters = _defaultStringParameters;
	m->_customIntParameters = _customIntParameters;
	m->_customFloatParameters = _customFloatParameters;
	m->_customVectorParameters = _customVectorParameters;
	m->_customTextureParameters = _customTextureParameters;
	m->_customStringParameters = _customStringParameters;
	m->_blend = _blend;
	m->_depth = _depth;

	return m;
}

void Material::serialize(Dictionary stream, const std::string& basePath)
{
	Dictionary intValues;
	Dictionary floatValues;
	Dictionary vectorValues;
	Dictionary textureValues;
	for (size_t i = 0; i < MaterialParameter_max; ++i)
	{
		if (_defaultIntParameters[i].set)
			intValues.setIntegerForKey(materialKeys[i], _defaultIntParameters[i].value);

		if (_defaultFloatParameters[i].set)
			floatValues.setFloatForKey(materialKeys[i], _defaultFloatParameters[i].value);

		if (_defaultVectorParameters[i].set)
			vectorValues.setArrayForKey(materialKeys[i], vec4ToArray(_defaultVectorParameters[i].value));

		if (_defaultTextureParameters[i].set)
		{
			const auto& tex = _defaultTextureParameters[i];
			if (tex.value.valid())
			{
				textureValues.setStringForKey(materialKeys[i], tex.value->origin());
			}
		}
	}

	for (const auto& kv : _customIntParameters)
		intValues.setIntegerForKey("id" + intToStr(kv.first), kv.second);

	for (const auto& kv : _customFloatParameters)
		floatValues.setFloatForKey("id" + intToStr(kv.first), kv.second);

	for (const auto& kv : _customVectorParameters)
		vectorValues.setArrayForKey("id" + intToStr(kv.first), vec4ToArray(kv.second));

	for (const auto& kv : _customTextureParameters)
	{
		if (kv.second.valid() && !kv.second->origin().empty())
			textureValues.setStringForKey("id" + intToStr(kv.first), kv.second->origin());
	}
	
	Dictionary blend;
	blend.setBooleanForKey(kBlendEnabled, _blend.blendEnabled);
	blend.setStringForKey(kSourceColor, blendFunctionToString(_blend.color.source));
	blend.setStringForKey(kDestColor, blendFunctionToString(_blend.color.dest));
	blend.setStringForKey(kSourceAlpha, blendFunctionToString(_blend.alpha.source));
	blend.setStringForKey(kDestAlpha, blendFunctionToString(_blend.alpha.dest));
	blend.setStringForKey(kColorOperation, blendOperationToString(_blend.colorOperation));
	blend.setStringForKey(kAlphaOperation, blendOperationToString(_blend.alphaOperation));
	
	Dictionary depth;
	depth.setBooleanForKey(kDepthWriteEnabled, _depth.depthWriteEnabled);
	depth.setBooleanForKey(kDepthTestEnabled, _depth.depthTestEnabled);
	depth.setStringForKey(kDepthFunction, compareFunctionToString(_depth.compareFunction));

	stream.setStringForKey(kName, name());
	stream.setDictionaryForKey(kBlendState, blend);
	stream.setDictionaryForKey(kDepthState, depth);

	if (!intValues.empty())
		stream.setDictionaryForKey(kIntegerValues, intValues);

	if (!floatValues.empty())
		stream.setDictionaryForKey(kFloatValues, floatValues);

	if (!vectorValues.empty())
		stream.setDictionaryForKey(kVectorValues, vectorValues);

	if (!textureValues.empty())
		stream.setDictionaryForKey(kTextures, textureValues);
}

void Material::deserializeWithOptions(Dictionary stream, RenderContext* rc, ObjectsCache& cache,
	const std::string& basePath, uint32_t options)
{
	setName(stream.stringForKey(kName)->content);
	
	Dictionary blend = stream.dictionaryForKey(kBlendState);
	_blend.blendEnabled = blend.boolForKey(kBlendEnabled, false)->content != 0;
	_blend.color.source = stringToBlendFunction(blend.stringForKey(kSourceColor, blendFunctionToString(BlendFunction::One))->content);
	_blend.color.dest = stringToBlendFunction(blend.stringForKey(kDestColor, blendFunctionToString(BlendFunction::Zero))->content);
	_blend.colorOperation = stringToBlendOperation(blend.stringForKey(kColorOperation, blendOperationToString(BlendOperation::Add))->content);
	_blend.alpha.source = stringToBlendFunction(blend.stringForKey(kSourceAlpha, blendFunctionToString(BlendFunction::One))->content);
	_blend.alpha.dest = stringToBlendFunction(blend.stringForKey(kDestAlpha, blendFunctionToString(BlendFunction::Zero))->content);
	_blend.alphaOperation = stringToBlendOperation(blend.stringForKey(kAlphaOperation, blendOperationToString(BlendOperation::Add))->content);
	
	Dictionary depth = stream.dictionaryForKey(kDepthState);
	_depth.compareFunction = stringToCompareFunction(depth.stringForKey(kDepthFunction, compareFunctionToString(CompareFunction::Less))->content);
	_depth.depthWriteEnabled = depth.boolForKey(kDepthWriteEnabled, true)->content != 0;
	_depth.depthTestEnabled = depth.boolForKey(kDepthTestEnabled, true)->content != 0;

	bool shouldCreateTextures = (options & DeserializeOption_CreateTextures) == DeserializeOption_CreateTextures;
	Dictionary intValues = stream.dictionaryForKey(kIntegerValues);
	Dictionary floatValues = stream.dictionaryForKey(kFloatValues);
	Dictionary vectorValues = stream.dictionaryForKey(kVectorValues);
	Dictionary textureValues = stream.dictionaryForKey(kTextures);
	for (uint32_t i = 0; i < MaterialParameter_max; ++i)
	{
		const auto& key = materialKeys[i];

		if (intValues.hasKey(key))
			setInt(i, static_cast<int>(intValues.integerForKey(key)->content));

		if (floatValues.hasKey(key))
			setFloat(i, intValues.floatForKey(key)->content);

		if (vectorValues.hasKey(key))
			setVector(i, arrayToVec4(vectorValues.arrayForKey(key)));

		if (shouldCreateTextures && textureValues.hasKey(key))
		{
			auto textureFileName = textureValues.stringForKey(key)->content;
			
			if (!fileExists(textureFileName))
				textureFileName = basePath + textureFileName;
			
			setTexture(i, rc->textureFactory().loadTexture(textureFileName, cache));
		}
	}
}

Texture::Pointer Material::loadTexture(RenderContext* rc, const std::string& path, const std::string& basePath,
	ObjectsCache& cache, bool async)
{
	if (path.empty())
		return Texture::Pointer();

	auto paths = application().resolveFolderNames(basePath);
	application().pushSearchPaths(paths);
	auto result = rc->textureFactory().loadTexture(normalizeFilePath(path), cache, async, this);
	application().popSearchPaths(paths.size());
	
	return result;
}

void Material::clear()
{
	for (uint32_t i = 0; i < MaterialParameter_max; ++i)
	{
		_defaultIntParameters[i].set = 0;
		_defaultIntParameters[i].value = 0;
		_defaultFloatParameters[i].set = 0;
		_defaultFloatParameters[i].value = 0.0f;
		_defaultVectorParameters[i].set = 0;
		_defaultVectorParameters[i].value = vec4();
		_defaultStringParameters[i].set = 0;
		_defaultStringParameters[i].value = emptyString;
		_defaultTextureParameters[i].set = 0;
		_defaultTextureParameters[i].value = Texture::Pointer();
	}
	
	_customIntParameters.clear();
	_customFloatParameters.clear();
	_customVectorParameters.clear();
	_customTextureParameters.clear();
	_customStringParameters.clear();
}

/*
 * Setters / getters
 */
const int Material::getInt(uint32_t param) const
{
	if (param < MaterialParameter_max)
		return _defaultIntParameters[param].value;

	auto i = _customIntParameters.find(param);
	return i == _customIntParameters.end() ? 0 : i->second; 
}

const float Material::getFloat(uint32_t param) const
{ 
	if (param < MaterialParameter_max)
		return _defaultFloatParameters[param].value;

	auto i = _customFloatParameters.find(param);
	return i == _customFloatParameters.end() ? 0 : i->second; 
}

const vec4& Material::getVector(uint32_t param) const
{ 
	if (param < MaterialParameter_max)
		return _defaultVectorParameters[param].value;

	auto i = _customVectorParameters.find(param);
	return i == _customVectorParameters.end() ? nullVector : i->second;
}

const std::string& Material::getString(uint32_t param) const
{ 
	if (param < MaterialParameter_max)
		return _defaultStringParameters[param].value;

	auto i = _customStringParameters.find(param);
	return i == _customStringParameters.end() ? emptyString : i->second;
}

const Texture::Pointer& Material::getTexture(uint32_t param) const 
{ 
	if (param < MaterialParameter_max)
		return _defaultTextureParameters[param].value;

	auto i = _customTextureParameters.find(param);
	return i == _customTextureParameters.end() ? nullTexture : i->second;
}

void Material::setInt(uint32_t param, int value)
{
	if (param < MaterialParameter_max)
		_defaultIntParameters[param] = value;
	else
		_customIntParameters[param] = value; 
}

void Material::setFloat(uint32_t param, float value)
{
	if (param < MaterialParameter_max)
		_defaultFloatParameters[param] = value;
	else
		_customFloatParameters[param] = value; 
}

void Material::setVector(uint32_t param, const vec4& value)
{
	if (param < MaterialParameter_max)
		_defaultVectorParameters[param] = value;
	else
		_customVectorParameters[param] = value; 
}

void Material::setTexture(uint32_t param, const Texture::Pointer& value)
{
	if (param < MaterialParameter_max)
		_defaultTextureParameters[param] = value;
	else
		_customTextureParameters[param] = value; 
}

void Material::setString(uint32_t param, const std::string& value)
{
	if (param < MaterialParameter_max)
		_defaultStringParameters[param] = value;
	else
		_customStringParameters[param] = value; 
}

bool Material::hasVector(uint32_t param) const
{ 
	return (param < MaterialParameter_max) ? (_defaultVectorParameters[param].set > 0) : 
		(_customVectorParameters.find(param) != _customVectorParameters.end());
}

bool Material::hasFloat(uint32_t param) const
{
	return (param < MaterialParameter_max) ? (_defaultFloatParameters[param].set > 0) : 
		(_customFloatParameters.find(param) != _customFloatParameters.end());
}

bool Material::hasTexture(uint32_t param) const
{ 
	return (param < MaterialParameter_max) ? (_defaultTextureParameters[param].set > 0) : 
		(_customTextureParameters.find(param) != _customTextureParameters.end()); 
}

bool Material::hasInt(uint32_t param) const
{ 
	return (param < MaterialParameter_max) ? (_defaultIntParameters[param].set > 0) : 
		(_customIntParameters.find(param) != _customIntParameters.end()); 
}

bool Material::hasString(uint32_t param) const
{ 
	return (param < MaterialParameter_max) ? (_defaultStringParameters[param].set > 0) : 
		(_customStringParameters.find(param) != _customStringParameters.end()); 
}

void Material::reloadObject(LoadableObject::Pointer, ObjectsCache&)
{
	ET_FAIL("Material reloading is disabled.");
}

void Material::textureDidStartLoading(Texture::Pointer t)
{
	ET_ASSERT(t.valid());
	
#if (ET_DEBUG)
	bool pendingTextureFound = false;
	
	for (uint32_t i = 0; i < MaterialParameter_max; ++i)
	{
		if ((_defaultTextureParameters[i].value == t) && (_texturesToLoad.count(i) > 0))
		{
			pendingTextureFound = true;
			break;
		}
	}
	

	for (const auto& p : _customTextureParameters)
	{
		if ((p.second == t) && (_texturesToLoad.count(p.first) > 0))
		{
			pendingTextureFound = true;
			break;
		}
	}
	
	ET_ASSERT(pendingTextureFound);
#endif
}

void Material::textureDidLoad(Texture::Pointer t)
{
	ET_ASSERT(t.valid());
	
	uint32_t invalidParameter = static_cast<uint32_t>(-1);
	uint32_t param = invalidParameter;
	
	for (uint32_t i = 0; i < MaterialParameter_max; ++i)
	{
		if ((_defaultTextureParameters[i].value == t) && (_texturesToLoad.count(i) > 0))
		{
			param = i;
			break;
		}
	}
	
	if (param == invalidParameter)
	{
		for (const auto& p : _customTextureParameters)
		{
			if ((p.second == t) && (_texturesToLoad.count(p.first) > 0))
			{
				param = p.first;
				break;
			}
		}
	}
	
	ET_ASSERT(param != invalidParameter);
	
	_texturesToLoad.erase(param);
	
	if (_texturesToLoad.size() == 0)
		loaded.invokeInMainRunLoop(this);
}

void Material::setBlendState(const BlendState& bs)
{
	_blend = bs;
}

void Material::setDepthState(const DepthState& ds)
{
	_depth = ds;
}

uint32_t Material::sortingKey() const
{
	return _blend.sortingKey() << 16 | _depth.sortingKey();
}

/*
 * Support stuff
 */
extern const std::string materialKeys[MaterialParameter_max] =
{
	std::string("unknown"),				//	MaterialParameter_Undefined,
	std::string("ambient-color"),		//	MaterialParameter_AmbientColor,
	std::string("diffuse-color"),		//	MaterialParameter_DiffuseColor,
	std::string("specular-color"),		//	MaterialParameter_SpecularColor,
	std::string("emissive-color"),		//	MaterialParameter_EmissiveColor,
	std::string("ambient-map"),			//	MaterialParameter_AmbientMap,
	std::string("diffuse-map"),			//	MaterialParameter_DiffuseMap,
	std::string("specular-map"),		//	MaterialParameter_SpecularMap,
	std::string("emissive-map"),		//	MaterialParameter_EmissiveMap,
	std::string("normalmap-map"),		//	MaterialParameter_NormalMap,
	std::string("bump-map"),			//	MaterialParameter_BumpMap,
	std::string("reflection-map"),		//	MaterialParameter_ReflectionMap,
	std::string("transparency-map"),	//	MaterialParameter_TransparencyMap,
	std::string("ambient-factor"),		//	MaterialParameter_AmbientFactor,
	std::string("diffuse-factor"),		//	MaterialParameter_DiffuseFactor,
	std::string("specular-factor"),		//	MaterialParameter_SpecularFactor,
	std::string("bump-factor"),			//	MaterialParameter_BumpFactor,
	std::string("reflection-factor"),	//	MaterialParameter_ReflectionFactor,
	std::string("roughness"),			//	MaterialParameter_Roughness,
	std::string("transparency"),		//	MaterialParameter_Transparency,
	std::string("shading-model"),		//	MaterialParameter_ShadingModel,
	std::string("transparent-color"),	//	MaterialParameter_TransparentColor,
	std::string("opacity"),				//	MaterialParameter_Opacity,
};

size_t stringToMaterialParameter(const std::string& s)
{
	for (size_t i = 0; i < MaterialParameter_max; ++i)
	{
		if (s == materialKeys[i])
			return i;
	}
	
	return MaterialParameter_max + 1;
}
