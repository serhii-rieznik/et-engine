/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/apiobjects/textureloadingthread.h>
#include <et/rendering/renderstate.h>
#include <et/scene3d/serialization.h>
#include <et/scene3d/material.parameters.h>

typedef struct _xmlNode xmlNode;

namespace et
{
	namespace s3d
	{
		class Material : public LoadableObject, public TextureLoaderDelegate
		{
		public:
			struct Pointer : public IntrusivePtr<Material>
			{
				Pointer() :
					IntrusivePtr<Material>(sharedObjectFactory().createObject<Material>()) { }
				
				explicit Pointer(Material* data) :
					IntrusivePtr<Material>(data) { }
			};
			
			typedef std::vector<Material::Pointer> List;

		public:
			Material();
			
			const int getInt(size_t param) const;
			const float getFloat(size_t param) const;
			const vec4& getVector(size_t param) const;
			const std::string& getString(size_t param) const;
			const Texture& getTexture(size_t param) const;

			void setInt(size_t param, int value);
			void setFloat(size_t param, float value);
			void setVector(size_t param, const vec4& value);
			void setTexture(size_t param, const Texture& value);
			void setString(size_t param, const std::string& value);

			bool hasInt(size_t param) const;
			bool hasFloat(size_t param) const;
			bool hasVector(size_t param) const;
			bool hasTexture(size_t param) const;
			bool hasString(size_t param) const;

			void serialize(std::ostream& stream, StorageFormat format) const;

			void deserialize(std::istream& stream, RenderContext* rc, ObjectsCache& cache,
				const std::string& basePath, StorageFormat format, bool async);

			void clear();
			
			Material* duplicate() const;
			
			ET_DECLARE_EVENT1(loaded, Material*)
			
		public:
			ET_DECLARE_PROPERTY_GET_COPY_SET_COPY(BlendState, blendState, setBlendState)
			ET_DECLARE_PROPERTY_GET_COPY_SET_COPY(bool, depthWriteEnabled, setDepthWriteEnabled)

		public:
			int tag;

		private:
			void reloadObject(LoadableObject::Pointer obj, ObjectsCache&);
			
			void serializeBinary(std::ostream& stream) const;
			void serializeReadable(std::ostream& stream) const;

			void deserializeBinary(std::istream& stream, RenderContext* rc, ObjectsCache& cache,
				const std::string& basePath, bool async);
			
			void deserializeReadable(std::istream& stream, RenderContext* rc, ObjectsCache& cache,
				const std::string& basePath, bool async);

			/*
			 * Loading from XML
			 */
			void loadProperties(xmlNode*);

			void loadDefaultValues(xmlNode*, RenderContext* rc, ObjectsCache& cache, bool async);
			void loadDefaultValue(xmlNode*, MaterialParameters, RenderContext* rc, ObjectsCache& cache, bool async);

			/*
			 * Textures loading stuff
			 */
			Texture loadTexture(RenderContext* rc, const std::string& path,
				const std::string& basePath, ObjectsCache& cache, bool async);
			
			void textureDidStartLoading(Texture);
			void textureDidLoad(Texture);

		private:
			RenderContext* _rc;
			
			DefaultIntParameters _defaultIntParameters;
			DefaultFloatParameters _defaultFloatParameters;
			DefaultVectorParameters _defaultVectorParameters;
			DefaultTextureParameters _defaultTextureParameters;
			DefaultStringParameters _defaultStringParameters;

			CustomIntParameters _customIntParameters;
			CustomFloatParameters _customFloatParameters;
			CustomVectorParameters _customVectorParameters;
			CustomTextureParameters _customTextureParameters;
			CustomStringParameters _customStringParameters;
			
			std::map<size_t, std::string> _texturesToLoad;
		};
		
		typedef ObjectsCache MaterialCache;
	}
}
