#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <errno.h>

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import <UIKit/UIKit.h>
#import <UIKit/UIPasteboard.h>
#import <MobileCoreServices/UTCoreTypes.h>
#import <MessageUI/MFMailComposeViewController.h>

#include "core/core.h"
#include "sundog_bridge.h"

int g_iphone_sundog_screen_xsize = 320;
int g_iphone_sundog_screen_ysize = 480;

extern char* g_osx_docs_path;
extern char* g_osx_caches_path;
extern char* g_osx_tmp_path;
char* g_osx_resources_path = 0;
char* g_iphone_host_addr = 0;
char* g_iphone_host_name = 0;

char* g_ui_lang = 0;

UIWindow* g_sundog_window = 0;
MainViewController* g_sundog_view_controller = 0;
MyView* g_sundog_view = 0;
CGDataProviderRef g_fb_provider;
CGColorSpaceRef g_fb_colorspace;
CGImageRef g_fb_image;
CGRect g_fb_rect;
EAGLContext* g_gl_context;
uint g_view_renderbuffer;
uint g_view_framebuffer;

pthread_mutex_t g_ios_gl_mutex;

char* g_open_document_name = 0;

typedef struct sundog_info_struct
{
    int initialized;
    pthread_t pth;
    int pth_finished;
    sundog_engine s;
} sundog_info_struct;
sundog_info_struct g_sd;

//
// These functions will be called from the App Delegate
//

volatile bool break_bg_loop = 0;
void* bthread( void* arg )
{
    sundog_info_struct* sd = (sundog_info_struct*)arg;

    while( [ UIApplication sharedApplication ].applicationState == UIApplicationStateBackground )
    {
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 1000000 * 20; //20 milliseconds
	if( g_sd.pth_finished ) break;
	nanosleep( &delay, NULL ); //Sleep for delay time
	if( break_bg_loop ) return 0;
    }

    [ EAGLContext setCurrentContext: g_gl_context ];
    
    sundog_main( &sd->s, true );

    sd->pth_finished = 1;
    pthread_exit( NULL );
}

void sundog_init( UIWindow* window )
{
    int err;

    blog( "sundog_init() ...\n" );

    g_sundog_window = window;
    g_sundog_view_controller = (MainViewController*)window.rootViewController;
    g_sundog_view = (MyView*)g_sundog_view_controller.view;
    
#ifdef AUDIOBUS
    audiobus_init();
#endif
    
    //Init the view:
    if( [ g_sundog_view viewInit ] ) return;
    
    memset( &g_sd, 0, sizeof( sundog_info_struct ) );

    //Get system paths:
    NSArray* paths = NSSearchPathForDirectoriesInDomains( NSDocumentDirectory, NSUserDomainMask, YES );
    const char* docs_path_utf8 = [ [ paths objectAtIndex: 0 ] UTF8String ];
    size_t len = strlen( docs_path_utf8 );
    g_osx_docs_path = (char*)malloc( len + 2 );
    memcpy( g_osx_docs_path, docs_path_utf8, len + 1 );
    strcat( g_osx_docs_path, "/" );     
    //
    paths = NSSearchPathForDirectoriesInDomains( NSCachesDirectory, NSUserDomainMask, YES );
    const char* caches_path_utf8 = [ [ paths objectAtIndex: 0 ] UTF8String ];
    len = strlen( caches_path_utf8 );
    g_osx_caches_path = (char*)malloc( len + 2 );
    memcpy( g_osx_caches_path, caches_path_utf8, len + 1 );
    strcat( g_osx_caches_path, "/" );
    //
    NSString* tmp = NSTemporaryDirectory();
    const char* tmp_path_utf8 = [ tmp UTF8String ];
    len = strlen( tmp_path_utf8 );
    g_osx_tmp_path = (char*)malloc( len + 2 );
    memcpy( g_osx_tmp_path, tmp_path_utf8, len + 1 );
    strcat( g_osx_tmp_path, "/" );
    //
    const char* res_path_utf8 = [ [ [ NSBundle mainBundle ] resourcePath ] UTF8String ];
    len = strlen( res_path_utf8 );
    g_osx_resources_path = (char*)malloc( len + 2 );
    memcpy( g_osx_resources_path, res_path_utf8, len + 1 );
    strcat( g_osx_resources_path, "/" );
    //
    blog( "%s\n", g_osx_docs_path );
    blog( "%s\n", g_osx_caches_path );
    blog( "%s\n", g_osx_tmp_path );
    blog( "%s\n", g_osx_resources_path );
    
    g_ui_lang = strdup( [ [ [ NSLocale preferredLanguages ] objectAtIndex:0 ] UTF8String ] );

    blog( "Default screen width: %d\n", g_iphone_sundog_screen_xsize );
    blog( "Default screen height: %d\n", g_iphone_sundog_screen_ysize );

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
	    blog( "sundog_init(): pthread_detach error %d\n", err );
	    return;
	}
    }
    else
    {
	blog( "sundog_init(): pthread_create error %d\n", err );
	return;
    }
        
    blog( "sundog_init(): done\n" );
    g_sd.initialized = 1;
}

