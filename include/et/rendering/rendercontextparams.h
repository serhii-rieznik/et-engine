/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	vec2i nativeScreenSize();
	
	enum MultisamplingQuality
	{
		MultisamplingQuality_None,
		MultisamplingQuality_Best
	};
	
    enum InterfaceOrientation
    {
        InterfaceOrientation_Portrait = 0x01,
        InterfaceOrientation_PortraitUpsideDown = 0x02,
        InterfaceOrientation_LandscapeLeft = 0x04,
        InterfaceOrientation_LandscapeRight = 0x08,
        
        InterfaceOrientation_AnyPortrait =
			InterfaceOrientation_Portrait | InterfaceOrientation_PortraitUpsideDown,
		
        InterfaceOrientation_AnyLandscape =
			InterfaceOrientation_LandscapeLeft | InterfaceOrientation_LandscapeRight,
		
        InterfaceOrientation_Any =
			InterfaceOrientation_AnyPortrait | InterfaceOrientation_AnyLandscape
    };
	
	struct RenderContextParameters
	{
		MultisamplingQuality multisamplingQuality = MultisamplingQuality_Best;
		
		bool multipleTouch = true;
		bool bindDefaultFramebufferEachFrame = true;
		bool lockScreenScaleToInitial = true;
		bool compatibilityProfile = false;
		bool debugContext = false;
        bool enableHighResolutionContext = true;

		size_t swapInterval = 1;
        size_t supportedInterfaceOrientations = InterfaceOrientation_Any;

		vec2i contextSize;
		vec2i contextBaseSize;
		vec2i contextMinimumSize;
		vec2i contextTargetVersion;

		RenderContextParameters()
		{
#		if defined(ET_PLATFORM_IOS)
			contextSize = nativeScreenSize();
			contextBaseSize = nativeScreenSize();
#		else
			contextTargetVersion = vec2i(4, 5);
			contextSize = vec2i(800, 600);
			contextBaseSize = vec2i(800, 600);
#		endif
		}
	};
}
