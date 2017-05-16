#define DEVICE_VIDEO_FUNCTIONS

#include <dlfcn.h>
#include <ctype.h>
#include "../../various/android/sundog_bridge.h"

enum {
    ANDROID_CAMERA_PROPERTY_FRAMEWIDTH = 0,
    ANDROID_CAMERA_PROPERTY_FRAMEHEIGHT = 1,
    ANDROID_CAMERA_PROPERTY_SUPPORTED_PREVIEW_SIZES_STRING = 2,
    ANDROID_CAMERA_PROPERTY_PREVIEW_FORMAT_STRING = 3,
    ANDROID_CAMERA_PROPERTY_FPS = 4,
    ANDROID_CAMERA_PROPERTY_EXPOSURE = 5,
    ANDROID_CAMERA_PROPERTY_FLASH_MODE = 101,
    ANDROID_CAMERA_PROPERTY_FOCUS_MODE = 102,
    ANDROID_CAMERA_PROPERTY_WHITE_BALANCE = 103,
    ANDROID_CAMERA_PROPERTY_ANTIBANDING = 104,
    ANDROID_CAMERA_PROPERTY_FOCAL_LENGTH = 105,
    ANDROID_CAMERA_PROPERTY_FOCUS_DISTANCE_NEAR = 106,
    ANDROID_CAMERA_PROPERTY_FOCUS_DISTANCE_OPTIMAL = 107,
    ANDROID_CAMERA_PROPERTY_FOCUS_DISTANCE_FAR = 108
};

enum {
    ANDROID_CAMERA_FLASH_MODE_AUTO = 0,
    ANDROID_CAMERA_FLASH_MODE_OFF,
    ANDROID_CAMERA_FLASH_MODE_ON,
    ANDROID_CAMERA_FLASH_MODE_RED_EYE,
    ANDROID_CAMERA_FLASH_MODE_TORCH,
    ANDROID_CAMERA_FLASH_MODES_NUM
};

enum {
    ANDROID_CAMERA_FOCUS_MODE_AUTO = 0,
    ANDROID_CAMERA_FOCUS_MODE_CONTINUOUS_VIDEO,
    ANDROID_CAMERA_FOCUS_MODE_EDOF,
    ANDROID_CAMERA_FOCUS_MODE_FIXED,
    ANDROID_CAMERA_FOCUS_MODE_INFINITY,
    ANDROID_CAMERA_FOCUS_MODES_NUM
};

enum {
    ANDROID_CAMERA_WHITE_BALANCE_AUTO = 0,
    ANDROID_CAMERA_WHITE_BALANCE_CLOUDY_DAYLIGHT,
    ANDROID_CAMERA_WHITE_BALANCE_DAYLIGHT,
    ANDROID_CAMERA_WHITE_BALANCE_FLUORESCENT,
    ANDROID_CAMERA_WHITE_BALANCE_INCANDESCENT,
    ANDROID_CAMERA_WHITE_BALANCE_SHADE,
    ANDROID_CAMERA_WHITE_BALANCE_TWILIGHT,
    ANDROID_CAMERA_WHITE_BALANCE_WARM_FLUORESCENT,
    ANDROID_CAMERA_WHITE_BALANCE_MODES_NUM
};

enum {
    ANDROID_CAMERA_ANTIBANDING_50HZ = 0,
    ANDROID_CAMERA_ANTIBANDING_60HZ,
    ANDROID_CAMERA_ANTIBANDING_AUTO,
    ANDROID_CAMERA_ANTIBANDING_OFF,
    ANDROID_CAMERA_ANTIBANDING_MODES_NUM
};

enum {
    ANDROID_CAMERA_FOCUS_DISTANCE_NEAR_INDEX = 0,
    ANDROID_CAMERA_FOCUS_DISTANCE_OPTIMAL_INDEX,
    ANDROID_CAMERA_FOCUS_DISTANCE_FAR_INDEX
};
    