void sundog_deinit( void )
{
    int err;
    
    blog( "sundog_deinit() ...\n" );
    
    if( g_sd.initialized == 0 ) 
    {
	break_bg_loop = 1;
	return;
    }

    //Stop the thread:
    win_exit_request( g_sd.s.wm );
    {
	g_eventloop_stop_request = 0;
	while( g_eventloop_stop_answer == 1 ) { }
    }
    int timeout_counter = 1000 * 6; //Timeout in milliseconds
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
	blog( "sundog_deinit(): thread timeout\n" );
	err = pthread_cancel( g_sd.pth );
	if( err != 0 )
	{
	    blog( "sundog_deinit(): pthread_cancel error %d\n", err );
	}
    }
    
    //Free the paths and global names:
    free( g_osx_docs_path );
    free( g_osx_caches_path );
    free( g_osx_tmp_path );
    free( g_osx_resources_path );
    free( g_iphone_host_addr );
    free( g_iphone_host_name );
    free( g_open_document_name );
    free( g_ui_lang );
    
    [ g_sundog_view viewDeinit ];

#ifdef AUDIOBUS
    audiobus_deinit();
#endif
    
    pthread_mutex_destroy( &g_ios_gl_mutex );

    blog( "sundog_deinit(): done\n" );
}

void sundog_send_opendocument_request( const char* file_path )
{
    if( file_path == 0 ) return;
    free( g_open_document_name );
    g_open_document_name = strdup( file_path );
    g_open_document_request = g_open_document_name;
}

volatile bool g_eventloop_stop_request = 0;
volatile bool g_eventloop_stop_answer = 0;

void sundog_pause( void )
{
    blog( "App pause\n" );
#ifdef IDLE_TIMER_DISABLED
    [ UIApplication sharedApplication ].idleTimerDisabled = NO;
#endif
    g_eventloop_stop_request = 1;
    while( g_eventloop_stop_answer == 0 ) { }
}

void sundog_resume( void )
{
    blog( "App resume\n" );
#ifdef IDLE_TIMER_DISABLED
    [ UIApplication sharedApplication ].idleTimerDisabled = YES;
#endif
    g_eventloop_stop_request = 0;
    while( g_eventloop_stop_answer == 1 ) { }
}

//
// These functions will be called from the SunDog (wm_iphone.h and others)
//

void iphone_sundog_gl_lock( void )
{
    pthread_mutex_lock( &g_ios_gl_mutex );
}

void iphone_sundog_gl_unlock( void )
{
    pthread_mutex_unlock( &g_ios_gl_mutex );
}

void iphone_sundog_event_handler( void )
{
    HANDLE_THREAD_EVENTS;
    bool sound_pause = 0;
    int start_idle_seconds = g_sound_idle_seconds;
    if( g_eventloop_stop_request )
    {
	blog( "SunDog event loop stopped\n" );
	g_eventloop_stop_answer = 1;
	win_suspend( true, g_sd.s.wm );
	while( g_eventloop_stop_request )
	{
	    if( g_sound_idle_seconds - start_idle_seconds > 60 * 4 )
	    {
		if( sound_pause == 0 )
		{
		    blog( "sound_stream_device_stop...\n" );
		    sound_stream_device_stop();
		    //exit( 0 ); //https://developer.apple.com/library/ios/#qa/qa2008/qa1561.html
		    sound_pause = 1;
		    //... and then app will be frozen (because audiosession was closed in sound_stream_device_stop())
		}
	    }
	    usleep( 1000 * 200 );
	}
	g_eventloop_stop_answer = 0;
	win_suspend( false, g_sd.s.wm );
	sound_stream_device_play();
	blog( "SunDog event loop started\n" );
    }
}

