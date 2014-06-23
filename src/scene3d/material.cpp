/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <et/core/serialization.h>
#include <et/core/tools.h>
#include <et/core/cout.h>

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/scene3d/material.h>

using namespace et;
using namespace et::s3d;

static const Texture _emptyTexture;
static const std::string _emptyString;
static const vec4 _emptyVector;

static const char* etMaterialCstr = "material";
static const char* etNameCstr = "name";

static const char* kDefaultValues = "default_values";
static const char* kCustomValues = "custom_values";
static const char* kType = "type";
static const char* kValue = "value";
static const char* kSource = "source";
static const char* kKey = "key";
static const char* kCapacity = "capacity";
static const char* kInt = "int";
static const char* kFloat = "float";
static const char* kVector = "vector";
static const char* kTexture = "texture";
static const char* kString = "string";
static const char* kVersion = "version";
static const char* kDepthWrite = "depth_write";
static const char* kBlend = "blend";

const std::string materialKeys[MaterialParameter_max] =
{
	std::string(),						//	MaterialParameter_Undefined,
	
	std::string("ambient_color"),		//	MaterialParameter_AmbientColor,
	std::string("diffuse_color"),		//	MaterialParameter_DiffuseColor,
	std::string("specular_color"),		//	MaterialParameter_SpecularColor,
	std::string("emissive_color"),		//	MaterialParameter_EmissiveColor,
	std::string("ambient_map"),			//	MaterialParameter_AmbientMap,
	std::string("diffuse_map"),			//	MaterialParameter_DiffuseMap,
	std::string("specular_map"),		//	MaterialParameter_SpecularMap,
	std::string("emissive_map"),		//	MaterialParameter_EmissiveMap,
	std::string("normalmap_map"),		//	MaterialParameter_NormalMap,
	std::string("bump_map"),			//	MaterialParameter_BumpMap,
	std::string("reflection_map"),		//	MaterialParameter_ReflectionMap,

	std::string("ambient_factor"),		//	MaterialParameter_AmbientFactor,
	std::string("diffuse_factor"),		//	MaterialParameter_DiffuseFactor,
	std::string("specular_factor"),		//	MaterialParameter_SpecularFactor,
	std::string("bump_factor"),			//	MaterialParameter_BumpFactor,
	std::string("reflection_factor"),	//	MaterialParameter_ReflectionFactor,

	std::string("roughness"),			//	MaterialParameter_Roughness,
	std::string("transparency"),		//	MaterialParameter_Transparency,
	std::string("shading_model"),		//	MaterialParameter_ShadingModel,

	std::string("transparent_color"),	//	MaterialParameter_TransparentColor,
};

const int MaterialVersion1_0_2 = ET_COMPOSE_UINT32('M', 'A', 'T', '3');
const int MaterialVersion1_0_3 = ET_COMPOSE_UINT32('M', 'A', 'T', '4');
const int MaterialCurrentVersion = MaterialVersion1_0_3;

inline size_t keyToMaterialParameter(const std::string& k)
{
	for (size_t i = 0; i < MaterialParameter_max; ++i)
	{
		if (k == materialKeys[i])
			return i;
	}

	return MaterialParameter_Undefined;
}

Material::Material() :
	LoadableObject("default"),_blendState(BlendState_Disabled), _depthWriteEnabled(true), tag(0)
{
	setVector(MaterialParameter_DiffuseColor, vec4(1.0f));
}

Material* Material::duplicate() const
{
	Material* m = new Material();
	
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
	m->_blendState = _blendState;
	m->_depthWriteEnabled = _depthWriteEnabled;

	return m;
}

void Material::serialize(std::ostream& stream, StorageFormat format) const
{
	if (format == StorageFormat_Binary)
		serializeBinary(stream);
	else if (format == StorageFormat_HumanReadableMaterials)
		serializeReadable(stream);
	else
		ET_FAIL("Unknown storage format specified.");
}

template <typename T>
void keyValue(std::ostream& s, const std::string& key, const T& value)
	{ s << " " << key << "=\"" << value << "\""; }

