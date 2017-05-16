#define DEVICE_VIDEO_FUNCTIONS

#include <fcntl.h> //low-level i/o
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#define V4L2_ERROR( msg ) blog( msg " error: %d, %s\n", errno, strerror( errno ) ) 

enum 
{
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
};

struct vbuffer
{
    void* start;
    size_t length;
};

static int xioctl( int fh, int request, void* arg )
{
    int r;
    do 
    {
	r = ioctl( fh, request, arg );
    } while( r == -1 && errno == EINTR );
    return r;
}

static void v4l2_process_image( bvideo_struct* vid, void* p, int size )
{
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
        vid->capture_buffer = p;
        vid->capture_buffer_size = size;
        vid->capture_callback( vid );
    }
    bmutex_unlock( &vid->callback_mutex );
}

static int v4l2_read_frame( bvideo_struct* vid )
{
    int rv = -1;
    struct v4l2_buffer buf;
    unsigned int i;
    vbuffer* bufs = (vbuffer*)vid->buffers;
    switch( vid->dev_io_method )
    {
	case IO_METHOD_READ:
	    break;
	case IO_METHOD_MMAP:
	    {
		bmem_clear_struct( buf );
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                if( xioctl( vid->dev_fd, VIDIOC_DQBUF, &buf ) == -1 ) 
                {
                    V4L2_ERROR( "VIDIOC_DQBUF" );
                    break;
                }
                v4l2_process_image( vid, bufs[ buf.index ].start, buf.bytesused );
                if( xioctl( vid->dev_fd, VIDIOC_QBUF, &buf ) == -1 )
                {
            	    V4L2_ERROR( "VIDIOC_QBUF" );
        	    break;
                }
                rv = 0;
	    }
	    break;
	case IO_METHOD_USERPTR:
	    break;
    }
    return rv;
}

static void* v4l2_dev_thread( void* user_data )
{
    bvideo_struct* vid = (bvideo_struct*)user_data;
    
    while( vid->dev_thread_exit_request == 0 ) 
    {
        fd_set fds;
        struct timeval tv;
        int r;
        
        FD_ZERO( &fds );
        FD_SET( vid->dev_fd, &fds );

        //Timeout:
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select( vid->dev_fd + 1, &fds, NULL, NULL, &tv );

        if( r == -1 ) 
        {
            if( errno == EINTR )
        	continue;
            V4L2_ERROR( "v4l2_dev_thread: select" );
            break;
        }
                        
        if( r == 0 ) 
        {
    	    blog( "v4l2_dev_thread: select timeout\n" );
    	    break;
        }
                        
        v4l2_read_frame( vid );
    }
    
    vid->dev_thread_exit_request = 0;
    
    return 0;
}

static int v4l2_init_read( bvideo_struct* vid, uint buffer_size )
{
    int rv = -1;
    return rv;
}

static int v4l2_init_mmap( bvideo_struct* vid )
{
    int rv = -1;

    vid->buffers_num = 4;
    
    while( 1 )
    {
	struct v4l2_requestbuffers req;
	bmem_clear_struct( req );
	req.count  = vid->buffers_num;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if( xioctl( vid->dev_fd, VIDIOC_REQBUFS, &req ) == -1 ) 
        {
    	    if( errno == EINVAL ) 
    	    {
    		blog( "%s does not support memory mapping\n", vid->file_name );
    		break;
            } 
            else 
            {
                V4L2_ERROR( "VIDIOC_REQBUFS" );
                break;
            }
        }
    
	if( req.count < 2 ) 
	{
    	    blog( "Insufficient buffer memory on %s\n", vid->file_name );
    	    break;
        }
        vid->buffers_num = req.count;

        vid->buffers = bmem_new( vid->buffers_num * sizeof( vbuffer ) );
        if( vid->buffers == 0 ) 
        {
            blog( "Out of memory\n" );
            break;
        }
        vbuffer* bufs = (vbuffer*)vid->buffers;
        
        bool buf_err = 0;
        v4l2_buffer buf;
        for( int i = 0; i < vid->buffers_num; i++ ) 
        {
	    bmem_clear_struct( buf );
            buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory      = V4L2_MEMORY_MMAP;
            buf.index       = i;
            if( xioctl( vid->dev_fd, VIDIOC_QUERYBUF, &buf ) == -1 )
            {
        	V4L2_ERROR( "VIDIOC_QUERYBUF" );
        	buf_err = 1;
        	break;
    	    }
	    bufs[ i ].length = buf.length;
    	    bufs[ i ].start =
    		mmap( 
    		    NULL, // start anywhere
            	    buf.length,
                    PROT_READ | PROT_WRITE, // required
                    MAP_SHARED, // recommended
                    vid->dev_fd, buf.m.offset 
                    );
            if( bufs[ i ].start == MAP_FAILED )
            {
        	V4L2_ERROR( "mmap" );
        	buf_err = 1;
        	break;
    	    }
    	}
    	if( buf_err ) break;
	
    	buf_err = 0;
    	for( int i = 0; i < vid->buffers_num; i++ )
    	{
            bmem_clear_struct( buf );
    	    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            if( xioctl( vid->dev_fd, VIDIOC_QBUF, &buf ) == -1 )
            {
                V4L2_ERROR( "VIDIOC_QBUF" );
        	buf_err = 1;
        	break;
	    }
    	}
    	if( buf_err ) break;
        
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if( xioctl( vid->dev_fd, VIDIOC_STREAMON, &type ) == -1 )
        {
    	    V4L2_ERROR( "VIDIOC_STREAMON" );
    	    break;
    	}
    
	vid->dev_thread_exit_request = 0;	
    	if( bthread_create( &vid->dev_thread, v4l2_dev_thread, vid, 0 ) )
    	{
    	    blog( "Can't create v4l2 thread\n" );
    	    break;
    	}
    
	rv = 0;
	break;
    }
    
    return rv;
}

