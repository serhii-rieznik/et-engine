/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/objectscache.h>
#include <et/app/application.h>
#include <et/threading/threading.h>
#include <et/imaging/textureloader.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/texturefactory.h>

namespace et
{
	float randomFloat(float, float);

	class TextureFactoryPrivate
	{
	public:
		struct Loader : public ObjectLoader
		{
			TextureFactory* owner = nullptr;
			
			Loader(TextureFactory* aOwner) :
				owner(aOwner) { }
			
			void reloadObject(LoadableObject::Pointer o, ObjectsCache& c)
				{ owner->reloadObject(o, c); }
		};
		
		TextureFactoryPrivate(TextureFactory* owner) :
		loader(sharedObjectFactory().createObject<Loader>(owner))
		{
			supportedExtensions.push_back("png");
			supportedExtensions.push_back("dds");
			supportedExtensions.push_back("pvr");
			supportedExtensions.push_back("jpg");
			supportedExtensions.push_back("tga");
			supportedExtensions.push_back("hdr");
			supportedExtensions.push_back("jpeg");
		}
		
	public:
		IntrusivePtr<Loader> loader;
		StringList supportedExtensions;
	};
}

using namespace et;

TextureFactory::TextureFactory(RenderContext* rc) :
	APIObjectFactory(rc)
{
	ET_PIMPL_INIT(TextureFactory, this)
	
	_loadingThread = sharedObjectFactory().createObject<TextureLoadingThread>(this);
}

TextureFactory::~TextureFactory()
{
	_loadingThread->stop();
	_loadingThread->waitForTermination();

	ET_PIMPL_FINALIZE(TextureFactory)
}

const StringList& TextureFactory::supportedTextureExtensions() const
{
	return _private->supportedExtensions;
}

ObjectLoader::Pointer TextureFactory::objectLoader()
{
	return _private->loader;
}

