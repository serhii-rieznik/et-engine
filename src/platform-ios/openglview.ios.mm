/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#import <QuartzCore/QuartzCore.h>

#include <et/opengl/openglcaps.h>
#include <et/platform-ios/openglview.h>
#include <et/rendering/rendercontext.h>
#include <et/app/applicationnotifier.h>

extern NSString* etKeyboardRequiredNotification;
extern NSString* etKeyboardNotRequiredNotification;

class et::RenderContextNotifier
{
public:
	void resized(const et::vec2i& sz, et::RenderContext* rc)
		{ rc->resized(sz); }
};

using namespace et;

@interface etOpenGLView()
{
    EAGLContext* _context;
	NSMutableCharacterSet* allowedCharacters;
	
	Framebuffer::Pointer _mainFramebuffer;
	Framebuffer::Pointer _multisampledFramebuffer;
	
	RenderContext* _rc;
	RenderContextNotifier _rcNotifier;
	
	BOOL _keyboardAllowed;
	BOOL _multisampled;
	BOOL _isOpenGLES3;
}

- (void)performInitializationWithParameters:(const RenderContextParameters&)params;

- (void)createFramebuffer;
- (void)deleteFramebuffer;
- (void)onNotificationRecevied:(NSNotification*)notification;

@end

@implementation etOpenGLView

@synthesize context = _context;

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame parameters:(et::RenderContextParameters&)params
{
	self = [super initWithFrame:frame];
	
	if (self)
	{
		allowedCharacters = [NSMutableCharacterSet alphanumericCharacterSet];
		[allowedCharacters formUnionWithCharacterSet:[NSCharacterSet punctuationCharacterSet]];
		[allowedCharacters formUnionWithCharacterSet:[NSCharacterSet symbolCharacterSet]];
		[allowedCharacters formUnionWithCharacterSet:[NSCharacterSet symbolCharacterSet]];
		[allowedCharacters formUnionWithCharacterSet:[NSCharacterSet whitespaceCharacterSet]];
		
#if (!ET_OBJC_ARC_ENABLED)
		[allowedCharacters retain];
#endif
		
		[self performInitializationWithParameters:params];
	}
	
	return self;
}

- (void)dealloc
{
    [self deleteFramebuffer];
	
#if (!ET_OBJC_ARC_ENABLED)
    [_context release];
    [super dealloc];
#endif
}

- (void)performInitializationWithParameters:(const RenderContextParameters&)params
{
	_multisampled = params.multisamplingQuality == MultisamplingQuality_Best;
	
	CAEAGLLayer* eaglLayer = (CAEAGLLayer*)self.layer;
	
	eaglLayer.opaque = YES;
	eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
		kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
	
	_context = nil;
	_keyboardAllowed = NO;
	
	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(onNotificationRecevied:) name:etKeyboardRequiredNotification object:nil];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(onNotificationRecevied:) name:etKeyboardNotRequiredNotification object:nil];
	
	self.multipleTouchEnabled = params.multipleTouch;
}

- (void)setRenderContext:(RenderContext*)rc
{
	_rc = rc;
}

- (void)setContext:(EAGLContext*)newContext
{
    if (_context == newContext) return;
	
#if defined(GL_ES_VERSION_3_0)
	_isOpenGLES3 = (newContext.API == kEAGLRenderingAPIOpenGLES3);
#else
	_isOpenGLES3 = false;
#endif
	
#if (ET_OBJC_ARC_ENABLED)
	_context = newContext;
#else
	[_context release];
	_context = [newContext retain];
#endif
}

- (void)beginRender
{
	[EAGLContext setCurrentContext:_context];
	_rc->renderState().bindDefaultFramebuffer();
}