static int v4l2_init_userp( bvideo_struct* vid, uint buffer_size )
{
    int rv = -1;
    return rv;
}

static int v4l2_init( bvideo_struct* vid )
{
    int rv = -1;

    vid->dev_io_method = IO_METHOD_MMAP;
    
    while( 1 )
    {
	v4l2_capability cap;
        v4l2_format fmt;
        
        if( xioctl( vid->dev_fd, VIDIOC_QUERYCAP, &cap ) == -1 ) 
        {
            if( errno == EINVAL ) 
            {
                blog( "%s is no V4L2 device\n", vid->file_name );
            } 
            else 
            {
        	V4L2_ERROR( "VIDIOC_QUERYCAP" );
            }
            break;
        }
        
        blog( 
    	    "\tdriver: %s\n"
    	    "\tcard: %s \n"
            "\tbus_info: %s\n",
            cap.driver, cap.card, cap.bus_info );
        blog( 
    	    "\tversion: %u.%u.%u\n",
            ( cap.version >> 16 ) & 0xFF,
            ( cap.version >> 8 ) & 0xFF,
            cap.version & 0xFF );
        blog(
    	    "\tcapabilities: 0x%08x\n", 
    	    cap.capabilities );
                
        if( !( cap.capabilities & V4L2_CAP_VIDEO_CAPTURE ) ) 
        {
    	    blog( "%s is no video capture device\n", vid->file_name );
    	    break;
        }

	bool io_err = 0;
        switch( vid->dev_io_method ) 
        {
    	    case IO_METHOD_READ:
                if( !( cap.capabilities & V4L2_CAP_READWRITE ) ) 
                {
            	    blog( "%s does not support read i/o\n", vid->file_name );
            	    io_err = 1;
                }
                break;
    	    case IO_METHOD_MMAP:
    	    case IO_METHOD_USERPTR:
                if( !( cap.capabilities & V4L2_CAP_STREAMING ) ) 
                {
            	    blog( "%s does not support streaming i/o\n", vid->file_name );
            	    io_err = 1;
                }
                break;
        }
        if( io_err ) break;
        
        //Select video input, video standard and tune here:
        
        bmem_clear_struct( fmt );
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if( xioctl( vid->dev_fd, VIDIOC_G_FMT, &fmt ) == -1 )
        {
    	    V4L2_ERROR( "VIDIOC_G_FMT" );
    	    break;
    	}
        fmt.fmt.pix.width = 640;
        fmt.fmt.pix.height = 480;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        if( xioctl( vid->dev_fd, VIDIOC_S_FMT, &fmt ) == -1 )
        {
    	    V4L2_ERROR( "VIDIOC_S_FMT" );
    	}
    	vid->frame_width = fmt.fmt.pix.width;
    	vid->frame_height = fmt.fmt.pix.height;
    	
        //Buggy driver paranoia
        uint min = fmt.fmt.pix.width * 2;
        if( fmt.fmt.pix.bytesperline < min )
            fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if( fmt.fmt.pix.sizeimage < min )
            fmt.fmt.pix.sizeimage = min;

        switch( vid->dev_io_method ) 
        {
    	    case IO_METHOD_READ:
                rv = v4l2_init_read( vid, fmt.fmt.pix.sizeimage );
                break;
    	    case IO_METHOD_MMAP:
                rv = v4l2_init_mmap( vid );
                break;
    	    case IO_METHOD_USERPTR:
                rv = v4l2_init_userp( vid, fmt.fmt.pix.sizeimage );
                break;
        }

	break;
    }
    
    return rv;
}