typedef void* (*InitCameraConnectC)( void* cameraCallback, int cameraId, void* userData );
typedef void (*CloseCameraConnectC)( void** );
typedef double (*GetCameraPropertyC)( void* camera, int propIdx );
typedef void (*SetCameraPropertyC)( void* camera, int propIdx, double value );
typedef void (*ApplyCameraPropertiesC)( void** camera );
InitCameraConnectC 		pInitCameraConnectC = 0;
CloseCameraConnectC		pCloseCameraConnectC = 0;
GetCameraPropertyC		pGetCameraPropertyC = 0;
SetCameraPropertyC		pSetCameraPropertyC = 0;
ApplyCameraPropertiesC		pApplyCameraPropertiesC = 0;

void* g_native_camera_lib = 0;
bool g_native_camera_fns_loaded = 0;

bvideo_struct* g_vid = 0;

enum 
{
    ANDROID_CAM_NONE,
    ANDROID_CAM_NATIVE, //Based on the binary camera library from OpenCV http://opencv.org
    ANDROID_CAM_JAVA,
};
int g_android_camera_mode = ANDROID_CAM_NONE;

void* android_camera_get_fn( const utf8_char* fn_name )
{
    void* rv = 0;
    rv = dlsym( g_native_camera_lib, fn_name ); 
    if( rv == 0 ) 
    {
        blog( "%s not found in the camera library\n", fn_name );
        g_native_camera_fns_loaded = 0;
    }
    return rv;
}
    
utf8_char* android_camera_get_lib_path( void )
{
    Dl_info dl_info;
    if( dladdr( (void*)android_camera_get_lib_path, &dl_info ) != 0 )
    {
	blog( "Library name: %s\n", dl_info.dli_fname );
        
    	const utf8_char* libName = dl_info.dli_fname;
    	while( ( (*libName) == '/' ) || ( (*libName) == '.' ) ) libName++;
        
    	char lineBuf[ 2048 ];
    	FILE* file = fopen( "/proc/self/smaps", "rt" );
    	    
    	if( file )
    	{
    	    while( fgets( lineBuf, sizeof lineBuf, file ) != NULL )
    	    {
        	//verify that line ends with library name
            	int lineLength = strlen( lineBuf );
            	int libNameLength = strlen( libName );
                
            	//trim end
            	for( int i = lineLength - 1; i >= 0 && isspace( lineBuf[ i ] ); i-- )
            	{
            	    lineBuf[ i ] = 0;
            	    lineLength--;
            	}
	
            	if( strncmp( lineBuf + lineLength - libNameLength, libName, libNameLength ) != 0 )
            	{
            	    //the line does not contain the library name
            	    continue;
            	}
         
            	//extract path from smaps line
            	char* pathBegin = strchr( lineBuf, '/' );
            	if( pathBegin == 0 )
            	{
            	    blog( "Strange error: could not find path beginning in line \"%s\"\n", lineBuf );
            	    continue;
            	}
                
            	char* pathEnd = strrchr( pathBegin, '/' );
            	pathEnd[ 1 ] = 0;
                
            	blog( "Libraries folder found: %s\n", pathBegin );
                
            	fclose( file );
            	return bmem_strdup( pathBegin );
    	    }
    	    fclose( file );
    	    blog( "Could not find library path\n" );
    	}
    	else
    	{
    	    blog( "Could not read /proc/self/smaps\n" );
    	}
    }
    else
    {
	blog( "Could not get library name and base address\n" );
    }
    return 0;
}

int android_camera_flist_comp( const void* elem1, const void* elem2 )
{
    utf8_char* name1 = ((utf8_char**)elem1)[ 0 ];
    utf8_char* name2 = ((utf8_char**)elem2)[ 0 ];
    return bmem_strcmp( name1, name2 );
}
    
