#include "core/core.h"
#include "../video.h"

#import "sys/utsname.h"

#import <AVFoundation/AVFoundation.h>

@interface iOSAVCapture: NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
    AVCaptureSession* session;
    AVCaptureDevice* device;
    int width;
    int height;
    bvideo_struct* vid;
}
+ (iOSAVCapture*)newCapture: (const utf8_char*)name baseVideoObj: (bvideo_struct*)vid;
- (int)openSession: (const utf8_char*) name;
- (void)closeSession;
- (void)startCapture;
- (void)stopCapture;
- (int)getWidth;
- (int)getHeight;
@end

@implementation iOSAVCapture

+ (iOSAVCapture*)newCapture: (const utf8_char*)name baseVideoObj: (bvideo_struct*)vid;
{
    iOSAVCapture* cap = [ [ iOSAVCapture alloc ] init ];
    cap->vid = vid;
    if( [ cap openSession: name ] != 0 )
    {
	[ cap release ];
	cap = 0;
    }
    return cap;
}

- (int)openSession: (const utf8_char*) name
{
    int rv = -1;
    while( 1 )
    {
	NSError* error = nil;
	
	// Create the session
	session = [ [ AVCaptureSession alloc ] init ];
	if( session == 0 )
	{
	    blog( "AVCapture: session creation error\n" );
	    break;
	}
	
	// Find a suitable AVCaptureDevice
	int i = -1;
	bool device_hd_support = 0;
	if( bmem_strcmp( name, "camera" ) == 0 )
	{
	    i = profile_get_int_value( KEY_CAMERA, 0, 0 );
	}
	if( bmem_strcmp( name, "camera_back" ) == 0 ) i = 0;
	if( bmem_strcmp( name, "camera_front" ) == 0 ) i = 1;
	NSArray* devices = [ AVCaptureDevice devices ];
	for( AVCaptureDevice* d in devices )
	{
	    if( [ d hasMediaType:AVMediaTypeVideo ] )
	    {
		if( [ d position ] == AVCaptureDevicePositionBack )
		{
		    if( i == 0 ) { device = d; break; }
		}
		if( [ d position ] == AVCaptureDevicePositionFront )
		{
		    if( i == 1 ) { device = d; break; }
		}
	    }
	}
	if( device == 0 )
	{
	    blog( "AVCapture: can't find the device\n" );
	    break;
	}
	if( AVCaptureSessionPreset1280x720 != 0 && [ device supportsAVCaptureSessionPreset:AVCaptureSessionPreset1280x720 ] == YES ) 
	{
	    device_hd_support = 1;
	    blog( "HD mode is supported by selected device\n" );
	}
	
	//Get device info:
	struct utsname systemInfo;
        uname( &systemInfo );
    	blog( "Machine: %s\n", systemInfo.machine );
        if( strstr( systemInfo.machine, "iPhone1" ) ||
	    strstr( systemInfo.machine, "iPhone2" ) ||
	    strstr( systemInfo.machine, "iPhone3" ) ||
	    strstr( systemInfo.machine, "iPod1" ) ||
	    strstr( systemInfo.machine, "iPod2" ) ||
	    strstr( systemInfo.machine, "iPod3" ) ||
	    strstr( systemInfo.machine, "iPod4" ) ||
	    strstr( systemInfo.machine, "iPad1" ) )
        {
    	    blog( "HD mode will be disabled on slow devices (< A5 CPU) ...\n" );
    	    device_hd_support = 0;
        }
	else
	{
	    blog( "CPU is ready for HD video processing.\n" );
	}
        
	// Create a device input with the device and add it to the session.
	AVCaptureDeviceInput* input = [ AVCaptureDeviceInput deviceInputWithDevice:device error:&error ];
	if( input == 0 )
	{
	    // Handling the error appropriately.
	    blog( "AVCapture: input device creation error\n" );
	    break;
	}
	[ session addInput:input ];
	
	// Create a VideoDataOutput and add it to the session
	AVCaptureVideoDataOutput* output = [ [ [ AVCaptureVideoDataOutput alloc ] init ] autorelease ];
	[ session addOutput:output ];

	if( device_hd_support && [ session canSetSessionPreset:AVCaptureSessionPreset1280x720 ] )
	{
	    session.sessionPreset = AVCaptureSessionPreset1280x720;
	    width = 1280;
	    height = 720;
	    blog( "AVCaptureSessionPreset1280x720\n" );
	}
	else
	{
	    session.sessionPreset = AVCaptureSessionPreset640x480;
	    width = 640;
	    height = 480;
	    blog( "AVCaptureSessionPreset640x480\n" );
	}
	
	// Configure your output.
	dispatch_queue_t queue = dispatch_queue_create( "myQueue", NULL );
	[ output setSampleBufferDelegate:self queue:queue ];
	dispatch_release( queue );
	
	// Specify the pixel format
	output.videoSettings = [ NSDictionary dictionaryWithObject: [ NSNumber numberWithInt:kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange ] forKey:(id)kCVPixelBufferPixelFormatTypeKey ];
	
	rv = 0;
	break;
    }
    return rv;
}

