#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGL/gl.h>
#include "glw_internal.h"

@interface GLView : NSOpenGLView <NSWindowDelegate> {
  CVDisplayLinkRef displayLink;
@public
  int pendingReshape;
  glwWinContext context;
  glwSystemFontTexture systemFontTexture;
  glwImGui imGUI;
}

- (CVReturn)getFrameForTime:(const CVTimeStamp *)outputTime;
- (void)drawFrame;
@end

static CVReturn onDisplayLinkCallback(CVDisplayLinkRef displayLink, 
    const CVTimeStamp *now, const CVTimeStamp *outputTime, 
    CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
  CVReturn result = [(GLView *)displayLinkContext getFrameForTime:outputTime];
  return result;
}

glwSystemFontTexture *glwGetSystemFontTexture(glwWin *win) {
  GLView *view = ((NSWindow *)win).contentView;
  return &view->systemFontTexture;
}

glwWinContext *glwGetWinContext(glwWin *win) {
  GLView *view = ((NSWindow *)win).contentView;
  return &view->context;
}

@implementation GLView
- (id)initWithFrame:(NSRect)frameRect {
  NSOpenGLPixelFormatAttribute attribs[] = {NSOpenGLPFAMultisample,
		NSOpenGLPFASampleBuffers, 1,
		NSOpenGLPFASamples, 0,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 32,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
		0};
  NSOpenGLPixelFormat *windowedPixelFormat = [[NSOpenGLPixelFormat alloc] 
    initWithAttributes:attribs];
  self = [super initWithFrame:frameRect pixelFormat:windowedPixelFormat];
  [windowedPixelFormat release];
  
  [self setWantsBestResolutionOpenGLSurface:YES];
  
  return self;
}

- (void) prepareOpenGL {
	[super prepareOpenGL];

  GLint vblSynch = 1;
  [[self openGLContext] setValues:&vblSynch 
    forParameter:NSOpenGLCPSwapInterval];

  CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
  CVDisplayLinkSetOutputCallback(displayLink, onDisplayLinkCallback, self);
  CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, 
    [[self openGLContext] CGLContextObj], 
    [[self pixelFormat] CGLPixelFormatObj]);
  
  CVDisplayLinkStart(displayLink);
}

- (void)drawFrame {
  @autoreleasepool {
    NSOpenGLContext *currentContext = [self openGLContext];
    [currentContext makeCurrentContext];
    CGLLockContext([currentContext CGLContextObj]);
    if (self->pendingReshape) {
      if (self->context.onReshape) {
        self->pendingReshape = 0;
        if (!self->context.iconTexId) {
          glwInitSystemIconTexture((glwWin *)self.window);
        }
        if (!self->systemFontTexture.texId) {
          glwInitSystemFontTexture((glwWin *)self.window);
        }
        self->context.onReshape((glwWin *)self.window, self->context.viewWidth,
          self->context.viewHeight);
      }
    }
    if (self->context.onDisplay) {
      self->context.onDisplay((glwWin *)self.window);
    }
    CGLFlushDrawable([[self openGLContext] CGLContextObj]);
    CGLUnlockContext([currentContext CGLContextObj]);
  }
}

- (CVReturn)getFrameForTime:(const CVTimeStamp *)outputTime {
  [self drawFrame];
  return kCVReturnSuccess;
}