bool g_android_camera_first_frame = 0;
bool android_camera_next_frame_callback( void* buffer, size_t buffer_size, void* user_data )
{
    if( g_android_camera_first_frame == 0 )
    {
	blog( "First camera frame %d\n", (int)buffer_size );
	g_android_camera_first_frame = 1;
    }
    if( user_data == NULL ) return 1;
    bvideo_struct* vid = (bvideo_struct*)user_data;
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
	vid->capture_buffer = buffer;
	vid->capture_buffer_size = buffer_size;
	vid->capture_callback( vid );
    }
    bmutex_unlock( &vid->callback_mutex );
    return 1; //Return 0 to close the camera connection (in native mode)
}

extern "C" JNIEXPORT jint JNICALL Java_nightradio_androidlib_AndroidLib_camera_1frame_1callback( JNIEnv* je, jclass jc, jbyteArray data, jobject camera )
{
    jint rv = 0;
    size_t data_size = je->GetArrayLength( data );
    jbyte* data_ptr = je->GetByteArrayElements( data, NULL );
    android_camera_next_frame_callback( data_ptr, data_size, g_vid );
    je->ReleaseByteArrayElements( data, data_ptr, 0 );
    return rv;
}
    
int device_bvideo_global_init( uint flags )
{
    blog( "Android version (correct): %s\n", g_android_version_correct );
    
    g_android_camera_mode = ANDROID_CAM_NONE;
    //if( ( g_android_version_nums[ 0 ] == 4 && g_android_version_nums[ 1 ] >= 4 ) || g_android_version_nums[ 0 ] > 4 )
    if( 1 )
    {
	g_android_camera_mode = ANDROID_CAM_JAVA;
	blog( "Camera mode = JAVA\n" );
	return 0;
    }
    
    if( g_native_camera_lib ) return 0; //Already initialized
    utf8_char* lib_path = android_camera_get_lib_path();
    if( lib_path == 0 ) return -1;

    //Get list of libs:
    utf8_char** flist = (utf8_char**)bmem_new( sizeof( utf8_char* ) * 256 );
    int flist_count = 0;
    bmem_zero( flist );
    bfs_find_struct fs;
    bmem_set( &fs, sizeof( fs ), 0 );
    fs.start_dir = lib_path;
    bool first = 1;
    while( 1 )
    {
        int v;
        if( first )
        {
    	    v = bfs_find_first( &fs );
    	    first = 0;
	}
	else
	{
	    v = bfs_find_next( &fs );
	}
	if( v == 0 ) break;
	if( bmem_strstr( fs.name, "libnative_camera" ) )
	{
	    flist[ flist_count ] = bmem_strdup( fs.name );
	    flist_count++;
	}
    }
    bfs_find_close( &fs );
    if( flist_count )
    {
        qsort( flist, flist_count, sizeof( utf8_char* ), android_camera_flist_comp );
        blog( "Camera libs:\n" );
        for( int i = 0; i < flist_count; i++ )
        {
    	    blog( " * %s\n", flist[ i ] );
	}

	int i2 = -1;
	for( int i = 0; i < flist_count; i++ )
	{
	    utf8_char version[ 32 ];
	    version[ 0 ] = 0;
	    bmem_strcat( version, sizeof( version ), flist[ i ] + 18 );
	    utf8_char* ext = bmem_strstr( version, ".so" );
	    if( ext ) *ext = 0;
	    int cmp = bmem_strcmp( version, g_android_version_correct );
	    if( cmp <= 0 ) i2 = i;
	}
	while( i2 >= 0 )
	{
	    utf8_char* path = (utf8_char*)bmem_new( strlen( lib_path ) + strlen( flist[ i2 ] ) + 1 );
	    path[ 0 ] = 0;
	    bmem_strcat_resize( path, lib_path );
	    bmem_strcat_resize( path, flist[ i2 ] );
	    blog( "Trying to open %s\n", flist[ i2 ] );
	    g_native_camera_lib = dlopen( path, RTLD_NOW );
	    bmem_free( path );
	    if( g_native_camera_lib ) 
	    {
	        blog( "%s opened successfully\n", flist[ i2 ] );
	        break;
	    }
	    i2--; //Try to open old version
	}	
    }
    	
    //Free all:
    for( int i = 0; i < flist_count; i++ )
    {
        bmem_free( flist[ i ] );	
    }	
    bmem_free( flist );
    bmem_free( lib_path );

    if( g_native_camera_lib == 0 )
    {
        blog( "Can't open camera library\n" );
        return -1;
    }
    g_native_camera_fns_loaded = 1;
    pInitCameraConnectC = (InitCameraConnectC)android_camera_get_fn( "initCameraConnectC" );
    pCloseCameraConnectC = (CloseCameraConnectC)android_camera_get_fn( "closeCameraConnectC" );
    pGetCameraPropertyC = (GetCameraPropertyC)android_camera_get_fn( "getCameraPropertyC" );
    pSetCameraPropertyC = (SetCameraPropertyC)android_camera_get_fn( "setCameraPropertyC" );
    pApplyCameraPropertiesC = (ApplyCameraPropertiesC)android_camera_get_fn( "applyCameraPropertiesC" );
    if( g_native_camera_fns_loaded == 0 )
    {
	return -1;
    }
    
    g_android_camera_mode = ANDROID_CAM_NATIVE;
    blog( "Camera mode = NATIVE\n" );

    return 0;
}
                            	
