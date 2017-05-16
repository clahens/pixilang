/*
    wm_unix_sdl.h. Platform-dependent module : SDL
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

#pragma once

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>   	//for getenv()
#include <sched.h>	//for sched_yield()
#include <errno.h>
#include <sys/sem.h>    //semaphores ...

#include "SDL/SDL.h"
#include "SDL/SDL_syswm.h"

#ifdef MAEMO
    extern "C" int osso_display_blanking_pause( void* osso );
    extern "C" void* osso_initialize( const char* application, const char* version, bool activation, void* context );
    extern "C" void osso_deinitialize( void* osso );
#endif

#define EVENT semctl( wm->sdl_event_loop_sem, 0, SETVAL, 1 )

void* frames_thread( void* arg )
{
    window_manager* wm = (window_manager*)arg;
    while( !wm->sdl_frames_thread_exit_request )
    {
	time_sleep( 1000 / wm->max_fps );
	wm->frame_event_request = 1;
	EVENT;
    }
    return NULL;
}

void* screen_control_thread( void* arg )
{
    window_manager* wm = (window_manager*)arg;
#ifdef MAEMO
    int c = 0;
    void* ossocontext = osso_initialize( "sunvox", "1", 0, NULL );
    while( !wm->sdl_screen_control_thread_exit_request )
    {
	if( c == 0 )
	    osso_display_blanking_pause( ossocontext );
	time_sleep( 1000 );
	c++;
	if( c >= 30 ) c = 0;
    }
    osso_deinitialize( ossocontext );
#endif
    return NULL;
}

void* event_thread( void* arg )
{
    window_manager* wm = (window_manager*)arg;
    SDL_Event event;
    volatile int need_exit = 0;
    int mod = 0; //Shift / alt / ctrl...
    int hide_mod = 0;
    int key = 0;
    int exit_request = 0;
    uint video_flags = 0;
    int fs = 0;
    
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) 
    {
	blog( "Can't initialize SDL: %s\n", SDL_GetError() );
	wm->sdl_thread_initialized = -1;
	goto thread_end;
    }

    if( wm->sdl_wm_flags & WIN_INIT_FLAG_FULLSCREEN ) fs = 1;
    
    if( fs )
    {
	//Get list of available video-modes:
	SDL_Rect** modes;
	modes = SDL_ListModes( NULL, SDL_FULLSCREEN | SDL_HWSURFACE );
	if( modes != (SDL_Rect **)-1 )
	{
	    blog( "Available Video Modes:\n" );
	    for( int i = 0; modes[ i ]; ++i )
		printf( "  %d x %d\n", modes[ i ]->w, modes[ i ]->h );
	}
    }
    
    if( wm->sdl_wm_flags & WIN_INIT_FLAG_SCALABLE ) video_flags |= SDL_RESIZABLE;
    if( wm->sdl_wm_flags & WIN_INIT_FLAG_NOBORDER ) video_flags |= SDL_NOFRAME;
    if( fs ) video_flags = SDL_FULLSCREEN;
#if defined(MAEMO)
    wm->sdl_screen = SDL_SetVideoMode( 0, 0, COLORBITS, SDL_HWSURFACE | video_flags );
#else
    wm->sdl_screen = SDL_SetVideoMode( wm->sdl_wm_xsize, wm->sdl_wm_ysize, COLORBITS, SDL_HWSURFACE | video_flags );
#endif
    if( wm->sdl_screen == 0 ) 
    {
	blog( "SDL. Can't set videomode: %s\n", SDL_GetError() );
	wm->sdl_thread_initialized = -2;
	goto thread_end;
    }

    wm->real_window_width = wm->sdl_wm_xsize;
    wm->real_window_height = wm->sdl_wm_ysize;
    
    if( wm->screen_angle & 1 )
    {
        wm->screen_xsize = wm->sdl_wm_ysize;
        wm->screen_ysize = wm->sdl_wm_xsize;
    }
    else
    {
        wm->screen_xsize = wm->sdl_wm_xsize;
        wm->screen_ysize = wm->sdl_wm_ysize;
    }

    if( wm->screen_ppi == 0 ) wm->screen_ppi = 110;
    win_zoom_init( wm );

    if( wm->screen_zoom > 1 )
    {
	wm->screen_xsize /= wm->screen_zoom;
	wm->screen_ysize /= wm->screen_zoom;
	wm->sdl_zoom_buffer = bmem_new( wm->screen_xsize * wm->screen_ysize * COLORLEN );
    }
    
    //Set window name:
    if( fs == 0 )
    {
	SDL_WM_SetCaption( wm->sdl_wm_windowname, wm->sdl_wm_windowname );
    }
    
    if( profile_get_int_value( KEY_NOCURSOR, -1, 0 ) != -1 ) SDL_ShowCursor( 0 );

    /*{
	SDL_SysWMinfo wm_info;
	if( SDL_GetWMInfo( &wm_info ) == 1 )
	{
	    if( wm_info.subsystem == SDL_SYSWM_X11 ) 
	    {
		Display* display;
		Window window;
		wm_info.info.x11.lock_func();
		display = wm_info.info.x11.display;
		window = wm_info.info.x11.window;
		Atom atom = XInternAtom( display, "_HILDON_DO_NOT_DISTURB", 1 );
		if( !atom ) 
		{
		    blog( "Unable to obtain _HILDON_DO_NOT_DISTURB. The \"noautolock\" option will only work on a Maemo 5 device.\n" );
		}
		else 
		{
		    int state = 1;
		    XChangeProperty( display, window, atom, XA_INTEGER, 32, PropModeReplace, (unsigned char*)&state, 1 );
		}
		wm_info.info.x11.unlock_func();
	    }
	}
    }*/
    
    SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
    
    wm->sdl_thread_initialized = 1;
    
    while( exit_request == 0 )
    {
	int key = 0;
	SDL_WaitEvent( &event );
	switch( event.type ) 
	{
	    case SDL_MOUSEBUTTONDOWN:
	    case SDL_MOUSEBUTTONUP:
		key = 0;
		switch( event.button.button )
		{
		    case SDL_BUTTON_LEFT: key = MOUSE_BUTTON_LEFT; break;
		    case SDL_BUTTON_MIDDLE: key = MOUSE_BUTTON_MIDDLE; break;
		    case SDL_BUTTON_RIGHT: key = MOUSE_BUTTON_RIGHT; break;
		    case SDL_BUTTON_WHEELUP: if( event.type == SDL_MOUSEBUTTONDOWN ) key = MOUSE_BUTTON_SCROLLUP; break;
		    case SDL_BUTTON_WHEELDOWN: if( event.type == SDL_MOUSEBUTTONDOWN ) key = MOUSE_BUTTON_SCROLLDOWN; break;
		}
		if( key )
		{
		    int x = event.button.x;
		    int y = event.button.y;
		    convert_real_window_xy( x, y, wm );
		    if( event.type == SDL_MOUSEBUTTONDOWN )
			send_event( 0, EVT_MOUSEBUTTONDOWN, mod, x, y, key, 0, 1024, 0, wm );
		    else
			send_event( 0, EVT_MOUSEBUTTONUP, mod, x, y, key, 0, 1024, 0, wm );
		    EVENT;
		}
		break;
	    case SDL_MOUSEMOTION:
		{
		    key = 0;
		    if( event.motion.state & SDL_BUTTON( SDL_BUTTON_LEFT ) ) key |= MOUSE_BUTTON_LEFT;
		    if( event.motion.state & SDL_BUTTON( SDL_BUTTON_MIDDLE ) ) key |= MOUSE_BUTTON_MIDDLE;
		    if( event.motion.state & SDL_BUTTON( SDL_BUTTON_RIGHT ) ) key |= MOUSE_BUTTON_RIGHT;
		    int x = event.motion.x;
		    int y = event.motion.y;
		    convert_real_window_xy( x, y, wm );
		    send_event( 0, EVT_MOUSEMOVE, mod, x, y, key, 0, 1024, 0, wm );
		    EVENT;
		}
		break;
	    case SDL_KEYDOWN:
	    case SDL_KEYUP:
		hide_mod = 0;
		key = event.key.keysym.sym;
		if( key > 255 )
		{
		    switch( key )
		    {
			case SDLK_UP: key = KEY_UP; break;
			case SDLK_DOWN: key = KEY_DOWN; break;
			case SDLK_LEFT: key = KEY_LEFT; break;
			case SDLK_RIGHT: key = KEY_RIGHT; break;
			case SDLK_INSERT: key = KEY_INSERT; break;
			case SDLK_HOME: key = KEY_HOME; break;
			case SDLK_END: key = KEY_END; break;
			case SDLK_PAGEUP: key = KEY_PAGEUP; break;
			case SDLK_PAGEDOWN: key = KEY_PAGEDOWN; break;
			case SDLK_F1: key = KEY_F1; break;
			case SDLK_F2: key = KEY_F2; break;
			case SDLK_F3: key = KEY_F3; break;
			case SDLK_F4: key = KEY_F4; break;
			case SDLK_F5: key = KEY_F5; break;
			case SDLK_F6: key = KEY_F6; break;
			case SDLK_F7: key = KEY_F7; break;
			case SDLK_F8: key = KEY_F8; break;
			case SDLK_F9: key = KEY_F9; break;
			case SDLK_F10: key = KEY_F10; break;
			case SDLK_F11: key = KEY_F11; break;
			case SDLK_F12: key = KEY_F12; break;
			case SDLK_CAPSLOCK: key = KEY_CAPS; break;
			case SDLK_RSHIFT:
			case SDLK_LSHIFT:
			    if( event.type == SDL_KEYDOWN ) mod |= EVT_FLAG_SHIFT; else mod &= ~EVT_FLAG_SHIFT;
			    key = KEY_SHIFT;
			    break;
			case SDLK_RCTRL:
			case SDLK_LCTRL:
			    if( event.type == SDL_KEYDOWN ) mod |= EVT_FLAG_CTRL; else mod &= ~EVT_FLAG_CTRL;
			    key = KEY_CTRL;
			    break;
			case SDLK_RALT:
			case SDLK_LALT:
			    if( event.type == SDL_KEYDOWN ) mod |= EVT_FLAG_ALT; else mod &= ~EVT_FLAG_ALT;
			    key = KEY_ALT;
			    break;
			case SDLK_MODE:
			    if( event.type == SDL_KEYDOWN ) mod |= EVT_FLAG_MODE; else mod &= ~EVT_FLAG_MODE;
			    key = 0;
			    break;
			case SDLK_KP0: key = '0'; break;
			case SDLK_KP1: key = '1'; break;
			case SDLK_KP2: key = '2'; break;
			case SDLK_KP3: key = '3'; break;
			case SDLK_KP4: key = '4'; break;
			case SDLK_KP5: key = '5'; break;
			case SDLK_KP6: key = '6'; break;
			case SDLK_KP7: key = '7'; break;
			case SDLK_KP8: key = '8'; break;
			case SDLK_KP9: key = '9'; break;
			case SDLK_KP_ENTER: key = KEY_ENTER; break;
			default: key += KEY_UNKNOWN; break;
		    }	    
		}
		else
		{
		    switch( key )
		    {
			case SDLK_RETURN: key = KEY_ENTER; break;
			case SDLK_DELETE: key = KEY_DELETE; break;
			case SDLK_BACKSPACE: key = KEY_BACKSPACE; break;
		    }
		}
#if defined(MAEMO)
		if( mod & EVT_FLAG_MODE )
		{
		    switch( key )
		    {
			case 'q': key = '1'; hide_mod = EVT_FLAG_MODE; break;
			case 'w': key = '2'; hide_mod = EVT_FLAG_MODE; break;
			case 'e': key = '3'; hide_mod = EVT_FLAG_MODE; break;
			case 'r': key = '4'; hide_mod = EVT_FLAG_MODE; break;
			case 't': key = '5'; hide_mod = EVT_FLAG_MODE; break;
			case 'y': key = '6'; hide_mod = EVT_FLAG_MODE; break;
			case 'u': key = '7'; hide_mod = EVT_FLAG_MODE; break;
			case 'i': key = '8'; hide_mod = EVT_FLAG_MODE; break;
			case 'o': key = '9'; hide_mod = EVT_FLAG_MODE; break;
			case 'p': key = '0'; hide_mod = EVT_FLAG_MODE; break;
			case 'Q': key = '1'; hide_mod = EVT_FLAG_MODE; break;
			case 'W': key = '2'; hide_mod = EVT_FLAG_MODE; break;
			case 'E': key = '3'; hide_mod = EVT_FLAG_MODE; break;
			case 'R': key = '4'; hide_mod = EVT_FLAG_MODE; break;
			case 'T': key = '5'; hide_mod = EVT_FLAG_MODE; break;
			case 'Y': key = '6'; hide_mod = EVT_FLAG_MODE; break;
			case 'U': key = '7'; hide_mod = EVT_FLAG_MODE; break;
			case 'I': key = '8'; hide_mod = EVT_FLAG_MODE; break;
			case 'O': key = '9'; hide_mod = EVT_FLAG_MODE; break;
			case 'P': key = '0'; hide_mod = EVT_FLAG_MODE; break;
			case KEY_LEFT: key = KEY_UP; hide_mod = EVT_FLAG_MODE; break;
			case KEY_RIGHT: key = KEY_DOWN; hide_mod = EVT_FLAG_MODE; break;
		    }
		}
#endif
		if( key != 0 )
		{
		    if( event.type == SDL_KEYDOWN || key == KEY_CAPS )
	    		send_event( 0, EVT_BUTTONDOWN, mod & ~hide_mod, 0, 0, key, event.key.keysym.scancode, 1024, event.key.keysym.unicode, wm );
		    else
	    		send_event( 0, EVT_BUTTONUP, mod & ~hide_mod, 0, 0, key, event.key.keysym.scancode, 1024, event.key.keysym.unicode, wm );
		    EVENT;
		}
		break;
	    case SDL_VIDEORESIZE:
		//blog( "RESIZE %d %d; ZOOM %d;\n", event.resize.w, event.resize.h, wm->screen_zoom );
    		pthread_mutex_lock( &wm->sdl_lock_mutex );
		SDL_SetVideoMode( 
		    event.resize.w, event.resize.h, COLORBITS,
                    SDL_HWSURFACE | SDL_RESIZABLE ); // Resize window
		wm->real_window_width = event.resize.w;
		wm->real_window_height = event.resize.h;
		wm->screen_xsize = event.resize.w / wm->screen_zoom;
		wm->screen_ysize = event.resize.h / wm->screen_zoom;
                if( wm->screen_angle & 1 )
                {
                    int temp = wm->screen_xsize;
                    wm->screen_xsize = wm->screen_ysize;
                    wm->screen_ysize = temp;
                }
		wm->sdl_zoom_buffer = bmem_resize( wm->sdl_zoom_buffer, wm->screen_xsize * wm->screen_ysize * COLORLEN );
		if( wm->root_win )
		{
		    int need_recalc = 0;
		    if( wm->root_win->x + wm->root_win->xsize > wm->screen_xsize ) 
		    {
			wm->root_win->xsize = wm->screen_xsize - wm->root_win->x;
			need_recalc = 1;
		    }
		    if( wm->root_win->y + wm->root_win->ysize > wm->screen_ysize ) 
		    {
			wm->root_win->ysize = wm->screen_ysize - wm->root_win->y;
			need_recalc = 1;
		    }
		    if( need_recalc ) recalc_regions( wm );
		}
		pthread_mutex_unlock( &wm->sdl_lock_mutex );
		send_event( wm->root_win, EVT_SCREENRESIZE, 0, 0, 0, 0, 0, 0, 0, wm );
		EVENT;
		break;
	    case SDL_QUIT:
		send_event( 0, EVT_QUIT, 0, 0, 0, 0, 0, 1024, 0, wm );
		EVENT;
		break;
	    case SDL_USEREVENT:
		if( event.user.code == 33 )
		{
		    exit_request = 1;
		}
		EVENT;
		break;
	}
    }
    
    blog( "SDL_Quit()...\n" );
    SDL_Quit();
    
