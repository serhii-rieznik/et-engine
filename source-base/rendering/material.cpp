/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/renderstate.h>
#include <et/rendering/materialfactory.h>

using namespace et;

namespace
{
	const std::string kVertexSource = "vertex-source";
	const std::string kFragmentSource = "fragment-source";
	const std::string kDefines = "defines";
	const std::string kRenderPriority = "render-priority";
}

Material::Material(MaterialFactory* mf) :
	_factory(mf), _propertiesData(2 * sizeof(mat4), 0)
{
	_setFloatFunctions[uint32_t(DataType::Float)] = &Program::setFloatUniform;
	_setFloatFunctions[uint32_t(DataType::Vec2)] = &Program::setFloat2Uniform;
	_setFloatFunctions[uint32_t(DataType::Vec3)] = &Program::setFloat3Uniform;
	_setFloatFunctions[uint32_t(DataType::Vec4)] = &Program::setFloat4Uniform;
	_setFloatFunctions[uint32_t(DataType::Mat3)] = &Program::setMatrix3Uniform;
	_setFloatFunctions[uint32_t(DataType::Mat4)] = &Program::setMatrix4Uniform;
	
	_setIntFunctions[uint32_t(DataType::Int)] = &Program::setIntUniform;
	_setIntFunctions[uint32_t(DataType::IntVec2)] = &Program::setInt2Uniform;
	_setIntFunctions[uint32_t(DataType::IntVec3)] = &Program::setInt3Uniform;
	_setIntFunctions[uint32_t(DataType::IntVec4)] = &Program::setInt4Uniform;
}

void Material::loadFromJson(const std::string& jsonString, const std::string& baseFolder)
{
	Dictionary obj;
	if (obj.loadFromJson(jsonString) == false)
	{
		log::error("Unable to load material from json.");
		return;
	}
	
	application().pushSearchPath(baseFolder);
	
	auto name = obj.stringForKey(kName)->content;
	auto cullMode = obj.stringForKey(kCullMode)->content;
	auto definesArray = obj.arrayForKey(kDefines);
	auto vertexSource = application().resolveFileName(obj.stringForKey(kVertexSource)->content);
	auto fragmentSource = application().resolveFileName(obj.stringForKey(kFragmentSource)->content);
	
	application().popSearchPaths();
	
	StringList defines;
	defines.reserve(definesArray->content.size());
	
	auto addDefine = [&defines](std::string& define)
	{
		if (!define.empty())
		{
			std::transform(define.begin(), define.end(), define.begin(), [](char c)
				{ return (c == '=') ? ' ' : c; });
			defines.push_back("#define " + define + "\n");
		}
	};
	
	for (auto def : definesArray->content)
	{
		if (def->variantClass() == VariantClass::String)
		{
			addDefine(StringValue(def)->content);
		}
	}
	
	setName(name);
	setOrigin(baseFolder);
	
	_additionalPriority = static_cast<uint32_t>(obj.integerForKey(kRenderPriority, 0ll)->content);
	_blend = deserializeBlendState(obj.dictionaryForKey(kBlendState));
	_depth = deserializeDepthState(obj.dictionaryForKey(kDepthState));
	
	if (stringToCullMode(cullMode, _cullMode) == false)
	{
		log::warning("Invalid or unsupported cull mode in material: %s", cullMode.c_str());
	}
	
	_program = _factory->genProgram(name, loadTextFile(vertexSource), loadTextFile(fragmentSource), defines, baseFolder);
	_program->addOrigin(vertexSource);
	_program->addOrigin(fragmentSource);
	loadProperties();
}

void Material::enableInRenderState(RenderState& rs)
{
	rs.setCulling(_cullMode);
	rs.setBlendState(_blend);
	rs.setDepthState(_depth);

	for (auto& i : _textures)
	{
		rs.bindTexture(i.second.unit, i.second.texture);
	}
	
	rs.bindProgram(_program);
	for (auto& i : _properties)
	{
		if (i.second.requireUpdate)
		{
			applyProperty(i.second);
			i.second.requireUpdate = false;
		}
	}
}

void Material::loadProperties()
{
	uint32_t textureUnitCounter = 0;
	for (const auto& u : _program->uniforms())
	{
		String name(u.first.c_str());
		if (_program->isSamplerUniformType(u.second.type))
		{
			addTexture(name, u.second.location, textureUnitCounter);
			_program->setUniform(u.second.location, u.second.type, textureUnitCounter);
			++textureUnitCounter;
		}
		else if (!_program->isBuiltInUniformName(u.first))
		{
			addDataProperty(name, _program->uniformTypeToDataType(u.second.type), u.second.location);
		}
	}
}

void Material::addTexture(const String& name, int32_t location, uint32_t unit)
{
	_textures.emplace(name, TextureProperty(location, unit));
	_shouldUpdateSnapshot = true;
}

