#include "core/core.h"
#include "sundog_bridge.h"

utf8_char* g_android_cache_int_path = 0;
utf8_char* g_android_cache_ext_path = 0;
utf8_char* g_android_files_int_path = 0;
utf8_char* g_android_files_ext_path = 0;
utf8_char* g_android_dcim_ext_path = 0;
utf8_char* g_android_pics_ext_path = 0;
utf8_char* g_android_movies_ext_path = 0;
utf8_char* g_android_version = 0;
utf8_char g_android_version_correct[ 16 ] = { 0 };
int g_android_version_nums[ 8 ] = { 0 };
utf8_char* g_android_lang = 0;
utf8_char* g_intent_file_name = 0;
bool g_glcapture_supported = false;

#ifndef NOMAIN

int engine_init_display( void );
void engine_term_display( void );

extern void android_sundog_key_event( bool down, uint16 key, uint16 scancode, uint16 flags, window_manager* wm );

int g_android_sundog_screen_xsize = 0;
int g_android_sundog_screen_ysize = 0;
int g_android_sundog_screen_ppi = 0;
bool g_android_sundog_gl_buffer_preserved = 0;
void* g_android_sundog_framebuffer = 0;
volatile int g_android_sundog_resize_request = 0;
volatile int g_android_sundog_resize_request_rp = 0;
EGLDisplay g_android_sundog_display;
EGLSurface g_android_sundog_surface;
EGLContext g_android_sundog_context;

sundog_bridge_struct g_sundog_bridge;

volatile bool g_eventloop_stop_request = 0;
volatile bool g_eventloop_stop_answer = 0;

int g_android_camera_status = 0;
uint g_android_camera_texture = 0;

static int option_exists( const utf8_char* name )
{
    int rv = 0;
    utf8_char* ts = (utf8_char*)malloc( strlen( g_android_files_ext_path ) + strlen( name ) + 64 );
    if( ts )
    {
	ts[ 0 ] = 0;
	strcat( ts, g_android_files_ext_path );
	strcat( ts, name );
	FILE* f = fopen( ts, "rb" );
	if( f == 0 )
	{
	    strcat( ts, ".txt" );
	    f = fopen( ts, "rb" );
	}
	if( f )
	{
	    rv = fgetc( f );
	    if( rv < 0 ) rv = 1;
	    fclose( f );
	}
	free( ts );
    }
    return rv;
}

JNIEnv* java_open( void )
{
    JNIEnv* jni;
    g_sundog_bridge.app->activity->vm->AttachCurrentThread( &jni, NULL );
    return jni;
}

void java_close( void )
{
    g_sundog_bridge.app->activity->vm->DetachCurrentThread();
}