thread_end:

    wm->sdl_thread_finished = 1;
    pthread_exit( 0 );
}

int device_start( const utf8_char* windowname, int xsize, int ysize, window_manager* wm )
{
    int retval = 0;
    
    if( XInitThreads() == 0 ) blog( "XInitThreads failed\n" );

    wm->sdl_frames_thread_exit_request = 0;
    wm->sdl_screen_control_thread_exit_request = 0;
    wm->sdl_zoom_buffer = 0;
    wm->sdl_thread_finished = 0;
    wm->sdl_thread_initialized = 0;
    wm->sdl_wm_flags = 0;
    wm->sdl_wm_windowname = 0;
    wm->sdl_wm_xsize = 0;
    wm->sdl_wm_ysize = 0;
        
    //Init the semaphore:
    wm->sdl_event_loop_sem = semget( (key_t)64911234, 1, 0666 | IPC_CREAT );
    if( wm->sdl_event_loop_sem == -1 )
    {
	blog( "Error in semget() (%d)\n", errno );
	return 1;
    }
    semctl( wm->sdl_event_loop_sem, 0, SETVAL, 0 );

    //Create SDL screen lock mutex:
    if( pthread_mutex_init( &wm->sdl_lock_mutex, 0 ) != 0 )
    {
	blog( "Can't create SDL screen lock mutex\n" );
	return 1;
    }
    
    xsize = profile_get_int_value( KEY_SCREENX, xsize, 0 );
    ysize = profile_get_int_value( KEY_SCREENY, ysize, 0 );
    
    wm->sdl_wm_windowname = windowname;
    wm->sdl_wm_xsize = xsize;
    wm->sdl_wm_ysize = ysize;
    wm->sdl_wm_flags = wm->flags;

    int err;
    
    if( profile_get_int_value( "noautolock", -1, 0 ) != -1 ) 
    {
#ifdef MAEMO
	err = bthread_create( &wm->sdl_screen_control_thread, &screen_control_thread, (void*)wm, 0 );
	if( err )
	{
	    blog( "bthread_create error %d\n", err );
	    return 1;
	}
#endif
    }

    //Create frames thread:
    err = bthread_create( &wm->sdl_frames_thread, &frames_thread, (void*)wm, 0 );
    if( err )
    {
	blog( "bthread_create error %d\n", err );
	return 1;
    }
    
    //Start event thread:
    if( pthread_create( &wm->sdl_evt_pth, NULL, event_thread, wm ) != 0 )
    {
	blog( "Can't create event thread\n" );
	return 1;
    }
    
    while( wm->sdl_thread_initialized == 0 ) time_sleep( 10 );
    if( wm->sdl_thread_initialized < 0 )
	return 1;
    
    return retval;
}

