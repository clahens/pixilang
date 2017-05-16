#pragma once

// possible names for bvideo_open():
//   * video file name;
//   * device name;
//   * camera - default camera or value from the profile (KEY_CAMERA; 0 - back, 1 - front, 2, 3, 4 ...);
//   * camera_front;
//   * camera_back;

enum 
{
    BVIDEO_PROP_NONE = 0,
    BVIDEO_PROP_FRAME_WIDTH_I,
    BVIDEO_PROP_FRAME_HEIGHT_I,
    BVIDEO_PROP_PIXEL_FORMAT_I,
    BVIDEO_PROP_FPS_I,
    BVIDEO_PROP_FOCUS_MODE_I,
    BVIDEO_PROP_SUPPORTED_PREVIEW_SIZES_P
};

enum
{
    BVIDEO_PIXEL_FORMAT_YCbCr422, //YUYV / YUY2
    BVIDEO_PIXEL_FORMAT_YCbCr422_SEMIPLANAR,
    BVIDEO_PIXEL_FORMAT_YCbCr420_SEMIPLANAR,
    BVIDEO_PIXEL_FORMAT_YCrCb420_SEMIPLANAR,
    BVIDEO_PIXEL_FORMAT_RGB565,    
    BVIDEO_PIXEL_FORMAT_GRAYSCALE8,
    BVIDEO_PIXEL_FORMAT_COLOR, //native app pixel format (COLOR type)
    BVIDEO_PIXEL_FORMAT_JPEG
};

enum 
{
    BVIDEO_FOCUS_MODE_AUTO = 0,
    BVIDEO_FOCUS_MODE_CONTINUOUS,
    BVIDEO_FOCUS_MODE_FIXED,
    BVIDEO_FOCUS_MODE_INFINITY,
};

#define BVIDEO_OPEN_FLAG_READ		( 1 << 0 )
#define BVIDEO_OPEN_FLAG_WRITE		( 1 << 1 )

struct bvideo_struct;
typedef void (*bvideo_capture_callback_t)( bvideo_struct* vid );

union bvideo_prop_val
{
    int64 i;
    double f;
    void* p;
};

struct bvideo_prop //for bvideo_set_props() and bvideo_get_props()
{
    int id;
    bvideo_prop_val val;
};

struct bvideo_struct
{
    utf8_char* name;
    uint flags; //BVIDEO_OPEN_FLAG_xxxx
    int frame_width;
    int frame_height;
    int pixel_format;
    int fps;
    int orientation; //degrees = cam_orientation * 90; can be compared with the window manager screen_angle
    //Capture callback:
    bvideo_capture_callback_t capture_callback;
    void* capture_user_data;
    void* capture_buffer;
    size_t capture_buffer_size;
    //Device specific:
#ifdef ANDROID
    void* android_cam;
    int cam_index;
#endif
#ifdef IPHONE
    void* ios_capture_obj;
#endif
#ifdef LINUX
    const utf8_char* file_name;
    bool is_device;
    int dev_fd;
    int dev_io_method;
    bthread dev_thread;
    volatile bool dev_thread_exit_request;
    void* buffers;
    int buffers_num;
#endif
    bmutex callback_mutex;
    bool callback_active;
    int fps_counter;
    ticks_t fps_time;
};

int bvideo_global_init( uint flags );
int bvideo_global_deinit( uint flags );
int bvideo_open( bvideo_struct* vid, const utf8_char* name, uint flags, bvideo_capture_callback_t capture_callback, void* capture_user_data );
int bvideo_close( bvideo_struct* vid );
int bvideo_start( bvideo_struct* vid );
int bvideo_stop( bvideo_struct* vid );
int bvideo_set_props( bvideo_struct* vid, bvideo_prop* props ); //call this function outside of the start() ... stop() block only!
int bvideo_get_props( bvideo_struct* vid, bvideo_prop* props );
int bvideo_pixel_convert( void* src, int src_xsize, int src_ysize, int src_pixel_format, void* dest, int dest_pixel_format );
// To do:
// bvideo_read_frame()
// bvideo_write_frame()

int device_bvideo_global_init( uint flags );
int device_bvideo_global_deinit( uint flags );
int device_bvideo_open( bvideo_struct* vid, const utf8_char* name, uint flags );
int device_bvideo_close( bvideo_struct* vid );
int device_bvideo_start( bvideo_struct* vid );
int device_bvideo_stop( bvideo_struct* vid );
int device_bvideo_set_props( bvideo_struct* vid, bvideo_prop* props );
int device_bvideo_get_props( bvideo_struct* vid, bvideo_prop* props );