Texture::Pointer TextureFactory::loadTexture(const std::string& fileName, ObjectsCache& cache,
	bool async, TextureLoaderDelegate* delegate)
{
	if (fileName.length() == 0)
		return Texture::Pointer();
	
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
			return Texture::Pointer();
	}
	
	uint64_t cachedFileProperty = 0;
    Texture::Pointer texture = cache.findAnyObject(file, &cachedFileProperty);
	if (texture.invalid())
	{
		TextureDescription::Pointer desc =
			async ? et::loadTextureDescription(file, false) : et::loadTexture(file);
		
		int maxTextureSize = static_cast<int>(RenderingCapabilities::instance().maxTextureSize());
		if ((desc->size.x > maxTextureSize) || (desc->size.y > maxTextureSize))
		{
			log::warning("Attempt to load texture with dimensions (%d x %d) larger than max allowed (%d)",
				desc->size.x, desc->size.y, maxTextureSize);
		}
		
		if (desc.valid())
		{
			bool calledFromAnotherThread = Threading::currentThread() != threading().renderingThread();
			
			texture = Texture::Pointer::create(renderContext(), desc, desc->origin(), async || calledFromAnotherThread);
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

Texture::Pointer TextureFactory::genTexture(TextureTarget target, TextureFormat internalformat, const vec2i& size,
	TextureFormat format, DataType type, const BinaryDataStorage& data, const std::string& id)
{
	TextureDescription::Pointer desc = TextureDescription::Pointer::create();
	
	desc->target = target;
	
	desc->format = format;
	desc->internalformat = internalformat;
	desc->type = type;
	
	desc->size = size;
	
	desc->mipMapCount = 1;
	desc->layersCount = 1;
	desc->bitsPerPixel = bitsPerPixelForTextureFormat(internalformat, type);
	desc->channels = channelsForTextureFormat(internalformat);
	
	desc->data = data;
	
	return Texture::Pointer::create(renderContext(), desc, id, false);
}

Texture::Pointer TextureFactory::genCubeTexture(TextureFormat internalformat, uint32_t size, TextureFormat format,
	DataType type, const std::string& aName)
{
	TextureDescription::Pointer desc = TextureDescription::Pointer::create();
	
	desc->target = TextureTarget::Texture_Cube;
	
	desc->format = format;
	desc->internalformat = internalformat;
	desc->type = type;
	
	desc->size = vec2i(size);
	
	desc->mipMapCount = 1;
	desc->layersCount = 6;
	desc->bitsPerPixel = bitsPerPixelForTextureFormat(internalformat, type);
	
	desc->data = BinaryDataStorage(desc->layersCount * desc->dataSizeForAllMipLevels(), 0);
	
	return Texture::Pointer::create(renderContext(), desc, aName, false);
}

Texture::Pointer TextureFactory::genTexture2DArray(const vec3i& size, TextureTarget textureTarget, 
	TextureFormat internalformat, TextureFormat format, DataType type, const BinaryDataStorage& data,
	const std::string& aName)
{
	ET_ASSERT(textureTarget == TextureTarget::Texture_2D_Array)

	TextureDescription::Pointer desc = TextureDescription::Pointer::create();

	desc->target = textureTarget;

	desc->format = format;
	desc->internalformat = internalformat;
	desc->type = type;

	desc->size = size.xy();

	desc->mipMapCount = 1;
	desc->layersCount = size.z;
	desc->bitsPerPixel = bitsPerPixelForTextureFormat(internalformat, type);
	desc->channels = channelsForTextureFormat(internalformat);

	desc->data = data;

	return Texture::Pointer::create(renderContext(), desc, aName, false);
}

Texture::Pointer TextureFactory::genTexture(TextureDescription::Pointer desc)
{
	return Texture::Pointer::create(renderContext(), desc, desc->origin(), false);
}

Texture::Pointer TextureFactory::genNoiseTexture(const vec2i& size, bool norm, const std::string& id)
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

	TextureDescription::Pointer desc = TextureDescription::Pointer::create();
	desc->data = BinaryDataStorage(4 * size.square());
	desc->target = TextureTarget::Texture_2D;
	desc->internalformat = TextureFormat::RGBA;
	desc->format = TextureFormat::RGBA;
	desc->type = DataType::UnsignedChar;
	desc->size = size;
	desc->mipMapCount = 1;
	desc->layersCount = 1;
    desc->bitsPerPixel = 32;
    
	etCopyMemory(desc->data.data(), randata.data(), randata.dataSize());

	return Texture::Pointer::create(renderContext(), desc, id, false);
}

void TextureFactory::textureLoadingThreadDidLoadTextureData(TextureLoadingRequest* request)
{
	CriticalSectionScope lock(_csTextureLoading);

	request->texture->updateData(renderContext(), request->textureDescription);
	textureDidLoad.invoke(request->texture);

	if (request->delegate)
		request->delegate->textureDidLoad(request->texture);

	sharedObjectFactory().deleteObject(request);
}

Texture::Pointer TextureFactory::loadTexturesToCubemap(const std::string& posx, const std::string& negx,
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

	int maxCubemapSize = static_cast<int>(RenderingCapabilities::instance().maxCubemapTextureSize());
	
	for (size_t l = 0; l < 6; ++l)
	{
		if (layers[l].valid())
		{
			if ((layers[l]->size.x > maxCubemapSize) || (layers[l]->size.y > maxCubemapSize))
			{
				log::error("Cubemap %s size of (%d x %d) is larger than allowed %dx%d",
					layers[l]->origin().c_str(), layers[l]->size.x, layers[l]->size.y, maxCubemapSize, maxCubemapSize);
				return Texture::Pointer();
			}
		}
		else
		{
			log::error("Unable to load cubemap face.");
			return Texture::Pointer();
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
			return Texture::Pointer();
		}
	}

	size_t layerSize = layers[0]->dataSizeForAllMipLevels();
	TextureDescription::Pointer desc = TextureDescription::Pointer::create();
	desc->target = TextureTarget::Texture_Cube;
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

	Texture::Pointer result = Texture::Pointer::create(renderContext(), desc, texId, false);
	
	for (size_t i = 0; i < 6; ++i)
		result->addOrigin(layers[i]->origin());
	
	cache.manage(result, _private->loader);
	
	return result;
}

Texture::Pointer TextureFactory::createTextureWrapper(uint32_t texture, const vec2i& size, const std::string& name)
{
	return Texture::Pointer::create(renderContext(), texture, size, name);
}

void TextureFactory::reloadObject(LoadableObject::Pointer object, ObjectsCache&)
{
	TextureDescription::Pointer newData = et::loadTexture(object->origin());
	
	if (newData.valid())
		Texture::Pointer(object)->updateData(renderContext(), newData);
}