- (void)endRender
{
	checkOpenGLError("endRender");
	
	_rc->renderState().bindDefaultFramebuffer();
	
	if (_multisampled)
	{
		_rc->renderState().bindReadFramebuffer(_multisampledFramebuffer->glID());
		_rc->renderState().bindDrawFramebuffer(_mainFramebuffer->glID());
		
#if defined(GL_ES_VERSION_3_0)
		if (_isOpenGLES3)
		{
			glBlitFramebuffer(0, 0, _multisampledFramebuffer->size().x, _multisampledFramebuffer->size().y,
				0, 0, _mainFramebuffer->size().x, _mainFramebuffer->size().y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			checkOpenGLError("glBlitFramebuffer");
		}
		else
		{
			glResolveMultisampleFramebufferAPPLE();
			checkOpenGLError("glResolveMultisampleFramebuffer");
		}
#else
		glResolveMultisampleFramebufferAPPLE();
		checkOpenGLError("glResolveMultisampleFramebuffer");
#endif
	}
	
	const GLenum discards[]  = { GL_DEPTH_ATTACHMENT, GL_COLOR_ATTACHMENT0 };
	glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER, (_multisampled ? 2 : 1), discards);
	checkOpenGLError("glDiscardFramebufferEXT");
	
	_rc->renderState().bindFramebuffer(_mainFramebuffer);
	_rc->renderState().bindRenderbuffer(_mainFramebuffer->colorRenderbuffer());
	[_context presentRenderbuffer:GL_RENDERBUFFER];
	checkOpenGLError("[_context presentRenderbuffer:GL_RENDERBUFFER]");
	
	[EAGLContext setCurrentContext:nil];
}

- (void)createFramebuffer
{
	@synchronized(_context)
	{
		[EAGLContext setCurrentContext:_context];
		
		CAEAGLLayer* glLayer = (CAEAGLLayer*)self.layer;
		glLayer.opaque = YES;
		glLayer.contentsScale = [[UIScreen mainScreen] scale];
		self.contentScaleFactor = glLayer.contentsScale;
		
		int colorFormat = GL_RGBA8;
		int depthFormat = GL_DEPTH_COMPONENT16;
		
		vec2i size(static_cast<int>(glLayer.bounds.size.width * glLayer.contentsScale),
			static_cast<int>(glLayer.bounds.size.height * glLayer.contentsScale));
		
		GLuint colorRenderBuffer = 0;
		
		if (_mainFramebuffer.invalid())
		{
			_mainFramebuffer = _rc->framebufferFactory().createFramebuffer(size, "et-main-fbo", 0, 0,
				0, depthFormat, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, true, false);
			glGenRenderbuffers(1, &colorRenderBuffer);
		}
		else
		{
			_rc->renderState().bindFramebuffer(_mainFramebuffer);
			colorRenderBuffer = _mainFramebuffer->colorRenderbuffer();
		}
		
		_rc->renderState().bindRenderbuffer(colorRenderBuffer);
		if (![_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:glLayer])
			ET_FAIL("Unable to create render buffer.");
		_mainFramebuffer->setColorRenderbuffer(colorRenderBuffer);
		
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &size.x);
		checkOpenGLError("glGetRenderbufferParameteriv");
		
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &size.y);
		checkOpenGLError("glGetRenderbufferParameteriv");
		
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &colorFormat);
		checkOpenGLError("glGetRenderbufferParameteriv");

		_mainFramebuffer->resize(size);
		
		_rc->renderState().bindRenderbuffer(_mainFramebuffer->depthRenderbuffer());
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &depthFormat);
		
		if (_multisampled)
		{
			if (_multisampledFramebuffer.invalid())
			{
				_multisampledFramebuffer = _rc->framebufferFactory().createFramebuffer(size,
					"et-multisampled-framebuffer", colorFormat, GL_RGBA, GL_UNSIGNED_BYTE,
					depthFormat, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, true, openGLCapabilites().maxSamples());
			}
			else
			{
				_multisampledFramebuffer->resize(size);
			}
		}
		
		_rc->renderState().setDefaultFramebuffer([self defaultFramebuffer]);
		
		[self beginRender];
		_rcNotifier.resized(size, _rc);
		[self endRender];
	}
}

- (void)deleteFramebuffer
{
	_mainFramebuffer.reset(nullptr);
	_multisampledFramebuffer.reset(nullptr);
}

- (void)layoutSubviews
{
	[self createFramebuffer];
}

