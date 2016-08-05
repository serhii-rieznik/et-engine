/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/datastorage.h>
#include <et/rendering/interface/program.h>
#include <et/rendering/interface/texture.h>
#include <et/rendering/interface/renderstate.h>
#include <et/rendering/interface/pipelinestate.h>

namespace et
{
	class RenderState;
    class RenderInterface;
    
	class Material : public LoadableObject
	{
	public:
		ET_DECLARE_POINTER(Material);
		
	public:
        Material(RenderInterface*);
		
		void loadFromJson(const std::string&, const std::string& baseFolder);
		
		PipelineState::Pointer createPipelineState();

		void enableInRenderState(RenderState::Pointer);
		void enableSnapshotInRenderState(RenderState::Pointer, uint64_t);

		void setBlendState(const BlendState&);
		void setDepthState(const DepthState&);
		void setCullMode(CullMode);
		void setProgram(Program::Pointer);
		
		et::Program::Pointer& program()
			{ return _program; }
		
		const et::Program::Pointer& program() const
			{ return _program; }
		
		const DepthState& depthState() const
			{ return _depth; }
		
		const BlendState& blendState() const
			{ return _blend; }
		
		CullMode cullMode() const
			{ return _cull; }
		
		uint64_t makeSnapshot();
		void clearSnapshots();
		
		void setProperty(const String& name, const float value);
		void setProperty(const String& name, const vec2& value);
		void setProperty(const String& name, const vec3& value);
		void setProperty(const String& name, const vec4& value);
		void setProperty(const String& name, const int value);
		void setProperty(const String& name, const vec2i& value);
		void setProperty(const String& name, const vec3i& value);
		void setProperty(const String& name, const vec4i& value);
		void setProperty(const String& name, const mat3& value);
		void setProperty(const String& name, const mat4& value);
		
		void setTexutre(const String& name, const Texture::Pointer&);
		
		uint32_t sortingKey() const;

	private:
		struct DataProperty
		{
			DataType type = DataType::max;
			int32_t locationInProgram = -1;
			uint32_t offset = 0;
			uint32_t length = 0;
			bool requireUpdate = true;
			
			DataProperty(const DataProperty&) = default;
			DataProperty(DataType dt, int32_t loc, uint32_t o, uint32_t len) :
				type(dt), locationInProgram(loc), offset(o), length(len) { }
		};
		
		struct TextureProperty
		{
			int32_t locationInProgram = -1;
			uint32_t unit = 0;
			Texture::Pointer texture;
			
			TextureProperty(const TextureProperty&) = default;
			TextureProperty(int32_t loc, uint32_t u) :
				locationInProgram(loc), unit(u) { }
		};
		
		struct Snapshot
		{
			Vector<DataProperty> properties;
			Vector<TextureProperty> textures;
			BinaryDataStorage propertiesData;
		};
		
		void loadProperties();
		void addDataProperty(const String&, DataType, int32_t location);
		void addTexture(const String&, int32_t location, uint32_t unit);
		void updateDataProperty(DataProperty&, const void*);
		
		void applyProperty(const DataProperty&, const BinaryDataStorage&);
		
	public:
        RenderInterface* _renderer = nullptr;
		Vector<Snapshot> _snapshots;
		UnorderedMap<String, DataProperty> _properties;
		UnorderedMap<String, TextureProperty> _textures;

		Program::Pointer _program;
		DepthState _depth;
		BlendState _blend;
		CullMode _cull = CullMode::Disabled;

		BinaryDataStorage _propertiesData;
		uint32_t _additionalPriority = 0;
		uint64_t _lastShapshotIndex = uint64_t(-1);
		bool _shouldUpdateSnapshot = true;
	};
	
	class MaterialProvider
	{
	public:
		virtual Material::Pointer materialWithName(const std::string&) = 0;
	};
}
