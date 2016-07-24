/*
* This file is part of `et engine`
* Copyright 2009-2016 by Sergey Reznik
* Please, modify content only if you know what are you doing.
*
*/

#include <et/core/conversion.h>
#include <et/core/serialization.h>
#include <et/core/tools.h>
#include <et/core/cout.h>
#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/scene3d/scenematerial.h>

using namespace et;
using namespace et::s3d;

static const vec4 nullVector;
static const Texture::Pointer nullTexture = Texture::Pointer();

extern const String materialKeys[MaterialParameter_max];

SceneMaterial::SceneMaterial() :
	LoadableObject("default-material")
{
	setVector(MaterialParameter::DiffuseColor, vec4(1.0f));
}

SceneMaterial* SceneMaterial::duplicate() const
{
	SceneMaterial* m = etCreateObject<SceneMaterial>();
	
	m->setName(name());
	m->setOrigin(origin());
	m->_intParams = _intParams;
	m->_floatParams = _floatParams;
	m->_vectorParams = _vectorParams;
	m->_textureParams = _textureParams;

	return m;
}

void SceneMaterial::serialize(Dictionary stream, const std::string& /* basePath */)
{
	Dictionary intValues;
	Dictionary floatValues;
	Dictionary vectorValues;
	Dictionary textureValues;
	for (uint32_t ii = 0; ii < MaterialParameter_max; ++ii)
	{
		MaterialParameter i = static_cast<MaterialParameter>(ii);
		
		if (_intParams[i].set)
			intValues.setIntegerForKey(materialKeys[ii].c_str(), _intParams[i].value);

		if (_floatParams[i].set)
			floatValues.setFloatForKey(materialKeys[ii].c_str(), _floatParams[i].value);

		if (_vectorParams[i].set)
			vectorValues.setArrayForKey(materialKeys[ii].c_str(), vec4ToArray(_vectorParams[i].value));

		if (_textureParams[i].set && _textureParams[i].value.valid())
			textureValues.setStringForKey(materialKeys[ii].c_str(), _textureParams[i].value->origin());
	}
	
	stream.setStringForKey(kName, name());
	
	if (!intValues.empty())
		stream.setDictionaryForKey(kIntegerValues, intValues);

	if (!floatValues.empty())
		stream.setDictionaryForKey(kFloatValues, floatValues);

	if (!vectorValues.empty())
		stream.setDictionaryForKey(kVectorValues, vectorValues);

	if (!textureValues.empty())
		stream.setDictionaryForKey(kTextures, textureValues);
}

