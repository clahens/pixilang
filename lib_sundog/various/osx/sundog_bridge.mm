#import <stdio.h>
#import <time.h>
#import <string.h>
#import <pthread.h>

#import <OpenGL/gl.h>
#import <OpenGL/OpenGL.h>

#import "core/core.h"
#import "sundog_bridge.h"

//Defined in wm_osx.h:
extern void osx_sundog_touches( int x, int y, int mode, int touch_id, window_manager* );
extern void osx_sundog_key( unsigned short vk, bool pushed, window_manager* );

int g_osx_sundog_screen_xsize = 320;
int g_osx_sundog_screen_ysize = 480;
int g_osx_sundog_resize_request = 0;

extern char* g_osx_docs_path;
extern char* g_osx_caches_path;
extern char* g_osx_tmp_path;

char* g_ui_lang = 0;

MyView* g_sundog_view;
NSWindow* g_sundog_window;
NSOpenGLContext* g_gl_context = 0;
void* g_gl_context_obj = 0;

typedef struct sundog_info_struct
{
    int initialized;
    pthread_t pth;
    int pth_finished;
    sundog_engine s;
} sundog_info_struct;
sundog_info_struct g_sd;

void* bthread( void* arg )
{
    sundog_info_struct* sd = (sundog_info_struct*)arg;

    [ g_gl_context makeCurrentContext ];
    
    sundog_main( &sd->s, true );

    sd->pth_finished = 1;
    [ NSApp terminate: NSApp ];
    pthread_exit( NULL );
}

void sundog_init( MyView* view )
{
    int err;
    
    printf( "sundog_init() ...\n" );

    //Init the view:
    int view_ok = [ view viewInit ];
    if( view_ok ) return;
    
    memset( &g_sd, 0, sizeof( sundog_info_struct ) );
 
    //Save link to current view:
    g_sundog_view = view;
    g_sundog_window = [ view window ];
    
    //Get system paths:
    NSArray* paths;
    NSBundle* mb = [ NSBundle mainBundle ];
    const char* url_cstr = [ [ mb bundlePath ] UTF8String ];
    int len = (int)strlen( url_cstr );
    g_osx_docs_path = (char*)malloc( len + 2 );
    memcpy( g_osx_docs_path, url_cstr, len + 1 );
    int i;
    for( i = len; i > 0; i-- )
    {
	if( g_osx_docs_path[ i ] == '/' )
	{
	    g_osx_docs_path[ i + 1 ] = 0;
	    break;
	}
    }
    //
    paths = NSSearchPathForDirectoriesInDomains( NSCachesDirectory, NSUserDomainMask, YES );
    const char* caches_path_utf8 = [ [ paths objectAtIndex: 0 ] UTF8String ];
    len = (int)strlen( caches_path_utf8 );
    g_osx_caches_path = (char*)malloc( len + 2 );
    memcpy( g_osx_caches_path, caches_path_utf8, len + 1 );
    strcat( g_osx_caches_path, "/" );
    //
    //NSString *tmp = NSTemporaryDirectory();
    //const char *tmp_path_utf8 = [ tmp UTF8String ];
    const char *tmp_path_utf8 = "/tmp";
    len = (int)strlen( tmp_path_utf8 );
    g_osx_tmp_path = (char*)malloc( len + 2 );
    memcpy( g_osx_tmp_path, tmp_path_utf8, len + 1 );
    strcat( g_osx_tmp_path, "/" );
    //
    printf( "%s\n", g_osx_docs_path );
    printf( "%s\n", g_osx_caches_path );
    printf( "%s\n", g_osx_tmp_path );
    
    g_ui_lang = strdup( [ [ [ NSLocale preferredLanguages ] objectAtIndex:0 ] UTF8String ] );
    
    printf( "sundog_init(): width  %d\n", g_osx_sundog_screen_xsize );
    printf( "sundog_init(): height %d\n", g_osx_sundog_screen_ysize );
    
    //Create main thread:    
    err = pthread_create( &g_sd.pth, 0, &bthread, (void*)&g_sd );
    if( err == 0 )
    {
	//The pthread_detach() function marks the thread identified by thread as
	//detached. When a detached thread terminates, its resources are 
	//automatically released back to the system.
	err = pthread_detach( g_sd.pth );
	if( err != 0 )
	{
	    printf( "sundog_init(): pthread_detach error %d\n", err );
	    return;
	}
    }
    else
    {
	printf( "sundog_init(): pthread_create error %d\n", err );
	return;
    }
    
    printf( "sundog_init(): done\n" );
    g_sd.initialized = 1;
}