int device_bvideo_global_deinit( uint flags )
{
    if( g_android_camera_mode == ANDROID_CAM_NATIVE )
    {	
	if( g_native_camera_lib == 0 ) return 0;
	dlclose( g_native_camera_lib );
	g_native_camera_lib = 0;
    }
    return 0;
}

int device_bvideo_open( bvideo_struct* vid, const utf8_char* name, uint flags )
{
    int rv = -1;
    g_vid = vid;
    vid->cam_index = -9999;
    vid->orientation = 0;
    bmutex_init( &vid->callback_mutex, 0 );
    if( g_android_camera_mode == ANDROID_CAM_NATIVE )
    {
	while( 1 )
	{
	    if( bmem_strcmp( name, "camera" ) == 0 ) 
	    {
		vid->cam_index = profile_get_int_value( KEY_CAMERA, -1, 0 );
		if( vid->cam_index != -1 )
		{
		    vid->cam_index = 99 - vid->cam_index;
		}
	    }
	    if( bmem_strcmp( name, "camera_back" ) == 0 ) vid->cam_index = 99;
	    if( bmem_strcmp( name, "camera_front" ) == 0 ) vid->cam_index = 98;
	    if( vid->cam_index == -9999 ) break;
	    if( g_native_camera_lib == 0 ) break;	
	    vid->android_cam = pInitCameraConnectC( (void*)android_camera_next_frame_callback, vid->cam_index, (void*)vid );
	    if( vid->android_cam )
	    {
		rv = 0;
	    }
	    break;
	}
    }
    if( g_android_camera_mode == ANDROID_CAM_JAVA )
    {
	while( 1 )
	{
	    if( bmem_strcmp( name, "camera" ) == 0 ) 
	    {
		//Default:
		vid->cam_index = profile_get_int_value( KEY_CAMERA, 0, 0 );
	    }
	    if( bmem_strcmp( name, "camera_back" ) == 0 ) vid->cam_index = 0;
	    if( bmem_strcmp( name, "camera_front" ) == 0 ) vid->cam_index = 1;
	    if( vid->cam_index == -9999 ) break;
	    rv = android_sundog_open_camera( vid->cam_index );
	    break;
	}
    }
    if( rv == -1 )
    {
	blog( "device_bvideo_open() error %d\n", rv );
	bmutex_destroy( &vid->callback_mutex );
    }
    else
    {
	blog( "device_bvideo_open() successful\n" );
    }
    return rv;
}

int device_bvideo_close( bvideo_struct* vid )
{
    int rv = -1;
    if( g_android_camera_mode == ANDROID_CAM_NATIVE )
    {
	while( 1 )
	{
	    if( g_native_camera_lib == 0 ) break;
	    pCloseCameraConnectC( &vid->android_cam );
	    rv = 0;
	    break;
	}
    }
    if( g_android_camera_mode == ANDROID_CAM_JAVA )
    {
	rv = android_sundog_close_camera();
    }
    bmutex_destroy( &vid->callback_mutex );
    g_vid = 0;
    if( rv == -1 )
    {
	blog( "device_bvideo_close() error %d\n", rv );
    }
    else
    {
	blog( "device_bvideo_close() successful\n" );
    }
    return rv;
}

