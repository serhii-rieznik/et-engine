/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <sys/time.h>
#include <sys/stat.h>
#include <et/core/datastorage.h>
#include <et/core/tools.h>
#include <et/core/hardware.h>
#include <et/platform-apple/apple.h>

#if (ET_PLATFORM_MAC)
#	include <Foundation/NSFileManager.h>
#	include <Foundation/NSPredicate.h>
#	include <Foundation/NSString.h>
#	include <Foundation/NSURL.h>
#	include <Foundation/NSBundle.h>
#	include <AppKit/NSWorkspace.h>
#	include <AppKit/NSScreen.h>
#elif (ET_PLATFORM_IOS)
#	include <UIKit/UIApplication.h>
#	include <UIKit/UIScreen.h>
#endif

static uint64_t startTime = 0;
static bool startTimeInitialized = false;

const char et::pathDelimiter = '/';
const char et::invalidPathDelimiter = '\\';

const std::string kDefaultBundleId = "com.cheetek.et-engine.application";

uint64_t queryActualTime();

float et::queryContiniousTimeInSeconds()
{
	return static_cast<float>(queryContiniousTimeInMilliSeconds()) / 1000.0f;
}

uint64_t et::queryContiniousTimeInMilliSeconds()
{
	if (!startTimeInitialized)
	{
		startTime = queryActualTime();
		startTimeInitialized = true;
	};
	
	return queryActualTime() - startTime;
}

uint64_t et::queryCurrentTimeInMicroSeconds()
{
	timeval tv = { };
	gettimeofday(&tv, 0);
	return static_cast<uint64_t>(tv.tv_sec) * 1000000 + static_cast<uint64_t>(tv.tv_usec);
}

uint64_t queryActualTime()
{
	timeval tv = { };
	gettimeofday(&tv, 0);
	return static_cast<uint64_t>(tv.tv_sec) * 1000 + static_cast<uint64_t>(tv.tv_usec) / 1000;
}

uint64_t et::getFileDate(const std::string& path)
{
	struct stat s = { };
	stat(path.c_str(), &s);
	return s.st_mtimespec.tv_sec;
}

std::string et::applicationPath()
{
	static std::string result;
	if (result.empty())
	{
		char buffer[512] = { };
		
		CFURLRef bUrl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
		CFURLGetFileSystemRepresentation(bUrl, true, reinterpret_cast<UInt8*>(buffer), 256);
		CFRelease(bUrl);
		
		size_t i = 0;
		while ((i < sizeof(buffer)) && buffer[++i]);
		buffer[i] = '/';
		
		result = std::string(buffer);
	}
    return result;
}

std::string et::applicationPackagePath()
{
    return et::applicationPath();
}

std::string et::applicationDataFolder()
{
	static std::string result;
	if (result.empty())
	{
		char buffer[512] = { };
		
		CFURLRef bUrl = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
		CFURLGetFileSystemRepresentation(bUrl, true, reinterpret_cast<UInt8*>(buffer), 256);
		CFRelease(bUrl);
		
		size_t i = 0;
		while ((i < sizeof(buffer)) && buffer[++i]);
		buffer[i] = '/';
		
		result = std::string(buffer);
	}
	
	return result;
}

bool et::fileExists(const std::string& name)
{
	BOOL isDir = NO;
    NSString* fileName = ET_OBJC_AUTORELEASE([[NSString alloc] initWithUTF8String:name.c_str()]);
    BOOL exists = [[NSFileManager defaultManager] fileExistsAtPath:fileName isDirectory:&isDir];
    return exists && !isDir;
}

bool et::folderExists(const std::string& name)
{
	BOOL isDir = NO;
    NSString* fileName = ET_OBJC_AUTORELEASE([[NSString alloc] initWithUTF8String:name.c_str()]);
	BOOL exists = [[NSFileManager defaultManager] fileExistsAtPath:fileName isDirectory:&isDir];
	return exists && isDir;
}

