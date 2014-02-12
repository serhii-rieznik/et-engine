/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <AppKit/NSOpenPanel.h>
#include <et/platform-apple/objc.h>
#include <et/platform/platformtools.h>

using namespace et;

@interface FilePicker : NSObject
{
	NSString* _defaultName;
	NSString* _result;
}

+ (FilePicker*)pickerWithDefaultName:(NSString*)name;
- (instancetype)initWithDefaultName:(NSString*)name;

- (void)openFile;
- (void)saveFile;

@property (nonatomic, readonly) NSString* result;

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
	
	FilePicker* picker = ET_OBJC_AUTORELEASE([[FilePicker alloc]
		initWithDefaultName:[NSString stringWithUTF8String:defaultName.c_str()]]);
	
	[picker performSelectorOnMainThread:selector withObject:nil waitUntilDone:YES];
	
	return [picker.result UTF8String];
}

@implementation FilePicker

@synthesize result = _result;

+ (FilePicker*)pickerWithDefaultName:(NSString*)name
{
	return ET_OBJC_AUTORELEASE([[FilePicker alloc] initWithDefaultName:name]);
}

- (instancetype)initWithDefaultName:(NSString*)name
{
	self = [super init];
	if (self)
	{
		_defaultName = name;
		_result = nil;
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
	
	_result = ET_OBJC_RETAIN(([openDlg runModal] == NSOKButton) ?
		[[[openDlg URLs] objectAtIndex:0] path] : [NSString string]);
}

- (void)saveFile
{
	NSSavePanel* saveDlg = [NSSavePanel savePanel];
	[saveDlg setNameFieldStringValue:_defaultName];
	_result = ET_OBJC_RETAIN(([saveDlg runModal] == NSOKButton) ? [[saveDlg URL] path] : [NSString string]);
}

@end