void iphone_sundog_get_host_info( void )
{
    if( g_iphone_host_addr ) free( g_iphone_host_addr );
    if( g_iphone_host_name ) free( g_iphone_host_name );
    g_iphone_host_addr = 0;
    g_iphone_host_name = 0;
    
    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];
    
    /*NSHost* host = [ NSHost currentHost ];
    if( host )
    {
	NSString* str = [ host address ];
	if( str )
	{
	    const char* addr = [ str UTF8String ];
	    int len = strlen( addr );
	    g_iphone_host_addr = (char*)malloc( len + 1 );
	    memcpy( g_iphone_host_addr, addr, len + 1 );
	}
	str = [ host name ];
	if( str )
	{
	    const char* name = [ str UTF8String ];
	    int len = strlen( name );
	    g_iphone_host_name = (char*)malloc( len + 1 );
	    memcpy( g_iphone_host_name, name, len + 1 );
	}
    }*/
    
    struct ifaddrs* myaddrs;
    struct ifaddrs* ifa;
    struct sockaddr_in* s4;
    int status;
    char buf[ 128 ];
    buf[ 0 ] = 0;
    
    status = getifaddrs( &myaddrs );
    if( status != 0 )
    {
	perror( "getifaddrs" );
    }
    
    for( ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next )
    {
	if( ifa->ifa_addr == NULL ) continue;
	if( ( ifa->ifa_flags & IFF_UP ) == 0 ) continue;
	
	if( ifa->ifa_addr->sa_family == AF_INET )
	{
	    s4 = (struct sockaddr_in *)(ifa->ifa_addr);
	    if( inet_ntop( ifa->ifa_addr->sa_family, (void *)&(s4->sin_addr), buf, sizeof( buf ) ) == NULL )
	    {
		blog( "%s: inet_ntop failed!\n", ifa->ifa_name );
	    }
	    else
	    {
		blog( "%s: %s\n", ifa->ifa_name, buf );
		unsigned char* a = (unsigned char*)&(s4->sin_addr);
		if( a[ 0 ] == 192 ) break;
	    }
	}
	/*else if( ifa->ifa_addr->sa_family == AF_INET6 )
	{
	    struct sockaddr_in6* s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);
	    if( inet_ntop( ifa->ifa_addr->sa_family, (void *)&(s6->sin6_addr), buf, sizeof( buf ) ) == NULL )
	    {
		blog( "%s: inet_ntop failed!\n", ifa->ifa_name );
	    }
	    else
	    {
		blog( "%s: %s\n", ifa->ifa_name, buf );
	    }
	}*/
    }
    
    freeifaddrs( myaddrs );
    
    size_t len = strlen( buf );
    if( len )
    {
	g_iphone_host_addr = (char*)malloc( len + 1 );
	memcpy( g_iphone_host_addr, buf, len + 1 );
    }
    
    if( g_iphone_host_addr )
	blog( "Host addr: %s\n", g_iphone_host_addr );
    if( g_iphone_host_name )
	blog( "Host name: %s\n", g_iphone_host_name );
    
    [ pool release ];
}

// 5MB max per item in iPhone OS clipboard
#define BM_CLIPBOARD_CHUNK_SIZE ( 5 * 1024 * 1024 )

