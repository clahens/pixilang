/*
    wm_android.h. Platform-dependent module : Android
    This file is part of the SunDog engine.
    Copyright (C) 2011 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

#pragma once

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>   	//for getenv()
#include <errno.h>

#include "various/android/sundog_bridge.h"

#include "wm_opengl.h"

int g_frame_len;
ticks_t g_ticks_per_frame;
ticks_t g_prev_frame_time = 0;

void android_sundog_key_event( bool down, uint16 key, uint16 scancode, uint16 flags, window_manager* wm )
{
    if( wm == 0 ) return;
    if( wm->wm_initialized == 0 ) return;

    int type;
    if( down )
	type = EVT_BUTTONDOWN;
    else
	type = EVT_BUTTONUP;
    send_event( 0, type, flags, 0, 0, key, scancode, 1024, 0, wm );
}

int device_start( const utf8_char* windowname, int xsize, int ysize, window_manager* wm )
{
    int retval = 0;
    
    blog( "Android version: %s\n", g_android_version_correct );
    
    g_prev_frame_time = 0;

    g_frame_len = 1000 / wm->max_fps;
    g_ticks_per_frame = time_ticks_per_second() / wm->max_fps;

    wm->flags |= WIN_INIT_FLAG_FULLSCREEN;
    wm->flags |= WIN_INIT_FLAG_TOUCHCONTROL;
        
    xsize = g_android_sundog_screen_xsize;
    ysize = g_android_sundog_screen_ysize;
    wm->real_window_width = xsize;
    wm->real_window_height = ysize;

    if( wm->screen_angle & 1 )
    {
	wm->screen_xsize = ysize;
	wm->screen_ysize = xsize;
    }
    else
    {
	wm->screen_xsize = xsize;
	wm->screen_ysize = ysize;
    }
    
    if( wm->screen_ppi == 0 ) wm->screen_ppi = g_android_sundog_screen_ppi;
    win_zoom_init( wm );
    
#ifdef FRAMEBUFFER
    wm->fb = (COLORPTR)g_android_sundog_framebuffer;
    wm->fb_offset = 0;
    wm->fb_ypitch = g_android_sundog_screen_xsize;
    wm->fb_xpitch = 1;
#else
    wm->screen_buffer_preserved = g_android_sundog_gl_buffer_preserved;
#endif

#ifdef OPENGL    
    if( gl_init( wm ) ) return 1;
    gl_resize( wm );
#endif

    return retval;
}

void device_end( window_manager* wm )
{
#ifdef OPENGL
    gl_deinit( wm );
#endif
}

void device_event_handler( window_manager* wm )
{
    int pause;
    
    android_sundog_event_handler();
    if( wm->exit_request ) return;
           
    if( g_android_sundog_resize_request_rp < g_android_sundog_resize_request )
    {
	g_android_sundog_resize_request_rp++;
	blog( "Screen resize request\n" );
	int i;
	for( i = 0; i < 40; i++ )
	{
	    EGLint w, h;
	    eglQuerySurface( g_android_sundog_display, g_android_sundog_surface, EGL_WIDTH, &w );
	    eglQuerySurface( g_android_sundog_display, g_android_sundog_surface, EGL_HEIGHT, &h );
	    if( g_android_sundog_screen_xsize != w || g_android_sundog_screen_ysize != h )
	    {
		blog( "Screen resize: %d %d\n", w, h );
		g_android_sundog_screen_xsize = w;
		g_android_sundog_screen_ysize = h;
		wm->real_window_width = g_android_sundog_screen_xsize;
		wm->real_window_height = g_android_sundog_screen_ysize;
		wm->screen_xsize = wm->real_window_width / wm->screen_zoom;
		wm->screen_ysize = wm->real_window_height / wm->screen_zoom;
		break;
	    }
	    android_sundog_screen_redraw();
	    time_sleep( 10 );
	}
	gl_resize( wm );
	send_event( wm->root_win, EVT_SCREENRESIZE, 0, 0, 0, 0, 0, 0, 0, wm );
    }

    if( wm->flags & WIN_INIT_FLAG_FULL_CPU_USAGE )
    {
	pause = 1;
    }
    else
    {
	pause = g_frame_len;
    }
    
    if( wm->events_count )
    {
	pause = 0;
    }
    
    if( pause )
    {
	time_sleep( pause );
    }
    
    ticks_t cur_t = time_ticks();
    if( cur_t - g_prev_frame_time >= g_ticks_per_frame )
    {
	wm->frame_event_request = 1;
	g_prev_frame_time = cur_t;
    }
}

void device_screen_lock( WINDOWPTR win, window_manager* wm )
{
    if( wm->screen_lock_counter == 0 )
    {
	gl_lock( wm );
    }
    wm->screen_lock_counter++;

    if( wm->screen_lock_counter > 0 )
	wm->screen_is_active = true;
    else
	wm->screen_is_active = false;
}

void device_screen_unlock( WINDOWPTR win, window_manager* wm )
{
    if( wm->screen_lock_counter == 1 )
    {
	gl_unlock( wm );
    }

    if( wm->screen_lock_counter > 0 )
    {
	wm->screen_lock_counter--;
    }

    if( wm->screen_lock_counter > 0 )
	wm->screen_is_active = true;
    else
	wm->screen_is_active = false;
}

bool device_video_capture_supported( window_manager* wm )
{
    return g_glcapture_supported;
}

const utf8_char* device_video_capture_get_file_ext( window_manager* wm )
{
    return "mp4";
}

int device_video_capture_start( window_manager* wm )
{
    int rv = android_sundog_glcapture_start( wm->screen_xsize, wm->screen_ysize, wm->vcap_in_fps, wm->vcap_in_bitrate_kb );
    if( rv == 0 )
    {	
	sound_stream_capture_start( "1:/glcapture.wav" );
    }
    return rv;
}

int device_video_capture_frame_begin( window_manager* wm )
{
    return android_sundog_glcapture_frame_begin();
}

int device_video_capture_frame_end( window_manager* wm )
{
    return android_sundog_glcapture_frame_end();
}

int device_video_capture_stop( window_manager* wm )
{
    int rv = android_sundog_glcapture_stop();
    sound_stream_capture_stop();
    return rv;
}

int device_video_capture_encode( window_manager* wm )
{
    int rv = -1;
    utf8_char* name_in = bfs_make_filename( wm->vcap_in_name );
    rv = android_sundog_glcapture_encode( name_in );
    bmem_free( name_in );
    return rv;
}
