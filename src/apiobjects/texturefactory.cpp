/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/objectscache.h>
#include <et/geometry/geometry.h>
#include <et/rendering/rendercontext.h>
#include <et/opengl/openglcaps.h>
#include <et/threading/threading.h>
#include <et/imaging/textureloader.h>
#include <et/apiobjects/texturefactory.h>
#include <et/app/application.h>

using namespace et;

class et::TextureFactoryPrivate
{
public:
	struct Loader : public ObjectLoader
	{
		TextureFactory* owner;

		Loader(TextureFactory* aOwner) : 
			owner(aOwner) { }

		void reloadObject(LoadableObject::Pointer o, ObjectsCache& c)
			{ owner->reloadObject(o, c); }
	};

	IntrusivePtr<Loader> loader;

	TextureFactoryPrivate(TextureFactory* owner) : 
		loader(new Loader(owner))
	{
		supportedExtensions.push_back("png");
		supportedExtensions.push_back("pvr");
		supportedExtensions.push_back("jpg");
		supportedExtensions.push_back("jpeg");
		supportedExtensions.push_back("dds");
		supportedExtensions.push_back("hdr");
	}
	
	StringList supportedExtensions;
};

TextureFactory::TextureFactory(RenderContext* rc) :
	APIObjectFactory(rc)
{
	_private = new TextureFactoryPrivate(this);
	_loadingThread = new TextureLoadingThread(this);
}

TextureFactory::~TextureFactory()
{
	_loadingThread->stop();
	_loadingThread->waitForTermination();

	delete _private;
}

ObjectLoader::Pointer TextureFactory::objectLoader()
{
	return _private->loader;
}

Texture TextureFactory::loadTexture(const std::string& fileName, ObjectsCache& cache,
	bool async, TextureLoaderDelegate* delegate)
{
	if (fileName.length() == 0)
		return Texture();
	
	CriticalSectionScope lock(_csTextureLoading);
	
	auto file = application().resolveFileName(fileName);
	
	if (!fileExists(file))
	{
		auto fileExt = lowercase(getFileExt(fileName));
		for (const auto& ext : _private->supportedExtensions)
		{
			if (ext == fileExt) continue;
			
			file = replaceFileExt(fileName, "." + ext);
			file = application().resolveFileName(file);
			
			if (fileExists(file))
				break;
		}
		
		if (!fileExists(file))
			return Texture();
	}
	
	uint64_t cachedFileProperty = 0;
    Texture texture = cache.findAnyObject(file, &cachedFileProperty);
	if (texture.invalid())
	{
		TextureDescription::Pointer desc =
			async ? et::loadTextureDescription(file, false) : et::loadTexture(file);

		if (desc.valid())
		{
			bool calledFromAnotherThread = Threading::currentThread() != threading().renderingThread();
			
			texture = Texture(new TextureData(renderContext(), desc, desc->origin(), async || calledFromAnotherThread));
			cache.manage(texture, _private->loader);
			
			if (async)
				_loadingThread->addRequest(desc->origin(), texture, delegate);
			else if (calledFromAnotherThread)
				ET_FAIL("ERROR: Unable to load texture synchronously from non-rendering thread.");
		}
		
	}
	else
	{
		auto newProperty = cache.getFileProperty(file);
		if (cachedFileProperty != newProperty)
			reloadObject(texture, cache);
	
		if (async)
		{
			textureDidStartLoading.invokeInMainRunLoop(texture);
			if (delegate != nullptr)
			{
				Invocation1 i;
				i.setTarget(delegate, &TextureLoaderDelegate::textureDidStartLoading, texture);
				i.invokeInMainRunLoop();
			}
			
			textureDidLoad.invokeInMainRunLoop(texture);
			if (delegate != nullptr)
			{
				Invocation1 i;
				i.setTarget(delegate, &TextureLoaderDelegate::textureDidLoad, texture);
				i.invokeInMainRunLoop();
			}
		}

	}
   
	return texture;
}

Texture TextureFactory::genTexture(uint32_t target, int32_t internalformat, const vec2i& size,
	uint32_t format, uint32_t type, const BinaryDataStorage& data, const std::string& id)
{
	TextureDescription::Pointer desc(new TextureDescription);
	
	desc->target = target;
	
	desc->format = format;
	desc->internalformat = internalformat;
	desc->type = type;
	
	desc->size = size;
	
	desc->mipMapCount = 1;
	desc->layersCount = 1;
	desc->bitsPerPixel = bitsPerPixelForTextureFormat(internalformat, type);
	
	desc->data = data;
	
	return Texture(new TextureData(renderContext(), desc, id, false));
}

Texture TextureFactory::genCubeTexture(int32_t internalformat, GLsizei size, uint32_t format, uint32_t type,
	const std::string& id)
{
	TextureDescription::Pointer desc(new TextureDescription);
	
	desc->target = GL_TEXTURE_CUBE_MAP;
	
	desc->format = format;
	desc->internalformat = internalformat;
	desc->type = type;
	
	desc->size = vec2i(size);
	
	desc->mipMapCount = 1;
	desc->layersCount = 6;
	desc->bitsPerPixel = bitsPerPixelForTextureFormat(internalformat, type);
	
	desc->data = BinaryDataStorage(desc->layersCount * desc->dataSizeForAllMipLevels(), 0);
	
	return Texture(new TextureData(renderContext(), desc, id, false));
}

Texture TextureFactory::genTexture(TextureDescription::Pointer desc)
{
	return Texture(new TextureData(renderContext(), desc, desc->origin(), false));
}