int iphone_sundog_copy( const char* filename )
{
    int rv = -1;

    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];
    
    while( 1 )
    {
	NSString* dataType = (NSString*)kUTTypeUTF8PlainText;
	bfs_file_type ftype = bfs_get_file_type( filename, 0 );
	switch( ftype )
	{
	    case BFS_FILE_TYPE_WAVE:
	    case BFS_FILE_TYPE_AIFF:
	    case BFS_FILE_TYPE_OGG:
	    case BFS_FILE_TYPE_MP3:
	    case BFS_FILE_TYPE_FLAC:
		dataType = (NSString*)kUTTypeAudio;
		break;
	    case BFS_FILE_TYPE_JPEG:
	    case BFS_FILE_TYPE_GIF:
	    case BFS_FILE_TYPE_PNG:
		dataType = (NSString*)kUTTypeImage;
		break;
	    default:
		break;
	}

        // More types:
        // kUTTypeAudiovisualContent - An abstract type identifier for audio and/or video content.
        // kUTTypeMovie - An abstract type identifier for a media format which may contain both video and audio. Corresponds to what users would label a "movie"
        // kUTTypeVideo - An abstract type identifier for pure video data(no audio).
	// kUTTypeAudio - An abstract type identifier for pure audio data (no video).
	// kUTTypePlainText - The type identifier for text with no markup and in an unspecified encoding.
	// kUTTypeUTF8PlainText - The type identifier for plain text in a UTF-8 encoding. 

	UIPasteboard* board = [ UIPasteboard generalPasteboard ];
	
	//Open file as binary data
	NSData* dataFile = [ NSData dataWithContentsOfMappedFile:[ NSString stringWithUTF8String:filename ] ];
	if( !dataFile ) 
	{
	    NSLog( @"doCopyButton: Can't open file" );
	    break;
	}
	
	//Create chunked data and append to clipboard
	NSUInteger sz = [ dataFile length ];
	NSUInteger chunkNumbers = ( sz / BM_CLIPBOARD_CHUNK_SIZE ) + 1;
	NSMutableArray* items = [ NSMutableArray arrayWithCapacity:chunkNumbers ];
	NSRange curRange;
	
	for( NSUInteger i = 0; i < chunkNumbers; i++ ) 
	{
	    curRange.location = i * BM_CLIPBOARD_CHUNK_SIZE;
	    curRange.length = MIN( BM_CLIPBOARD_CHUNK_SIZE, sz - curRange.location );
	    NSData* subData = [ dataFile subdataWithRange:curRange ];
	    NSDictionary* dict = [ NSDictionary dictionaryWithObject:subData forKey:dataType ];
	    [ items addObject:dict ];
	    rv = 0;
	}
	
	board.items = items;
	
	break;
    }
    
    [ pool release ];
    
    return rv;
}

char* iphone_sundog_paste( void )
{
    char* rv = 0;
    
    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];
    
    while( 1 )
    {
	UIPasteboard* board = [ UIPasteboard generalPasteboard ];
	
	NSArray* typeArray = [ NSArray arrayWithObject:(NSString *) kUTTypeAudio ];
	NSIndexSet* set = [ board itemSetWithPasteboardTypes:typeArray ];
	if( !set ) 
	{
	    NSLog( @"doPasteButton: Can't get item set" );
	    break;
	}
	
	//Get the subset of kUTTypeAudio elements, and write each chunk to a temporary file:
	NSArray* items = [ board dataForPasteboardType:(NSString *) kUTTypeAudio inItemSet:set ];
	if( items ) 
	{
	    size_t cnt = [ items count ];
	    if( !cnt ) 
	    {
		NSLog( @"doPasteButton: Nothing to paste" );
		break;
	    }
	    
	    //Create a file and write each chunks to it:
	    NSString* path = [ NSTemporaryDirectory() stringByAppendingPathComponent:@"temp-pasteboard" ];
	    if( ! [ [ NSFileManager defaultManager ] createFileAtPath:path contents:nil attributes:nil ] ) 
	    {
		NSLog( @"doPasteButton: Can't create file" );
	    }
	    
	    NSFileHandle* handle = [ NSFileHandle fileHandleForWritingAtPath:path ];
	    if( !handle ) 
	    {
		NSLog( @"doPasteButton: Can't open file for writing" );
		break;
	    }
	    
	    //Write each chunk to file:
	    for( UInt32 i = 0; i < cnt; i++ ) 
	    {
		[ handle writeData:[ items objectAtIndex:i ] ];
	    }
	    [ handle closeFile ];

	    rv = (char*)malloc( strlen( [ path UTF8String ] ) + 1 );
	    rv[ 0 ] = 0;
	    strcat( rv, [ path UTF8String ] );
	}
	
	break;
    }

    [ pool release ];
    
    return rv;
}

void iphone_sundog_open_url( const char* url_text )
{
    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];
    
    NSURL* url = [ NSURL URLWithString:[ NSString stringWithUTF8String:url_text ] ];
    [ [ UIApplication sharedApplication ] openURL:url ];

    [ pool release ];
}

