/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/apiobjects/texturedescription.h>

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
