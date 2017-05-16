/*
    log.cpp. Logger (thread safe)
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100% with global init/deinit

#include "core/core.h"

#ifdef ANDROID
    #include <android/log.h>
#endif

static bool g_blog_ready = false;
static bmutex g_blog_mutex;
static utf8_char* g_blog_file = 0;
static int g_blog_disable_counter = 0;
static utf8_char g_blog_buf[ 8192 ];

void blog_global_init( const utf8_char* filename )
{
    g_blog_file = 0;
    g_blog_disable_counter = 0;
    bmutex_init( &g_blog_mutex, 0 );
    utf8_char* name = bfs_make_filename( filename );
    if( name )
    {
        g_blog_file = (utf8_char*)strdup( name );
        bmem_free( name );
	if( 0 )
	{
	    FILE* f = fopen( g_blog_file, "rb" );
	    if( f )
	    {
		printf( "Previous log: =======\n" );
		while( 1 )
		{
		    int c = fgetc( f );
		    if( c < 0 ) break;
		    printf( "%c", c );
		}
		fclose( f );
		printf( "=====================\n" );
	    }
	}
	bfs_remove( g_blog_file );
    }
    g_blog_ready = true;
}

void blog_global_deinit( void )
{
    if( g_blog_ready == false ) return;
    bmutex_destroy( &g_blog_mutex );
    free( g_blog_file );
    g_blog_file = 0;
    g_blog_ready = false;
}

void blog_disable( void )
{
    if( g_blog_ready == false ) return;
    if( bmutex_lock( &g_blog_mutex ) ) return;
    g_blog_disable_counter++;
    bmutex_unlock( &g_blog_mutex );
}

void blog_enable( void )
{
    if( g_blog_ready == false ) return;
    if( bmutex_lock( &g_blog_mutex ) ) return;
    if( g_blog_disable_counter > 0 )
	g_blog_disable_counter--;
    bmutex_unlock( &g_blog_mutex );
}

const utf8_char* blog_get_file( void )
{
    return g_blog_file;
}

utf8_char* blog_get_latest( uint size )
{
    const utf8_char* log_file = blog_get_file();
    size_t log_size = bfs_get_file_size( log_file );
    if( log_size == 0 ) return 0;
    utf8_char* rv = (utf8_char*)bmem_new( size + 1 );
    if( rv == 0 ) return 0;
    rv[ 0 ] = 0;
    FILE* f = fopen( log_file, "rb" );
    if( f )
    {
	if( log_size >= size )
	{
	    fseek( f, log_size - size, SEEK_SET );
	    fread( rv, 1, size, f );
	    rv[ size ] = 0;
	}
	else
	{
	    fread( rv, 1, log_size, f );
	    rv[ log_size ] = 0;
	}
	fclose( f );
    }
    return rv;
}

void blog( const utf8_char* str, ... )
{
#ifdef NOLOG
    return;
#endif

    if( g_blog_ready == false )
    {
	va_list p;
	va_start( p, str );
	vprintf( str, p );
	va_end( p );
	return;
    }
    if( bmutex_lock( &g_blog_mutex ) ) return;

    while( 1 )
    {
	if( g_blog_disable_counter ) break;
	if( g_blog_file == 0 ) break;
    
	va_list p;
	va_start( p, str );

	vsprintf( g_blog_buf, str, p );
    
	va_end( p );
	
	//Save result:
	FILE* f = fopen( g_blog_file, "ab" );
	if( f )
	{
	    fwrite( g_blog_buf, 1, bmem_strlen( g_blog_buf ), f );
	    fclose( f );
	}
	printf( "%s", g_blog_buf );
#ifdef ANDROID
	__android_log_print( ANDROID_LOG_INFO, "native-activity", g_blog_buf );
#endif

	break;
    }
    
    bmutex_unlock( &g_blog_mutex );
}

void blog_show_error_report( void )
{
#ifndef NOGUI
#ifdef IPHONE
    char* log = 0;
    const utf8_char* log_file = blog_get_file();
    size_t log_size = 0;
    size_t log_msize = 0;
    FILE* f = fopen( log_file, "rb" );
    if( f )
    {
	while( 1 )
	{
	    int c = fgetc( f );
	    if( c < 0 )
	    {
		c = 0;
	    }
	    if( log == 0 )
	    {
		log = (char*)bmem_new( 1024 );
		log_msize = 1024;
	    }
	    if( log_size >= log_msize )
	    {
		log_msize += 1024;
		log = (char*)bmem_resize( log, log_msize );
	    }
	    log[ log_size ] = c;
	    log_size++;
	    if( c == 0 ) break;
	}
	fclose( f );
    }
    if( log )
    {
	utf8_char ts[ 256 ];
	sprintf( ts, "%s ERROR REPORT", user_window_name );
	send_mail( "nightradio@gmail.com", ts, log );
	bmem_free( log );
    }
#endif
#endif
}
