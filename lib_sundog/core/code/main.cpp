/*
    main.cpp. SunDog Engine main()
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

#include "core/core.h"

//################################
//## APPLICATION MAIN:          ##
//################################

#ifndef NOMAIN

int g_argc = 0;
utf8_char* g_argv[ 64 ];

SUNDOG_GLOBAL_VARS;

#if defined(WIN) || defined(WINCE)
void make_arguments( utf8_char* cmd_line )
{
    //Make standart argc and argv[] from windows lpszCmdLine:
    g_argv[ 0 ] = (utf8_char*)"prog";
    g_argc = 1;
    if( cmd_line && cmd_line[ 0 ] != 0 )
    {
        int str_ptr = 0;
        int space = 1;
	int len = 0;
	for( len = 0; ; len++ ) if( cmd_line[ len ] == 0 ) break;
	int r = 0;
	int w = 0;
	int string = 0;
	while( r <= len )
	{
	    utf8_char c = cmd_line[ r ];
	    if( c == '"' ) 
	    {
		string ^= 1;
		r++;
		continue;
	    }
	    if( string && c == ' ' ) c = 1;
	    cmd_line[ w ] = c;
	    r++;
	    w++;
	}
        for( int i = 0; i < len; i++ )
        {
    	    if( cmd_line[ i ] != ' ' )
    	    {
    		if( cmd_line[ i ] == 1 ) cmd_line[ i ] = ' ';
	        if( space == 1 )
	        {
	    	    g_argv[ g_argc ] = &cmd_line[ i ];
		    g_argc++;
		}
		space = 0;
	    }
	    else
	    {
		cmd_line[ i ] = 0;
	        space = 1;
	    }
	}
    }
}
#endif

#ifdef WIN
WCHAR g_cmd_line[ 2048 ];
utf8_char g_cmd_line_utf8[ 2048 ];
int APIENTRY WinMain( HINSTANCE hCurrentInst, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow )
{
    sundog_engine sd;
    bmem_set( &sd, sizeof( sd ), 0 );
    sd.hCurrentInst = hCurrentInst;
    sd.hPreviousInst = hPreviousInst; 
    sd.lpszCmdLine = lpszCmdLine;
    sd.nCmdShow = nCmdShow;
    mbstowcs( g_cmd_line, lpszCmdLine, 2047 );
    utf16_to_utf8( g_cmd_line_utf8, 2048, (const utf16_char*)g_cmd_line );
    make_arguments( g_cmd_line_utf8 );
    sd.argc = g_argc;
    sd.argv = g_argv;
    if( sundog_main( &sd, true ) == 0 )
	return sd.exit_code;
    else
	return -1;
}
#endif

#ifdef WINCE
utf8_char g_cmd_line_utf8[ 2048 ];
WCHAR g_window_name[ 256 ];
extern WCHAR* className; //defined in window manager (wm_wince.h)
int WINAPI WinMain( HINSTANCE hCurrentInst, HINSTANCE hPreviousInst, LPWSTR lpszCmdLine, int nCmdShow )
{
    sundog_engine sd;
    bmem_set( &sd, sizeof( sd ), 0 );
    sd.hCurrentInst = hCurrentInst;
    sd.hPreviousInst = hPreviousInst;
    sd.lpszCmdLine = lpszCmdLine;
    sd.nCmdShow = nCmdShow;
    utf16_to_utf8( g_cmd_line_utf8, 2048, (utf16_char*)lpszCmdLine );
    make_arguments( g_cmd_line_utf8 );
    utf8_to_utf16( (utf16_char*)g_window_name, 256, user_window_name );
    HWND wnd = FindWindow( className, g_window_name );
    if( wnd )
    {
        //Already opened:
        SetForegroundWindow( wnd ); //Make it foreground
        return 0;
    }
    sd.argc = g_argc;
    sd.argv = g_argv;
    if( sundog_main( &sd, true ) == 0 )
	return sd.exit_code;
    else
	return -1;
}
#endif

#if defined(UNIX) && !defined(OSX) && !defined(IPHONE) && !defined(ANDROID)
#include <pthread.h>
#include <signal.h>
sundog_engine g_sd;
void main_process_signal_handler( int sig )
{
    switch( sig )
    {
	case SIGINT:
	case SIGTERM:
	    win_exit_request( g_sd.wm );
	    break;
	default:
	    break;
    }
}
int main( int argc, char* argv[] )
{
    signal( SIGINT, main_process_signal_handler ); //Interrupt program
    signal( SIGTERM, main_process_signal_handler ); //Software termination signal
    bmem_set( &g_sd, sizeof( g_sd ), 0 );
    g_sd.argc = argc;
    g_sd.argv = argv;
    if( sundog_main( &g_sd, true ) == 0 )
	return g_sd.exit_code;
    else
	return 0;
}
#endif

int sundog_main( sundog_engine* sd, bool global_init )
{
main_begin:
    bool restart_request = false;
    int err_count = 0;
    
    if( global_init )
    {
	SUNDOG_GLOBAL_INIT();
    }

    //Set defaults:
#ifdef MAEMO
    profile_set_int_value( KEY_SCREENX, profile_get_int_value( KEY_SCREENX, 800, 0 ), 0 );
    profile_set_int_value( KEY_SCREENY, profile_get_int_value( KEY_SCREENY, 480, 0 ), 0 );
    profile_set_int_value( KEY_SOUNDBUFFER, profile_get_int_value( KEY_SOUNDBUFFER, 2048, 0 ), 0 );
    profile_set_str_value( KEY_AUDIODRIVER, profile_get_str_value( KEY_AUDIODRIVER, "sdl", 0 ), 0 );
    profile_set_int_value( KEY_FULLSCREEN, profile_get_int_value( KEY_FULLSCREEN, 1, 0 ), 0 );
    profile_set_int_value( KEY_ZOOM, profile_get_int_value( KEY_ZOOM, 2, 0 ), 0 );
    profile_set_int_value( KEY_PPI, profile_get_int_value( KEY_PPI, 240, 0 ), 0 );
    profile_set_int_value( KEY_SCALE, profile_get_int_value( KEY_SCALE, 130, 0 ), 0 );
    profile_set_int_value( KEY_NOCURSOR, profile_get_int_value( KEY_NOCURSOR, 1, 0 ), 0 );
    profile_set_int_value( KEY_NOAUTOLOCK, profile_get_int_value( KEY_NOAUTOLOCK, 1, 0 ), 0 );
#endif

    blog( "%s / %s %s\n", SUNDOG_VERSION, SUNDOG_DATE, SUNDOG_TIME );
    
    window_manager* wm = (window_manager*)bmem_new( sizeof( window_manager ) );
    bmem_zero( wm );
    COMPILER_MEMORY_BARRIER();
    sd->wm = wm;
    if( wm && win_init( user_window_name, user_window_xsize, user_window_ysize, user_window_flags, sd ) == 0 )
    {
        if( wm->exit_request == 0 )
        {
	    sundog_midi_init();
	    if( user_init( wm ) == 0 )
	    {
		while( 1 )
	    	{
	    	    sundog_event evt;
		    EVENT_LOOP_BEGIN( &evt, wm );
		    if( EVENT_LOOP_END( wm ) ) break;
		}
		user_close( wm );
		if( wm->restart_request ) restart_request = 1;
	    }
	    else
	    {
		err_count++;
	    }
	    sundog_midi_deinit();
	}
	win_close( wm ); //Close window manager
    }
    else
    {
	err_count++;
    }
    sd->wm = 0;
    COMPILER_MEMORY_BARRIER();
    bmem_free( wm );
    wm = 0;
	
    if( err_count > 0 )
    {
	blog_show_error_report();
    }
	
    if( global_init )
    {
	SUNDOG_GLOBAL_DEINIT();
    }
	
    if( restart_request ) goto main_begin;
	
    return err_count;
}

#endif // ... ifndef NOMAIN

//################################
//################################
//################################