void device_end( window_manager* wm )
{
    SDL_Event quit_event;
    quit_event.type = SDL_USEREVENT;
    quit_event.user.code = 33;
    SDL_PushEvent( &quit_event );
    while( wm->sdl_thread_finished == 0 ) time_sleep( 10 );

    //Remove mutexes:
    blog( "Removing SDL mutexes...\n" );
    pthread_mutex_destroy( &wm->sdl_lock_mutex );
    
    //Stop the frames thread:
    wm->sdl_frames_thread_exit_request = 1;
    bthread_destroy( &wm->sdl_frames_thread, 1000 * 5 );

    if( profile_get_int_value( "noautolock", -1, 0 ) != -1 ) 
    {
#ifdef MAEMO
	wm->sdl_screen_control_thread_exit_request = 1;
	bthread_destroy( &wm->sdl_screen_control_thread, 1000 * 5 );
#endif
    }
    
    //Close the semaphore:
    semctl( wm->sdl_event_loop_sem, 0, IPC_RMID, 0 );
    
    bmem_free( wm->sdl_zoom_buffer );
}

void device_event_handler( window_manager* wm )
{
    if( wm->flags & WIN_INIT_FLAG_FULL_CPU_USAGE )
    {
	sched_yield();
    }
    else
    {
	if( wm->events_count == 0 )
	{
	    struct sembuf operation[ 1 ];
	    operation[ 0 ].sem_op  = -1; //while( semaphore == 0 ) {} semaphore--;
	    operation[ 0 ].sem_num = 0;
	    operation[ 0 ].sem_flg = SEM_UNDO;
	    semop( wm->sdl_event_loop_sem, operation, 1 );
	}
    }
}

