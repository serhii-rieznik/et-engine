/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <AppKit/NSOpenPanel.h>
#include <et/platform-apple/apple.h>
#include <et/platform/platformtools.h>

using namespace et;

typedef void (^filePickerCallback)(__strong NSString* path);

@interface FilePicker : NSObject
{
	NSString* _defaultName;
	filePickerCallback _callback;
}

- (instancetype)initWithDefaultName:(NSString*)name callback:(filePickerCallback)cb;

- (void)openFile;
- (void)saveFile;

@end

std::string et::selectFile(const StringList&, SelectFileMode mode, const std::string& defaultName)
{
	SEL selector = nil;
	
	if (mode == SelectFileMode_Open)
		selector = @selector(openFile);
	else if (mode == SelectFileMode_Save)
		selector = @selector(saveFile);
	else
		ET_FAIL("Invalid SelectFieMode value");
	
	__block std::string result;
	
	FilePicker* picker = ET_OBJC_AUTORELEASE([[FilePicker alloc]
		initWithDefaultName:[NSString stringWithUTF8String:defaultName.c_str()] callback:^(__strong NSString* path)
	{
		result = std::string([path UTF8String]);
	}]);
	[picker performSelectorOnMainThread:selector withObject:nil waitUntilDone:YES];
	
	return result;
}

@implementation FilePicker

- (instancetype)initWithDefaultName:(NSString*)name callback:(filePickerCallback)cb
{
	self = [super init];
	if (self)
	{
		_defaultName = name;
		_callback = cb;
	}
	return self;
}

- (void)openFile
{
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];
	
	[openDlg setCanChooseFiles:YES];
	[openDlg setCanChooseDirectories:NO];
	[openDlg setAllowsMultipleSelection:NO];
	[openDlg setResolvesAliases:YES];
	[openDlg setAllowedFileTypes:nil];
	
	_callback([openDlg runModal] == NSOKButton ? [[[openDlg URLs] objectAtIndex:0] path] : [NSString string]);
}

- (void)saveFile
{
	NSSavePanel* saveDlg = [NSSavePanel savePanel];
	[saveDlg setNameFieldStringValue:_defaultName];
	_callback([saveDlg runModal] == NSOKButton ? [[saveDlg URL] path] : [NSString string]);
}

@end