int device_bvideo_start( bvideo_struct* vid )
{
    int rv = -1;
    if( g_android_camera_mode == ANDROID_CAM_NATIVE )
    {
	while( 1 )
	{
	    if( g_native_camera_lib == 0 ) break;
	    rv = 0;
	    break;
	}
    }
    if( g_android_camera_mode == ANDROID_CAM_JAVA )
    {
	rv = 0;
    }
    if( rv == 0 )
    {
	bmutex_lock( &vid->callback_mutex );
	vid->callback_active = 1;
	vid->fps_time = time_ticks();
        vid->fps_counter = 0;
	bmutex_unlock( &vid->callback_mutex );
    }
    return rv;
}

int device_bvideo_stop( bvideo_struct* vid )
{
    int rv = -1;
    if( g_android_camera_mode == ANDROID_CAM_NATIVE )
    {
	while( 1 )
	{
	    if( g_native_camera_lib == 0 ) break;
	    rv = 0;
	    break;
	}
    }
    if( g_android_camera_mode == ANDROID_CAM_JAVA )
    {
	rv = 0;
    }
    if( rv == 0 )
    {
	bmutex_lock( &vid->callback_mutex );
	vid->callback_active = 0;
	bmutex_unlock( &vid->callback_mutex );
    }
    return rv;
}

int device_bvideo_set_props( bvideo_struct* vid, bvideo_prop* props )
{
    int rv = -1;
    if( g_android_camera_mode == ANDROID_CAM_NATIVE )
    {
        while( 1 )
	{
	    if( g_native_camera_lib == 0 ) break;
	    if( props == 0 ) break;
	    bool props_handled = 0;
	    bool apply_request = 0;
	    bool camera_reinit_request = 0;
	    for( int i = 0; ; i++ )
	    {
		bvideo_prop* prop = &props[ i ];
		if( prop->id == BVIDEO_PROP_NONE ) break;
		int device_prop = -1;
		double device_val = 0;
		switch( prop->id )
		{
		    case BVIDEO_PROP_FRAME_WIDTH_I: device_prop = ANDROID_CAMERA_PROPERTY_FRAMEWIDTH; device_val = prop->val.i; apply_request = 1; break;
		    case BVIDEO_PROP_FRAME_HEIGHT_I: device_prop = ANDROID_CAMERA_PROPERTY_FRAMEHEIGHT; device_val = prop->val.i; apply_request = 1; break;
		    case BVIDEO_PROP_FOCUS_MODE_I: 
			device_prop = ANDROID_CAMERA_PROPERTY_FOCUS_MODE;
			switch( prop->val.i )
			{
			    case BVIDEO_FOCUS_MODE_AUTO: device_val = ANDROID_CAMERA_FOCUS_MODE_AUTO; camera_reinit_request = 1; break;
			    case BVIDEO_FOCUS_MODE_CONTINUOUS: device_val = ANDROID_CAMERA_FOCUS_MODE_CONTINUOUS_VIDEO; break;
			    case BVIDEO_FOCUS_MODE_FIXED: device_val = ANDROID_CAMERA_FOCUS_MODE_FIXED; break;
			    case BVIDEO_FOCUS_MODE_INFINITY: device_val = ANDROID_CAMERA_FOCUS_MODE_INFINITY; break;
			    default: break;
			}
			break;
		    default: break;
		}
		if( device_prop == -1 )
		{
		    blog( "Can't set camera property %d\n", (int)prop->id );
		}
		else
		{
		    pSetCameraPropertyC( vid->android_cam, device_prop, device_val );
		    props_handled = 1;
		}
	    }
	    if( props_handled ) rv = 0;
	    if( apply_request ) pApplyCameraPropertiesC( &vid->android_cam );
	    if( camera_reinit_request )
	    {
		pCloseCameraConnectC( &vid->android_cam );
		vid->android_cam = pInitCameraConnectC( (void*)android_camera_next_frame_callback, vid->cam_index, (void*)vid );
	    }
	    break;
	}
    }
    if( g_android_camera_mode == ANDROID_CAM_JAVA )
    {
        while( 1 )
	{
	    if( props == 0 ) break;
	    for( int i = 0; ; i++ )
	    {
		bvideo_prop* prop = &props[ i ];
		if( prop->id == BVIDEO_PROP_NONE ) break;
		int device_prop = -1;
		switch( prop->id )
		{
		    case BVIDEO_PROP_FOCUS_MODE_I: 
			{
			    int mode = -1;
			    switch( prop->val.i )
			    {
				case BVIDEO_FOCUS_MODE_AUTO: mode = 0; break;
				case BVIDEO_FOCUS_MODE_CONTINUOUS: mode = 1; break;
				case BVIDEO_FOCUS_MODE_FIXED: mode = 2; break;
				case BVIDEO_FOCUS_MODE_INFINITY: mode = 3; break;
			    }
			    if( mode != -1 )
			    {
				android_sundog_set_camera_focus_mode( mode );
			    }
			}
			break;
		    default: break;
		}
	    }
	    rv = 0;
	    break;
	}
    }
    return rv;
}