static int v4l2_deinit( bvideo_struct* vid )
{
    int rv = -1;

    while( 1 )
    {
        switch( vid->dev_io_method ) 
        {
    	    case IO_METHOD_READ:
                break;
    	    case IO_METHOD_MMAP:
    		{
    		    vid->dev_thread_exit_request = 1;
    		    bthread_destroy( &vid->dev_thread, 1000 * 10 );
    		    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            	    xioctl( vid->dev_fd, VIDIOC_STREAMOFF, &type );
            	    vbuffer* bufs = (vbuffer*)vid->buffers;
    		    for( int i = 0; i < vid->buffers_num; i++ )
    		    {
                        if( munmap( bufs[ i ].start, bufs[ i ].length ) )
                        {
                    	    V4L2_ERROR( "munmap" );
                    	}
                    }
    		    bmem_free( vid->buffers );
    		    vid->buffers = 0;
    		}
                break;
    	    case IO_METHOD_USERPTR:
                break;
        }
        rv = 0;
        break;
    }

    return rv;
}    

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
    
    vid->orientation = 0;
    vid->file_name = name;
    vid->is_device = 0;
    bmutex_init( &vid->callback_mutex, 0 );
    
    while( 1 )
    {
        if( bmem_strcmp( name, "camera" ) == 0 ) 
        {
    	    int cam_num = profile_get_int_value( KEY_CAMERA, -1, 0 );
    	    switch( cam_num )
    	    {
    		case 0: vid->file_name = "/dev/video0"; break;
    		case 1: vid->file_name = "/dev/video1"; break;
    		case 2: vid->file_name = "/dev/video2"; break;
    		case 3: vid->file_name = "/dev/video3"; break;
    		case 4: vid->file_name = "/dev/video4"; break;
    		case 5: vid->file_name = "/dev/video5"; break;
    		case 6: vid->file_name = "/dev/video6"; break;
    		case 7: vid->file_name = "/dev/video7"; break;
    		default: vid->file_name = "/dev/video0"; break; //any
    	    }
    	}
        if( bmem_strcmp( name, "camera_back" ) == 0 ) vid->file_name = "/dev/video0";
        if( bmem_strcmp( name, "camera_front" ) == 0 ) vid->file_name = "/dev/video1";
        
        struct stat st;
        if( stat( vid->file_name, &st ) )
        {
    	    blog( "Cannot identify '%s': %d, %s\n", vid->file_name, errno, strerror( errno ) );
    	    break;
        }
        if( !S_ISCHR( st.st_mode ) )
    	    vid->is_device = 0;
    	else
    	    vid->is_device = 1;
        
        if( vid->is_device )
        {
    	    //Open device:
    	    blog( "Trying to open video device %s ...\n", vid->file_name );
    	    vid->dev_fd = open( vid->file_name, O_RDWR | O_NONBLOCK, 0 );
    	    if( vid->dev_fd == -1 )
    	    {
    		blog( "Cannot open '%s': %d, %s\n", vid->file_name, errno, strerror( errno ) );
    		break;
    	    }
    	    if( v4l2_init( vid ) )
    	    {
    		close( vid->dev_fd );
    		vid->dev_fd = -1;
    		break;
    	    }
    	    rv = 0;
        }
        else
        {
    	    //Open file:
    	    blog( "Trying to open video file %s ...\n", vid->file_name );
        }
        
        break;
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

    if( vid->is_device )
    {
	//Close device:
    	v4l2_deinit( vid );
	close( vid->dev_fd );
	vid->dev_fd = -1;
	rv = 0;
    }
    else
    {
	//Close file:
    }
    bmutex_destroy( &vid->callback_mutex );

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
    bmutex_lock( &vid->callback_mutex );
    vid->callback_active = 1;
    vid->fps_time = time_ticks();
    vid->fps_counter = 0;
    bmutex_unlock( &vid->callback_mutex );
    return 0;
}

int device_bvideo_stop( bvideo_struct* vid )
{
    bmutex_lock( &vid->callback_mutex );
    vid->callback_active = 0;
    bmutex_unlock( &vid->callback_mutex );
    return 0;
}

int device_bvideo_set_props( bvideo_struct* vid, bvideo_prop* props )
{
    int rv = -1;
    while( 1 )
    {
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
        if( props == 0 ) break;
        bool props_handled = 0;
        for( int i = 0; ; i++ )
        {
            bvideo_prop* prop = &props[ i ];
            if( prop->id == BVIDEO_PROP_NONE ) break;
            switch( prop->id )
            {
                case BVIDEO_PROP_FRAME_WIDTH_I:
                    prop->val.i = vid->frame_width;
                    props_handled = 1;
                    break;
                case BVIDEO_PROP_FRAME_HEIGHT_I:
                    prop->val.i = vid->frame_height;
                    props_handled = 1;
                    break;
                case BVIDEO_PROP_PIXEL_FORMAT_I:
                    prop->val.i = BVIDEO_PIXEL_FORMAT_YCbCr422;
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
