#pragma once

#ifdef __OBJC__

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

#ifdef __cplusplus
#define C_EXTERN extern "C"
#else
#define C_EXTERN extern
#endif

extern CGDataProviderRef g_fb_provider;
extern CGColorSpaceRef g_fb_colorspace;
extern CGImageRef g_fb_image;
extern CGRect g_fb_rect;

extern volatile bool g_eventloop_stop_request;
extern volatile bool g_eventloop_stop_answer;

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow* window;

@end

@interface MainViewController : UIViewController
{
    
}

@end

@interface MyView : UIView
{
    // The pixel dimensions of the backbuffer
    GLint backingWidth;
    GLint backingHeight;
    
    EAGLContext* context;
    
    // OpenGL names for the renderbuffer and framebuffers used to render to this view
    GLuint viewFramebuffer, viewRenderbuffer, depthRenderbuffer;
}

- (int)viewInit;
- (void)viewDeinit;
- (void)redrawAll;

@end

C_EXTERN void sundog_init( UIWindow* window );
C_EXTERN void sundog_deinit( void );
C_EXTERN void sundog_send_opendocument_request( const char* file_path );
C_EXTERN void sundog_pause( void );
C_EXTERN void sundog_resume( void );

#endif // __OBJC__

extern int g_iphone_sundog_screen_xsize;
extern int g_iphone_sundog_screen_ysize;
extern void* g_iphone_sundog_framebuffer;
extern uint g_view_renderbuffer;
extern uint g_view_framebuffer;

extern char* g_ui_lang;

void iphone_sundog_gl_lock( void );
void iphone_sundog_gl_unlock( void );
void iphone_sundog_screen_redraw( void );
void iphone_sundog_event_handler( void );
int iphone_sundog_copy( const char* filename );
char* iphone_sundog_paste( void );
void iphone_sundog_open_url( const char* url );
void iphone_sundog_send_mail( const char* email_text, const char* subj_text, const char* body_text );
void iphone_sundog_send_file_to_email( const char* subj_text, const char* file_path );
void iphone_sundog_send_file_to_gallery( const char* file_path );