void sundog_deinit( void )
{
    int err;
    
    printf( "sundog_deinit() ...\n" );
    
    if( g_sd.initialized == 0 ) return;

    //Stop the thread:
    win_exit_request( g_sd.s.wm );
    int timeout_counter = 1000 * 5; //Timeout in milliseconds
    while( timeout_counter > 0 )
    {
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 1000000 * 20; //20 milliseconds
	if( g_sd.pth_finished ) break;
	nanosleep( &delay, NULL ); //Sleep for delay time
	timeout_counter -= 20;
    }
    if( timeout_counter <= 0 )
    {
	printf( "sundog_deinit(): thread timeout\n" );
	err = pthread_cancel( g_sd.pth );
	if( err != 0 )
	{
	    printf( "sundog_deinit(): pthread_cancel error %d\n", err );
	}
    }
    
    //Free the paths and global names:
    free( g_osx_docs_path );
    free( g_osx_caches_path );
    free( g_osx_tmp_path );
    free( g_ui_lang );

    printf( "sundog_deinit(): done\n" );
}

void osx_sundog_screen_redraw( void )
{
    CGLFlushDrawable( (CGLContextObj)g_gl_context_obj );
}

void osx_sundog_rename_window( const char* name )
{
    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];
    [ g_sundog_window performSelectorOnMainThread:@selector(setTitle:) withObject:[ NSString stringWithUTF8String:name ] waitUntilDone:NO ];
    [ pool release ];
}

volatile bool g_eventloop_stop_request = 0;
volatile bool g_eventloop_stop_answer = 0;

void osx_sundog_event_handler( void )
{
    HANDLE_THREAD_EVENTS;
    if( g_eventloop_stop_request )
    {
	g_eventloop_stop_answer = 1;
	while( g_eventloop_stop_request )
	{
	}
	g_eventloop_stop_answer = 0;
    }
}

#define DIV 1

@interface MainWindowDelegate : NSObject
{
}
- (void)windowDidBecomeKey:(NSNotification *)notification;
- (void)windowDidResignKey:(NSNotification *)notification;
- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize;
- (void)windowDidResize:(NSNotification *)notification;
- (void)windowWillStartLiveResize:(NSNotification *)notification;
- (void)windowDidEndLiveResize:(NSNotification *)notification;
@end
@implementation MainWindowDelegate
- (void)windowDidBecomeKey:(NSNotification *)notification
{
}
- (void)windowDidResignKey:(NSNotification *)notification
{
    printf( "UNFOCUS\n" );
    osx_sundog_key( 55, 0, g_sd.s.wm ); //CMD up
    osx_sundog_key( 59, 0, g_sd.s.wm ); //CTRL up
    osx_sundog_key( 58, 0, g_sd.s.wm ); //ALT up
}
- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
    g_eventloop_stop_request = 1;
    while( g_eventloop_stop_answer == 0 ) { usleep( 1000 * 5 ); }
    
    /*NSRect frameRect;
    frameRect.origin.x = 0;
    frameRect.origin.y = 0;
    frameRect.size.width = frameSize.width;
    frameRect.size.height = frameSize.height;
    NSRect content_frame = [ NSWindow contentRectForFrameRect:frameRect styleMask:[ sender styleMask ] ];
    g_osx_sundog_screen_xsize = (int)content_frame.size.width;
    g_osx_sundog_screen_ysize = (int)content_frame.size.height;*/

    return frameSize;
}
- (void)windowDidResize:(NSNotification *)notification
{
    if( g_sundog_view )
    {
	NSRect r = [ g_sundog_view frame ];
	g_osx_sundog_screen_xsize = (int)r.size.width;
	g_osx_sundog_screen_ysize = (int)r.size.height;
    }
    
    g_eventloop_stop_request = 0;
    while( g_eventloop_stop_answer == 1 ) { usleep( 1000 * 5 ); }
}
- (void)windowWillStartLiveResize:(NSNotification *)notification
{
}
- (void)windowDidEndLiveResize:(NSNotification *)notification
{
    g_eventloop_stop_request = 1;
    while( g_eventloop_stop_answer == 0 ) { usleep( 1000 * 5 ); }
    
    g_osx_sundog_resize_request = 1;
    
    g_eventloop_stop_request = 0;
    while( g_eventloop_stop_answer == 1 ) { usleep( 1000 * 5 ); }
}
@end

static MainWindowDelegate* g_main_window_delegate;

@implementation MyView