Texture TextureFactory::genNoiseTexture(const vec2i& size, bool norm, const std::string& id)
{
	DataStorage<vec4ub> randata(size.square());
	for (size_t i = 0; i < randata.size(); ++i)
	{
		vec4 rand_f = vec4(randomFloat(-1.0f, 1.0f), randomFloat(-1.0f, 1.0f),
			randomFloat(-1.0f, 1.0f), randomFloat(-1.0f, 1.0f));
		
		if (norm)
			rand_f.xyz().normalize();
		
		randata[i].x = static_cast<unsigned char>(255.0f * clamp(0.5f + 0.5f * rand_f.x, 0.0f, 1.0f));
		randata[i].y = static_cast<unsigned char>(255.0f * clamp(0.5f + 0.5f * rand_f.y, 0.0f, 1.0f));
		randata[i].z = static_cast<unsigned char>(255.0f * clamp(0.5f + 0.5f * rand_f.z, 0.0f, 1.0f));
		randata[i].w = static_cast<unsigned char>(255.0f * clamp(0.5f + 0.5f * rand_f.w, 0.0f, 1.0f));
	}

	TextureDescription::Pointer desc(new TextureDescription);
	desc->data = BinaryDataStorage(4 * size.square());
	desc->target = GL_TEXTURE_2D;
	desc->format = GL_RGBA;
	desc->internalformat = GL_RGBA;
	desc->type = GL_UNSIGNED_BYTE;
	desc->size = size;
	desc->mipMapCount = 1;
	desc->layersCount = 1;
    desc->bitsPerPixel = 32;
    
	etCopyMemory(desc->data.data(), randata.data(), randata.dataSize());

	return Texture(new TextureData(renderContext(), desc, id, false));
}

void TextureFactory::textureLoadingThreadDidLoadTextureData(TextureLoadingRequest* request)
{
	CriticalSectionScope lock(_csTextureLoading);

	request->texture->updateData(renderContext(), request->textureDescription);
	textureDidLoad.invoke(request->texture);

	if (request->delegate)
		request->delegate->textureDidLoad(request->texture);

	delete request;
}

Texture TextureFactory::loadTexturesToCubemap(const std::string& posx, const std::string& negx,
	const std::string& posy, const std::string& negy, const std::string& posz, const std::string& negz,
	ObjectsCache& cache)
{
	TextureDescription::Pointer layers[6] = 
	{
		et::loadTexture(application().resolveFileName(posx)),
		et::loadTexture(application().resolveFileName(negx)),
		et::loadTexture(application().resolveFileName(negy)),
		et::loadTexture(application().resolveFileName(posy)),
		et::loadTexture(application().resolveFileName(posz)),
		et::loadTexture(application().resolveFileName(negz))
	};

	int maxCubemapSize = static_cast<int>(openGLCapabilites().maxCubemapTextureSize());
	
	for (size_t l = 0; l < 6; ++l)
	{
		if (layers[l].valid())
		{
			if ((layers[l]->size.x > maxCubemapSize) || (layers[l]->size.y > maxCubemapSize))
			{
				log::error("Cubemap %s size of (%d x %d) is larger than allowed %dx%d",
					layers[l]->origin().c_str(), layers[l]->size.x, layers[l]->size.y, maxCubemapSize, maxCubemapSize);
				return Texture();
			}
		}
		else
		{
			log::error("Unable to load cubemap face.");
			return Texture();
		}
	}

	std::string texId = layers[0]->origin() + ";";
	for (size_t l = 1; l < 6; ++l)
	{
		texId += (l < 5) ? layers[l]->origin() + ";" : layers[l]->origin();
		if ((layers[l-1]->size != layers[l]->size) || 
			(layers[l-1]->format != layers[l]->format) ||
			(layers[l-1]->internalformat != layers[l]->internalformat) || 
			(layers[l-1]->type != layers[l]->type) || 
			(layers[l-1]->mipMapCount != layers[l]->mipMapCount) || 
			(layers[l-1]->compressed != layers[l]->compressed) ||
			(layers[l-1]->data.size() != layers[l]->data.size()))
		{
			log::error("Failed to load cubemap textures. Textures `%s` and `%s` aren't identical",
				layers[l-1]->origin().c_str(), layers[l]->origin().c_str());
			return Texture();
		}
	}

	size_t layerSize = layers[0]->dataSizeForAllMipLevels();
	TextureDescription::Pointer desc(new TextureDescription);
	desc->target = GL_TEXTURE_CUBE_MAP;
	desc->layersCount = 6;
	desc->bitsPerPixel = layers[0]->bitsPerPixel;
	desc->channels = layers[0]->channels;
	desc->compressed = layers[0]->compressed;
	desc->format = layers[0]->format;
	desc->internalformat = layers[0]->internalformat;
	desc->mipMapCount= layers[0]->mipMapCount;
	desc->size = layers[0]->size;
	desc->type = layers[0]->type;
	desc->data.resize(desc->layersCount * layerSize);
	for (size_t l = 0; l < desc->layersCount; ++l)
		etCopyMemory(desc->data.element_ptr(l * layerSize), layers[l]->data.element_ptr(0), layerSize);

	Texture result(new TextureData(renderContext(), desc, texId, false));
	
	for (size_t i = 0; i < 6; ++i)
		result->addOrigin(layers[i]->origin());
	
	cache.manage(result, _private->loader);
	
	return result;
}

Texture TextureFactory::createTextureWrapper(uint32_t texture, const vec2i& size, const std::string& name)
{
	return Texture(new TextureData(renderContext(), texture, size, name));
}

void TextureFactory::reloadObject(LoadableObject::Pointer object, ObjectsCache&)
{
	TextureDescription::Pointer newData = et::loadTexture(object->origin());
	if (newData.valid())
		Texture(object)->updateData(renderContext(), newData);
}
