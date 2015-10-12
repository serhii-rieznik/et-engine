/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/platform/platformtools.h>

#if (ET_PLATFORM_MAC)

#include <AppKit/NSAlert.h>
#include <AppKit/NSOpenPanel.h>
#include <et/platform-apple/apple.h>

using namespace et;

typedef void (^filePickerCallback)(__strong NSString* path);

@interface FilePicker : NSObject<NSOpenSavePanelDelegate>
{
	NSString* _defaultName;
	NSArray* _allowedExtensions;
	filePickerCallback _callback;
}
- (instancetype)initWithDefaultName:(NSString*)name fileTypes:(NSArray*)fileTypes
	callback:(filePickerCallback)cb;
- (void)openFile;
- (void)saveFile;
@end

void et::alert(const std::string& title, const std::string& message, const std::string& button, AlertType)
{
#if (__MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_10)
	NSAlert* alert = [[NSAlert alloc] init];
	
	if (type == AlertType::Error)
		alert.alertStyle = NSCriticalAlertStyle;
	else if (type == AlertType::Warning)
		alert.alertStyle = NSWarningAlertStyle;
	else
		alert.alertStyle = NSInformationalAlertStyle;
	
	alert.messageText = [NSString stringWithUTF8String:title.c_str()];
	alert.informativeText = [NSString stringWithUTF8String:message.c_str()];
	[alert addButtonWithTitle:[NSString stringWithUTF8String:button.c_str()]];
	[alert runModal];
#else
	NSString* nsTitle = [NSString stringWithUTF8String:title.c_str()];
	NSString* nsMessage = [NSString stringWithUTF8String:message.c_str()];
	NSString* nsButton = [NSString stringWithUTF8String:button.c_str()];
	[[NSAlert alertWithMessageText:nsTitle defaultButton:nsButton alternateButton:nil otherButton:nil
		 informativeTextWithFormat:@"%@", nsMessage, nil] runModal];
#endif
}

std::string et::selectFile(const StringList& allowedTypes, SelectFileMode mode, const std::string& defName)
{
	SEL selector = nil;
	
	if (mode == SelectFileMode::Open)
		selector = @selector(openFile);
	else if (mode == SelectFileMode::Save)
		selector = @selector(saveFile);
	else
		ET_FAIL("Invalid SelectFileMode value");
	
	__block std::string result;
	
	NSString* defaultName = [NSString stringWithUTF8String:defName.c_str()];
	
	NSMutableArray* fileTypes = [NSMutableArray array];
	for (const auto& ext : allowedTypes)
		[fileTypes addObject:[[NSString stringWithUTF8String:ext.c_str()] lowercaseString]];
	
	FilePicker* picker = ET_OBJC_AUTORELEASE([[FilePicker alloc] initWithDefaultName:defaultName
		fileTypes:fileTypes callback:^(__strong NSString* path)
	{
		result = std::string([path UTF8String]);
	}]);
	[picker performSelectorOnMainThread:selector withObject:nil waitUntilDone:YES];
	
	return result;
}

@implementation FilePicker

- (instancetype)initWithDefaultName:(NSString*)name fileTypes:(NSArray*)fileTypes callback:(filePickerCallback)cb
{
	self = [super init];
	if (self)
	{
		_allowedExtensions = fileTypes;
		_defaultName = name;
		_callback = cb;
	}
	return self;
}

- (BOOL)panel:(id)sender shouldEnableURL:(NSURL*)url
{
	(void)sender;
	
	NSError* error = nil;
	NSDictionary* resourceValues = [url resourceValuesForKeys:@[NSURLIsRegularFileKey, NSURLIsDirectoryKey] error:&error];
	
	if ([[resourceValues objectForKey:NSURLIsDirectoryKey] boolValue])
		return YES;

	if ([_allowedExtensions count] == 0)
		return YES;
	
	if ([[resourceValues objectForKey:NSURLIsRegularFileKey] boolValue])
	{
		NSString* fileExtension = [[url pathExtension] lowercaseString];
		return [_allowedExtensions containsObject:fileExtension];
	}

	return NO;
}

- (void)openFile
{
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];
	[openDlg setDelegate:self];
	[openDlg setCanChooseFiles:YES];
	[openDlg setCanChooseDirectories:NO];
	[openDlg setAllowsMultipleSelection:NO];
	[openDlg setResolvesAliases:YES];
	
	_callback([openDlg runModal] ? [[[openDlg URLs] objectAtIndex:0] path] : [NSString string]);
}

- (void)saveFile
{
	NSSavePanel* saveDlg = [NSSavePanel savePanel];
	[saveDlg setNameFieldStringValue:_defaultName];
	_callback([saveDlg runModal] ? [[saveDlg URL] path] : [NSString string]);
}

@end

#endif // ET_PLATFORM_MAC