void iphone_sundog_send_mail( const char* email_text, const char* subj_text, const char* body_text )
{
    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];
    
    NSString* mailLinkTemplate = @"mailto:%@?subject=%@&body=%@"; // to subject body
    NSString* body = [ NSString stringWithUTF8String:body_text ];
    
    NSString* bodyEncoded = [ body stringByAddingPercentEscapesUsingEncoding: NSUTF8StringEncoding ];
    NSString* themeEncoded = [ [ NSString stringWithUTF8String:subj_text ] stringByAddingPercentEscapesUsingEncoding: NSUTF8StringEncoding ];
    NSString* emailEncoded = [ [ NSString stringWithUTF8String:email_text ] stringByAddingPercentEscapesUsingEncoding: NSUTF8StringEncoding ];
    
    NSString* eMailURL = [ NSString stringWithFormat: mailLinkTemplate, emailEncoded, themeEncoded, bodyEncoded ];
    [ [ UIApplication sharedApplication ] openURL:[ NSURL URLWithString:eMailURL ] ];
    
    [ pool release ];
}

@interface mailDelegate: NSObject <MFMailComposeViewControllerDelegate>
{
}
@end
@implementation mailDelegate
- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error
{
    [ g_sundog_view_controller dismissViewControllerAnimated:NO completion:0 ];
}
@end

void iphone_sundog_send_file_to_email( const char* subj_text, const char* file_path )
{
    const char* body_text = "";
    const char* mime_type = bfs_get_mime_type( bfs_get_file_type( file_path, 0 ) );
    const char* file_desc = "File";
    
    /*NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];
    
    mailDelegate* d = [ [ mailDelegate alloc ] init ];
    
    if( [ MFMailComposeViewController canSendMail ] )
    {
	MFMailComposeViewController* controller = [ [ MFMailComposeViewController alloc ] init ];
	//controller.mailComposeDelegate = d;
	[ controller setSubject:[ NSString stringWithUTF8String:subj_text ] ];
	[ controller setMessageBody:[ NSString stringWithUTF8String:body_text ] isHTML:NO ];
	[ controller addAttachmentData:[ NSData dataWithContentsOfFile:[ NSString stringWithUTF8String:file_path ] ] mimeType:[ NSString stringWithUTF8String:mime_type ] fileName:[ NSString stringWithUTF8String:file_desc ] ];
	if( controller )
	{
	    [ g_sundog_view.window.rootViewController presentViewController:controller animated:YES completion:nil ];
	    //[ controller release ];
	}
    }
    else
    {
	blog( "Can't send E-mail\n" );
    }
    
    [ d release ];
    
    [ pool release ];*/
}

void iphone_sundog_send_file_to_gallery( const char* file_path )
{
    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];

    switch( bfs_get_file_type( file_path, 0 ) )
    {
	case BFS_FILE_TYPE_JPEG:
	case BFS_FILE_TYPE_PNG:
	case BFS_FILE_TYPE_GIF:
	    {
		UIImage* img = [ UIImage imageWithContentsOfFile:[ NSString stringWithUTF8String:file_path ] ];
		if( img == 0 )
		{
		    blog( "Can't load image %s\n", file_path );
		}
		else
		{
		    UIImageWriteToSavedPhotosAlbum( img, 0, 0, 0 );
		    blog( "Sent to Photos (%s)\n", file_path );
		}
	    }
	    break;
	case BFS_FILE_TYPE_AVI:
	    {
		if( UIVideoAtPathIsCompatibleWithSavedPhotosAlbum( [ NSString stringWithUTF8String:file_path ] ) )
		{
		    UISaveVideoAtPathToSavedPhotosAlbum( [ NSString stringWithUTF8String:file_path ], 0, 0, 0 );
		    blog( "Sent to Videos (%s)\n", file_path );
		}
		else
		{
		    blog( "Video format is not supported (%s)\n", file_path );
		}
	    }
	    break;
	default:
	    break;
    }

    [ pool release ];
}

void iphone_sundog_screen_redraw( void )
{
    [ g_sundog_view redrawAll ];
}

//
// App Delegate
//

@interface AppDelegate ()

@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    sundog_init( self.window );
    return YES;
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
    const char* path = [ [ url path ] UTF8String ];
    if( path )
    {
	sundog_send_opendocument_request( path );
	return YES;
    }
    else
    {
	return NO;
    }
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) 
    // or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    sundog_pause();
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of transition from the background to the inactive state: here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    sundog_resume();
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    sundog_deinit();
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    // Free up as much memory as possible by purging cached data objects that can be recreated (or reloaded from disk) later.
}