std::string et::libraryBaseFolder()
{
    @autoreleasepool
    {
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
        NSString* folder = [[paths objectAtIndex:0] stringByAppendingString:@"/"];
        return std::string([folder cStringUsingEncoding:NSUTF8StringEncoding]);
    }
}

std::string et::temporaryBaseFolder()
{
    @autoreleasepool
    {
        return std::string([NSTemporaryDirectory() cStringUsingEncoding:NSUTF8StringEncoding]);
    }
}

std::string et::documentsBaseFolder()
{
    @autoreleasepool 
    {
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString* folder = [[paths objectAtIndex:0] stringByAppendingString:@"/"];
        return std::string([folder cStringUsingEncoding:NSUTF8StringEncoding]);
    }
}

std::string et::workingFolder()
{
    const size_t bufferSize = 1024;
    char buffer[bufferSize] = { };
    getcwd(buffer, bufferSize);
    return addTrailingSlash(normalizeFilePath(std::string(buffer)));
}

bool et::createDirectory(const std::string& name, bool intermediates)
{
	NSError* error = nil;
	NSString* path = [NSString stringWithUTF8String:name.c_str()];
	
	BOOL result = [[NSFileManager defaultManager] createDirectoryAtPath:path
		withIntermediateDirectories:(intermediates ? YES : NO) attributes:nil error:&error];
	
	if (error)
		NSLog(@"Unable to create directory at %@, error: %@", path, error);
	
	return result;
}

bool et::removeDirectory(const std::string& name)
{
	NSError* error = nil;
	
	NSString* path = [NSString stringWithUTF8String:name.c_str()];
	BOOL result = [[NSFileManager defaultManager] removeItemAtPath:path error:&error];
	
	if (error)
		NSLog(@"Unable to remove directory at %@, error: %@", path, error);
	
	return result;
}

bool et::removeFile(const std::string& name)
{
	NSError* error = nil;
	
	NSString* path = [NSString stringWithUTF8String:name.c_str()];
	BOOL result = [[NSFileManager defaultManager] removeItemAtPath:path error:&error];
	
	if (error)
		NSLog(@"Unable to remove file at %@, error: %@", path, error);
	
	return result;
}

bool et::copyFile(const std::string& fromName, const std::string& toName)
{
	NSError* error = nil;
	
	NSString* fileFrom = [NSString stringWithUTF8String:fromName.c_str()];
	NSString* fileTo = [NSString stringWithUTF8String:toName.c_str()];
	
	if ([[NSFileManager defaultManager] fileExistsAtPath:fileTo])
		removeFile(toName);

	BOOL result = [[NSFileManager defaultManager] copyItemAtPath:fileFrom toPath:fileTo error:&error];
	
	if (error)
		NSLog(@"Unable to copy file from %s to %s, error: %@", fromName.c_str(), toName.c_str(), error);
	
	return result;
}

void et::getFolderContent(const std::string& folder, StringList& list)
{
	NSError* err = nil;
	NSString* path = [NSString stringWithCString:folder.c_str() encoding:NSUTF8StringEncoding];
	NSArray* files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:path error:&err];
	
	for (NSString* file in files)
		list.push_back(folder + std::string([file cStringUsingEncoding:NSUTF8StringEncoding]));
}

