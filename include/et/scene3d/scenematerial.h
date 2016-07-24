/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
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
	class Material;
	namespace s3d
	{
		class SceneMaterial : public LoadableObject, public TextureLoaderDelegate
		{
		public:
			ET_DECLARE_POINTER(SceneMaterial);
			using Collection = Vector<SceneMaterial::Pointer>;
			using Map = et::UnorderedMap<std::string, SceneMaterial::Pointer>;

		public:
			SceneMaterial();
			~SceneMaterial() = default;
			
			int64_t getInt(MaterialParameter param) const;
			float getFloat(MaterialParameter param) const;
			const vec4& getVector(MaterialParameter param) const;
			const Texture::Pointer& getTexture(MaterialParameter param) const;

			void setInt(MaterialParameter param, int64_t value);
			void setFloat(MaterialParameter param, float value);
			void setVector(MaterialParameter param, const vec4& value);
			void setTexture(MaterialParameter param, const Texture::Pointer& value);

			bool hasInt(MaterialParameter param) const;
			bool hasFloat(MaterialParameter param) const;
			bool hasVector(MaterialParameter param) const;
			bool hasTexture(MaterialParameter param) const;
			
			void serialize(Dictionary, const std::string& basePath);
			void deserializeWithOptions(Dictionary, RenderContext*, ObjectsCache&, const std::string&, uint32_t);
			void clear();
			
			SceneMaterial* duplicate() const;
			
			ET_DECLARE_EVENT1(loaded, SceneMaterial*)
			
			void bindToMaterial(et::IntrusivePtr<Material>&);

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
			ParameterSet<int64_t> _intParams;
			ParameterSet<float> _floatParams;
			ParameterSet<vec4> _vectorParams;
			ParameterSet<Texture::Pointer> _textureParams;
			std::map<MaterialParameter, std::string> _texturesToLoad;
		};
		
		typedef ObjectsCache MaterialCache;
	}
}
