/*
    wm_iphone.h. Platform-dependent module : iOS
    This file is part of the SunDog engine.
    Copyright (C) 2009 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

#pragma once

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>   	//for getenv()
#include <errno.h>
#include <sys/sem.h>    //semaphores ...

#include "various/iphone/sundog_bridge.h"

int g_prev_screen_angle = -1;

#ifdef OPENGL
    #include "wm_opengl.h"
#endif

int g_frame_len;
ticks_t g_ticks_per_frame;
ticks_t g_prev_frame_time = 0;

int device_start( const utf8_char* windowname, int xsize, int ysize, window_manager* wm )
{
    int retval = 0;
    
    g_prev_frame_time = 0;

    g_frame_len = 1000 / wm->max_fps;
    g_ticks_per_frame = time_ticks_per_second() / wm->max_fps;

    wm->flags |= WIN_INIT_FLAG_FULLSCREEN;
    wm->flags |= WIN_INIT_FLAG_TOUCHCONTROL;
        
    xsize = g_iphone_sundog_screen_xsize;
    ysize = g_iphone_sundog_screen_ysize;
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
    
    if( wm->screen_ppi == 0 ) wm->screen_ppi = 160;
    win_zoom_init( wm );

#ifdef FRAMEBUFFER
    wm->fb = (COLORPTR)g_iphone_sundog_framebuffer;
    wm->fb_offset = 0;
    wm->fb_ypitch = g_iphone_sundog_screen_xsize;
    wm->fb_xpitch = 1;
#else
    #ifdef GLNORETAIN
	wm->screen_buffer_preserved = 0;
    #else
	wm->screen_buffer_preserved = 1;
    #endif
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
    
    iphone_sundog_event_handler();
    if( wm->exit_request ) return;
    
    if( wm->real_window_width != g_iphone_sundog_screen_xsize && wm->real_window_height != g_iphone_sundog_screen_ysize )
    {
	wm->real_window_width = g_iphone_sundog_screen_xsize;
	wm->real_window_height = g_iphone_sundog_screen_ysize;
	wm->screen_xsize = g_iphone_sundog_screen_xsize / wm->screen_zoom;
	wm->screen_ysize = g_iphone_sundog_screen_ysize / wm->screen_zoom;
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