- (void)closeSession
{
    [ session release ];
    session = 0;
}

- (void)startCapture
{
    [ session startRunning ];
}

- (void)stopCapture
{
    [ session stopRunning ];
}

- (int)getWidth
{
    return width;
}

- (int)getHeight
{
    return height;
}

- (void)dealloc
{
    [ self closeSession ];
    [ super dealloc ];
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer( sampleBuffer );
    CVPixelBufferLockBaseAddress( imageBuffer, 0 );
    CVPlanarPixelBufferInfo_YCbCrBiPlanar* info = (CVPlanarPixelBufferInfo_YCbCrBiPlanar*)CVPixelBufferGetBaseAddress( imageBuffer );
    void* buffer = ( (char*)CVPixelBufferGetBaseAddress( imageBuffer ) + INT32_SWAP( info->componentInfoY.offset ) );
    size_t buffer_size = CVPixelBufferGetDataSize( imageBuffer ) - INT32_SWAP( info->componentInfoY.offset );
    bmutex_lock( &vid->callback_mutex );
    if( vid->callback_active && vid->capture_callback )
    {
	ticks_t t = time_ticks();
	if( t - vid->fps_time > time_ticks_per_second() )
	{
	    vid->fps = vid->fps_counter;
	    vid->fps_counter = 0;
	    vid->fps_time = t;
	}
	vid->fps_counter++;
	int connection_orientation = 0;
	switch( [ connection videoOrientation ] )
	{
	    case AVCaptureVideoOrientationPortrait: connection_orientation = 0; break;
	    case AVCaptureVideoOrientationPortraitUpsideDown: connection_orientation = 2; break;
	    case AVCaptureVideoOrientationLandscapeRight: connection_orientation = 3; break;
	    case AVCaptureVideoOrientationLandscapeLeft: connection_orientation = 1; break;
	}
	int ui_orientation = 0;
	switch( [ [ UIApplication sharedApplication] statusBarOrientation ] )
	{
	    case UIInterfaceOrientationPortrait: ui_orientation = 0; break;
	    case UIInterfaceOrientationPortraitUpsideDown: ui_orientation = 2; break;
	    case UIInterfaceOrientationLandscapeRight: ui_orientation = 3; break;
	    case UIInterfaceOrientationLandscapeLeft: ui_orientation = 1; break;
	}
	vid->orientation = ( ui_orientation - connection_orientation ) & 3;
	vid->capture_buffer = buffer;
	vid->capture_buffer_size = buffer_size;
	vid->capture_callback( vid );
    }
    bmutex_unlock( &vid->callback_mutex );
    CVPixelBufferUnlockBaseAddress( imageBuffer, 0 );
}

@end

int device_bvideo_global_init( uint flags )
{
    return 0;
}