void et::findFiles(const std::string& folder, const std::string& mask, bool /* recursive */, StringList& list)
{
	size_t maskLength = mask.length();
	size_t nameSearchCriteria = mask.find_last_of(".*");
	BOOL searchByName = nameSearchCriteria == maskLength - 1;
	BOOL searchByExt = mask.find_first_of("*.") == 0;
	
	NSError* err = nil;
	NSString* path = [NSString stringWithCString:folder.c_str() encoding:NSASCIIStringEncoding];
	NSArray* files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:path error:&err];
	
	NSArray* filtered = files;
	
	if (searchByExt)
	{
		std::string extMask = mask.substr(2, mask.length() - 2);
		NSString* objcMask = [NSString stringWithCString:extMask.c_str() encoding:NSASCIIStringEncoding];
		filtered = [filtered filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"self ENDSWITH %@", objcMask]];
	}
	
	if (searchByName)
	{
		std::string nameMask = mask.substr(0, mask.length() - 3);
		NSString* objcMask = [NSString stringWithCString:nameMask.c_str() encoding:NSASCIIStringEncoding];
		objcMask = [objcMask stringByAppendingString:@"*"];
		filtered = [filtered filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"self LIKE %@", objcMask]];
	}
	
	for (NSString* file in filtered)
		list.push_back(folder + std::string([file cStringUsingEncoding:NSUTF8StringEncoding]));
}

void et::findSubfolders(const std::string& folder, bool recursive, StringList& list)
{
	NSString* path = [NSString stringWithCString:folder.c_str() encoding:NSASCIIStringEncoding];
	if (![path hasSuffix:@"/"])
		path = [path stringByAppendingString:@"/"];
	
	NSError* err = nil;
	NSArray* files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:path error:&err];
	
	if (err != nil)
	{
		NSLog(@"Unable to findSubfolders: %@", err);
		return;
	}
	
	for (NSString* entry in files)
	{
		BOOL isDir = false;
		NSString* expandedPath = [path stringByAppendingFormat:@"%@/", entry];
 		if ([[NSFileManager defaultManager] fileExistsAtPath:expandedPath isDirectory:&isDir] && isDir)
		{
			std::string entryStr([expandedPath cStringUsingEncoding:NSASCIIStringEncoding]);
			list.push_back(entryStr);
			if (recursive)
				findSubfolders(entryStr, true, list);
		}
	}
}

void et::openUrl(const std::string& url)
{
	if (url.empty()) return;
	
#if (ET_PLATFORM_IOS)
#
#	define URLProcessor [UIApplication sharedApplication]
#
#else
#
#	define URLProcessor [NSWorkspace sharedWorkspace]
#
#endif
	
	NSString* urlString = [NSString stringWithUTF8String:url.c_str()];
	NSURL* aUrl = fileExists(url) ? [NSURL fileURLWithPath:urlString] : [NSURL URLWithString:urlString];
	[URLProcessor openURL:aUrl];
	
#undef URLProcessor
}

std::string et::unicodeToUtf8(const std::wstring& w)
{
	NSString* s = ET_OBJC_AUTORELEASE([[NSString alloc] initWithBytes:w.c_str() length:w.length() * sizeof(wchar_t)
		encoding:NSUTF32LittleEndianStringEncoding]);
	
	if (s == nil)
	{
		NSLog(@"Unable to convert wstring to NSString.");
		return emptyString;
	}
	
	if (![s canBeConvertedToEncoding:NSUTF8StringEncoding])
	{
		NSLog(@"Unable to convert %@ to NSUTF8StringEncoding", s);
		return emptyString;
	}
	
	NSUInteger actualLength = 0;
	
	[s getBytes:0 maxLength:0 usedLength:&actualLength
	   encoding:NSUTF8StringEncoding options:0 range:NSMakeRange(0, [s length]) remainingRange:0];
	
	BinaryDataStorage result(actualLength + sizeof(char), 0);
	
	[s getBytes:result.data() maxLength:result.dataSize() usedLength:0
	   encoding:NSUTF8StringEncoding options:0 range:NSMakeRange(0, [s length]) remainingRange:0];
	
	return std::string(reinterpret_cast<const char*>(result.data()));
}