#define START_BLOCK(NAME, TABS, E)	{ s << TABS << "<" << NAME; { E; } s << ">" << std::endl; }

#define END_BLOCK(NAME, TABS)		{ s << TABS << "</" << NAME << ">" << std::endl; }

#define SINGLE_BLOCK(NAME, TABS, E)	{ s << TABS << "<" << NAME; { E; } s << "/>" << std::endl; }


void Material::serializeReadable(std::ostream& s) const
{
	s << "<?xml version=\"1.0\" encoding='UTF-8'?>" << std::endl;

	START_BLOCK(etMaterialCstr, "",
		keyValue(s, etNameCstr, name());
		keyValue(s, kVersion, MaterialCurrentVersion);
		keyValue(s, kKey, intToStr(this));
		keyValue(s, kBlend, blendState());
		keyValue(s, kDepthWrite, depthWriteEnabled());
	);

	START_BLOCK(kDefaultValues, "\t",
		keyValue(s, kCapacity, MaterialParameter_max);
	)

	for (size_t i = 0; i < MaterialParameter_max; ++i)
	{
		if (_defaultIntParameters[i].set)
		{
			SINGLE_BLOCK(materialKeys[i], "\t\t",
				keyValue(s, kType, kInt);
				keyValue(s, kValue, _defaultIntParameters[i].value)
			);
		}

		if (_defaultFloatParameters[i].set)
		{
			SINGLE_BLOCK(materialKeys[i], "\t\t",
				keyValue(s, kType, kFloat);
				keyValue(s, kValue, _defaultFloatParameters[i].value)
			);
		}

		if (_defaultVectorParameters[i].set)
		{
			SINGLE_BLOCK(materialKeys[i], "\t\t",
				keyValue(s, kType, kVector);
				keyValue(s, kValue, _defaultVectorParameters[i].value);
			);
		}

		if (_defaultTextureParameters[i].set && _defaultTextureParameters[i].value.valid())
		{
			SINGLE_BLOCK(materialKeys[i], "\t\t",
				keyValue(s, kType, kTexture);
				keyValue(s, kSource, _defaultTextureParameters[i].value->origin());
			);
		}

		if (_defaultStringParameters[i].set && _defaultStringParameters[i].value.size())
		{
			SINGLE_BLOCK(materialKeys[i], "\t\t",
				keyValue(s, kType, kString);
				keyValue(s, kValue, _defaultStringParameters[i].value)
			);
		}
	}
	END_BLOCK(kDefaultValues, "\t");

	START_BLOCK(kCustomValues, "\t", ; )

	for (const auto& i : _customIntParameters)
		SINGLE_BLOCK(kValue, "\t\t", keyValue(s, kType, kInt); keyValue(s, kKey, i.first); keyValue(s, kValue, i.second))

	for (const auto& i : _customFloatParameters)
		SINGLE_BLOCK(kValue, "\t\t", keyValue(s, kType, kFloat); keyValue(s, kKey, i.first); keyValue(s, kValue, i.second))

	for (const auto& i : _customVectorParameters)
		SINGLE_BLOCK(kValue, "\t\t", keyValue(s, kType, kVector); keyValue(s, kKey, i.first); keyValue(s, kValue, i.second))

	for (const auto& i : _customStringParameters)
		SINGLE_BLOCK(kValue, "\t\t", keyValue(s, kType, kString); keyValue(s, kKey, i.first); keyValue(s, kValue, i.second))

	for (const auto& i : _customTextureParameters)
	{
		if (i.second.valid())
		{
			SINGLE_BLOCK(kValue, "\t\t",
				keyValue(s, kType, kTexture);
				keyValue(s, kKey, i.first);
				keyValue(s, kValue, i.second->origin()))
		}
	};

	END_BLOCK(kCustomValues, "\t")
	END_BLOCK(etMaterialCstr, "");
}

