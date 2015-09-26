/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/imaging/textureloaderthread.h>
#include <et/rendering/renderstate.h>
#include <et/scene3d/serialization.h>
#include <et/scene3d/material.parameters.h>

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
					IntrusivePtr<Material>(data = nullptr ? sharedObjectFactory().createObject<Material>() : data) { }
			};
			
			typedef std::vector<Material::Pointer> List;
			typedef std::map<std::string, Material::Pointer> Map;

		public:
			Material();
			~Material();
			
			const int getInt(uint32_t param) const;
			const float getFloat(uint32_t param) const;
			const vec4& getVector(uint32_t param) const;
			const std::string& getString(uint32_t param) const;
			const Texture::Pointer& getTexture(uint32_t param) const;

			void setInt(uint32_t param, int value);
			void setFloat(uint32_t param, float value);
			void setVector(uint32_t param, const vec4& value);
			void setTexture(uint32_t param, const Texture::Pointer& value);
			void setString(uint32_t param, const std::string& value);

			bool hasInt(uint32_t param) const;
			bool hasFloat(uint32_t param) const;
			bool hasVector(uint32_t param) const;
			bool hasTexture(uint32_t param) const;
			bool hasString(uint32_t param) const;

			BlendState blendState() const 
				{ return _blendState; }
			
			void setBlendState(BlendState bs)
				{ _blendState = bs; }
			
			void setDepthMask(bool dm)
				{ _depthMask = dm; }

			void serialize(Dictionary, const std::string&);
			
			void deserializeWithOptions(Dictionary, RenderContext*, ObjectsCache&, const std::string&,
				uint32_t);

			void clear();
			
			Material* duplicate() const;
			
			ET_DECLARE_EVENT1(loaded, Material*)
			
		public:
			int tag = 0;

		private:
			void reloadObject(LoadableObject::Pointer obj, ObjectsCache&);

			/*
			 * Textures loading stuff
			 */
			Texture::Pointer loadTexture(RenderContext* rc, const std::string& path,
				const std::string& basePath, ObjectsCache& cache, bool async);
			
			void textureDidStartLoading(Texture::Pointer);
			void textureDidLoad(Texture::Pointer);

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
			
			std::map<uint32_t, std::string> _texturesToLoad;
			
			BlendState _blendState = BlendState::Disabled;
			bool _depthMask = true;
		};
		
		typedef ObjectsCache MaterialCache;
	}
}
