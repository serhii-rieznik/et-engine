/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>

#if (ET_PLATFORM_IOS)

#include <MessageUI/MessageUI.h>
#include <et/platform-apple/objc.h>
#include <et/platform-ios/mailcomposer.h>

namespace et
{

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

- (void)present;

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
	UIViewController* mainViewController = (__bridge UIViewController*)
		reinterpret_cast<void*>(application().renderingContextHandle());
	[mainViewController dismissViewControllerAnimated:YES completion:nil];
}

- (void)present
{
	UIViewController* mainViewController = (__bridge UIViewController*)
		reinterpret_cast<void*>(application().renderingContextHandle());
	
	[mainViewController presentViewController:_private->viewController animated:YES completion:^{
		(void)(ET_OBJC_AUTORELEASE(_private->viewController));
		_private->viewController = nil;
	}];
}
						   
@end

/*
 *
 * MailComposer
 *
 */

MailComposer::MailComposer()
{
	ET_PIMPL_INIT(MailComposer)
}

MailComposer::~MailComposer()
{
	[MailComposerProxy sharedInstanceWithPrivatePtr:nullptr];
	ET_PIMPL_FINALIZE(MailComposer)
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
	ET_ASSERT(_private->viewController);

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
	ET_ASSERT(_private->viewController);
	
	MailComposerProxy* proxy = [MailComposerProxy sharedInstanceWithPrivatePtr:_private];
	[proxy performSelectorOnMainThread:@selector(present) withObject:nil waitUntilDone:NO];
}

bool MailComposer::canSendEmail()
{
	return [MFMailComposeViewController canSendMail];
}

}
#endif // ET_PLATFORM_IOS
