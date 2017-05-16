#pragma once

#ifdef __cplusplus
#define C_EXTERN extern "C"
#else
#define C_EXTERN extern
#endif

#ifdef __OBJC__

@interface MyView : NSOpenGLView
{
    NSOpenGLContext* context;
}

- (void)viewBoundsChanged: (NSNotification*)aNotification;
- (void)viewFrameChanged: (NSNotification*)aNotification;
- (int)viewInit;

@end

C_EXTERN void sundog_init( MyView* view );
C_EXTERN void sundog_deinit( void );

#endif

void osx_sundog_screen_redraw( void );
void osx_sundog_rename_window( const char* name );
void osx_sundog_event_handler( void );

extern int g_osx_sundog_screen_xsize;
extern int g_osx_sundog_screen_ysize;
extern int g_osx_sundog_resize_request;
extern void* g_gl_context_obj;

extern char* g_ui_lang;
