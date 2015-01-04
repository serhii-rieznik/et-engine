
/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/opengl/opengl.h>
#include <et/rendering/rendercontext.h>
#include <et/app/applicationnotifier.h>

#if (ET_PLATFORM_IOS)

#include <QuartzCore/QuartzCore.h>
#include <et/platform-ios/openglview.h>

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
	
	float _scaleFactor;
	
	BOOL _keyboardAllowed;
	BOOL _multisampled;
}

- (void)performInitializationWithParameters:(const RenderContextParameters&)params;

- (void)createFramebuffer;
- (void)deleteFramebuffer;
- (void)onNotificationRecevied:(NSNotification*)notification;
- (PointerInputInfo)touchToPointerInputInfo:(UITouch*)touch;

@end

@implementation etOpenGLView

@synthesize context = _context;

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (BOOL)isOpaque
{
	return YES;
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
	
	if (_rc->parameters().bindDefaultFramebufferEachFrame)
		_rc->renderState().bindDefaultFramebuffer();
}

- (void)endRender
{
	checkOpenGLError("endRender");
	
	if (_multisampled)
	{
		_multisampledFramebuffer->invalidate(false, true);
		_multisampledFramebuffer->resolveMultisampledTo(_mainFramebuffer);
		_multisampledFramebuffer->invalidate(true, false);
	}
		
	_rc->renderState().bindFramebuffer(_mainFramebuffer);
	_rc->renderState().bindRenderbuffer(_mainFramebuffer->colorRenderbuffer());
	_mainFramebuffer->invalidate(false, true);
	
	[_context presentRenderbuffer:GL_RENDERBUFFER];
	checkOpenGLError("[_context presentRenderbuffer:GL_RENDERBUFFER]");
}

- (void)createFramebuffer
{
	@synchronized(_context)
	{
		[EAGLContext setCurrentContext:_context];
		
		_scaleFactor = static_cast<float>([[UIScreen mainScreen] scale]);
		
		CAEAGLLayer* glLayer = (CAEAGLLayer*)self.layer;
		glLayer.opaque = YES;
		glLayer.contentsScale = _scaleFactor;
		
		self.contentScaleFactor = _scaleFactor;
		
		vec2i size(static_cast<int>(glLayer.bounds.size.width * glLayer.contentsScale),
			static_cast<int>(glLayer.bounds.size.height * glLayer.contentsScale));
		
		GLuint colorRenderBuffer = 0;
		
		if (_mainFramebuffer.invalid())
		{
			_mainFramebuffer = _rc->framebufferFactory().createFramebuffer(size, "__et_main_framebuffer__",
				TextureFormat::Invalid, TextureFormat::Invalid, DataType::UnsignedChar, TextureFormat::Depth16,
				TextureFormat::Depth, DataType::UnsignedInt, true, false);
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
		
		_mainFramebuffer->resize(size);
		
		if (_multisampled)
		{
			if (_multisampledFramebuffer.invalid())
			{
				_multisampledFramebuffer = _rc->framebufferFactory().createFramebuffer(size,
					"__et_multisampled_framebuffer__", TextureFormat::RGBA8, TextureFormat::RGBA, DataType::UnsignedChar,
					TextureFormat::Depth16, TextureFormat::Depth, DataType::UnsignedInt, true,
					RenderingCapabilities::instance().maxSamples());
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

- (PointerInputInfo)touchToPointerInputInfo:(UITouch*)touch
{
	CGRect ownFrame = self.bounds;
	CGPoint touchPoint = [touch locationInView:self];
	
	float nx = 2.0f * static_cast<float>(touchPoint.x / ownFrame.size.width) - 1.0f;
	float ny = 1.0f - 2.0f * static_cast<float>(touchPoint.y / ownFrame.size.height);
	
	touchPoint.x *= _scaleFactor;
	touchPoint.y *= _scaleFactor;

	return PointerInputInfo(PointerType_General, vec2(static_cast<float>(touchPoint.x), static_cast<float>(touchPoint.y)),
		vec2(nx, ny), vec2(0.0f), [touch hash], mainTimerPool()->actualTime(), PointerOrigin_Touchscreen);
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	(void)event;
	for (UITouch* touch in touches)
		Input::PointerInputSource().pointerPressed([self touchToPointerInputInfo:touch]);
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
	(void)event;
	for (UITouch* touch in touches)
		Input::PointerInputSource().pointerMoved([self touchToPointerInputInfo:touch]);
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	(void)event;
	for (UITouch* touch in touches)
		Input::PointerInputSource().pointerReleased([self touchToPointerInputInfo:touch]);
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
	(void)event;
	for (UITouch* touch in touches)
		Input::PointerInputSource().pointerCancelled([self touchToPointerInputInfo:touch]);
}

- (void)onNotificationRecevied:(NSNotification*)notification
{
	if (!_keyboardAllowed && [notification.name isEqualToString:etKeyboardRequiredNotification])
	{
		_keyboardAllowed = YES;
		[self performSelectorOnMainThread:@selector(becomeFirstResponder) withObject:nil waitUntilDone:NO];
	}
	else if (_keyboardAllowed && [notification.name isEqualToString:etKeyboardNotRequiredNotification])
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

#endif // ET_PLATFORM_IOS