void device_screen_lock( WINDOWPTR win, window_manager* wm )
{
    if( wm->screen_lock_counter == 0 )
    {
	pthread_mutex_lock( &wm->sdl_lock_mutex );
	if( wm->screen_zoom == 1 )
	{
	    if( SDL_MUSTLOCK( wm->sdl_screen ) ) 
	    {
		if( SDL_LockSurface( wm->sdl_screen ) < 0 ) 
		{
		    pthread_mutex_unlock( &wm->sdl_lock_mutex );
		    return;
		}
	    }
	    wm->fb = (COLORPTR)wm->sdl_screen->pixels;
	    wm->fb_offset = 0;
	    wm->fb_ypitch = wm->sdl_screen->pitch / COLORLEN;
	    wm->fb_xpitch = 1;
	}
	else 
	{
	    wm->fb = (COLORPTR)wm->sdl_zoom_buffer;
	    wm->fb_offset = 0;
	    wm->fb_ypitch = wm->real_window_width / wm->screen_zoom;
	    wm->fb_xpitch = 1;
	}
	switch( wm->screen_angle )
	{
	    case 1:
		//90:
		{
		    wm->fb_offset = wm->fb_ypitch * ( wm->screen_xsize - 1 );
	            int ttt = wm->fb_ypitch;
    		    wm->fb_ypitch = wm->fb_xpitch;
    		    wm->fb_xpitch = -ttt;
		}
		break;
	    case 2:
		//180:
		{
		    wm->fb_offset = wm->fb_ypitch * ( wm->screen_ysize - 1 ) + wm->fb_xpitch * ( wm->screen_xsize - 1 );
	    	    wm->fb_ypitch = -wm->fb_ypitch;
    		    wm->fb_xpitch = -wm->fb_xpitch;
		}
		break;
	    case 3:
		//270:
		{
		    wm->fb_offset = wm->fb_xpitch * ( wm->screen_ysize - 1 );
	            int ttt = wm->fb_ypitch;
	            wm->fb_ypitch = -wm->fb_xpitch;
	            wm->fb_xpitch = ttt;
		}
		break;
	}
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
	if( wm->screen_zoom == 1 )
	{
	    if( SDL_MUSTLOCK( wm->sdl_screen ) ) 
	    {
		SDL_UnlockSurface( wm->sdl_screen );
	    }
	}
	else 
	{
	}
	wm->fb = 0;
	pthread_mutex_unlock( &wm->sdl_lock_mutex );
    }
    
    /*if( win )
    {
	if( win->reg && win->reg->numRects )
    	{
    	    for( int r = 0; r < win->reg->numRects; r++ )
    	    {
        	int rx1 = win->reg->rects[ r ].left;
            	int rx2 = win->reg->rects[ r ].right;
            	int ry1 = win->reg->rects[ r ].top;
            	int ry2 = win->reg->rects[ r ].bottom;
		int xsize = rx2 - rx1;
		int ysize = ry2 - ry1;
		if( xsize > 0 && ysize > 0 )
		{
		    SDL_UpdateRect( wm->sdl_screen, rx1, ry1, xsize, ysize );
		}
    	    }
        }
    }*/
    
    if( wm->screen_lock_counter > 0 )
    {
	wm->screen_lock_counter--;
    }

    if( wm->screen_lock_counter > 0 )
	wm->screen_is_active = true;
    else
	wm->screen_is_active = false;
}

