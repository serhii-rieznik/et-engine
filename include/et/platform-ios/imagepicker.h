/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/imaging/texturedescription.h>

namespace et
{
	enum class ImagePickerSource
	{
		Camera,
		PhotoAlbum,
		PreferCamera
	};
	
	class ImagePickerPrivate;
	class ImagePicker
	{
	public:
		ImagePicker();
		~ImagePicker();
		
		void selectImage(ImagePickerSource source = ImagePickerSource::PreferCamera);
		
		ET_DECLARE_EVENT1(imageSelected, TextureDescription::Pointer)
		ET_DECLARE_EVENT0(cancelled)
		
	private:
		ET_DECLARE_PIMPL(ImagePicker, 32)
	};
}
