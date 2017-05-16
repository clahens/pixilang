#pragma once

#include <jni.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android/window.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

struct sundog_info_struct
{
    int initialized;
    pthread_t pth;
    volatile bool pth_finished;
    volatile bool exit_request;
    sundog_engine s;
};

struct sundog_bridge_struct
{
    struct android_app* app;
    
    utf8_char package_name[ 256 ];
    utf8_char user_class_name[ 256 ];
                
    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;
                        
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

    sundog_info_struct sd;
};

extern sundog_bridge_struct g_sundog_bridge;

extern utf8_char* g_android_cache_int_path;
extern utf8_char* g_android_cache_ext_path;
extern utf8_char* g_android_files_int_path;
extern utf8_char* g_android_files_ext_path;
extern utf8_char* g_android_dcim_ext_path;
extern utf8_char* g_android_pics_ext_path;
extern utf8_char* g_android_movies_ext_path;
extern utf8_char* g_android_version;
extern utf8_char g_android_version_correct[ 16 ];
extern int g_android_version_nums[ 8 ];
extern utf8_char* g_android_lang;
extern bool g_glcapture_supported;

extern int g_android_sundog_screen_xsize;
extern int g_android_sundog_screen_ysize;
extern int g_android_sundog_screen_ppi;
extern bool g_android_sundog_gl_buffer_preserved;
extern void* g_android_sundog_framebuffer;
extern volatile int g_android_sundog_resize_request;
extern volatile int g_android_sundog_resize_request_rp;
extern EGLDisplay g_android_sundog_display;
extern EGLSurface g_android_sundog_surface;
extern EGLContext g_android_sundog_context;

#ifndef NOMAIN
void android_sundog_screen_redraw( void );
void android_sundog_event_handler( void );
int android_sundog_copy_resources( void );
void android_sundog_open_url( const utf8_char* url_text );
void android_sundog_scan_media( const utf8_char* path );
int android_sundog_open_camera( int cam_id );
int android_sundog_close_camera( void );
int android_sundog_get_camera_width( void );
int android_sundog_get_camera_height( void );
int android_sundog_get_camera_focus_mode( void );
int android_sundog_set_camera_focus_mode( int mode );
int android_sundog_glcapture_start( int width, int height, int fps, int bitrate_kb );
int android_sundog_glcapture_frame_begin( void );
int android_sundog_glcapture_frame_end( void );
int android_sundog_glcapture_stop( void );
int android_sundog_glcapture_encode( const utf8_char* name );
#endif