#ifndef OPENGL

void device_vsync( bool vsync, window_manager* wm )
{
}

void device_redraw_framebuffer( window_manager* wm )
{
    if( wm->screen_changed == 0 ) return;
    
    if( wm->flags & WIN_INIT_FLAG_FRAMEBUFFER )
    {
	if( wm->screen_lock_counter == 0 )
	{
    	    pthread_mutex_lock( &wm->sdl_lock_mutex );
    	    if( wm->screen_zoom == 2 )
    	    {
    		while( 1 )
        	{
            	    if( SDL_MUSTLOCK( wm->sdl_screen ) )
            	    {
                	if( SDL_LockSurface( wm->sdl_screen ) < 0 )
                	{
                    	    break;
                	}
            	    }
            	    COLORPTR s = (COLORPTR)wm->sdl_screen->pixels;
            	    uint fb_ypitch = wm->sdl_screen->pitch / COLORLEN;
            	    uint fb_xpitch = 1;
        	    COLORPTR src = (COLORPTR)wm->sdl_zoom_buffer;
            	    for( int y = 0; y < wm->real_window_height / 2; y++ )
            	    {
                	COLORPTR s2 = s;
                	for( int x = 0; x < wm->real_window_width / 2; x++ )
                	{
                    	    COLOR c = *src;
                    	    s2[ 0 ] = c;
                    	    s2[ 1 ] = c;
                    	    s2[ fb_ypitch ] = c;
                    	    s2[ fb_ypitch + 1 ] = c;
                    	    s2 += fb_xpitch * 2;
                    	    src++;
                	}
                	s += fb_ypitch * 2;
            	    }
            	    if( SDL_MUSTLOCK( wm->sdl_screen ) )
            	    {
                	SDL_UnlockSurface( wm->sdl_screen );
            	    }
            	    break;
        	}
    	    }
    	    SDL_UpdateRect( wm->sdl_screen, 0, 0, 0, 0 );
    	    wm->screen_changed = 0;
    	    pthread_mutex_unlock( &wm->sdl_lock_mutex );
	}
	else
	{
    	    blog( "Can't redraw the framebuffer: screen locked!\n" );
	}
    }
    else
    {
	//No framebuffer:
	wm->screen_changed = 0;
    }
}

void device_draw_line( int x1, int y1, int x2, int y2, COLOR color, window_manager* wm ) 
{
}

void device_draw_frect( int x, int y, int xsize, int ysize, COLOR color, window_manager* wm )
{
}

void device_draw_image(
    int dest_x, int dest_y,
    int dest_xs, int dest_ys,
    int src_x, int src_y,
    sundog_image* img,
    window_manager* wm )
{
}

#endif //!OPENGL