void Material::serializeBinary(std::ostream& stream) const
{
	serializeInt(stream, MaterialCurrentVersion);
	serializeString(stream, name());
	serializeInt(stream, blendState());
	serializeInt(stream, depthWriteEnabled());

	serializeInt(stream, MaterialParameter_max);
	for (size_t i = 0; i < MaterialParameter_max; ++i)
	{
		const Texture& t = _defaultTextureParameters[i].value;

		serializeInt(stream, _defaultIntParameters[i].set);
		serializeInt(stream, _defaultIntParameters[i].value);

		serializeInt(stream, _defaultFloatParameters[i].set);
		serializeFloat(stream, _defaultFloatParameters[i].value);

		serializeInt(stream, _defaultVectorParameters[i].set);
		serializeVector(stream, _defaultVectorParameters[i].value);

		serializeInt(stream, _defaultTextureParameters[i].set);
		serializeString(stream, t.valid() ? t->origin() : std::string());

		serializeInt(stream, _defaultStringParameters[i].set);
		serializeString(stream, _defaultStringParameters[i].value);
	}

	serializeInt(stream, static_cast<int>(_customIntParameters.size()));
	for (const auto& i : _customIntParameters)
	{
		serializeInt(stream, i.first);
		serializeInt(stream, i.second);
	}

	serializeInt(stream, static_cast<int>(_customFloatParameters.size()));
	for (const auto& i : _customFloatParameters)
	{
		serializeInt(stream, i.first);
		serializeFloat(stream, i.second);
	}

	serializeInt(stream, static_cast<int>(_customVectorParameters.size()));
	for (const auto& i : _customVectorParameters)
	{
		serializeInt(stream, i.first);
		serializeVector(stream, i.second);
	}

	serializeInt(stream, static_cast<int>(_customTextureParameters.size()));
	for (const auto& i : _customTextureParameters)
	{
		serializeInt(stream, i.first);
		std::string path = i.second.valid() ? i.second->origin() : std::string();
		serializeInt(stream, i.first);
		serializeString(stream, path);
	}

	serializeInt(stream, static_cast<int>(_customStringParameters.size()));
	for (const auto& i : _customStringParameters)
	{
		serializeInt(stream, i.first);
		serializeString(stream, i.second);
	}
}

void Material::deserialize(std::istream& stream, RenderContext* rc, ObjectsCache& cache,
	const std::string& basePath, StorageFormat format, bool async)
{
	_texturesToLoad.clear();
	
	if (format == StorageFormat_HumanReadableMaterials)
	{
		deserializeReadable(stream, rc, cache, basePath, async);
	}
	else if (format == StorageFormat_Binary)
	{
		int version = deserializeInt(stream);

		setName(deserializeString(stream));

		_blendState = static_cast<BlendState>(deserializeInt(stream));
		_depthWriteEnabled = deserializeInt(stream) != 0;

		if (version >= MaterialVersion1_0_2)
		{
			deserializeBinary(stream, rc, cache, basePath, async);
		}
		else
		{
			ET_FAIL("Legacy material versions are not supported anymore");
		}
	}
	else
	{
		ET_FAIL("Invalid storage format specified");
	}
	
	if (async && _texturesToLoad.empty())
		loaded.invokeInMainRunLoop(this);
	
	for (const auto& p : _texturesToLoad)
		setTexture(p.first, loadTexture(rc, p.second, basePath, cache, async));
}

