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

Texture::Pointer SceneMaterial::loadTexture(RenderContext* rc, const std::string& path,
	const std::string& basePath, ObjectsCache& cache)
{
	if (path.empty())
		return Texture::Pointer();

	auto paths = application().resolveFolderNames(basePath);
	application().pushSearchPaths(paths);
	auto result = rc->renderer()->loadTexture(normalizeFilePath(path), cache);
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
	String("ambientMap"), // AmbientMap,
	String("albedoTexture"), // DiffuseMap,
	String("specularMap"), // SpecularMap,
	String("emissiveMap"), // EmissiveMap,
	String("normalMap"), // NormalMap,
	String("bumpMap"), // BumpMap,
	String("reflectionMap"), // ReflectionMap,
	String("opacityMap"), // OpacityMap,
	
	String("ambientColor"), // AmbientColor,
	String("diffuseColor"), // DiffuseColor,
	String("specularColor"), // SpecularColor,
	String("emissiveColor"), // EmissiveColor,
	
	String("ambientFactor"), // AmbientFactor,
	String("diffuseFactor"), // DiffuseFactor,
	String("specularFactor"), // SpecularFactor,
	String("emissiveFactor"), // EmissiveFactor,
	String("MaterialParameter::NormalTextureScale"), // MaterialParameter::NormalTextureScale,
	
	String("transparency"), // Transparency,
	String("roughness"), // Roughness,
};