@end

//
// Main View Controller
//

@implementation MainViewController

- (void)viewDidLoad
{
    [ super viewDidLoad ];
}

- (BOOL)shouldAutorotate
{
    return YES;
}

@end

//
// MyView
//

@implementation MyView

+ (Class)layerClass
{
    return [ CAEAGLLayer class ];
}

- (id)initWithCoder:(NSCoder*)coder
{
    //call the init method of our parent view
    self = [ super initWithCoder:coder ];
    
    if( !self )
	return nil;
    
    viewFramebuffer = 0;
    viewRenderbuffer = 0;
    depthRenderbuffer = 0;
    
    self.multipleTouchEnabled = YES;
    
    //Init the layer:
    CAEAGLLayer* eaglLayer = (CAEAGLLayer*) self.layer;
    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties = [
	NSDictionary dictionaryWithObjectsAndKeys:
#ifdef GLNORETAIN
	[ NSNumber numberWithBool:FALSE ],
#else
	[ NSNumber numberWithBool:TRUE ],
#endif
	kEAGLDrawablePropertyRetainedBacking,
	kEAGLColorFormatRGB565,
	kEAGLDrawablePropertyColorFormat, nil
    ];
    
    //Create context:
    context = [ [ EAGLContext alloc ] initWithAPI:kEAGLRenderingAPIOpenGLES2 ];
    g_gl_context = context;
    
    return self;
}

- (void)didMoveToWindow
{
    [ super didMoveToWindow ];
    self.contentScaleFactor = 1.0f;
}

- (BOOL)createFramebuffer
{
    bool rv = NO;
    blog( "createFramebuffer() ...\n" );
    
    iphone_sundog_gl_lock();
    
    while( 1 )
    {
	if( context == 0 ) break;
	if( ![ EAGLContext setCurrentContext:context ] ) break;
    
	glGenFramebuffers( 1, &viewFramebuffer );
	glGenRenderbuffers( 1, &viewRenderbuffer );
    	g_view_framebuffer = viewRenderbuffer;
	g_view_renderbuffer = viewFramebuffer;
    
	glBindFramebuffer( GL_FRAMEBUFFER, viewFramebuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, viewRenderbuffer );
	[ context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(id<EAGLDrawable>)self.layer ];
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, viewRenderbuffer );
	
	glGetRenderbufferParameteriv( GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth );
	glGetRenderbufferParameteriv( GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight );
	g_iphone_sundog_screen_xsize = backingWidth;
	g_iphone_sundog_screen_ysize = backingHeight;
    
#ifdef GLZBUF
	glGenRenderbuffers( 1, &depthRenderbuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, depthRenderbuffer );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer );
#endif
    
	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
	{
	    NSLog( @"failed to make complete framebuffer object %x", glCheckFramebufferStatus( GL_FRAMEBUFFER ) );
	    break;
	}
	
	rv = YES;
	break;
    }
    
    iphone_sundog_gl_unlock();
    
    return rv;
}

- (void)removeFramebuffer
{
    blog( "removeFramebuffer() ...\n" );
    
    iphone_sundog_gl_lock();
    
    while( 1 )
    {
	if( context == 0 ) break;
	if( ![ EAGLContext setCurrentContext:context ] ) break;

	if( viewFramebuffer )
	{
	    glDeleteFramebuffers( 1, &viewFramebuffer );
	    viewFramebuffer = 0;
	    g_view_framebuffer = 0;
	}
	if( viewRenderbuffer )
	{
	    glDeleteRenderbuffers( 1, &viewRenderbuffer );
	    viewRenderbuffer = 0;
	    g_view_renderbuffer = 0;
	}
	if( depthRenderbuffer )
	{
	    glDeleteRenderbuffers( 1, &depthRenderbuffer );
	    depthRenderbuffer = 0;
	}
	
	break;
    }
    
    iphone_sundog_gl_unlock();
}

- (int)viewInit
{
    blog( "viewInit() ...\n" );
    self.contentScaleFactor = 1.0f;
    if( ![ self createFramebuffer ] )
    {
	[ self release ];
	return -1;
    }
    return 0;
}