void Material::deserializeBinary(std::istream& stream, RenderContext* rc, ObjectsCache& cache,
	const std::string& basePath, bool async)
{
	size_t numParameters = deserializeUInt(stream);
	for (size_t i = 0; i < numParameters; ++i)
	{
		int has = deserializeInt(stream);
		int ival = deserializeInt(stream);
		if (has)
		{
			setInt(i, ival);
		}

		has = deserializeInt(stream);
		float fval = deserializeFloat(stream);
		if (has)
		{
			setFloat(i, fval);
		}

		has = deserializeInt(stream);
		vec4 vval = deserializeVector<vec4>(stream);
		if (has)
		{
			setVector(i, vval);
		}

		has = deserializeInt(stream);
		std::string path = deserializeString(stream);
		if (has)
		{
			_texturesToLoad[i] = path;
		}

		has = deserializeInt(stream);
		std::string sval = deserializeString(stream);
		if (has)
		{
			setString(i, sval);
		}
	}

	int count = deserializeInt(stream);
	for (int i = 0; i < count; ++i)
	{
		size_t param = deserializeUInt(stream);
		int value = deserializeInt(stream);
		setInt(param, value);
	}
	
	count = deserializeInt(stream);
	for (int i = 0; i < count; ++i)
	{
		size_t param = deserializeUInt(stream);
		float value = deserializeFloat(stream);
		setFloat(param, value);
	}
	
	count = deserializeInt(stream);
	for (int i = 0; i < count; ++i)
	{
		size_t param = deserializeUInt(stream);
		vec4 value = deserializeVector<vec4>(stream);
		setVector(param, value);
	}
	
	count = deserializeInt(stream);
	for (int i = 0; i < count; ++i)
	{
		size_t param = deserializeUInt(stream);
		std::string path = deserializeString(stream);
		_texturesToLoad[param] = path;
	}
	
	count = deserializeInt(stream);
	for (int i = 0; i < count; ++i)
	{
		size_t param = deserializeUInt(stream);
		std::string value = deserializeString(stream);
		setString(param, value);
	}
}

void Material::loadProperties(xmlNode* root)
{
	for (xmlAttr* prop = root->properties; prop; prop = prop->next)
	{
		xmlChar* value = xmlNodeListGetString(prop->doc, prop->children, 1);
		const char* pName = reinterpret_cast<const char*>(prop->name);
		const char* pValue = reinterpret_cast<const char*>(value);
		if (strcmp(pName, etNameCstr) == 0)
			setName(std::string(pValue));
		else if (strcmp(pName, kDepthWrite) == 0)
			_depthWriteEnabled = strToBool(pValue);
		else if (strcmp(pName, kBlend) == 0)
			_blendState = static_cast<BlendState>(strToInt(pValue));
		else if (strcmp(pName, kKey) && strcmp(pName, kVersion))
		{
			log::warning("Unknown material property: %s = \"%s\"", pName, pValue);
		}
		xmlFree(value);
	}
}

void Material::loadDefaultValues(xmlNode* node, RenderContext* rc, ObjectsCache& cache, bool async)
{
	for (xmlNode* c = node->children; c; c = c->next)
	{
		if (c->type == XML_ELEMENT_NODE)
		{
			const char* cName = reinterpret_cast<const char*>(c->name);
			for (size_t i = 0; i < MaterialParameter_max; ++i)
			{
				if (materialKeys[i] == cName)
					loadDefaultValue(c, static_cast<MaterialParameters>(i), rc, cache, async);
			}
		}
	}
}

void Material::loadDefaultValue(xmlNode* node, MaterialParameters param, RenderContext* rc,
	ObjectsCache& cache, bool async)
{
	std::string type;
	std::string value;
	std::string source;

	for (xmlAttr* prop = node->properties; prop; prop = prop->next)
	{
		xmlChar* xmlValue = xmlNodeListGetString(prop->doc, prop->children, 1);
		const char* pName = reinterpret_cast<const char*>(prop->name);
		const char* pValue = reinterpret_cast<const char*>(xmlValue);

		if (strcmp(pName, kType) == 0)
			type = std::string(pValue);
		else if (strcmp(pName, kValue) == 0)
			value = std::string(pValue);
		else if (strcmp(pName, kSource) == 0)
			source = std::string(pValue);
		else
			log::warning("Unknown material parameter property: %s = \"%s\"", pName, pValue);

		xmlFree(xmlValue);
	}

	if (type == kInt)
	{
		setInt(param, strToInt(value));
	}
	else if (type == kFloat)
	{
		setFloat(param, strToFloat(value));
	}
	else if (type == kVector)
	{
		vec4 v;
		size_t i = 0;
		while ((value.find_first_of(ET_DEFAULT_DELIMITER) != std::string::npos) && (i < 4))
		{
			std::string subVal = value.substr(0, value.find_first_of(ET_DEFAULT_DELIMITER));
			v[i++] = strToFloat(subVal);
			value.erase(0, subVal.size() + 1);
		}
		if ((i < 4) && value.size())
			v[i++] = strToFloat(value);

		setVector(param, v);
	}
	else if (type == kString)
	{
		setString(param, value);
	}
	else if (type == kTexture)
	{
		_texturesToLoad[param] = source;
	}
	else
	{
		log::warning("Unknown type %s in material parameter %s", type.c_str(), materialKeys[param].c_str());
	}
}

