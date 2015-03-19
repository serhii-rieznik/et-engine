/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/events.h>

namespace et
{
	class RenderContext;
	struct RenderContextParameters;
	template <typename T> struct vector2;
	typedef vector2<int> vec2i;
	
	struct ApplicationIdentifier
	{
		std::string identifier;
		std::string companyName;
		std::string applicationName;
		
		ApplicationIdentifier() { }
		
		ApplicationIdentifier(const std::string& aIdentifier, const std::string& aCompanyName,
			const std::string& aApplicationName) : identifier(aIdentifier), companyName(aCompanyName),
			applicationName(aApplicationName) { }
	};

	enum WindowStyle
	{
		WindowStyle_Borderless = 0x00,
		WindowStyle_Caption = 0x01,
		WindowStyle_Sizable = 0x02,
	};
	
	enum class WindowSize
	{
		Predefined,
		FillWorkarea,
		Fullscreen
	};

	struct ApplicationParameters
	{
		size_t windowStyle;
		WindowSize windowSize;
		bool shouldSuspendOnDeactivate;
		bool keepWindowAspectOnResize;

#if (ET_PLATFORM_IOS || ET_PLATFORM_ANDROID)
		ApplicationParameters() :
			windowStyle(WindowStyle_Borderless), windowSize(WindowSize::Predefined),
			shouldSuspendOnDeactivate(true), keepWindowAspectOnResize(false) { }
#else
		ApplicationParameters() :
			windowStyle(WindowStyle_Caption), windowSize(WindowSize::Predefined),
			shouldSuspendOnDeactivate(false), keepWindowAspectOnResize(false) { }
#endif
	};
	
	class IApplicationDelegate : virtual public EventReceiver
	{
	public:
		virtual ~IApplicationDelegate() { }

#	if !defined(ET_EMBEDDED_APPLICATION)
		virtual et::ApplicationIdentifier applicationIdentifier() const = 0;
#	endif

		virtual void setApplicationParameters(et::ApplicationParameters&) { }
		virtual void setRenderContextParameters(et::RenderContextParameters&) { }
		
		virtual void applicationDidLoad(et::RenderContext*) { }
		virtual void applicationWillActivate() { }
		virtual void applicationWillDeactivate() { }
		virtual void applicationWillSuspend() { }
		virtual void applicationWillResume() { }
		virtual void applicationWillTerminate() { }

		virtual void applicationWillResizeContext(const et::vec2i&) { }

		virtual void render(et::RenderContext*) { }
		virtual void idle(float) { }
	};

}