- (const Framebuffer::Pointer&)defaultFramebuffer
{
	return _multisampled ? _multisampledFramebuffer : _mainFramebuffer;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	(void)event;
	float scale = self.contentScaleFactor;
	CGSize ownSize = self.bounds.size;
	ownSize.width *= scale;
	ownSize.height *= scale;
	
	for (UITouch* touch in touches)
	{
		CGPoint touchPoint = [touch locationInView:self];
		touchPoint.x *= scale;
		touchPoint.y *= scale;
		
		PointerInputInfo pt;
		pt.id = [touch hash];
		pt.pos = vec2(touchPoint.x, touchPoint.y);
		pt.scroll = vec2(0.0f);
		pt.timestamp = static_cast<float>(touch.timestamp);
		pt.type = PointerType_General;
		
		float nx = 2.0f * pt.pos.x / ownSize.width - 1.0f;
		float ny = 1.0f - 2.0f * pt.pos.y / ownSize.height;
		pt.normalizedPos = vec2(nx, ny);
		Input::PointerInputSource().pointerPressed(pt);
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent*)event
{
	(void)event;
	float scale = self.contentScaleFactor;
	CGSize ownSize = self.bounds.size;
	ownSize.width *= scale;
	ownSize.height *= scale;
	
	for (UITouch* touch in touches)
	{
		CGPoint touchPoint = [touch locationInView:self];
		touchPoint.x *= scale;
		touchPoint.y *= scale;

		PointerInputInfo pt;
		pt.id = [touch hash];
		pt.pos = vec2(touchPoint.x, touchPoint.y);
		pt.scroll = vec2(0.0f);
		pt.timestamp = static_cast<float>(touch.timestamp);
		pt.type = PointerType_General;
		
		float nx = 2.0f * pt.pos.x / ownSize.width - 1.0f;
		float ny = 1.0f - 2.0f * pt.pos.y / ownSize.height;
		pt.normalizedPos = vec2(nx, ny);
		Input::PointerInputSource().pointerMoved(pt);
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	(void)event;
	float scale = self.contentScaleFactor;
	CGSize ownSize = self.bounds.size;
	ownSize.width *= scale;
	ownSize.height *= scale;
	
	for (UITouch* touch in touches)
	{
		CGPoint touchPoint = [touch locationInView:self];
		touchPoint.x *= scale;
		touchPoint.y *= scale;
		
		PointerInputInfo pt;
		pt.id = [touch hash];
		pt.pos = vec2(touchPoint.x, touchPoint.y);
		pt.scroll = vec2(0.0f);
		pt.timestamp = static_cast<float>(touch.timestamp);
		pt.type = PointerType_General;
		
		float nx = 2.0f * pt.pos.x / ownSize.width - 1.0f;
		float ny = 1.0f - 2.0f * pt.pos.y / ownSize.height;
		pt.normalizedPos = vec2(nx, ny);
		Input::PointerInputSource().pointerReleased(pt);
	}
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	(void)event;
	float scale = self.contentScaleFactor;
	CGSize ownSize = self.bounds.size;
	ownSize.width *= scale;
	ownSize.height *= scale;
	
	for (UITouch* touch in touches)
	{
		CGPoint touchPoint = [touch locationInView:self];
		touchPoint.x *= scale;
		touchPoint.y *= scale;
		
		PointerInputInfo pt;
		pt.id = [touch hash];
		pt.pos = vec2(touchPoint.x, touchPoint.y);
		pt.scroll = vec2(0.0f);
		pt.timestamp = static_cast<float>(touch.timestamp);
		pt.type = PointerType_General;
		
		float nx = 2.0f * pt.pos.x / ownSize.width - 1.0f;
		float ny = 1.0f - 2.0f * pt.pos.y / ownSize.height;
		pt.normalizedPos = vec2(nx, ny);
		Input::PointerInputSource().pointerCancelled(pt);
	}
}

- (void)onNotificationRecevied:(NSNotification*)notification
{
	if ([notification.name isEqualToString:etKeyboardRequiredNotification])
	{
		_keyboardAllowed = YES;
		[self performSelectorOnMainThread:@selector(becomeFirstResponder) withObject:nil waitUntilDone:NO];
	}
	else if ([notification.name isEqualToString:etKeyboardNotRequiredNotification])
	{
		[self performSelectorOnMainThread:@selector(resignFirstResponder) withObject:nil waitUntilDone:NO];
		_keyboardAllowed = NO;
	}
}

- (BOOL)canBecomeFirstResponder
{
	return _keyboardAllowed;
}

- (BOOL)hasText
{
	return YES;
}

- (void)insertText:(NSString*)text
{
	if ([text length] == 1)
	{
		unichar character = [text characterAtIndex:0];
		if (character == ET_NEWLINE)
			Input::KeyboardInputSource().keyPressed(ET_KEY_RETURN);
	}

	NSString* filteredString = [text stringByTrimmingCharactersInSet:[allowedCharacters invertedSet]];
	if ([filteredString length] > 0)
	{
		std::string cString([filteredString cStringUsingEncoding:NSUTF8StringEncoding]);
		Input::KeyboardInputSource().charactersEntered(cString);
	}
}

- (void)deleteBackward
{
	Input::KeyboardInputSource().keyPressed(ET_KEY_BACKSPACE);
}

@end