- (void)dealloc {
  CVDisplayLinkRelease(displayLink);
  [super dealloc];
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

- (void)reshape {
  NSRect bounds = [self bounds];
  NSRect backingBounds = [self convertRectToBacking:[self bounds]];
  self->context.viewWidth = (int)backingBounds.size.width;
  self->context.viewHeight = (int)backingBounds.size.height;
  self->context.scaleX = (float)self->context.viewWidth / bounds.size.width;
  self->context.scaleY = (float)self->context.viewHeight / bounds.size.height;
  self->pendingReshape = 1;
  [self drawFrame];
}

- (void)resumeDisplayRenderer  {
  NSOpenGLContext *currentContext = [self openGLContext];
  [currentContext makeCurrentContext];
  CGLLockContext([currentContext CGLContextObj]);
  CVDisplayLinkStart(displayLink);
  CGLUnlockContext([currentContext CGLContextObj]);
}

- (void)haltDisplayRenderer  {
  NSOpenGLContext *currentContext = [self openGLContext];
  [currentContext makeCurrentContext];
  CGLLockContext([currentContext CGLContextObj]);
  CVDisplayLinkStop(displayLink);
  CGLUnlockContext([currentContext CGLContextObj]);
}

- (void)saveMousePosFromEvent:(NSEvent*)event {
  NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
  self->context.x = loc.x * self->context.scaleX;
  self->context.y = loc.y * self->context.scaleY;
}

- (void)mouseMoved:(NSEvent*)event {
  [self saveMousePosFromEvent:event];
  if (GLW_DOWN == self->imGUI.mouseState) {
    if (self->context.onMotion) {
      self->context.onMotion((glwWin *)self.window,
        self->context.x, self->context.y);
    }
  } else {
    if (self->context.onPassiveMotion) {
      self->context.onPassiveMotion((glwWin *)self.window,
        self->context.x, self->context.y);
    }
  }
}

- (void)mouseDown:(NSEvent*)event {
  [self saveMousePosFromEvent:event];
  glwMouseEvent((glwWin *)self.window, GLW_LEFT_BUTTON, GLW_DOWN,
    self->context.x, self->context.y);
  if (self->context.onMouse) {
    self->context.onMouse((glwWin *)self.window,
      GLW_LEFT_BUTTON, GLW_DOWN,
      self->context.x, self->context.y);
  }
}

- (void)scrollWheel:(NSEvent *)event {
  [self saveMousePosFromEvent:event];
  if (self->context.onWheel) {
    self->context.onWheel((glwWin *)self.window, [event deltaY] * 10);
  }
}

- (void)mouseDragged:(NSEvent *)event {
  [self saveMousePosFromEvent:event];
  if (GLW_DOWN == self->imGUI.mouseState) {
    if (GLW_DOWN == self->imGUI.mouseState) {
      if (self->context.onMotion) {
        self->context.onMotion((glwWin *)self.window,
          self->context.x, self->context.y);
      }
    } else {
      if (self->context.onPassiveMotion) {
        self->context.onPassiveMotion((glwWin *)self.window,
          self->context.x, self->context.y);
      }
    }
  } else {
    glwMouseEvent((glwWin *)self.window, GLW_LEFT_BUTTON, GLW_DOWN,
      self->context.x, self->context.y);
    if (self->context.onMouse) {
      self->context.onMouse((glwWin *)self.window,
        GLW_LEFT_BUTTON, GLW_DOWN,
        self->context.x, self->context.y);
    }
  }
}

- (void)mouseUp:(NSEvent*)event {
  [self saveMousePosFromEvent:event];
  glwMouseEvent((glwWin *)self.window, GLW_LEFT_BUTTON, GLW_UP,
    self->context.x, self->context.y);
  if (self->context.onMouse) {
    self->context.onMouse((glwWin *)self.window, GLW_LEFT_BUTTON, GLW_UP,
      self->context.x, self->context.y);
  }
}

- (void)keyDown:(NSEvent *)event {
  NSString * const character = [event charactersIgnoringModifiers];
  if ([character length] > 0) {
    unichar c = [character characterAtIndex:0];
    NSCharacterSet *letters = [NSCharacterSet letterCharacterSet];
    if ([letters characterIsMember:c]) {
      if (self->context.onKeyboard) {
        self->context.onKeyboard((glwWin *)self.window, (unsigned char)c,
          self->context.x, self->context.y);
      }
    }
  }
}

- (BOOL)isFlipped {
  return YES;
}

-(void)windowWillClose:(NSNotification *)notification {
  [[NSApplication sharedApplication] terminate:nil];
}
@end

glwImGui *glwGetImGUI(glwWin *win) {
  GLView *view = ((NSWindow *)win).contentView;
  return &view->imGUI;
}

glwFont *glwCreateFont(char *name, int weight, int size, int bold) {
  NSString *fontFamily = [NSString stringWithCString:name
    encoding:NSMacOSRomanStringEncoding];
  NSFont *font = [[[NSFontManager sharedFontManager] fontWithFamily:fontFamily 
    traits:(bold ? NSBoldFontMask : NSUnboldFontMask) weight:weight size:size] retain];
  //NSLog(@"%@",[[[NSFontManager sharedFontManager] availableFontFamilies] description]);
  return (glwFont *)font;
}

glwSize glwMeasureText(char *text, glwFont *font) {
  NSString *aString = [NSString stringWithCString:text 
    encoding:NSMacOSRomanStringEncoding];
  NSSize frameSize = [aString sizeWithAttributes:
    [NSDictionary dictionaryWithObject:(NSFont *)font forKey:NSFontAttributeName]];
  glwSize size = {frameSize.width, frameSize.height};
  return size;
}

void glwInit(void) {
  [NSApplication sharedApplication];
  glwInitPrimitives();
}

void glwMainLoop(void) {
  @autoreleasepool {
    [NSApp run];
  }
}

glwWin *glwCreateWindow(int x, int y, int width, int height) {
  NSUInteger windowStyle = NSTitledWindowMask | NSClosableWindowMask | 
    NSResizableWindowMask | NSMiniaturizableWindowMask;
  NSRect fullRect = [[NSScreen mainScreen] visibleFrame];
  
  if (0 == width || 0 == height) {
    width = fullRect.size.width;
    height = fullRect.size.height;
  }
  
  NSRect viewRect = NSMakeRect(0, 0, width, height);
  NSRect windowRect = NSMakeRect(x, y, width, height);
  
  NSWindow *window = [[NSWindow alloc] initWithContentRect:windowRect 
    styleMask:windowStyle
    backing:NSBackingStoreBuffered 
    defer:NO];
  
  GLView *view = [[GLView alloc] initWithFrame:viewRect];
  memset(&view->context, 0, sizeof(view->context));
  memset(&view->systemFontTexture, 0, sizeof(view->systemFontTexture));
  memset(&view->imGUI, 0, sizeof(view->imGUI));
  view->pendingReshape = 0;
  view->context.viewWidth = width;
  view->context.viewHeight = height;
  view->systemFontTexture.texId = 0;
  view->context.scaleX = 1;
  view->context.scaleY = 1;
  
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
  CFURLGetFileSystemRepresentation(resourcesURL, TRUE,
    (UInt8 *)view->context.root,
    sizeof(view->context.root));
  CFRelease(resourcesURL);
  
  [window setAcceptsMouseMovedEvents:YES];
  [window setContentView:view];
  [window setDelegate:view];
  
  [window orderFrontRegardless];
  
  return (glwWin *)window;
}

unsigned char *glwRenderTextToRGBA(char *text, glwFont *font, glwSize size,
    int *pixelsWide, int *pixelsHigh) {
  NSString *aString = [NSString stringWithCString:text encoding:NSMacOSRomanStringEncoding];
  NSSize frameSize = NSMakeSize(size.width, size.height);
  NSImage *image = [[NSImage alloc] initWithSize:frameSize];
  unsigned char *rgba = 0;
  int rgbaBytes = 0;
  [image lockFocus];
  [aString drawAtPoint:NSMakePoint(0, 0) withAttributes:
    [NSDictionary dictionaryWithObject:(NSFont *)font forKey:NSFontAttributeName]];
  [image unlockFocus];
  NSBitmapImageRep *bitmap = [NSBitmapImageRep imageRepWithData:[image TIFFRepresentation]];
  *pixelsWide = [bitmap pixelsWide];
  *pixelsHigh = [bitmap pixelsHigh];
  rgbaBytes = 4 * (*pixelsWide) * (*pixelsHigh);
  rgba = malloc(rgbaBytes);
  memcpy(rgba, [bitmap bitmapData], rgbaBytes);
  [image release];
  return rgba;
}

void glwDestroyFont(glwFont *font) {
  NSFont *nsFont = (NSFont *)font;
  [nsFont release];
}