int device_bvideo_global_deinit( uint flags )
{
    return 0;
}

int device_bvideo_open( bvideo_struct* vid, const utf8_char* name, uint flags )
{
    int rv = -1;
    
    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];
    
    while( 1 )
    {
	bmutex_init( &vid->callback_mutex, 0 );
	vid->orientation = 0;
	
	iOSAVCapture* cap = [ iOSAVCapture newCapture: name baseVideoObj:vid ];
	vid->ios_capture_obj = (void*)cap;
	if( cap == 0 ) break;
	[ cap startCapture ];
	blog( "AVCapture initialized\n" );
	
	rv = 0;
	break;
    }
    if( rv == - 1 )
    {
	bmutex_destroy( &vid->callback_mutex );
    }
    
    [ pool release ];
    
    return rv;
}

int device_bvideo_close( bvideo_struct* vid )
{
    int rv = -1;
    
    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];
    
    while( 1 )
    {
	iOSAVCapture* cap = (iOSAVCapture*)vid->ios_capture_obj;
	[ cap release ];
	vid->ios_capture_obj = 0;
	
	bmutex_destroy( &vid->callback_mutex );
	rv = 0;
	break;
    }
    
    [ pool release ];
    
    return rv;
}

int device_bvideo_start( bvideo_struct* vid )
{
    int rv = -1;
    while( 1 )
    {
	iOSAVCapture* cap = (iOSAVCapture*)vid->ios_capture_obj;
	if( cap == 0 ) break;
	bmutex_lock( &vid->callback_mutex );
	vid->callback_active = 1;
	vid->fps_time = time_ticks();
	vid->fps_counter = 0;
	bmutex_unlock( &vid->callback_mutex );
	//[ cap startCapture ];
	rv = 0;
	break;
    }
    return rv;
}

int device_bvideo_stop( bvideo_struct* vid )
{
    int rv = -1;
    while( 1 )
    {
	iOSAVCapture* cap = (iOSAVCapture*)vid->ios_capture_obj;
	if( cap == 0 ) break;
	bmutex_lock( &vid->callback_mutex );
	vid->callback_active = 0;
	bmutex_unlock( &vid->callback_mutex );
	//[ cap stopCapture ]; //HANGS :(
	rv = 0;
	break;
    }
    return rv;
}

int device_bvideo_set_props( bvideo_struct* vid, bvideo_prop* props )
{
    int rv = -1;
    while( 1 )
    {
	iOSAVCapture* cap = (iOSAVCapture*)vid->ios_capture_obj;
	if( cap == 0 ) break;
	rv = 0;
	break;
    }
    return rv;
}

int device_bvideo_get_props( bvideo_struct* vid, bvideo_prop* props )
{
    int rv = -1;
    while( 1 )
    {
	iOSAVCapture* cap = (iOSAVCapture*)vid->ios_capture_obj;
	if( cap == 0 ) break;
	if( props == 0 ) break;
	bool props_handled = 0;
	for( int i = 0; ; i++ )
	{
	    bvideo_prop* prop = &props[ i ];
	    if( prop->id == BVIDEO_PROP_NONE ) break;
	    switch( prop->id )
	    {
		case BVIDEO_PROP_FRAME_WIDTH_I:
		    prop->val.i = [ cap getWidth ];
		    props_handled = 1;
		    break;
		case BVIDEO_PROP_FRAME_HEIGHT_I:
		    prop->val.i = [ cap getHeight ];
		    props_handled = 1;
		    break;
		case BVIDEO_PROP_PIXEL_FORMAT_I:
		    prop->val.i = BVIDEO_PIXEL_FORMAT_YCbCr420_SEMIPLANAR;
		    props_handled = 1;
		    break;
		case BVIDEO_PROP_FPS_I:
		    prop->val.i = vid->fps;
		    props_handled = 1;
		    break;
	    }
	}
	if( props_handled == 0 ) break;
	rv = 0;
	break;
    }
    return rv;
}