void SceneMaterial::deserializeWithOptions(Dictionary stream, RenderContext* rc, ObjectsCache& cache,
	const std::string& basePath, uint32_t options)
{
	setName(stream.stringForKey(kName)->content);

	bool shouldCreateTextures = (options & DeserializeOption_CreateTextures) == DeserializeOption_CreateTextures;
	Dictionary intValues = stream.dictionaryForKey(kIntegerValues);
	Dictionary floatValues = stream.dictionaryForKey(kFloatValues);
	Dictionary vectorValues = stream.dictionaryForKey(kVectorValues);
	Dictionary textureValues = stream.dictionaryForKey(kTextures);
	for (uint32_t ii = 0; ii < MaterialParameter_max; ++ii)
	{
		std::string key(materialKeys[ii].c_str());
		
		MaterialParameter i = static_cast<MaterialParameter>(ii);

		if (intValues.hasKey(key))
			setInt(i, static_cast<int32_t>(intValues.integerForKey(key)->content));

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

Texture::Pointer SceneMaterial::loadTexture(RenderContext* rc, const std::string& path, const std::string& basePath,
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

void SceneMaterial::clear()
{
	for (auto& i : _intParams)
		i.clear();
	for (auto& i : _floatParams)
		i.clear();
	for (auto& i : _vectorParams)
		i.clear();
	for (auto& i : _textureParams)
		i.clear();
}

/*
 * Setters / getters
 */
int64_t SceneMaterial::getInt(MaterialParameter param) const
{
	return _intParams[param].value;
}

float SceneMaterial::getFloat(MaterialParameter param) const
{
	return _floatParams[param].value;
}

const vec4& SceneMaterial::getVector(MaterialParameter param) const
{ 
	return _vectorParams[param].value;
}

const Texture::Pointer& SceneMaterial::getTexture(MaterialParameter param) const
{ 
	return _textureParams[param].value;
}

void SceneMaterial::setInt(MaterialParameter param, int64_t value)
{
	_intParams[param] = value;
}

void SceneMaterial::setFloat(MaterialParameter param, float value)
{
	_floatParams[param] = value;
}

void SceneMaterial::setVector(MaterialParameter param, const vec4& value)
{
	_vectorParams[param] = value;
}

void SceneMaterial::setTexture(MaterialParameter param, const Texture::Pointer& value)
{
	_textureParams[param] = value;
}

bool SceneMaterial::hasVector(MaterialParameter param) const
{ 
	return (_vectorParams[param].set > 0);
}

bool SceneMaterial::hasFloat(MaterialParameter param) const
{
	return (_floatParams[param].set > 0);
}

bool SceneMaterial::hasTexture(MaterialParameter param) const
{ 
	return (_textureParams[param].set > 0);
}

bool SceneMaterial::hasInt(MaterialParameter param) const
{ 
	return (_intParams[param].set > 0);
}

void SceneMaterial::reloadObject(LoadableObject::Pointer, ObjectsCache&)
{
	ET_FAIL("Material reloading is disabled.");
}

void SceneMaterial::textureDidStartLoading(Texture::Pointer t)
{
#if (ET_DEBUG)
	ET_ASSERT(t.valid());
	bool pendingTextureFound = false;
	
	for (uint32_t i = 0; i < MaterialParameter_max; ++i)
	{
		auto param = static_cast<MaterialParameter>(i);
		if ((_textureParams[param].value == t) && (_texturesToLoad.count(param) > 0))
		{
			pendingTextureFound = true;
			break;
		}
	}
	
	ET_ASSERT(pendingTextureFound);
#else
	(void)t;
#endif
}

void SceneMaterial::textureDidLoad(Texture::Pointer t)
{
	ET_ASSERT(t.valid());
	
	for (uint32_t i = 0; i < MaterialParameter_max; ++i)
	{
		auto param = static_cast<MaterialParameter>(i);
		if ((_textureParams[param].value == t) && (_texturesToLoad.count(param) > 0))
		{
			_texturesToLoad.erase(param);
			if (_texturesToLoad.size() == 0)
			{
				loaded.invokeInMainRunLoop(this);
				break;
			}
		}
	}
}

void SceneMaterial::bindToMaterial(et::Material::Pointer& m)
{
	for (uint32_t i = 0; i < MaterialParameter_max; ++i)
	{
		MaterialParameter param = static_cast<MaterialParameter>(i);
		if (hasTexture(param))
		{
			m->setTexutre(materialKeys[i], getTexture(param));
		}
		if (hasVector(param))
		{
			m->setProperty(materialKeys[i], getVector(param));
		}
		if (hasFloat(param))
		{
			m->setProperty(materialKeys[i], getFloat(param));
		}
		if (hasInt(param))
		{
			m->setProperty(materialKeys[i], static_cast<int32_t>(getInt(param)));
		}
	}
}

/*
 * Support stuff
 */
const String materialKeys[MaterialParameter_max] =
{
	String("ambient_map"), // AmbientMap,
	String("diffuse_map"), // DiffuseMap,
	String("specular_map"), // SpecularMap,
	String("emissive_map"), // EmissiveMap,
	String("normal_map"), // NormalMap,
	String("bump_map"), // BumpMap,
	String("reflection_map"), // ReflectionMap,
	String("opacity_map"), // OpacityMap,
	
	String("ambient_color"), // AmbientColor,
	String("diffuse_color"), // DiffuseColor,
	String("specular_color"), // SpecularColor,
	String("emissive_color"), // EmissiveColor,
	
	String("ambient_factor"), // AmbientFactor,
	String("diffuse_factor"), // DiffuseFactor,
	String("specular_factor"), // SpecularFactor,
	String("emissive_factor"), // EmissiveFactor,
	String("bump_factor"), // BumpFactor,
	
	String("transparency"), // Transparency,
	String("roughness"), // Roughness,
};