void Material::deserializeReadable(std::istream& stream, RenderContext* rc, ObjectsCache& cache,
	const std::string& basePath, bool async)
{
	_texturesToLoad.clear();
	
	size_t size = streamSize(stream) - static_cast<size_t>(stream.tellg());
	StringDataStorage data(size + 1, 0);
	stream.read(data.data(), static_cast<std::streamsize>(size));

	xmlInitParser();
	xmlDoc* xml = xmlParseMemory(data.data(), static_cast<int>(size));
	if (xml == nullptr)
	{
		log::error("Unable to deserialize material from xml.");
		xmlCleanupParser();
		return;
	}

	xmlNode* root = xmlDocGetRootElement(xml);
	if ((root == nullptr) || (strcmp(reinterpret_cast<const char*>(root->name), "material") != 0))
	{
		log::error("Unable to deserialize material from xml.");
		xmlFreeDoc(xml);
		xmlCleanupParser();
		return;
	}

	loadProperties(root);

	for (xmlNode* c = root->children; c; c = c->next)
	{
		if (c->type == XML_ELEMENT_NODE)
		{
			const char* cName = reinterpret_cast<const char*>(c->name);
			if (strcmp(cName, kDefaultValues) == 0)
			{
				loadDefaultValues(c, rc, cache, async);
			}
			else if (strcmp(cName, kCustomValues) == 0)
			{

			}
			else
			{
				log::warning("Unknown material entry: %s", cName);
			}
		}
	}

	xmlFreeDoc(xml);
	xmlCleanupParser();
}

Texture Material::loadTexture(RenderContext* rc, const std::string& path, const std::string& basePath,
	ObjectsCache& cache, bool async)
{
	if (path.empty()) return Texture();

	auto paths = application().resolveFolderNames(basePath);
	application().pushSearchPaths(paths);
	Texture t = rc->textureFactory().loadTexture(normalizeFilePath(path), cache, async, this);
	application().popSearchPaths(paths.size());
	
	/*
	if (fileExists(path))
	{
		t ;
	}
	else
	{
		std::string relativePath = normalizeFilePath(basePath + getFileName(path));
		if (fileExists(relativePath))
		{
			t = rc->textureFactory().loadTexture(relativePath, cache, async, this);
		}
		else
		{
			relativePath = normalizeFilePath(basePath + path);
			if (fileExists(relativePath))
				t = rc->textureFactory().loadTexture(relativePath, cache, async, this);
		}
	}
	*/

	return t;
}

void Material::clear()
{
	for (size_t i = 0; i < MaterialParameter_max; ++i)
	{
		_defaultIntParameters[i].set = 0;
		_defaultIntParameters[i].value = 0;
		_defaultFloatParameters[i].set = 0;
		_defaultFloatParameters[i].value = 0.0f;
		_defaultVectorParameters[i].set = 0;
		_defaultVectorParameters[i].value = vec4();
		_defaultStringParameters[i].set = 0;
		_defaultStringParameters[i].value = std::string();
		_defaultTextureParameters[i].set = 0;
		_defaultTextureParameters[i].value = Texture();
	}
	
	_customIntParameters.clear();
	_customFloatParameters.clear();
	_customVectorParameters.clear();
	_customTextureParameters.clear();
	_customStringParameters.clear();
}

/*
 *
 * Setters / getters
 *
 */
const int Material::getInt(size_t param) const
{
	if (param < MaterialParameter_max)
		return _defaultIntParameters[param].value;

	auto i = _customIntParameters.find(param);
	return i == _customIntParameters.end() ? 0 : i->second; 
}

const float Material::getFloat(size_t param) const
{ 
	if (param < MaterialParameter_max)
		return _defaultFloatParameters[param].value;

	auto i = _customFloatParameters.find(param);
	return i == _customFloatParameters.end() ? 0 : i->second; 
}