- (id) initWithFrame: (NSRect) frame
{
    GLuint attribs[] = 
    {
#ifndef GLNORETAIN
	NSOpenGLPFABackingStore,
#endif
	NSOpenGLPFANoRecovery,
	NSOpenGLPFAWindow,
	NSOpenGLPFAAccelerated,
	NSOpenGLPFADoubleBuffer,
	NSOpenGLPFAColorSize, 24,
	NSOpenGLPFAAlphaSize, 8,
#ifdef GLZBUF
        NSOpenGLPFADepthSize, 32,
#else
	NSOpenGLPFADepthSize, 0,
#endif
	NSOpenGLPFAStencilSize, 0,
	NSOpenGLPFAAccumSize, 0,
	0
    };
    
    NSOpenGLPixelFormat* fmt = [ [ NSOpenGLPixelFormat alloc ] initWithAttributes: (NSOpenGLPixelFormatAttribute*) attribs ]; 
    
    if( !fmt )
	NSLog( @"No OpenGL pixel format" );
    
    self = [ super initWithFrame:frame pixelFormat: [ fmt autorelease ] ];
    
    return self;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    return YES;
}

- (BOOL)resignFirstResponder
{
    return YES;
}

- (void)mouseDown:(NSEvent *)theEvent
{
    NSPoint point = [ theEvent locationInWindow ];
    NSPoint local_point = [ self convertPoint: point fromView: nil ];
    float x = local_point.x;
    float y = g_osx_sundog_screen_ysize - 1 - local_point.y;
    osx_sundog_touches( (int)x - 1, (int)y - 1, 0, 0, g_sd.s.wm );
}    

- (void)mouseUp:(NSEvent *)theEvent
{
    NSPoint point = [ theEvent locationInWindow ];
    NSPoint local_point = [ self convertPoint: point fromView: nil ];
    float x = local_point.x;
    float y = g_osx_sundog_screen_ysize - 1 - local_point.y;
    osx_sundog_touches( (int)x - 1, (int)y - 1, 2, 0, g_sd.s.wm );
}    

- (void)mouseDragged:(NSEvent *)theEvent
{
    NSPoint point = [ theEvent locationInWindow ];
    NSPoint local_point = [ self convertPoint: point fromView: nil ];
    float x = local_point.x;
    float y = g_osx_sundog_screen_ysize - 1 - local_point.y;
    osx_sundog_touches( (int)x, (int)y, 1, 0, g_sd.s.wm );
}    

- (void)rightMouseDown:(NSEvent *)theEvent
{
    NSPoint point = [ theEvent locationInWindow ];
    NSPoint local_point = [ self convertPoint: point fromView: nil ];
    float x = local_point.x;
    float y = g_osx_sundog_screen_ysize - 1 - local_point.y;
    osx_sundog_touches( (int)x - 1, (int)y - 1, 0 + ( 1 << 3 ), 0, g_sd.s.wm );
}    

- (void)rightMouseUp:(NSEvent *)theEvent
{
    NSPoint point = [ theEvent locationInWindow ];
    NSPoint local_point = [ self convertPoint: point fromView: nil ];
    float x = local_point.x;
    float y = g_osx_sundog_screen_ysize - 1 - local_point.y;
    osx_sundog_touches( (int)x - 1, (int)y - 1, 2 + ( 1 << 3 ), 0, g_sd.s.wm );
}    

- (void)rightMouseDragged:(NSEvent *)theEvent
{
    NSPoint point = [ theEvent locationInWindow ];
    NSPoint local_point = [ self convertPoint: point fromView: nil ];
    float x = local_point.x;
    float y = g_osx_sundog_screen_ysize - 1 - local_point.y;
    osx_sundog_touches( (int)x, (int)y, 1 + ( 1 << 3 ), 0, g_sd.s.wm );
}    

- (void)otherMouseDown:(NSEvent *)theEvent
{
    NSPoint point = [ theEvent locationInWindow ];
    NSPoint local_point = [ self convertPoint: point fromView: nil ];
    float x = local_point.x;
    float y = g_osx_sundog_screen_ysize - 1 - local_point.y;
    osx_sundog_touches( (int)x - 1, (int)y - 1, 0 + ( 2 << 3 ), 0, g_sd.s.wm );
}    

- (void)otherMouseUp:(NSEvent *)theEvent
{
    NSPoint point = [ theEvent locationInWindow ];
    NSPoint local_point = [ self convertPoint: point fromView: nil ];
    float x = local_point.x;
    float y = g_osx_sundog_screen_ysize - 1 - local_point.y;
    osx_sundog_touches( (int)x - 1, (int)y - 1, 2 + ( 2 << 3 ), 0, g_sd.s.wm );
}    

