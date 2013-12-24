/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
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
	
	enum OpenGLProfile
	{
		OpenGLProfile_Core,
		OpenGLProfile_Compatibility
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
		vec2i openGLTargetVersion;

		MultisamplingQuality multisamplingQuality = MultisamplingQuality_Best;
		OpenGLProfile openGLProfile = OpenGLProfile_Core;
		
		bool openGLForwardContext = true;
		bool multipleTouch = true;

		size_t swapInterval = 1;
        size_t supportedInterfaceOrientations = InterfaceOrientation_Any;

		vec2i contextSize;
		vec2i contextBaseSize;

		RenderContextParameters()
		{
#if defined(ET_PLATFORM_IOS)
			contextSize = nativeScreenSize();
			contextBaseSize = nativeScreenSize();
#else
			openGLTargetVersion = vec2i(4, 3);
			contextSize = vec2i(800, 600);
			contextBaseSize = vec2i(800, 600);
#endif
		}
	};
}