const vec4& Material::getVector(size_t param) const
{ 
	if (param < MaterialParameter_max)
		return _defaultVectorParameters[param].value;

	auto i = _customVectorParameters.find(param);
	return i == _customVectorParameters.end() ? _emptyVector : i->second; 
}

const std::string& Material::getString(size_t param) const
{ 
	if (param < MaterialParameter_max)
		return _defaultStringParameters[param].value;

	auto i = _customStringParameters.find(param);
	return i == _customStringParameters.end() ? _emptyString : i->second; 
}

const Texture& Material::getTexture(size_t param) const 
{ 
	if (param < MaterialParameter_max)
		return _defaultTextureParameters[param].value;

	auto i = _customTextureParameters.find(param);
	return i == _customTextureParameters.end() ? _emptyTexture : i->second; 
}

void Material::setInt(size_t param, int value)
{
	if (param < MaterialParameter_max)
		_defaultIntParameters[param] = value;
	else
		_customIntParameters[param] = value; 
}

void Material::setFloat(size_t param, float value)
{
	if (param < MaterialParameter_max)
		_defaultFloatParameters[param] = value;
	else
		_customFloatParameters[param] = value; 
}

void Material::setVector(size_t param, const vec4& value)
{
	if (param < MaterialParameter_max)
		_defaultVectorParameters[param] = value;
	else
		_customVectorParameters[param] = value; 
}

void Material::setTexture(size_t param, const Texture& value)
{
	if (param < MaterialParameter_max)
		_defaultTextureParameters[param] = value;
	else
		_customTextureParameters[param] = value; 
}

void Material::setString(size_t param, const std::string& value)
{
	if (param < MaterialParameter_max)
		_defaultStringParameters[param] = value;
	else
		_customStringParameters[param] = value; 
}

bool Material::hasVector(size_t param) const
{ 
	return (param < MaterialParameter_max) ? (_defaultVectorParameters[param].set > 0) : 
		(_customVectorParameters.find(param) != _customVectorParameters.end());
}

bool Material::hasFloat(size_t param) const
{
	return (param < MaterialParameter_max) ? (_defaultFloatParameters[param].set > 0) : 
		(_customFloatParameters.find(param) != _customFloatParameters.end());
}

bool Material::hasTexture(size_t param) const
{ 
	return (param < MaterialParameter_max) ? (_defaultTextureParameters[param].set > 0) : 
		(_customTextureParameters.find(param) != _customTextureParameters.end()); 
}

bool Material::hasInt(size_t param) const
{ 
	return (param < MaterialParameter_max) ? (_defaultIntParameters[param].set > 0) : 
		(_customIntParameters.find(param) != _customIntParameters.end()); 
}

bool Material::hasString(size_t param) const
{ 
	return (param < MaterialParameter_max) ? (_defaultStringParameters[param].set > 0) : 
		(_customStringParameters.find(param) != _customStringParameters.end()); 
}

void Material::reloadObject(LoadableObject::Pointer obj, ObjectsCache& cache)
{
	ET_FAIL("Material reloading is disabled.");
	/*
	clear();
	
	InputStream stream(obj->origin(), StreamMode_Text);
	
	if (stream.valid())
		deserializeReadable(stream.stream(), nullptr, cache, getFilePath(obj->origin()), false);
	*/
}

void Material::textureDidStartLoading(Texture t)
{
	ET_ASSERT(t.valid())
	
#if (ET_DEBUG)
	bool pendingTextureFound = false;
	
	for (size_t i = 0; i < MaterialParameter_max; ++i)
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
	
	ET_ASSERT(pendingTextureFound)
#endif
}

void Material::textureDidLoad(Texture t)
{
	ET_ASSERT(t.valid())
	
	size_t invalidParameter(-1);
	size_t param = invalidParameter;
	
	for (size_t i = 0; i < MaterialParameter_max; ++i)
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
	
	ET_ASSERT(param != invalidParameter)
	
	_texturesToLoad.erase(param);
	
	if (_texturesToLoad.size() == 0)
		loaded.invokeInMainRunLoop(this);
}
