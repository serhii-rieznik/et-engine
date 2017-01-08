/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	enum class MultisamplingQuality : uint32_t
	{
		None,
		Minimal,
		Best
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
		MultisamplingQuality multisamplingQuality = MultisamplingQuality::Best;
		
		bool multipleTouch = true;
		bool bindDefaultFramebufferEachFrame = true;
		bool lockScreenScaleToInitial = true;
		bool compatibilityProfile = false;
		bool debugContext = false;

		size_t swapInterval = 1;
        size_t supportedInterfaceOrientations = InterfaceOrientation_Any;

		vec2i contextTargetVersion = vec2i(4, 5);
	};
}