- (void)viewDeinit
{
    blog( "viewDeinit() ...\n" );
    [ self removeFramebuffer ];
}

-(void)layoutSubviews
{
    blog( "layoutSubviews() ...\n" );
    /*int m = (int)[ [ UIApplication sharedApplication ] supportedInterfaceOrientationsForWindow:g_sundog_window ];
    if( m == ( UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown ) ||
        m == ( UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight ) )
    {
	blog( "layoutSubviews: we can use the same GL framebuffer\n" );
	return;
    }*/
    self.contentScaleFactor = 1.0f;
    CGRect b = self.bounds;
    if( g_iphone_sundog_screen_xsize != (int)b.size.width || g_iphone_sundog_screen_ysize != (int)b.size.height )
    {
	iphone_sundog_gl_lock();
	[ self removeFramebuffer ];
	[ self createFramebuffer ];
	iphone_sundog_gl_unlock();
    }
    else
    {
	blog( "layoutSubviews: we can use the same GL framebuffer\n" );
    }
    blog( "layoutSubviews() OK\n" );
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSEnumerator* e = [ touches objectEnumerator ];
    UITouch* touch;
    while( ( touch = [ e nextObject ] ) )
    {
	CGPoint pos = [ touch locationInView: self ];
	collect_touch_events( 0, TOUCH_FLAG_REALWINDOW_XY | TOUCH_FLAG_LIMIT, 0, pos.x, pos.y, 1024, (WM_TOUCH_ID)touch, g_sd.s.wm );
    }
    send_touch_events( g_sd.s.wm );
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    size_t num = [ touches count ];
    NSEnumerator* e = [ touches objectEnumerator ];
    UITouch* touch;
    int touch_count = 0;
    while( ( touch = [ e nextObject ] ) )
    {
	uint evt_flags = 0;
	if( touch_count < num - 1 )
	    evt_flags |= EVT_FLAG_DONTDRAW;
	CGPoint pos = [ touch locationInView: self ];
	collect_touch_events( 1, TOUCH_FLAG_REALWINDOW_XY | TOUCH_FLAG_LIMIT, evt_flags, pos.x, pos.y, 1024, (WM_TOUCH_ID)touch, g_sd.s.wm );
	touch_count++;
    }
    send_touch_events( g_sd.s.wm );
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSEnumerator* e = [ touches objectEnumerator ];
    UITouch* touch;
    while( ( touch = [ e nextObject ] ) )
    {
	CGPoint pos = [ touch locationInView: self ];
	collect_touch_events( 2, TOUCH_FLAG_REALWINDOW_XY | TOUCH_FLAG_LIMIT, 0, pos.x, pos.y, 1024, (WM_TOUCH_ID)touch, g_sd.s.wm );
    }
    send_touch_events( g_sd.s.wm );
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSEnumerator* e = [ touches objectEnumerator ];
    UITouch* touch;
    while( ( touch = [ e nextObject ] ) )
    {
	CGPoint pos = [ touch locationInView: self ];
	collect_touch_events( 2, TOUCH_FLAG_REALWINDOW_XY | TOUCH_FLAG_LIMIT, 0, pos.x, pos.y, 1024, (WM_TOUCH_ID)touch, g_sd.s.wm );
    }
    send_touch_events( g_sd.s.wm );
}

- (void)redrawAll
{
    //Invoke setNeedsDisplay method on the main thread:
    //[ self performSelectorOnMainThread: @selector(setNeedsDisplay) withObject:nil waitUntilDone: NO ];
    
    iphone_sundog_gl_lock();
    glBindRenderbuffer( GL_RENDERBUFFER, viewRenderbuffer );
    [ context presentRenderbuffer:GL_RENDERBUFFER ];
    glBindFramebuffer( GL_FRAMEBUFFER, viewFramebuffer );
    iphone_sundog_gl_unlock();
}

@end

//
// iOS app main()
//

int main( int argc, char* argv[] )
{
    @autoreleasepool
    {
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init( &mutexattr );
	pthread_mutexattr_settype( &mutexattr, PTHREAD_MUTEX_RECURSIVE );
	pthread_mutex_init( &g_ios_gl_mutex, &mutexattr );
	pthread_mutexattr_destroy( &mutexattr );
	
	return UIApplicationMain( argc, argv, nil, NSStringFromClass( [ AppDelegate class ] ) );
    }
}