void Material::addDataProperty(const String& name, DataType type, int32_t location)
{
	uint32_t requiredSpace = dataTypeSize(type);
	uint32_t currentSize = _propertiesData.lastElementIndex();
	if (currentSize + requiredSpace > _propertiesData.size())
	{
		_propertiesData.resize(std::max(2 * _propertiesData.size(), size_t(currentSize + requiredSpace)));
	}
	_properties.emplace(name, DataProperty(type, location, currentSize, requiredSpace));
	_propertiesData.applyOffset(requiredSpace);
	_shouldUpdateSnapshot = true;
}

void Material::updateDataProperty(DataProperty& prop, const void* src)
{
	auto dst = _propertiesData.element_ptr(prop.offset);
	if (memcmp(src, dst, prop.length) != 0)
	{
		_shouldUpdateSnapshot = true;
		memcpy(dst, src, prop.length);
		prop.requireUpdate = true;
	}
}

#define ET_SET_PROPERTY_IMPL(TYPE) 	{ auto i = _properties.find(name); \
									if ((i != _properties.end()) && (i->second.type == DataType::TYPE)) { \
										updateDataProperty(i->second, &value); \
									}}

void Material::setProperty(const String& name, const float value) ET_SET_PROPERTY_IMPL(Float)
void Material::setProperty(const String& name, const vec2& value) ET_SET_PROPERTY_IMPL(Vec2)
void Material::setProperty(const String& name, const vec3& value) ET_SET_PROPERTY_IMPL(Vec3)
void Material::setProperty(const String& name, const vec4& value) ET_SET_PROPERTY_IMPL(Vec4)
void Material::setProperty(const String& name, const int value) ET_SET_PROPERTY_IMPL(Int)
void Material::setProperty(const String& name, const vec2i& value) ET_SET_PROPERTY_IMPL(IntVec2)
void Material::setProperty(const String& name, const vec3i& value) ET_SET_PROPERTY_IMPL(IntVec3)
void Material::setProperty(const String& name, const vec4i& value) ET_SET_PROPERTY_IMPL(IntVec4)
void Material::setProperty(const String& name, const mat3& value) ET_SET_PROPERTY_IMPL(Mat3)
void Material::setProperty(const String& name, const mat4& value) ET_SET_PROPERTY_IMPL(Mat4)

void Material::setTexutre(const String& name, const Texture::Pointer& tex)
{
	auto i = _textures.find(name);
	if ((i != _textures.end()) && (i->second.texture != tex))
	{
		i->second.texture = tex;
		_shouldUpdateSnapshot = true;
	}
}

uint32_t Material::sortingKey() const
{
	return _depth.sortingKey() | _blend.sortingKey() << 8 | _additionalPriority << 16;
}

uint64_t Material::makeSnapshot()
{
	if (_shouldUpdateSnapshot == false)
	{
		return _lastShapshotIndex;
	}
	
	if (_snapshots.empty())
		_snapshots.reserve(8);

	_snapshots.emplace_back();
	
	auto& snapshot = _snapshots.back();
	
	snapshot.blend = _blend;
	snapshot.depth = _depth;
	snapshot.cullMode = _cullMode;
	
	snapshot.textures.reserve(_textures.size());
	for (const auto& t : _textures)
	{
		snapshot.textures.emplace_back(t.second);
	}
	
	snapshot.properties.reserve(_properties.size());
	for (const auto& p : _properties)
	{
		snapshot.properties.emplace_back(p.second);
	}
	
	_shouldUpdateSnapshot = false;
	_lastShapshotIndex = _snapshots.size() - 1;
	return _lastShapshotIndex;
}

void Material::enableSnapshotInRenderState(RenderState& rs, uint64_t index)
{
	ET_ASSERT(index < _snapshots.size());
	
	const auto& snapshot = _snapshots.at(static_cast<size_t>(index));
	rs.setCulling(snapshot.cullMode);
	rs.setBlendState(snapshot.blend);
	rs.setDepthState(snapshot.depth);
	
	for (auto& i : snapshot.textures)
	{
		rs.bindTexture(i.unit, i.texture);
	}
	
	rs.bindProgram(_program);
	for (const auto& i : snapshot.properties)
	{
		applyProperty(i);
	}
}

void Material::applyProperty(const DataProperty& prop)
{
	auto format = dataTypeDataFormat(prop.type);
	auto ptr = _propertiesData.element_ptr(prop.offset);
	auto programPtr = _program.ptr();
	if (format == DataFormat::Int)
	{
		auto& fn = _setIntFunctions[uint32_t(prop.type)];
		(programPtr->*fn)(prop.locationInProgram, reinterpret_cast<const int*>(ptr), 1);
	}
	else
	{
		auto& fn = _setFloatFunctions[uint32_t(prop.type)];
		(programPtr->*fn)(prop.locationInProgram, reinterpret_cast<const float*>(ptr), 1);
	}
}

void Material::clearSnapshots()
{
	_snapshots.clear();
	_lastShapshotIndex = uint64_t(-1);
	_shouldUpdateSnapshot = true;
}
