#include <et/core/et.h>

#import <Foundation/Foundation.h>

#if (ET_PLATFORM_MAC)
#	import <AppKit/AppKit.h>
#endif

#include "../platform-apple/locale.apple.mm"
#include "../platform-apple/log.apple.mm"
#include "../platform-apple/memory.apple.mm"
#include "../platform-apple/tools.apple.mm"

#if (ET_ENABLE_APPLE_PURCHASES)
#	include "../platform-apple/iap.apple.mm"
#endif

#if (ET_PLATFORM_MAC)
#	include "../platform-apple/application.mac.mm"
#	include "../platform-apple/context.mac.mm"
#	include "../platform-apple/input.mac.mm"
#	include "../platform-apple/platformtools.mac.mm"
#	include "../platform-apple/rendercontext.mac.mm"
#endif