int device_bvideo_get_props( bvideo_struct* vid, bvideo_prop* props )
{
    int rv = -1;
    if( g_android_camera_mode == ANDROID_CAM_NATIVE )
    {
	while( 1 )
	{
	    if( g_native_camera_lib == 0 ) break;
	    if( props == 0 ) break;
	    bool props_handled = 0;
	    for( int i = 0; ; i++ )
	    {
		bvideo_prop* prop = &props[ i ];
		if( prop->id == BVIDEO_PROP_NONE ) break;
		int device_prop = -1;
		switch( prop->id )
		{
		    case BVIDEO_PROP_FRAME_WIDTH_I: device_prop = ANDROID_CAMERA_PROPERTY_FRAMEWIDTH; break;
		    case BVIDEO_PROP_FRAME_HEIGHT_I: device_prop = ANDROID_CAMERA_PROPERTY_FRAMEHEIGHT; break;
		    case BVIDEO_PROP_PIXEL_FORMAT_I: device_prop = ANDROID_CAMERA_PROPERTY_PREVIEW_FORMAT_STRING; break;
		    case BVIDEO_PROP_FPS_I: device_prop = ANDROID_CAMERA_PROPERTY_FPS; break;
		    case BVIDEO_PROP_FOCUS_MODE_I: device_prop = ANDROID_CAMERA_PROPERTY_FOCUS_MODE; break;
		    case BVIDEO_PROP_SUPPORTED_PREVIEW_SIZES_P: device_prop = ANDROID_CAMERA_PROPERTY_SUPPORTED_PREVIEW_SIZES_STRING; break;
		    default: break;
		}
		if( device_prop == -1 )
		{
		    blog( "Can't get camera property %d\n", (int)prop->id );
		}
		else
		{
		    volatile double device_val = pGetCameraPropertyC( vid->android_cam, device_prop );
		    switch( prop->id )
		    {
			case BVIDEO_PROP_FRAME_WIDTH_I: prop->val.i = (int64)device_val; break;
			case BVIDEO_PROP_FRAME_HEIGHT_I: prop->val.i = (int64)device_val; break;
			case BVIDEO_PROP_PIXEL_FORMAT_I: 
			    {
				volatile void** pprop = (volatile void**)&device_val;
    				volatile utf8_char* str = (volatile utf8_char*)pprop[ 0 ];
    				while( 1 )
    				{
    				    if( bmem_strcmp( (const char*)str, "yuv422sp" ) == 0 ) { prop->val.i = BVIDEO_PIXEL_FORMAT_YCbCr422_SEMIPLANAR; break; }
    				    if( bmem_strcmp( (const char*)str, "yuv422i" ) == 0 ) { prop->val.i = BVIDEO_PIXEL_FORMAT_YCbCr422; break; }
    				    if( bmem_strcmp( (const char*)str, "yuv420sp" ) == 0 ) { prop->val.i = BVIDEO_PIXEL_FORMAT_YCrCb420_SEMIPLANAR; break; }
    				    if( bmem_strcmp( (const char*)str, "yvu420sp" ) == 0 ) { prop->val.i = BVIDEO_PIXEL_FORMAT_YCbCr420_SEMIPLANAR; break; }
    				    if( bmem_strcmp( (const char*)str, "rgb565" ) == 0 ) { prop->val.i = BVIDEO_PIXEL_FORMAT_RGB565; break; }
    				    if( bmem_strcmp( (const char*)str, "jpeg" ) == 0 ) { prop->val.i = BVIDEO_PIXEL_FORMAT_JPEG; break; }
    				    break;
    				}
			    }
			    break;
			case BVIDEO_PROP_FPS_I: prop->val.i = vid->fps /*(int64)device_val*/ ; break;
			case BVIDEO_PROP_FOCUS_MODE_I: 
			    switch( (int)device_val )
			    {
			        case ANDROID_CAMERA_FOCUS_MODE_CONTINUOUS_VIDEO: prop->val.i = BVIDEO_FOCUS_MODE_CONTINUOUS; break;
				case ANDROID_CAMERA_FOCUS_MODE_FIXED: prop->val.i = BVIDEO_FOCUS_MODE_FIXED; break;
				case ANDROID_CAMERA_FOCUS_MODE_INFINITY: prop->val.i = BVIDEO_FOCUS_MODE_INFINITY; break;
				default: prop->val.i = BVIDEO_FOCUS_MODE_AUTO; break;
			    }
			    break;
			case BVIDEO_PROP_SUPPORTED_PREVIEW_SIZES_P: 
			    {
				volatile void** pprop = (volatile void**)&device_val;
				volatile utf8_char* str = (volatile utf8_char*)pprop[ 0 ];
    				prop->val.p = (void*)str;
    			    }
			    break;
			default: break;
		    }
		    props_handled = 1;
		}
	    }
	    if( props_handled == 0 ) break;
	    rv = 0;
	    break;
	}
    }
    if( g_android_camera_mode == ANDROID_CAM_JAVA )
    {
	while( 1 )
	{
	    if( props == 0 ) break;
	    for( int i = 0; ; i++ )
	    {
		bvideo_prop* prop = &props[ i ];
		if( prop->id == BVIDEO_PROP_NONE ) break;
		switch( prop->id )
		{
		    case BVIDEO_PROP_FRAME_WIDTH_I: prop->val.i = android_sundog_get_camera_width(); break;
		    case BVIDEO_PROP_FRAME_HEIGHT_I: prop->val.i = android_sundog_get_camera_height(); break;
		    case BVIDEO_PROP_PIXEL_FORMAT_I: prop->val.i = BVIDEO_PIXEL_FORMAT_YCrCb420_SEMIPLANAR; break;
		    case BVIDEO_PROP_FPS_I: prop->val.i = vid->fps; break;
		    case BVIDEO_PROP_FOCUS_MODE_I: 
			{
			    int focus_mode = android_sundog_get_camera_focus_mode();
			    switch( focus_mode )
			    {
				case 1: prop->val.i = BVIDEO_FOCUS_MODE_CONTINUOUS; break;
				case 2: prop->val.i = BVIDEO_FOCUS_MODE_FIXED; break;
				case 3: prop->val.i = BVIDEO_FOCUS_MODE_INFINITY; break;
				default: prop->val.i = BVIDEO_FOCUS_MODE_AUTO; break;
			    }
			}
			break;
		    case BVIDEO_PROP_SUPPORTED_PREVIEW_SIZES_P: break;
		    default: break;
		}
	    }
	    rv = 0;
	    break;
	}
    }
    return rv;
}