jclass java_find_class( JNIEnv* jni, const utf8_char* class_name )
{
    jclass class1 = jni->FindClass( "android/app/NativeActivity" );
    if( class1 == 0 )
    {
        LOGW( "NativeActivity class not found!" );
    }
    else
    {
        jmethodID getClassLoader = jni->GetMethodID( class1, "getClassLoader", "()Ljava/lang/ClassLoader;" );
        jobject class_loader_obj = jni->CallObjectMethod( g_sundog_bridge.app->activity->clazz, getClassLoader );
        jclass class_loader = jni->FindClass( "java/lang/ClassLoader" );
        jmethodID loadClass = jni->GetMethodID( class_loader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;" );
        jstring class_name_str = jni->NewStringUTF( class_name );
        jclass class2 = (jclass)jni->CallObjectMethod( class_loader_obj, loadClass, class_name_str );
        if( class2 == 0 )
        {
    	    LOGW( "%s class not found!", class_name );
        }
        return class2;
    }
    return 0;
}

int java_call_i_s( const utf8_char* method_name, const utf8_char* str_par )
{
    int rv = -1;
    
    JNIEnv* jni = java_open();
    jclass c = java_find_class( jni, (const utf8_char*)g_sundog_bridge.user_class_name );
    if( c )
    {
        jmethodID m = jni->GetMethodID( c, method_name, "(Ljava/lang/String;)I" );
	if( m )
    	{
	    rv = jni->CallIntMethod( g_sundog_bridge.app->activity->clazz, m, jni->NewStringUTF( str_par ) );
    	}
    }
    java_close();
    
    return rv;
}

int java_call_i_v( const utf8_char* method_name )
{
    int rv = -1;
    
    JNIEnv* jni = java_open();
    jclass c = java_find_class( jni, (const utf8_char*)g_sundog_bridge.user_class_name );
    if( c )
    {
        jmethodID m = jni->GetMethodID( c, method_name, "()I" );
	if( m )
    	{
	    rv = jni->CallIntMethod( g_sundog_bridge.app->activity->clazz, m );
    	}
    }
    java_close();
    
    return rv;
}

int java_call_i_i( const utf8_char* method_name, int int_par )
{
    int rv = -1;
    
    JNIEnv* jni = java_open();
    jclass c = java_find_class( jni, (const utf8_char*)g_sundog_bridge.user_class_name );
    if( c )
    {
        jmethodID m = jni->GetMethodID( c, method_name, "(I)I" );
	if( m )
    	{
	    rv = jni->CallIntMethod( g_sundog_bridge.app->activity->clazz, m, int_par );
    	}
    }
    java_close();
    
    return rv;
}

int java_call2_i_s( const utf8_char* method_name, const utf8_char* str_par )
{
    int rv = -1;
    
    JNIEnv* jni = java_open();
    jclass c = java_find_class( jni, (const utf8_char*)g_sundog_bridge.user_class_name );
    if( c )
    {
	jfieldID fid = jni->GetFieldID( c, "lib", "Lnightradio/androidlib/AndroidLib;" );
	if( fid )
	{
	    jobject lib_obj = jni->GetObjectField( g_sundog_bridge.app->activity->clazz, fid );
	    if( lib_obj )
	    {
		jclass lib_class = java_find_class( jni, "nightradio/androidlib/AndroidLib" );
		if( lib_class )
		{
	    	    jmethodID m = jni->GetMethodID( lib_class, method_name, "(Ljava/lang/String;)I" );
		    if( m )
		    {
			rv = jni->CallIntMethod( lib_obj, m, jni->NewStringUTF( str_par ) );
		    }
		}
	    }
	    else
	    {
		LOGW( "AndroidLib object not found" );
	    }
	}
	else
	{
	    LOGW( "AndroidLib field not found" );
	}
    }
    java_close();
    
    return rv;
}

int java_call2_i_i( const utf8_char* method_name, int pars_count, int int_par1, int int_par2, int int_par3, int int_par4 )
{
    int rv = -1;
    
    JNIEnv* jni = java_open();
    jclass c = java_find_class( jni, (const utf8_char*)g_sundog_bridge.user_class_name );
    if( c )
    {
	jfieldID fid = jni->GetFieldID( c, "lib", "Lnightradio/androidlib/AndroidLib;" );
	if( fid )
	{
	    jobject lib_obj = jni->GetObjectField( g_sundog_bridge.app->activity->clazz, fid );
	    if( lib_obj )
	    {
		jclass lib_class = java_find_class( jni, "nightradio/androidlib/AndroidLib" );
		if( lib_class )
		{
		    jmethodID m;
		    switch( pars_count )
		    {
	    		case 0: m = jni->GetMethodID( lib_class, method_name, "()I" ); if( m )	rv = jni->CallIntMethod( lib_obj, m ); break;
	    		case 1: m = jni->GetMethodID( lib_class, method_name, "(I)I" ); if( m )	rv = jni->CallIntMethod( lib_obj, m, int_par1 ); break;
	    		case 2: m = jni->GetMethodID( lib_class, method_name, "(II)I" ); if( m ) rv = jni->CallIntMethod( lib_obj, m, int_par1, int_par2 ); break;
	    		case 3: m = jni->GetMethodID( lib_class, method_name, "(III)I" ); if( m ) rv = jni->CallIntMethod( lib_obj, m, int_par1, int_par2, int_par3 ); break;
	    		case 4: m = jni->GetMethodID( lib_class, method_name, "(IIII)I" ); if( m ) rv = jni->CallIntMethod( lib_obj, m, int_par1, int_par2, int_par3, int_par4 ); break;
		    }
		}
	    }
	    else
	    {
		LOGW( "AndroidLib object not found" );
	    }
	}
	else
	{
	    LOGW( "AndroidLib field not found" );
	}
    }
    java_close();
    
    return rv;
}

utf8_char* java_call2_s_v( const utf8_char* method_name )
{
    utf8_char* rv = 0;
    
    JNIEnv* jni = java_open();
    jclass c = java_find_class( jni, (const utf8_char*)g_sundog_bridge.user_class_name );
    if( c )
    {
	jfieldID fid = jni->GetFieldID( c, "lib", "Lnightradio/androidlib/AndroidLib;" );
	if( fid )
	{
	    jobject lib_obj = jni->GetObjectField( g_sundog_bridge.app->activity->clazz, fid );
	    if( lib_obj )
	    {
		jclass lib_class = java_find_class( jni, "nightradio/androidlib/AndroidLib" );
		if( lib_class )
		{
	    	    jmethodID m = jni->GetMethodID( lib_class, method_name, "()Ljava/lang/String;" );
		    if( m )
		    {
			jstring s = (jstring)jni->CallObjectMethod( lib_obj, m );
            		if( s )
            		{
                	    const utf8_char* str = jni->GetStringUTFChars( s, 0 );
                	    rv = (utf8_char*)malloc( strlen( str ) + 4 );
                	    rv[ 0 ] = 0;
                	    strcat( rv, str );
                	    jni->ReleaseStringUTFChars( s, str );
                	}
		    }
		}
	    }
	    else
	    {
		LOGW( "AndroidLib object not found" );
	    }
	}
	else
	{
	    LOGW( "AndroidLib field not found" );
	}
    }
    java_close();
    
    return rv;
}

utf8_char* java_call2_s_s( const utf8_char* method_name, const utf8_char* str_par )
{
    utf8_char* rv = 0;
    
    JNIEnv* jni = java_open();
    jclass c = java_find_class( jni, (const utf8_char*)g_sundog_bridge.user_class_name );
    if( c )
    {
	jfieldID fid = jni->GetFieldID( c, "lib", "Lnightradio/androidlib/AndroidLib;" );
	if( fid )
	{
	    jobject lib_obj = jni->GetObjectField( g_sundog_bridge.app->activity->clazz, fid );
	    if( lib_obj )
	    {
		jclass lib_class = java_find_class( jni, "nightradio/androidlib/AndroidLib" );
		if( lib_class )
		{
	    	    jmethodID m = jni->GetMethodID( lib_class, method_name, "(Ljava/lang/String;)Ljava/lang/String;" );
		    if( m )
		    {
			jstring s = (jstring)jni->CallObjectMethod( lib_obj, m, jni->NewStringUTF( str_par ) );
            		if( s )
            		{
                	    const utf8_char* str = jni->GetStringUTFChars( s, 0 );
                	    rv = (utf8_char*)malloc( strlen( str ) + 4 );
                	    rv[ 0 ] = 0;
                	    strcat( rv, str );
                	    jni->ReleaseStringUTFChars( s, str );
                	}
		    }
		}
	    }
	    else
	    {
		LOGW( "AndroidLib object not found" );
	    }
	}
	else
	{
	    LOGW( "AndroidLib field not found" );
	}
    }
    java_close();
    
    return rv;
}

void* sundog_thread( void* arg )
{
    sundog_info_struct* sd = (sundog_info_struct*)arg;
    
    LOGI( "sundog_thread() ..." );

    if( engine_init_display() ) exit( -1 );
    ANativeActivity_setWindowFlags( g_sundog_bridge.app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0 );

    LOGI( "sundog_main() ..." );

    LOGI( "pause 100ms ..." );
    time_sleep( 100 ); //Small pause due to strange NativeActivity behaviour (trying to close window just after the first start)
    if( sd->exit_request == 0 )
    {
	sundog_main( &sd->s, true );
    }

    LOGI( "sundog_main() finished" );
    
    engine_term_display();
    
    if( g_android_camera_status )
	android_sundog_close_camera();
    
    if( sd->exit_request == 0 )
    {
	//No any exit requests from the OS. SunDog just closed.
	//So tell the OS to close this app:
	LOGI( "force close..." );
	ANativeActivity_finish( g_sundog_bridge.app->activity );
    }

    LOGI( "sundog_thread() finished" );

    sd->pth_finished = 1;
    pthread_exit( NULL );
}

void android_sundog_init( void )
{
    int err;
    sundog_info_struct* sd = &g_sundog_bridge.sd;

    LOGI( "android_sundog_init() ..." );

    if( sd->initialized == 1 ) return;
    
    g_android_sundog_resize_request = 0;
    g_android_sundog_resize_request_rp = 0;
    
    g_eventloop_stop_request = 0;
    g_eventloop_stop_answer = 0;

    memset( sd, 0, sizeof( sundog_info_struct ) );
 
    err = pthread_create( &sd->pth, 0, &sundog_thread, sd );
    if( err == 0 )
    {
	//The pthread_detach() function marks the thread identified by thread as
	//detached. When a detached thread terminates, its resources are 
	//automatically released back to the system.
	err = pthread_detach( sd->pth );
	if( err != 0 )
	{
	    LOGW( "android_sundog_init(): pthread_detach error %d", err );
	    return;
	}
    }
    else
    {
	LOGW( "android_sundog_init(): pthread_create error %d", err );
	return;
    }
    
    LOGI( "android_sundog_init(): done" );
    
    sd->initialized = 1;
}

void android_sundog_deinit( void )
{
    int err;
    sundog_info_struct* sd = &g_sundog_bridge.sd;
    
    if( sd->initialized == 0 ) return;

    LOGI( "android_sundog_deinit() ..." );

    //Stop the thread:
    sd->exit_request = 1;
    win_exit_request( sd->s.wm );
    g_eventloop_stop_request = 0; //Resume SunDog eventloop if it was in Pause state
    int timeout_counter = 1000 * 6; //Timeout in milliseconds
    while( timeout_counter > 0 )
    {
	win_exit_request( sd->s.wm );
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 1000000 * 20; //20 milliseconds
	if( sd->pth_finished ) break;
	nanosleep( &delay, NULL ); //Sleep for delay time
	timeout_counter -= 20;
    }
    if( timeout_counter <= 0 )
    {
	LOGW( "android_sundog_deinit(): thread timeout" );
    }
    
    LOGI( "android_sundog_deinit(): done" );

    sd->initialized = 0;
}

void android_sundog_screen_redraw( void )
{
    eglSwapBuffers( g_sundog_bridge.display, g_sundog_bridge.surface );
}

void android_sundog_event_handler( void )
{
    HANDLE_THREAD_EVENTS;
    if( g_eventloop_stop_request )
    {
        g_eventloop_stop_answer = 1;
        while( g_eventloop_stop_request )
        {
            time_sleep( 100 );
        }
        g_eventloop_stop_answer = 0;
    }
}

int android_sundog_copy_resources( void )
{
    return java_call_i_v( "CopyResources" );
}

void android_sundog_open_url( const utf8_char* url_text )
{
    java_call2_i_s( "OpenURL", url_text );
}

void android_sundog_scan_media( const utf8_char* path )
{
    java_call2_i_s( "ScanMedia", path );
}

int android_sundog_open_camera( int cam_id )
{
    int rv = java_call2_i_i( "OpenCamera", 1, cam_id, 0, 0, 0 );
    if( rv == 0 )
    {
	g_android_camera_status = 1;
    }
    else
    {
	android_sundog_close_camera();
    }
    return rv;
}

int android_sundog_close_camera( void )
{
    int rv = java_call2_i_i( "CloseCamera", 0, 0, 0, 0, 0 );
    if( rv == 0 )
    {
	g_android_camera_status = 0;
    }
    return rv;
}

int android_sundog_get_camera_width( void )
{
    return java_call2_i_i( "GetCameraWidth", 0, 0, 0, 0, 0 );
}

int android_sundog_get_camera_height( void )
{
    return java_call2_i_i( "GetCameraHeight", 0, 0, 0, 0, 0 );
}

int android_sundog_get_camera_focus_mode( void )
{
    return java_call2_i_i( "GetCameraFocusMode", 0, 0, 0, 0, 0 );
}

int android_sundog_set_camera_focus_mode( int mode )
{
    return java_call2_i_i( "SetCameraFocusMode", 1, mode, 0, 0, 0 );
}

int android_sundog_glcapture_start( int width, int height, int fps, int bitrate_kb )
{
    if( g_glcapture_supported )
	return java_call2_i_i( "GLCaptureStart", 4, width, height, fps, bitrate_kb );
    else
	return -1;
}

int android_sundog_glcapture_frame_begin( void )
{
    if( g_glcapture_supported )
	return java_call2_i_i( "GLCaptureFrameBegin", 0, 0, 0, 0, 0 );
    else
	return -1;
}

int android_sundog_glcapture_frame_end( void )
{
    if( g_glcapture_supported )
	return java_call2_i_i( "GLCaptureFrameEnd", 0, 0, 0, 0, 0 );
    else
	return -1;
}

int android_sundog_glcapture_stop( void )
{
    if( g_glcapture_supported )
	return java_call2_i_i( "GLCaptureStop", 0, 0, 0, 0, 0 );
    else
	return 0;
}

int android_sundog_glcapture_encode( const utf8_char* name )
{
    if( g_glcapture_supported )
	return java_call2_i_s( "GLCaptureEncode", name );
    else
	return 0;
}

// Initialize an EGL context for the current display.
int engine_init_display( void ) 
{
    // initialize OpenGL ES and EGL
    
    bool buf_preserved;
#ifdef GLNORETAIN
    buf_preserved = 0;
#else
    buf_preserved = 1;
#endif
    if( option_exists( "option_glnoretain" ) )
    {
        LOGI( "option_glnoretain found. OpenGL Buffer Preserved = NO" );
        buf_preserved = 0;	
    }

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs1[] = 
    {
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_SWAP_BEHAVIOR_PRESERVED_BIT,
	EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    const EGLint attribs2[] = 
    {
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    EGLint w, h, dummy, format;
    EGLint numConfigs = 0;
    EGLint num = 0;
    EGLConfig config[ 32 ];
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;

    g_sundog_bridge.display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
    if( g_sundog_bridge.display == EGL_NO_DISPLAY )
    {
	LOGW( "eglGetDisplay() error %d", eglGetError() );
	return -1;
    }

    if( eglInitialize( g_sundog_bridge.display, 0, 0 ) != EGL_TRUE )
    {
	LOGW( "eglInitialize() error %d", eglGetError() );
    	return -1;
    }

    if( buf_preserved )
    {
	if( eglChooseConfig( g_sundog_bridge.display, attribs1, &config[ numConfigs ], 1, &num ) != EGL_TRUE )
	{
    	    LOGW( "eglChooseConfig() error %d", eglGetError() );
	}
	LOGI( "EGL Configs with EGL_SWAP_BEHAVIOR_PRESERVED_BIT: %d", num );
	numConfigs += num;
    }
    num = 0;
    if( eglChooseConfig( g_sundog_bridge.display, attribs2, &config[ numConfigs ], 1, &num ) != EGL_TRUE )
    {
	LOGW( "eglChooseConfig() error %d", eglGetError() );
    }    
    LOGI( "EGL Configs without EGL_SWAP_BEHAVIOR_PRESERVED_BIT: %d", num );
    numConfigs += num;
    
    if( numConfigs == 0 )
    {
	LOGW( "ERROR: No matching configs (numConfigs == 0)" );
	return -1;	
    }    

    for( EGLint cnum = 0; cnum < numConfigs; cnum++ )
    {
	//Get config info:
	EGLint cv, r, g, b;
	eglGetConfigAttrib( g_sundog_bridge.display, config[ cnum ], EGL_RED_SIZE, &r );
	eglGetConfigAttrib( g_sundog_bridge.display, config[ cnum ], EGL_GREEN_SIZE, &g );
	eglGetConfigAttrib( g_sundog_bridge.display, config[ cnum ], EGL_BLUE_SIZE, &b );
	eglGetConfigAttrib( g_sundog_bridge.display, config[ cnum ], EGL_SURFACE_TYPE, &cv );
	char stype[ 256 ];
	stype[ 0 ] = 0;
	if( cv & EGL_PBUFFER_BIT ) strcat( stype, "PBUF " );
	if( cv & EGL_PIXMAP_BIT ) strcat( stype, "PIXMAP " );
	if( cv & EGL_WINDOW_BIT ) strcat( stype, "WIN " );
	if( cv & EGL_VG_COLORSPACE_LINEAR_BIT ) strcat( stype, "CL " );
	if( cv & EGL_VG_ALPHA_FORMAT_PRE_BIT ) strcat( stype, "AFP " );
	if( cv & EGL_MULTISAMPLE_RESOLVE_BOX_BIT ) strcat( stype, "MRB " );
	if( cv & EGL_SWAP_BEHAVIOR_PRESERVED_BIT ) strcat( stype, "PRESERVED " );
	LOGI( "EGL Config %d. RGBT: %d %d %d %x %s", cnum, r, g, b, cv, stype );
    }
    
    bool configErr = 1;
    for( EGLint cnum = 0; cnum < numConfigs; cnum++ )
    {
	LOGI( "EGL Config %d", cnum );
	
	if( surface != EGL_NO_SURFACE ) eglDestroySurface( g_sundog_bridge.display, surface );
	if( context != EGL_NO_CONTEXT ) eglDestroyContext( g_sundog_bridge.display, surface );
	surface = EGL_NO_SURFACE;
	context = EGL_NO_CONTEXT;

	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
         * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
         * As soon as we picked a EGLConfig, we can safely reconfigure the
         * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
        if( eglGetConfigAttrib( g_sundog_bridge.display, config[ cnum ], EGL_NATIVE_VISUAL_ID, &format ) != EGL_TRUE )
        {
    	    LOGW( "eglGetConfigAttrib() error %d", eglGetError() );
	    continue;
	}
	
        ANativeWindow_setBuffersGeometry( g_sundog_bridge.app->window, 0, 0, format );
	
        surface = eglCreateWindowSurface( g_sundog_bridge.display, config[ cnum ], g_sundog_bridge.app->window, NULL );
	if( surface == EGL_NO_SURFACE )
        {
    	    LOGW( "eglCreateWindowSurface() error %d", eglGetError() );
	    continue;
	}
	EGLint gles2_attrib[] = 
	{
	    EGL_CONTEXT_CLIENT_VERSION, 2,
	    EGL_NONE
	};
	context = eglCreateContext( g_sundog_bridge.display, config[ cnum ], NULL, gles2_attrib );
	if( surface == EGL_NO_CONTEXT )
        {
	    LOGW( "eglCreateContext() error %d", eglGetError() );
	    continue;
	}

	if( eglMakeCurrent( g_sundog_bridge.display, surface, surface, context ) != EGL_TRUE ) 
	{
    	    LOGW( "eglMakeCurrent error %d", eglGetError() );
    	    continue;
	}

	if( buf_preserved )
	{
	    EGLint sb;
    	    if( eglSurfaceAttrib( g_sundog_bridge.display, surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED ) != EGL_TRUE )
    	    {
    		LOGW( "eglSurfaceAttrib error %d", eglGetError() );
    	    }
    	    if( eglQuerySurface( g_sundog_bridge.display, surface, EGL_SWAP_BEHAVIOR, &sb ) != EGL_TRUE )
    	    {
    		LOGW( "eglQuerySurface error %d", eglGetError() );
    	    }
    	    if( sb == EGL_BUFFER_PRESERVED )
	    {
		g_android_sundog_gl_buffer_preserved = 1;
		LOGI( "EGL_BUFFER_PRESERVED" );
    	    }
	    else
    	    {
    	        g_android_sundog_gl_buffer_preserved = 0;
    		LOGI( "EGL_BUFFER_DESTROYED" );
	    }
	}
	else
	{
    	    g_android_sundog_gl_buffer_preserved = 0;
    	}

	configErr = 0;
	break;
    }
    if( configErr )
    {
	LOGW( "ERROR: No matching configs." );
	return -1;
    }    

    eglQuerySurface( g_sundog_bridge.display, surface, EGL_WIDTH, &w );
    eglQuerySurface( g_sundog_bridge.display, surface, EGL_HEIGHT, &h );

    g_sundog_bridge.context = context;    
    g_sundog_bridge.surface = surface;
    g_android_sundog_display = g_sundog_bridge.display;
    g_android_sundog_surface = surface;
    g_android_sundog_context = context;
    
    g_android_sundog_screen_xsize = w;
    g_android_sundog_screen_ysize = h;

    // Initialize GL state.
    glDisable( GL_DEPTH_TEST );
#ifndef GLNORETAIN
    if( g_android_sundog_gl_buffer_preserved == 0 )
    {
	glDisable( GL_DITHER );
    }
#endif

    glGenTextures( 1, &g_android_camera_texture );
    glBindTexture( GL_TEXTURE_2D, g_android_camera_texture );
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        32, 32,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        0 );
    switch( option_exists( "option_camera_mode" ) )
    {
	case '1':
	    LOGI( "Camera mode: 1 (empty texture)" );
	    break;
	case '2':
	    LOGI( "Camera mode: 2 (setPreviewDisplay instead of setPreviewTexture)" );
	    java_call2_i_i( "SetCameraTexture", 1, 0, 0, 0, 0 );
	    break;
	default:
	    LOGI( "Camera mode: default (texture)" );
	    java_call2_i_i( "SetCameraTexture", 1, g_android_camera_texture, 0, 0, 0 );
	    break;
    }

    return 0;
}

// Tear down the EGL context currently associated with the display.
void engine_term_display( void ) 
{
    if( g_sundog_bridge.display != EGL_NO_DISPLAY ) 
    {
	if( g_android_camera_texture != 0 )
	{
	    glDeleteTextures( 1, &g_android_camera_texture );
	    g_android_camera_texture = 0;
	}
        eglMakeCurrent( g_sundog_bridge.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
        if( g_sundog_bridge.context != EGL_NO_CONTEXT ) 
        {
            eglDestroyContext( g_sundog_bridge.display, g_sundog_bridge.context );
        }
        if( g_sundog_bridge.surface != EGL_NO_SURFACE ) 
        {
            eglDestroySurface( g_sundog_bridge.display, g_sundog_bridge.surface );
        }
        eglTerminate( g_sundog_bridge.display );
    }
    g_sundog_bridge.display = EGL_NO_DISPLAY;
    g_sundog_bridge.context = EGL_NO_CONTEXT;
    g_sundog_bridge.surface = EGL_NO_SURFACE;
}

uint16 convert_key( int32_t vk )
{
    uint16 rv = 0xFFFF;
    
    switch( vk )
    {
	case AKEYCODE_BACK: rv = KEY_ESCAPE; break;
	case AKEYCODE_MENU: rv = KEY_MENU; break;

	case AKEYCODE_SOFT_LEFT: rv = KEY_LEFT; break;
	case AKEYCODE_SOFT_RIGHT: rv = KEY_RIGHT; break;
	case AKEYCODE_0: case 0x90: rv = '0'; break;
	case AKEYCODE_1: case 0x91: rv = '1'; break;
	case AKEYCODE_2: case 0x92: rv = '2'; break;
        case AKEYCODE_3: case 0x93: rv = '3'; break;
	case AKEYCODE_4: case 0x94: rv = '4'; break;
	case AKEYCODE_5: case 0x95: rv = '5'; break;
	case AKEYCODE_6: case 0x96: rv = '6'; break;
	case AKEYCODE_7: case 0x97: rv = '7'; break;
        case AKEYCODE_8: case 0x98: rv = '8'; break;
	case AKEYCODE_9: case 0x99: rv = '9'; break;
        case AKEYCODE_STAR: rv = '*'; break;
        case AKEYCODE_POUND: rv = '#'; break;
	case AKEYCODE_DPAD_UP: rv = KEY_UP; break;
	case AKEYCODE_DPAD_DOWN: rv = KEY_DOWN; break;
	case AKEYCODE_DPAD_LEFT: rv = KEY_LEFT; break;
	case AKEYCODE_DPAD_RIGHT: rv = KEY_RIGHT; break;
	case AKEYCODE_DPAD_CENTER: rv = KEY_SPACE; break;
	case AKEYCODE_A: rv = 'a'; break;
	case AKEYCODE_B: rv = 'b'; break;
	case AKEYCODE_C: rv = 'c'; break;
	case AKEYCODE_D: rv = 'd'; break;
        case AKEYCODE_E: rv = 'e'; break;
	case AKEYCODE_F: rv = 'f'; break;
	case AKEYCODE_G: rv = 'g'; break;
        case AKEYCODE_H: rv = 'h'; break;
	case AKEYCODE_I: rv = 'i'; break;
        case AKEYCODE_J: rv = 'j'; break;
	case AKEYCODE_K: rv = 'k'; break;
	case AKEYCODE_L: rv = 'l'; break;
        case AKEYCODE_M: rv = 'm'; break;
	case AKEYCODE_N: rv = 'n'; break;
        case AKEYCODE_O: rv = 'o'; break;
	case AKEYCODE_P: rv = 'p'; break;
	case AKEYCODE_Q: rv = 'q'; break;
        case AKEYCODE_R: rv = 'r'; break;
	case AKEYCODE_S: rv = 's'; break;
	case AKEYCODE_T: rv = 't'; break;
	case AKEYCODE_U: rv = 'u'; break;
	case AKEYCODE_V: rv = 'v'; break;
	case AKEYCODE_W: rv = 'w'; break;
        case AKEYCODE_X: rv = 'x'; break;
        case AKEYCODE_Y: rv = 'y'; break;
        case AKEYCODE_Z: rv = 'z'; break;
        case AKEYCODE_COMMA: rv = ','; break;
        case AKEYCODE_PERIOD: rv = '.'; break;
	case AKEYCODE_ALT_LEFT: rv = KEY_ALT; break;
	case AKEYCODE_ALT_RIGHT: rv = KEY_ALT; break;
	case AKEYCODE_SHIFT_LEFT: rv = KEY_SHIFT; break;
        case AKEYCODE_SHIFT_RIGHT: rv = KEY_SHIFT; break;
        case AKEYCODE_TAB: rv = KEY_TAB; break;
        case AKEYCODE_SPACE: rv = ' '; break;
	case AKEYCODE_ENTER: rv = KEY_ENTER; break;
        case AKEYCODE_DEL: rv = KEY_BACKSPACE; break;
        //case 112: rv = KEY_DELETE; break; //AKEYCODE_FORWARD_DEL
        case AKEYCODE_GRAVE: rv = '`'; break;
	case AKEYCODE_MINUS: rv = '-'; break;
        case AKEYCODE_EQUALS: rv = '='; break;
        case AKEYCODE_LEFT_BRACKET: rv = '['; break;
	case AKEYCODE_RIGHT_BRACKET: rv = ']'; break;
	case AKEYCODE_BACKSLASH: rv = '\\'; break;
	case AKEYCODE_SEMICOLON: rv = ';'; break;
	case AKEYCODE_APOSTROPHE: rv = '\''; break;
	case AKEYCODE_SLASH: rv = '/'; break;
	case AKEYCODE_AT: rv = '@'; break;
	case AKEYCODE_PLUS: rv = '+'; break;
	case AKEYCODE_PAGE_UP: rv = KEY_PAGEUP; break;
	case AKEYCODE_PAGE_DOWN: rv = KEY_PAGEDOWN; break;
	case AKEYCODE_BUTTON_A: rv = 'a'; break;
	case AKEYCODE_BUTTON_B: rv = 'b'; break;
        case AKEYCODE_BUTTON_C: rv = 'c'; break;
        case AKEYCODE_BUTTON_X: rv = 'x'; break;
        case AKEYCODE_BUTTON_Y: rv = 'y'; break;
        case AKEYCODE_BUTTON_Z: rv = 'z'; break;
    }
    
    if( rv == 0xFFFF ) rv = KEY_UNKNOWN + vk;
    return rv;
}

// Process the next input event.
static int32_t engine_handle_input( struct android_app* app, AInputEvent* event ) 
{
    int32_t rv = 0;
    sundog_info_struct* sd = &g_sundog_bridge.sd;
    if( sd->initialized == 0 ) return 0;
    window_manager* wm = sd->s.wm;
    int x, y;
    int pressure;
    int32_t id;
    int32_t evt_type = AInputEvent_getType( event );
    if( evt_type == AINPUT_EVENT_TYPE_KEY )
    {
	int32_t sc = AKeyEvent_getScanCode( event );
	int32_t vk = AKeyEvent_getKeyCode( event );
	uint16 sundog_key = convert_key( vk );
	int32_t ms = AKeyEvent_getMetaState( event );
	uint16 sundog_flags = 0;
	if( ms & AMETA_ALT_ON ) sundog_flags |= EVT_FLAG_ALT;
	if( ms & AMETA_SHIFT_ON ) sundog_flags |= EVT_FLAG_SHIFT;
	if( ms & 4096 ) sundog_flags |= EVT_FLAG_CTRL;
	int32_t evt = AKeyEvent_getAction( event );
	switch( evt )
	{
	    case AKEY_EVENT_ACTION_DOWN:
		//LOGI( "AKEY_EVENT_ACTION_DOWN VK:%d SD:%d F:%d", vk, sundog_key, sundog_flags );
		android_sundog_key_event( 1, sundog_key, (uint16)sc, sundog_flags, wm );
		break;
	    case AKEY_EVENT_ACTION_UP:
		//LOGI( "AKEY_EVENT_ACTION_UP VK:%d SD:%d F:%d", vk, sundog_key, sundog_flags );
		android_sundog_key_event( 0, sundog_key, (uint16)sc, sundog_flags, wm );
		break;
	}
	switch( vk )
	{
	    case AKEYCODE_BACK:
	    case AKEYCODE_MENU:
		rv = 1; //prevent default handler
		break;
	}
    }
    if( evt_type == AINPUT_EVENT_TYPE_MOTION )
    {
	size_t pcount = AMotionEvent_getPointerCount( event );
	int32_t evt = AMotionEvent_getAction( event );
	switch( evt & AMOTION_EVENT_ACTION_MASK )
	{
	    case AMOTION_EVENT_ACTION_DOWN:
		//First touch (primary pointer):
		x = AMotionEvent_getX( event, 0 );
		y = AMotionEvent_getY( event, 0 );
		pressure = 1024; //AMotionEvent_getPressure( event, 0 ) * 1024;
		id = AMotionEvent_getPointerId( event, 0 );
		collect_touch_events( 0, TOUCH_FLAG_REALWINDOW_XY | TOUCH_FLAG_LIMIT, 0, x, y, pressure, id, wm );
		rv = 1;
		break;
	    case AMOTION_EVENT_ACTION_MOVE:
		{
		    for( size_t i = 0; i < pcount; i++ )
    		    {
			id = AMotionEvent_getPointerId( event, i );
			x = AMotionEvent_getX( event, i );
			y = AMotionEvent_getY( event, i );
			pressure = 1024; //AMotionEvent_getPressure( event, i ) * 1024;
			uint evt_flags = 0;
			if( i < pcount - 1 ) evt_flags |= EVT_FLAG_DONTDRAW;
			collect_touch_events( 1, TOUCH_FLAG_REALWINDOW_XY | TOUCH_FLAG_LIMIT, evt_flags, x, y, pressure, id, wm );
    		    }
		}
		rv = 1;
		break;
	    case AMOTION_EVENT_ACTION_UP:
	    case AMOTION_EVENT_ACTION_CANCEL:
		{
		    for( size_t i = 0; i < pcount; i++ )
    		    {
			id = AMotionEvent_getPointerId( event, i );
			x = AMotionEvent_getX( event, i );
			y = AMotionEvent_getY( event, i );
			pressure = 1024; //AMotionEvent_getPressure( event, i ) * 1024;
			collect_touch_events( 2, TOUCH_FLAG_REALWINDOW_XY | TOUCH_FLAG_LIMIT, 0, x, y, pressure, id, wm );
    		    }
		}
		rv = 1;
		break;
	    case AMOTION_EVENT_ACTION_POINTER_DOWN:
		{
		    int i = ( evt & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK ) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
		    id = AMotionEvent_getPointerId( event, i );
		    x = AMotionEvent_getX( event, i );
		    y = AMotionEvent_getY( event, i );
		    pressure = 1024; //AMotionEvent_getPressure( event, i ) * 1024;
		    collect_touch_events( 0, TOUCH_FLAG_REALWINDOW_XY | TOUCH_FLAG_LIMIT, 0, x, y, pressure, id, wm );
		}
		rv = 1;
		break;
	    case AMOTION_EVENT_ACTION_POINTER_UP:
		{
		    int i = ( evt & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK ) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
		    id = AMotionEvent_getPointerId( event, i );
		    x = AMotionEvent_getX( event, i );
		    y = AMotionEvent_getY( event, i );
		    pressure = 1024; //AMotionEvent_getPressure( event, i ) * 1024;
		    collect_touch_events( 2, TOUCH_FLAG_REALWINDOW_XY | TOUCH_FLAG_LIMIT, 0, x, y, pressure, id, wm );
		}
		rv = 1;
		break;
	}
	send_touch_events( wm );
    }
    return rv;
}

// Process the next main command.
static void engine_handle_cmd( struct android_app* app, int32_t cmd ) 
{
    switch( cmd ) 
    {
	case APP_CMD_PAUSE:
	    if( g_sundog_bridge.sd.initialized )
	    {
		g_eventloop_stop_request = 1;
		int i = 0;
		int timeout = 1000;
		int step = 20;
		for( i = 0; i < timeout; i += step )
		{
		    if( g_eventloop_stop_answer == 1 ) break;
		    time_sleep( step );
		}
		if( i >= timeout )
		{
		    LOGI( "SunDog PAUSE TIMEOUT" );
		}
	    }
	    break;
        case APP_CMD_STOP:
    	    LOGI( "SunDog STOP" );
            android_sundog_deinit();
    	    break;
	case APP_CMD_RESUME:
	    if( g_sundog_bridge.sd.initialized )
	    {
		g_eventloop_stop_request = 0;
		int i = 0;
		int timeout = 1000;
		int step = 20;
		for( i = 0; i < timeout; i += step )
		{
		    if( g_eventloop_stop_answer == 0 ) break;
		    time_sleep( step );
		}
		if( i >= timeout )
		{
		    LOGI( "SunDog RESUME TIMEOUT" );
		}
	    }
	    break;
        case APP_CMD_START:
    	    break;
    	    
        case APP_CMD_SAVE_STATE:
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if( g_sundog_bridge.app->window != NULL ) 
	    {
		android_sundog_init();
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            android_sundog_deinit();
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            if( g_sundog_bridge.accelerometerSensor != NULL) 
	    {
                // We'd like to get 60 events per second (in us).
                //ASensorEventQueue_enableSensor( engine->sensorEventQueue, engine->accelerometerSensor );
                //ASensorEventQueue_setEventRate( engine->sensorEventQueue, engine->accelerometerSensor, (1000L/60)*1000 );
            }
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if( g_sundog_bridge.accelerometerSensor != NULL ) 
	    {
                //ASensorEventQueue_disableSensor( engine->sensorEventQueue, engine->accelerometerSensor );
            }
            break;
	case APP_CMD_CONFIG_CHANGED:
	    g_android_sundog_resize_request++;
	    break;
    }
}

void android_main( struct android_app* state ) 
{
    LOGI( "ANDROID MAIN " ARCH_NAME " " __DATE__ " " __TIME__ );

    // Make sure glue isn't stripped.
    app_dummy();
    
    memset( &g_sundog_bridge, 0, sizeof( g_sundog_bridge ) );
    state->userData = &g_sundog_bridge;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    g_sundog_bridge.app = state;

    g_android_sundog_screen_ppi = AConfiguration_getDensity( state->config );

    {
	JNIEnv* jni;
	state->activity->vm->AttachCurrentThread( &jni, NULL );
	jclass na_class = jni->FindClass( "android/app/NativeActivity" ); 
	if( na_class == 0 ) 
	{
	    LOGW( "NativeActivity class not found!" );
	    return;
	}
	else
	{
    	    jmethodID getApplicationContext = jni->GetMethodID( na_class, "getApplicationContext", "()Landroid/content/Context;" );
    	    jobject context_obj = jni->CallObjectMethod( state->activity->clazz, getApplicationContext );
    	    if( context_obj == 0 ) 
    	    { 
    		LOGW( "Context error" );
    		return; 
    	    }
    	    jclass context_class = jni->GetObjectClass( context_obj );
    	    jmethodID getPackageName = jni->GetMethodID( context_class, "getPackageName", "()Ljava/lang/String;" );
    	    jstring packName = (jstring)jni->CallObjectMethod( context_obj, getPackageName );
    	    if( packName == 0 ) 
    	    {
    		LOGW( "Can't get package name" );
    		return; 
    	    }
    	    const char* pkg_name_cstr = jni->GetStringUTFChars( packName, NULL );
    	    g_sundog_bridge.package_name[ 0 ] = 0;
    	    sprintf( g_sundog_bridge.package_name, "%s", pkg_name_cstr );
    	    sprintf( g_sundog_bridge.user_class_name, "%s/MyNativeActivity", pkg_name_cstr );
            jni->ReleaseStringUTFChars( packName, pkg_name_cstr );
	}
	state->activity->vm->DetachCurrentThread();
    }

    // Get system info:
    
    utf8_char* str;
    
    str = java_call2_s_s( "GetDir", "internal_cache" );
    if( str )
    {
	mkdir( str, 0770 );
	strcat( str, "/" );
	g_android_cache_int_path = str;
    }
    else g_android_cache_int_path = strdup( "" );
    
    str = java_call2_s_s( "GetDir", "internal_files" );
    if( str )
    {
	mkdir( str, 0770 );
	strcat( str, "/" );
	g_android_files_int_path = str;
    }
    else g_android_files_int_path = strdup( "" );

    str = java_call2_s_s( "GetDir", "external_cache" );
    if( str )
    {
	mkdir( str, 0770 );
	strcat( str, "/" );
	g_android_cache_ext_path = str;
    }
    else g_android_cache_ext_path = strdup( "" );

    str = java_call2_s_s( "GetDir", "external_files" );
    if( str )
    {
	mkdir( str, 0770 );
	strcat( str, "/" );
	g_android_files_ext_path = str;
    }
    else g_android_files_ext_path = strdup( "" );

    str = java_call2_s_s( "GetDir", "external_dcim" );
    if( str )
    {
	mkdir( str, 0770 );
	strcat( str, "/" );
	g_android_dcim_ext_path = str;
    }
    else g_android_dcim_ext_path = strdup( "" );

    str = java_call2_s_s( "GetDir", "external_pictures" );
    if( str )
    {
	mkdir( str, 0770 );
	strcat( str, "/" );
	g_android_pics_ext_path = str;
    }
    else g_android_pics_ext_path = strdup( "" );

    str = java_call2_s_s( "GetDir", "external_movies" );
    if( str )
    {
	mkdir( str, 0770 );
	strcat( str, "/" );
	g_android_movies_ext_path = str;
    }
    else g_android_movies_ext_path = strdup( "" );

    str = java_call2_s_v( "GetOSVersion" );
    if( str )
    {
	g_android_version = str;
    }
    else g_android_version = strdup( "2.3" );

    str = java_call2_s_v( "GetLanguage" );
    if( str )
    {
	g_android_lang = str;
    }
    else g_android_lang = strdup( "en_US" );

    g_intent_file_name = java_call2_s_v( "GetIntentFile" );
    g_open_document_request = g_intent_file_name;

    LOGI( "Internal Cache Dir: %s", g_android_cache_int_path );
    LOGI( "Internal Files Dir: %s", g_android_files_int_path );
    LOGI( "External Cache Dir: %s", g_android_cache_ext_path );
    LOGI( "External Files Dir: %s", g_android_files_ext_path );
    LOGI( "External DCIM Dir: %s", g_android_dcim_ext_path );
    LOGI( "External Pictures Dir: %s", g_android_pics_ext_path );
    LOGI( "OS Version: %s", g_android_version );
    LOGI( "Language: %s", g_android_lang );
    if( g_intent_file_name ) LOGI( "Intent File Name: %s\n", g_intent_file_name );
    
    int android_version_ptr = 0;
    memset( g_android_version_nums, 0, sizeof( g_android_version_nums ) );
    for( int i = 0; ; i++ )
    {
        char c = g_android_version[ i ];
        if( c == 0 ) break;
        if( c == '.' ) android_version_ptr++;
        if( c >= '0' && c <= '9' )
        {
            g_android_version_nums[ android_version_ptr ] *= 10;
            g_android_version_nums[ android_version_ptr ] += c - '0';
        }
    }
    sprintf( g_android_version_correct, "%d.%d.%d", g_android_version_nums[ 0 ], g_android_version_nums[ 1 ], g_android_version_nums[ 2 ] );
    LOGI( "Android version (correct): %s\n", g_android_version_correct );

    if( ( g_android_version_nums[ 0 ] == 4 && g_android_version_nums[ 1 ] >= 3 ) || g_android_version_nums[ 0 ] > 4 )
	g_glcapture_supported = true;
    else
	g_glcapture_supported = false;
    if( option_exists( "option_no_system_glcapture" ) )
	g_glcapture_supported = false;
    
    // Prepare to monitor accelerometer
    /*g_sundog_bridge.sensorManager = ASensorManager_getInstance();
    g_sundog_bridge.accelerometerSensor = ASensorManager_getDefaultSensor( g_sundog_bridge.sensorManager, ASENSOR_TYPE_ACCELEROMETER );
    g_sundog_bridge.sensorEventQueue = ASensorManager_createEventQueue( g_sundog_bridge.sensorManager, state->looper, LOOPER_ID_USER, NULL, NULL );*/

    // loop waiting for stuff to do.

    while( 1 ) 
    {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while( ( ident = ALooper_pollAll( -1, NULL, &events, (void**)&source ) ) >= 0 ) 
        {

            // Process this event.
            if( source != NULL ) 
            {
                source->process( state, source );
            }

            // If a sensor has data, process it now.
            /*if( ident == LOOPER_ID_USER ) 
            {
                if( g_sundog_bridge.accelerometerSensor != NULL ) 
                {
                    ASensorEvent event;
                    while( ASensorEventQueue_getEvents( g_sundog_bridge.sensorEventQueue, &event, 1 ) > 0 ) 
                    {
                        //LOGI( "accelerometer: x=%f y=%f z=%f", event.acceleration.x, event.acceleration.y, event.acceleration.z );
                    }
                }
            }*/

            // Check if we are exiting.
            if( state->destroyRequested != 0 ) 
            {
                android_sundog_deinit();
                goto break_all;
            }
        }
    }

break_all:

    free( g_android_cache_int_path );
    free( g_android_cache_ext_path );
    free( g_android_files_int_path );
    free( g_android_files_ext_path );
    free( g_android_dcim_ext_path );
    free( g_android_pics_ext_path );
    free( g_android_movies_ext_path );
    free( g_android_version );
    free( g_android_lang );
    free( g_intent_file_name );

    LOGI( "ANDROID MAIN FINISHED" );
}

#endif //!NOMAIN
