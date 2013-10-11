/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <MessageUI/MessageUI.h>
#include <et/app/application.h>
#include <et/platform-ios/mailcomposer.h>

using namespace et;

class et::MailComposerPrivate
{
public:
	MFMailComposeViewController* viewController;
};

/*
 * MailComposer Obj-C Proxy
 */

@interface MailComposerProxy : NSObject<MFMailComposeViewControllerDelegate>
{
	MailComposerPrivate* _private;
}

+ (MailComposerProxy*)sharedInstanceWithPrivatePtr:(MailComposerPrivate*)p;

- (id)initWithPrivatePtr:(MailComposerPrivate*)p;
- (void)setPrivatePtr:(MailComposerPrivate*)p;

@end

@implementation MailComposerProxy

MailComposerProxy* _sharedInstance = nil;

+ (MailComposerProxy*)sharedInstanceWithPrivatePtr:(MailComposerPrivate*)p
{
	if (_sharedInstance == nil)
		_sharedInstance = [[MailComposerProxy alloc] initWithPrivatePtr:p];
	
	[_sharedInstance setPrivatePtr:p];
	return _sharedInstance;
}

- (id)initWithPrivatePtr:(MailComposerPrivate*)p
{
	self = [super init];
	if (self)
	{
		_private = p;
	}
	return self;
}

- (void)setPrivatePtr:(MailComposerPrivate*)p
{
	_private = p;
}

- (void)mailComposeController:(MFMailComposeViewController *)controller
	didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error
{
	UIViewController* mainViewController =
		reinterpret_cast<UIViewController*>(application().renderingContextHandle());
	
	[mainViewController dismissModalViewControllerAnimated:YES];
}
						   
@end

/*
 *
 * MailComposer
 *
 */

MailComposer::MailComposer() : _private(new MailComposerPrivate())
{
	
}

MailComposer::~MailComposer()
{
	[MailComposerProxy sharedInstanceWithPrivatePtr:0];
	delete _private;
}

void MailComposer::composeEmail(const std::string& recepient, const std::string& title,
	const std::string& text)
{
	_private->viewController = [[MFMailComposeViewController alloc] init];
    if (_private->viewController == nil) return;
    	
	[_private->viewController setSubject:
		[NSString stringWithUTF8String:title.c_str()]];

	[_private->viewController setToRecipients:
		[NSArray arrayWithObject:[NSString stringWithUTF8String:recepient.c_str()]]];
	
	[_private->viewController setMessageBody:
		[NSString stringWithUTF8String:text.c_str()] isHTML:NO];
	
	[_private->viewController setMailComposeDelegate:
		[MailComposerProxy sharedInstanceWithPrivatePtr:_private]];
}

void MailComposer::attachFile(const std::string& attachmentFileName)
{
	assert(_private->viewController);

	NSData* data = [NSData dataWithContentsOfFile:
		[NSString stringWithUTF8String:attachmentFileName.c_str()]];

	if (data != nil)
	{
		NSString* fileName = [NSString stringWithUTF8String:getFileName(attachmentFileName).c_str()];
		[_private->viewController addAttachmentData:data
			mimeType:@"application/octet-stream" fileName:fileName];
	}
}

void MailComposer::present()
{
	UIViewController* mainViewController =
		reinterpret_cast<UIViewController*>(application().renderingContextHandle());
	
	[mainViewController presentModalViewController:
		[_private->viewController autorelease] animated:YES];
	
	_private->viewController = nil;
}

bool MailComposer::canSendEmail() const
{
	return [MFMailComposeViewController canSendMail];
}