- (void)otherMouseDragged:(NSEvent *)theEvent
{
    NSPoint point = [ theEvent locationInWindow ];
    NSPoint local_point = [ self convertPoint: point fromView: nil ];
    float x = local_point.x;
    float y = g_osx_sundog_screen_ysize - 1 - local_point.y;
    osx_sundog_touches( (int)x, (int)y, 1 + ( 2 << 3 ), 0, g_sd.s.wm );
}    

- (void)scrollWheel:(NSEvent *)theEvent
{
    float x = [ theEvent locationInWindow ].x;
    float y = g_osx_sundog_screen_ysize - 1 - [ theEvent locationInWindow ].y;
    float dy = [ theEvent deltaY ];
    if( dy )
    {
	if( dy < 0 )
	    osx_sundog_touches( (int)x, (int)y, 3, 0, g_sd.s.wm );
	else
	    osx_sundog_touches( (int)x, (int)y, 4, 0, g_sd.s.wm );
    }
}

- (void)keyDown:(NSEvent *)theEvent
{
    unsigned short vk = [ theEvent keyCode ];
    osx_sundog_key( vk, 1, g_sd.s.wm );
}

- (void)keyUp:(NSEvent *)theEvent
{
    unsigned short vk = [ theEvent keyCode ];
    osx_sundog_key( vk, 0, g_sd.s.wm );
}

- (void)flagsChanged:(NSEvent *)theEvent
{
    unsigned short vk = [ theEvent keyCode ];
    bool pushed = 0;
    if( vk == 55 ) //Cmd
    {
	if( [ theEvent modifierFlags ] & NSCommandKeyMask )
	    pushed = 1;
	osx_sundog_key( vk, pushed, g_sd.s.wm );
    }
    if( vk == 56 || vk == 60 ) //Shift
    {
	if( [ theEvent modifierFlags ] & NSShiftKeyMask )
	    pushed = 1;
	osx_sundog_key( vk, pushed, g_sd.s.wm );
    }
    if( vk == 59 ) //Ctrl
    {
	if( [ theEvent modifierFlags ] & NSControlKeyMask )
	    pushed = 1;
	osx_sundog_key( vk, pushed, g_sd.s.wm );
    }
    if( vk == 58 ) //Alt
    {
	if( [ theEvent modifierFlags ] & NSAlternateKeyMask )
	    pushed = 1;
	osx_sundog_key( vk, pushed, g_sd.s.wm );
    }
    if( vk == 57 ) //Caps
    {
	osx_sundog_key( vk, 1, g_sd.s.wm );
	osx_sundog_key( vk, 0, g_sd.s.wm );
    }
}

- (void)viewBoundsChanged: (NSNotification*)aNotification
{
}

- (void)viewFrameChanged: (NSNotification*)aNotification
{
    /*NSRect r = [ self frame ];
    g_osx_sundog_screen_xsize = r.size.width;
    g_osx_sundog_screen_ysize = r.size.height;*/
}

- (int)viewInit
{
    g_gl_context = [ self openGLContext ];
    g_gl_context_obj = [ g_gl_context CGLContextObj ];
    
    NSRect r = [ self frame ];
    g_osx_sundog_screen_xsize = r.size.width;
    g_osx_sundog_screen_ysize = r.size.height;
    
    NSNotificationCenter *dnc;
    dnc = [ NSNotificationCenter defaultCenter ];
    [ dnc addObserver:self selector:@selector( viewFrameChanged:) name:NSViewFrameDidChangeNotification object:self ];
    [ dnc addObserver:self selector:@selector( viewBoundsChanged:) name:NSViewBoundsDidChangeNotification object:self ];

    g_main_window_delegate = [ [ MainWindowDelegate alloc ] init ];
    [ [ self window ] setDelegate:g_main_window_delegate ];
    
    {
	//Clear OpenGL context:
	
	[ g_gl_context makeCurrentContext ];
	
	int err = CGLLockContext( (CGLContextObj)g_gl_context_obj );
	if( err != 0 )
	{
	    printf( "[viewInit] CGLLockContext() error %d\n", err );
	}
	
	glClear( GL_COLOR_BUFFER_BIT );
	
	err = CGLUnlockContext( (CGLContextObj)g_gl_context_obj );
	if( err != 0 )
	{
	    printf( "[viewInit] CGLUnlockContext() error %d\n", err );
	}
    }
        
    return 0;
}

- (void)drawRect:(NSRect)rect
{
    if( g_gl_context_obj == 0 ) return;
    if( g_gl_context == 0 ) return;
}

@end