std::wstring et::utf8ToUnicode(const std::string& mbcs)
{
	NSString* s = [NSString stringWithUTF8String:mbcs.c_str()];
	
	if (s == nil)
	{
		NSLog(@"Unable to convert UTF-8 `%s` to NSString.", mbcs.c_str());
		return std::wstring();
	}
	
	if (![s canBeConvertedToEncoding:NSUTF32LittleEndianStringEncoding])
	{
		NSLog(@"Unable to convert %@ to NSUTF32LittleEndianStringEncoding", s);
		return std::wstring();
	}
	
	NSUInteger actualLength = 0;
    
	[s getBytes:0 maxLength:0 usedLength:&actualLength
	   encoding:NSUTF32LittleEndianStringEncoding options:0 range:NSMakeRange(0, [s length]) remainingRange:0];
	
	BinaryDataStorage result(actualLength + sizeof(wchar_t), 0);
	
	[s getBytes:result.data() maxLength:result.dataSize() usedLength:0
	   encoding:NSUTF32LittleEndianStringEncoding options:0 range:NSMakeRange(0, [s length]) remainingRange:0];
	
	return std::wstring(reinterpret_cast<const wchar_t*>(result.data()));
}

std::string et::applicationIdentifierForCurrentProject()
{
	CFStringRef bundleId = CFBundleGetIdentifier(CFBundleGetMainBundle());
	
	return (bundleId == nil) ? kDefaultBundleId :
		std::string(CFStringGetCStringPtr(bundleId, kCFStringEncodingMacRoman));
}

std::string et::bundleVersion()
{
	NSString* versionString = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
	return versionString == nil ? emptyString : std::string([versionString UTF8String]);
}


et::vec2i et::nativeScreenSize()
{
#if (ET_PLATFORM_IOS)

	CGSize size = [[UIScreen mainScreen] bounds].size;
	
	if (UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation]))
		return vec2i(static_cast<int>(size.height), static_cast<int>(size.width));
	else
		return vec2i(static_cast<int>(size.width), static_cast<int>(size.height));
	
#else
	
	NSSize size = [[NSScreen mainScreen] frame].size;
	return vec2i(static_cast<int>(size.width), static_cast<int>(size.height));
	
#endif
}

et::vec2i et::availableScreenSize()
{
#if (ET_PLATFORM_IOS)
	
	return nativeScreenSize();
	
#else 
	
	auto size = [[NSScreen mainScreen] visibleFrame].size;
	return vec2i(static_cast<int>(size.width), static_cast<int>(size.height));
	
#endif
	
}

#if (ET_PLATFORM_IOS)
	et::Screen uiScreenToScreen(UIScreen* screen);
#else
	et::Screen nsScreenToScreen(NSScreen* screen);
#endif

et::Screen et::currentScreen()
{
#if (ET_PLATFORM_IOS)
	return uiScreenToScreen([UIScreen mainScreen]);
#else
	return nsScreenToScreen([NSScreen mainScreen]);
#endif
}

et::Vector<et::Screen> et::availableScreens()
{
	Vector<et::Screen> result;
	
#if (ET_PLATFORM_IOS)
	for (UIScreen* screen in [UIScreen screens])
		result.push_back(uiScreenToScreen(screen));
#else
	for (NSScreen* screen in [NSScreen screens])
		result.push_back(nsScreenToScreen(screen));
#endif
	
	return result;
}

#if (ET_PLATFORM_IOS)
et::Screen uiScreenToScreen(UIScreen* screen)
{
	CGRect frame = [screen bounds];
	CGRect available = frame;
    float scaleFactor = [screen scale];
#else
et::Screen nsScreenToScreen(NSScreen* screen)
{
	NSRect frame = [screen frame];
	NSRect available = [screen visibleFrame];
    float scaleFactor = [screen backingScaleFactor];
#endif
		
	auto aFrame = et::recti(static_cast<int>(frame.origin.x), static_cast<int>(frame.origin.y),
		static_cast<int>(frame.size.width), static_cast<int>(frame.size.height));
	
	auto aAvailable = et::recti(static_cast<int>(available.origin.x), static_cast<int>(available.origin.y),
		static_cast<int>(available.size.width), static_cast<int>(available.size.height));
	
	return et::Screen(aFrame, aAvailable, scaleFactor);
}
