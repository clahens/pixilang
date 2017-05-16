/*
    video.cpp
    This file is part of the SunDog engine.
    Copyright (C) 2014 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100% with global init/deinit

#include "core/core.h"
#include "../video.h"

#ifndef NOVIDEO

#ifdef ANDROID
    #include "video_android.h"
#else
    #ifdef LINUX
	#include "video_linux.h"
    #endif
#endif

#ifdef IPHONE
    #include "video_iphone.h"
#endif

#endif //!NOVIDEO

int bvideo_global_init( uint flags )
{
    int rv = -1;
#ifndef NOVIDEO
#ifdef DEVICE_VIDEO_FUNCTIONS
    rv = device_bvideo_global_init( flags );
#endif	
#endif //!NOVIDEO
    return rv;
}

int bvideo_global_deinit( uint flags )
{	
    int rv = -1;
#ifndef NOVIDEO
    #ifdef DEVICE_VIDEO_FUNCTIONS
	rv = device_bvideo_global_deinit( flags );
    #endif
#endif //!NOVIDEO
    return rv;
}

int bvideo_open( bvideo_struct* vid, const utf8_char* name, uint flags, bvideo_capture_callback_t capture_callback, void* capture_user_data )
{
    int rv = -1;
#ifndef NOVIDEO
    while( 1 )
    {
	if( vid == 0 ) break;	
	bmem_set( vid, sizeof( bvideo_struct ), 0 );
	vid->name = (utf8_char*)bmem_strdup( name );
	vid->flags = flags;	
	vid->capture_callback = capture_callback;
	vid->capture_user_data = capture_user_data;
#ifdef DEVICE_VIDEO_FUNCTIONS
	rv = device_bvideo_open( vid, name, flags );
#endif
	break;
    }
    if( rv != 0 )
    {
	blog( "bvideo_open() error %d\n", rv );
	bmem_free( vid->name );
    }
#endif //!NOVIDEO
    return rv;
}

int bvideo_close( bvideo_struct* vid )
{
    int rv = -1;
#ifndef NOVIDEO
    while( 1 )
    {
	if( vid == 0 ) break;	
#ifdef DEVICE_VIDEO_FUNCTIONS
	rv = device_bvideo_close( vid );
#endif	
	bmem_free( vid->name );
	break;
    }
    if( rv != 0 )
    {
	blog( "bvideo_close() error %d\n", rv );
    }
#endif //!NOVIDEO
    return rv;
}

int bvideo_start( bvideo_struct* vid )
{
    int rv = -1;
#ifndef NOVIDEO
    while( 1 )
    {
	if( vid == 0 ) break;	
#ifdef DEVICE_VIDEO_FUNCTIONS
	rv = device_bvideo_start( vid );
#endif
	bvideo_prop props[ 5 ];
	bmem_set( props, sizeof( props ), 0 );
	props[ 0 ].id = BVIDEO_PROP_FRAME_WIDTH_I;
	props[ 1 ].id = BVIDEO_PROP_FRAME_HEIGHT_I;
	props[ 2 ].id = BVIDEO_PROP_PIXEL_FORMAT_I;
	props[ 3 ].id = BVIDEO_PROP_FOCUS_MODE_I;
	if( bvideo_get_props( vid, props ) == 0 )
	{
	    vid->frame_width = (int)props[ 0 ].val.i;
	    vid->frame_height = (int)props[ 1 ].val.i;
	    vid->pixel_format = (int)props[ 2 ].val.i;
	    blog( "bvideo_start(): %dx%d; pixel format:%d; focus mode:%d\n", vid->frame_width, vid->frame_height, vid->pixel_format, (int)props[ 3 ].val.i );
	}
	break;
    }
    if( rv != 0 )
    {
	blog( "bvideo_start() error %d\n", rv );
    }
#endif //!NOVIDEO
    return rv;
}

int bvideo_stop( bvideo_struct* vid )
{
    int rv = -1;
#ifndef NOVIDEO
    while( 1 )
    {
	if( vid == 0 ) break;	
#ifdef DEVICE_VIDEO_FUNCTIONS
	rv = device_bvideo_stop( vid );
#endif
	break;
    }
    if( rv != 0 )
    {
	blog( "bvideo_stop() error %d\n", rv );
    }
#endif //!NOVIDEO
    return rv;
}

int bvideo_set_props( bvideo_struct* vid, bvideo_prop* props )
{
    int rv = -1;
#ifndef NOVIDEO
    while( 1 )
    {
	if( vid == 0 ) break;
	if( props == 0 ) break;
#ifdef DEVICE_VIDEO_FUNCTIONS
	rv = device_bvideo_set_props( vid, props );
#endif
	break;
    }
    if( rv != 0 )
    {
	blog( "bvideo_set_props() error %d\n", rv );
    }
#endif //!NOVIDEO
    return rv;
}

int bvideo_get_props( bvideo_struct* vid, bvideo_prop* props )
{
    int rv = -1;
#ifndef NOVIDEO
    while( 1 )
    {
	if( vid == 0 ) break;
	if( props == 0 ) break;
#ifdef DEVICE_VIDEO_FUNCTIONS
	rv = device_bvideo_get_props( vid, props );
#endif
	break;
    }
    if( rv != 0 )
    {
	blog( "bvideo_get_props() error %d\n", rv );
    }
#endif //!NOVIDEO
    return rv;
}

inline COLOR YCbCr_to_COLOR( int Y, int Cb, int Cr ) 
{
    int r, g, b;
    b = Y + ( ( (int)( 1.402f * 256.0f ) * Cb ) >> 8 );
    g = Y - ( ( (int)( 0.344f * 256.0f ) * Cr + (int)( 0.714f * 256.0f ) * Cb ) >> 8 );
    r = Y + ( ( (int)( 1.772f * 256.0f ) * Cr ) >> 8 );
    r = r > 255 ? 255 : r < 0 ? 0 : r;
    g = g > 255 ? 255 : g < 0 ? 0 : g;
    b = b > 255 ? 255 : b < 0 ? 0 : b;
    return get_color( r, g, b );
}

int bvideo_pixel_convert( void* src, int src_xsize, int src_ysize, int src_pixel_format, void* dest, int dest_pixel_format )
{
    int rv = -1;
#ifndef NOVIDEO
    while( 1 )
    {
	if( src == 0 ) break;
	if( dest == 0 ) break;
	switch( src_pixel_format )
	{
	    case BVIDEO_PIXEL_FORMAT_YCbCr422:
		switch( dest_pixel_format )
		{
		    case BVIDEO_PIXEL_FORMAT_GRAYSCALE8:
			{
			    uchar* src2 = (uchar*)src;
			    uchar* dest2 = (uchar*)dest;
			    for( int i = 0; i < src_xsize * src_ysize; i++ )
			    {
				*dest2 = *src2;
				dest2++;
				src2 += 2;
			    }
			}
			rv = 0;
			break;
		    case BVIDEO_PIXEL_FORMAT_COLOR:
			{
			    uchar* src2 = (uchar*)src;
			    COLORPTR dest2 = (COLORPTR)dest;
			    for( int i = 0; i < src_xsize * src_ysize; i += 2 )
			    {
				int Y1 = src2[ 0 ];
				int Y2 = src2[ 2 ];
				int Cb = src2[ 1 ];
				int Cr = src2[ 3 ];
				Cb -= 128;
				Cr -= 128;
				*dest2 = YCbCr_to_COLOR( Y1, Cb, Cr ); dest2++;
				*dest2 = YCbCr_to_COLOR( Y2, Cb, Cr ); dest2++;
				src2 += 4;
			    }
			}
			rv = 0;
			break;
		    default: break;
		}
		break;
	    case BVIDEO_PIXEL_FORMAT_YCbCr422_SEMIPLANAR:
	    case BVIDEO_PIXEL_FORMAT_YCbCr420_SEMIPLANAR:
	    case BVIDEO_PIXEL_FORMAT_YCrCb420_SEMIPLANAR:
		switch( dest_pixel_format )
		{
		    case BVIDEO_PIXEL_FORMAT_GRAYSCALE8:
			bmem_copy( dest, src, src_xsize * src_ysize );
			rv = 0;
			break;
		    default: break;
		}
		break;
	    default: break;
	}
	if( rv != 0 ) switch( src_pixel_format )
	{
	    case BVIDEO_PIXEL_FORMAT_YCbCr420_SEMIPLANAR:
	    case BVIDEO_PIXEL_FORMAT_YCrCb420_SEMIPLANAR:
		switch( dest_pixel_format )
		{
		    case BVIDEO_PIXEL_FORMAT_COLOR:
			{
			    uchar* src2 = (uchar*)src; //Y
			    uchar* src3 = src2 + src_xsize * src_ysize; //Cb and Cr
			    COLOR* dest2 = (COLOR*)dest;
			    for( int y = 0; y < src_ysize / 2; y++ )
			    {
				for( int x = 0; x < src_xsize / 2; x++ )
				{
				    int Y1 = src2[ 0 ];
				    int Y2 = src2[ 1 ];
				    int Y3 = src2[ src_xsize ];
				    int Y4 = src2[ src_xsize + 1 ];
				    int Cb;
				    int Cr;
				    if( src_pixel_format == BVIDEO_PIXEL_FORMAT_YCbCr420_SEMIPLANAR )
				    {
					Cb = src3[ 0 ];
					Cr = src3[ 1 ];
				    }
				    else
				    {
					Cr = src3[ 0 ];
					Cb = src3[ 1 ];
				    }
				    Cb -= 128;
				    Cr -= 128;
				    dest2[ 0 ] = YCbCr_to_COLOR( Y1, Cb, Cr );
				    dest2[ 1 ] = YCbCr_to_COLOR( Y2, Cb, Cr );
				    dest2[ src_xsize ] = YCbCr_to_COLOR( Y3, Cb, Cr );
				    dest2[ src_xsize + 1 ] = YCbCr_to_COLOR( Y4, Cb, Cr );
				    src2 += 2;
				    src3 += 2;
				    dest2 += 2;
				}
				src2 += src_xsize;
				dest2 += src_xsize;
			    }
			}
			rv = 0;
			break;
		    default: break;
		}
		break;
	    default: break;
	}
	break;
    }
#endif
    return rv;
}
