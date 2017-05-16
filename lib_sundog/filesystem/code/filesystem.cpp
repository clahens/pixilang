/*
    filesystem.cpp. Multiplatform file system (thread safe open/close)
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100% with global init/deinit

#include "core/core.h"
#include "../filesystem.h"

#ifdef UNIX
    #include <errno.h>
    #include <stdlib.h>
    #include <unistd.h> //for current dir
    #include <sys/stat.h> //mkdir
#ifndef ANDROID
    #include <ftw.h>
#endif
#endif

#ifdef ANDROID
    #include "../../various/android/sundog_bridge.h"
#endif

bfs_fd_struct* g_bfs_fd[ BFS_MAX_DESCRIPTORS ] = { 0 };
bmutex g_bfs_mutex;

bool g_bfs_cant_write_disk1 = 0;
uint g_disk_count = 0; //Number of local disks
utf8_char disk_names[ DISKNAME_SIZE * MAX_DISKS ]; //Disk names. Each name - 4 bytes (with NULL) (for example: "C:/", "H:/")

uint bfs_get_disk_count( void ) 
{ 
    return g_disk_count; 
}

const utf8_char* bfs_get_disk_name( uint n ) 
{ 
    if( n < bfs_get_disk_count() )
        return disk_names + ( DISKNAME_SIZE * n ); 
    else
	return "";
}

#include "filesystem_file_type.h"

#ifdef WIN
void bfs_refresh_disks( void )
{
    char temp[ 512 ];
    int len, p, dp, tmp;
    g_disk_count = 0;
    len = GetLogicalDriveStrings( 512, temp );
    for( dp = 0, p = 0; p < len; p++ )
    {
	tmp = ( g_disk_count * DISKNAME_SIZE ) + dp;
	char c = temp[ p ];
	if( c == 92 ) c = '/';
	if( c > 0x60 && c < 0x7B )
	    c -= 0x20; //Make it capital
	disk_names[ tmp ] = c;
	if( temp[ p ] == 0 ) { g_disk_count++; dp = 0; continue; }
	dp++;
    }
}
uint bfs_get_current_disk( void )
{
    int rv = 0;
    utf8_char cur_dir[ MAX_DIR_LEN ];
    GetCurrentDirectory( MAX_DIR_LEN, cur_dir );
    if( cur_dir[ 0 ] > 0x60 && cur_dir[ 0 ] < 0x7B )
	cur_dir[ 0 ] -= 0x20; //Make it capital
    for( int i = 0; i < g_disk_count; i++ )
    {
	const utf8_char* disk_name = bfs_get_disk_name( i );
	if( disk_name && disk_name[ 0 ] == cur_dir[ 0 ] ) 
	{
	    rv = i;
	    break;
	}
    }
    return rv;
}
utf8_char g_current_path[ MAX_DIR_LEN ] = { 0 };
const utf8_char* bfs_get_work_path( void )
{
    if( g_bfs_cant_write_disk1 ) return bfs_get_conf_path();
    utf16_char ts_utf16[ MAX_DIR_LEN ];
    GetCurrentDirectoryW( MAX_DIR_LEN, (wchar_t*)ts_utf16 );
    utf16_to_utf8( g_current_path, MAX_DIR_LEN, (const utf16_char*)ts_utf16 );
    size_t len = bmem_strlen( g_current_path );
    for( size_t i = 0; i < len; i++ ) //Make "/mydir" from "\mydir"
    {
        if( g_current_path[ i ] == 92 ) g_current_path[ i ] = '/';
    }
    g_current_path[ len ] = '/';
    g_current_path[ len + 1 ] = 0;
    return g_current_path;
}
utf8_char g_user_path[ MAX_DIR_LEN ] = { 0 };
const utf8_char* bfs_get_conf_path( void )
{
    utf16_char ts_utf16[ MAX_DIR_LEN ];
    SHGetFolderPathW( NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, (wchar_t*)ts_utf16 );
    utf16_to_utf8( g_user_path, MAX_DIR_LEN, (const utf16_char*)ts_utf16 );
    size_t len = bmem_strlen( g_user_path );
    for( size_t i = 0; i < len; i++ ) //Make "/mydir" from "\mydir"
	if( g_user_path[ i ] == 92 ) g_user_path[ i ] = '/';
    g_user_path[ len ] = '/';
    g_user_path[ len + 1 ] = 0;
    return g_user_path;
}
utf8_char g_temp_path[ MAX_DIR_LEN ] = { 0 };
const utf8_char* bfs_get_temp_path( void )
{
    utf16_char ts_utf16[ MAX_DIR_LEN ];
    GetTempPathW( MAX_DIR_LEN, (wchar_t*)ts_utf16 );
    utf16_to_utf8( g_temp_path, MAX_DIR_LEN, (const utf16_char*)ts_utf16 );
    size_t len = bmem_strlen( g_temp_path );
    for( size_t i = 0; i < len; i++ ) //Make "/mydir" from "\mydir"
        if( g_temp_path[ i ] == 92 ) g_temp_path[ i ] = '/';
    g_temp_path[ len ] = '/';
    g_temp_path[ len + 1 ] = 0;
    return g_temp_path;
}
#endif
#ifdef UNIX
void bfs_refresh_disks( void )
{
    g_disk_count = 1;
    disk_names[ 0 ] = '/';
    disk_names[ 1 ] = 0;
}
uint bfs_get_current_disk( void )
{
    return 0;
}
#if defined(IPHONE) || defined(OSX)
char* g_osx_docs_path = 0;
char* g_osx_caches_path = 0;
char* g_osx_tmp_path = 0;
const utf8_char* bfs_get_work_path( void ) { return (utf8_char*)g_osx_docs_path; }
const utf8_char* bfs_get_conf_path( void ) { return (utf8_char*)g_osx_caches_path; }
const utf8_char* bfs_get_temp_path( void ) { return (utf8_char*)g_osx_tmp_path; }
#else
#ifdef ANDROID
const utf8_char* bfs_get_work_path( void ) { if( g_android_files_ext_path == 0 ) return ""; else return g_android_files_ext_path; }
const utf8_char* bfs_get_conf_path( void ) { if( g_android_files_int_path == 0 ) return ""; else return g_android_files_int_path; }
const utf8_char* bfs_get_temp_path( void ) { if( g_android_cache_int_path == 0 ) return ""; else return g_android_cache_int_path; }
#else
utf8_char g_current_path[ MAX_DIR_LEN ] = { 0 };
const utf8_char* bfs_get_work_path( void )
{
    if( g_bfs_cant_write_disk1 ) return bfs_get_conf_path();
    if( g_current_path[ 0 ] == 0 )
    {
	getcwd( g_current_path, MAX_DIR_LEN - 2 );
	size_t s = bmem_strlen( g_current_path );
	g_current_path[ s ] = '/';
	g_current_path[ s + 1 ] = 0;
    }
    return g_current_path;
}
utf8_char g_user_path[ MAX_DIR_LEN ] = { 0 };
const utf8_char* bfs_get_conf_path( void )
{
    if( g_user_path[ 0 ] == 0 )
    {
	utf8_char* user_p = getenv( "HOME" );
	utf8_char* cfg_p = getenv( "XDG_CONFIG_HOME" );
	g_user_path[ 0 ] = 0;
	if( cfg_p )
	{
	    bmem_strcat( g_user_path, sizeof( g_user_path ), (const utf8_char*)cfg_p );
	    bmem_strcat( g_user_path, sizeof( g_user_path ), "/" );
	}
	else
	{
	    bmem_strcat( g_user_path, sizeof( g_user_path ), (const utf8_char*)user_p );
	    bmem_strcat( g_user_path, sizeof( g_user_path ), "/.config/" );
	}
	bmem_strcat( g_user_path, sizeof( g_user_path ), user_window_name_short );
	bool dir_created = false;
	int err = mkdir( g_user_path, S_IRWXU | S_IRWXG | S_IRWXO );
	if( err == 0 ) dir_created = true;
	if( err != 0 )
	{
	    if( errno == EEXIST ) err = 0;
	}
	if( err != 0 )
	{
	    //Can't create the new config folder for our app; use default HOME directory:
	    g_user_path[ 0 ] = 0;
	    bmem_strcat( g_user_path, sizeof( g_user_path ), (const utf8_char*)user_p );
	    bmem_strcat( g_user_path, sizeof( g_user_path ), "/" );
	}
	else
	{
	    bmem_strcat( g_user_path, sizeof( g_user_path ), "/" );
	    if( dir_created )
	    {
		//HACK!!! TRANSITION FROM THE PREVIOUS FILESYSTEM VERSION:
		printf( "Copying old config files from the home user folder to %s ...\n", g_user_path );
		utf8_char* ts = (utf8_char*)bmem_new( 8000 );
		if( bmem_strcmp( user_window_name_short, "SunVox" ) == 0 )
		{
		    sprintf( ts, "mv -f ~/sunvox_config* %s", g_user_path );
		    system( ts );
		    sprintf( ts, "mv -f ~/.sunvox* %s", g_user_path );
		    system( ts );
		    sprintf( ts, "mv -f ~/.sundog* %s", g_user_path );
		    system( ts );
		}
		else
		{
		    sprintf( ts, "mv -f ~/pixilang_config* %s", g_user_path );
		    system( ts );
		    sprintf( ts, "mv -f ~/.pixilang* %s", g_user_path );
		    system( ts );
		    sprintf( ts, "mv -f ~/.virtual_ans* %s", g_user_path );
		    system( ts );
		    sprintf( ts, "mv -f ~/.pxtracker* %s", g_user_path );
		    system( ts );
		    sprintf( ts, "mv -f ~/.pixivisor* %s", g_user_path );
		    system( ts );
		    sprintf( ts, "mv -f ~/.phonopaper* %s", g_user_path );
		    system( ts );
		    sprintf( ts, "mv -f ~/.nosc* %s", g_user_path );
		    system( ts );
		}
		bmem_free( ts );
		//!!!!!!!
	    }
	}
    }
    return g_user_path;
}
const utf8_char* bfs_get_temp_path( void )
{
    return "/tmp/";
}
#endif //not ANDROID
#endif //not IPHONE
#endif
#ifdef WINCE
void bfs_refresh_disks( void )
{
    g_disk_count = 1;
    disk_names[ 0 ] = '/';
    disk_names[ 1 ] = 0;
}
uint bfs_get_current_disk( void )
{
    return 0;
}
const utf8_char* bfs_get_work_path( void )
{
    return (utf8_char*)"";
}
utf8_char g_user_path[ MAX_DIR_LEN ] = { 0 };
const utf8_char* bfs_get_conf_path( void )
{
    int a;
    utf16_char ts_utf16[ MAX_DIR_LEN ];
    SHGetSpecialFolderPath( NULL, (wchar_t*)ts_utf16, CSIDL_APPDATA, 1 );
    utf16_to_utf8( g_user_path, MAX_DIR_LEN, (const utf16_char*)ts_utf16 );
    for( a = 0; a < MAX_DIR_LEN; a++ ) //Make "/mydir" from "\mydir"
    {
        if( g_user_path[ a ] == 92 ) g_user_path[ a ] = '/';
    }
    a = bmem_strlen( g_user_path );
    g_user_path[ a ] = '/';
    g_user_path[ a + 1 ] = 0;
    return g_user_path;
}
utf8_char g_temp_path[ MAX_DIR_LEN ] = { 0 };
const utf8_char* bfs_get_temp_path( void )
{
    int a;
    utf16_char ts_utf16[ MAX_DIR_LEN ];
    GetTempPathW( MAX_DIR_LEN, (wchar_t*)ts_utf16 );
    utf16_to_utf8( g_temp_path, MAX_DIR_LEN, (const utf16_char*)ts_utf16 );
    for( a = 0; a < MAX_DIR_LEN; a++ ) //Make "/mydir" from "\mydir"
    {
        if( g_temp_path[ a ] == 92 ) g_temp_path[ a ] = '/';
    }
    a = bmem_strlen( g_temp_path );
    g_temp_path[ a ] = '/';
    g_temp_path[ a + 1 ] = 0;
    return g_temp_path;
}
#endif

//***********************************************************
//Main multiplatform functions:******************************
//***********************************************************

#define WIN_MAKE_LONG_PATH( TYPE, NAME, LEN ) \
    if( LEN >= MAX_PATH ) \
    { \
        utf8_char* name_ = (utf8_char*)bmem_new( LEN + 4 ); \
        name_[ 0 ] = '\\'; \
        name_[ 1 ] = '\\'; \
        name_[ 2 ] = '?'; \
        name_[ 3 ] = '\\'; \
        bmem_copy( name_ + 4, (void*)NAME, LEN ); \
        bmem_free( (void*)NAME ); \
        NAME = (TYPE)name_; \
        LEN += 4; \
    }

void bfs_global_init( void )
{
    g_disk_count = 0;
    g_bfs_fd[ 0 ] = 0;

    bmutex_init( &g_bfs_mutex, 0 );
    bfs_refresh_disks();
    bmem_set( g_bfs_fd, sizeof( void* ) * BFS_MAX_DESCRIPTORS, 0 );

#if defined(LINUX) || defined(OSX) || defined(WIN)
    g_bfs_cant_write_disk1 = 0;
    bfs_file f = bfs_open( "1:/file_write_test", "wb" );
    if( f )
    {
	bfs_close( f );
    }
    else
    {
	//Can't write to the 1:/
	g_bfs_cant_write_disk1 = 1;
    }
    bfs_remove( "1:/file_write_test" );
#endif

    bfs_fd_struct* fd = (bfs_fd_struct*)bmem_new( sizeof( bfs_fd_struct ) );
    fd->filename = 0;
    fd->f = (void*)stdin;
    fd->type = BFS_FILE_NORMAL;
    g_bfs_fd[ BFS_STDIN - 1 ] = fd;
    fd = (bfs_fd_struct*)bmem_new( sizeof( bfs_fd_struct ) );
    fd->filename = 0;
    fd->f = (void*)stdout;
    fd->type = BFS_FILE_NORMAL;
    g_bfs_fd[ BFS_STDOUT - 1 ] = fd;
    fd = (bfs_fd_struct*)bmem_new( sizeof( bfs_fd_struct ) );
    fd->filename = 0;
    fd->f = (void*)stderr;
    fd->type = BFS_FILE_NORMAL;
    g_bfs_fd[ BFS_STDERR - 1 ] = fd;

    bfs_get_work_path();
    bfs_get_conf_path();
    bfs_get_temp_path();
}

void bfs_global_deinit( void )
{
    bmutex_destroy( &g_bfs_mutex );
    
    bmem_free( g_bfs_fd[ BFS_STDIN - 1 ] );
    bmem_free( g_bfs_fd[ BFS_STDOUT - 1 ] );
    bmem_free( g_bfs_fd[ BFS_STDERR - 1 ] );
}

utf8_char* bfs_make_filename( const utf8_char* filename )
{
    utf8_char* rv = 0;

    if( filename == 0 ) return 0;
    
    if( bmem_strlen( filename ) >= 3 && 
	filename[ 0 ] > '0' && 
	filename[ 0 ] <= '9' &&
	filename[ 1 ] == ':' && 
	filename[ 2 ] == '/' )
    {
	//Add specific path to the name:
	const utf8_char* path = 0;
	switch( filename[ 0 ] - '0' )
	{
	    case 1: path = bfs_get_work_path(); break;
	    case 2: path = bfs_get_conf_path(); break;
	    case 3: path = bfs_get_temp_path(); break;
	}
	if( path )
	{
	    rv = (utf8_char*)bmem_new( bmem_strlen( path ) + ( bmem_strlen( filename ) - 3 ) + 1 );
	    if( rv == 0 ) return 0;
	    rv[ 0 ] = 0;
	    bmem_strcat_resize( rv, path );
	    bmem_strcat_resize( rv, filename + 3 );
	}
    }
    
    if( rv == 0 )
    {
	rv = (utf8_char*)bmem_new( bmem_strlen( filename ) + 1 );
	if( rv == 0 ) return 0;
	rv[ 0 ] = 0;
	bmem_strcat_resize( rv, filename );
    }
    
    return rv;
}

const utf8_char* bfs_get_filename_without_dir( const utf8_char* filename )
{
    int p = (int)bmem_strlen( filename ) - 1;
    while( p >= 0 )
    {
	utf8_char c = filename[ p ];
	if( c == '/' || c == '\\' ) break;
	p--;
    }
    p++;
    return filename + p;
}

const utf8_char* bfs_get_filename_extension( const utf8_char* filename )
{
    int p = (int)bmem_strlen( filename ) - 1;
    while( p >= 0 )
    {
	utf8_char c = filename[ p ];
	if( c == '.' ) break;
	p--;
    }
    p++;
    return filename + p;
}

bfs_fd_type bfs_get_type( bfs_file f )
{
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	return g_bfs_fd[ f ]->type;
    }
    else 
    {
	return BFS_FILE_NORMAL;
    }
}

void* bfs_get_data( bfs_file f )
{
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	return g_bfs_fd[ f ]->virt_file_data;
    }
    else 
    {
	return 0;
    }
}

size_t bfs_get_data_size( bfs_file f )
{
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	return g_bfs_fd[ f ]->virt_file_size;
    }
    else 
    {
	return 0;
    }
}

void bfs_set_user_data( bfs_file f, size_t user_data )
{
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	g_bfs_fd[ f ]->user_data = user_data;
    }
}

size_t bfs_get_user_data( bfs_file f )
{
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	return g_bfs_fd[ f ]->user_data;
    }
    else 
    {
	return 0;
    }
}

bfs_file bfs_open_in_memory( void* data, size_t size )
{
    int a;
    
    bmutex_lock( &g_bfs_mutex );
    
    for( a = 0; a < BFS_MAX_DESCRIPTORS; a++ ) if( g_bfs_fd[ a ] == 0 ) break;
    if( a == BFS_MAX_DESCRIPTORS ) 
    {
	bmutex_unlock( &g_bfs_mutex );
	return 0; //No free descriptors
    }
    //Create new descriptor:
    g_bfs_fd[ a ] = (bfs_fd_struct*)bmem_new( sizeof( bfs_fd_struct ) );
    
    bmutex_unlock( &g_bfs_mutex );
    
    bmem_set( g_bfs_fd[ a ], sizeof( bfs_fd_struct ), 0 );

    g_bfs_fd[ a ]->type = BFS_FILE_IN_MEMORY;
    g_bfs_fd[ a ]->virt_file_data = (char*)data;
    g_bfs_fd[ a ]->virt_file_size = size;

    return a + 1;
}

bfs_file bfs_open( const utf8_char* filename, const utf8_char* filemode )
{
    if( filename == 0 ) return 0;
    
    int a;
    
    bmutex_lock( &g_bfs_mutex );
    
    for( a = 0; a < BFS_MAX_DESCRIPTORS; a++ ) if( g_bfs_fd[ a ] == 0 ) break;
    if( a == BFS_MAX_DESCRIPTORS ) 
    {
	bmutex_unlock( &g_bfs_mutex );
        return 0; //No free descriptors
    }
    //Create new descriptor:
    g_bfs_fd[ a ] = (bfs_fd_struct*)bmem_new( sizeof( bfs_fd_struct ) );
    bfs_fd_struct* fd = g_bfs_fd[ a ];
    
    bmutex_unlock( &g_bfs_mutex );
    
    bmem_set( fd, sizeof( bfs_fd_struct ), 0 );
    
    fd->filename = bfs_make_filename( filename );
    if( fd->filename == 0 ) 
    {
	fd->f = 0;
    }
    else
    {
	//Check the filename. Is it some archive with packed filesystem?
	size_t namelen = bmem_strlen( fd->filename );
	bfs_file packed_fs = 0;
	utf8_char* dptr = 0;
	if( namelen > 5 )
	{
	    if( fd->filename[ 0 ] == 'v' &&
		fd->filename[ 1 ] == 'f' &&
		fd->filename[ 2 ] == 's' )
	    {
		dptr = bmem_strstr( fd->filename, ":/" );
		if( dptr )
		{
		    utf8_char num[ 16 ];
		    size_t i = 0;
		    for( i = 3; i < namelen && i < 15; i++ )
		    {
			utf8_char c = fd->filename[ i ];
			num[ i - 3 ] = c;
			if( c == ':' ) break;
		    }
		    num[ i - 3 ] = 0;
		    packed_fs = string_to_int( num );
		}
	    }
	}
	if( packed_fs )
	{
	    if( 1 )
	    {
		//Our file is in the TAR archive:
		utf8_char next_file[ 100 ];
		utf8_char temp[ 8 * 3 ];
		bfs_rewind( packed_fs );
		while( 1 )
		{
		    if( bfs_read( next_file, 1, 100, packed_fs ) != 100 ) break;
		    bfs_read( temp, 1, 8 * 3, packed_fs );
		    bfs_read( temp, 1, 12, packed_fs );
		    temp[ 12 ] = 0;
		    size_t filelen = 0;
		    for( int i = 0; i < 12; i++ )
		    {
			//Convert from OCT string to integer:
			if( temp[ i ] >= '0' && temp[ i ] <= '9' ) 
			{
			    filelen *= 8;
			    filelen += temp[ i ] - '0';
			}
		    }
		    bfs_seek( packed_fs, 376, 1 );
		    if( bmem_strcmp( next_file, dptr + 2 ) == 0 )
		    {
			//File found:
			fd->virt_file_data = (char*)bmem_new( filelen );
			fd->virt_file_data_autofree = 1;
			bfs_read( fd->virt_file_data, 1, filelen, packed_fs );
			fd->virt_file_size = filelen;
			fd->type = BFS_FILE_IN_MEMORY;
			break;
		    }
		    else
		    {
			if( filelen & 511 )
			    filelen += 512 - ( filelen & 511 );
			bfs_seek( packed_fs, filelen, 1 );
		    }
		}
	    }
	}
	if( fd->type == BFS_FILE_NORMAL )
	{
	    //Not the archive. Normal file:
#if defined(UNIX)
	    fd->f = (void*)fopen( fd->filename, filemode );
#ifdef UNIX
	    //if( fd->f == 0 )
		//blog( "Can't open file %s: %s\n", filename, strerror( errno ) );
#endif
#endif
#if defined(WIN) || defined(WINCE)
	    size_t len = bmem_strlen( fd->filename ) + 1;
	    WIN_MAKE_LONG_PATH( utf8_char*, fd->filename, len );
	    utf16_char* ts = (utf16_char*)bmem_new( len * 2 );
	    utf16_char ts2[ 16 ];
	    utf8_to_utf16( ts, len, fd->filename );
	    utf8_to_utf16( ts2, 16, filemode );
	    utf16_unix_slash_to_windows( ts );	
	    fd->f = (void*)_wfopen( (const wchar_t*)ts, (const wchar_t*)ts2 );
	    //if( fd->f == 0 )
		//blog( "Can't open file %s: %s\n", filename, strerror( errno ) );
	    bmem_free( ts );
#endif
	}
    }
    if( fd->type == BFS_FILE_NORMAL && fd->f == 0 )
    {
	//File not found:
	bfs_close( a + 1 );
	a = -1;
    }

    return a + 1;
}

int bfs_close( bfs_file f )
{
    int retval = 0;
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	if( g_bfs_fd[ f ]->filename ) bmem_free( g_bfs_fd[ f ]->filename );
	if( g_bfs_fd[ f ]->f )
	{
	    //Close standard file:
	    retval = (int)fclose( (FILE*)g_bfs_fd[ f ]->f );
	}
	if( g_bfs_fd[ f ]->virt_file_data_autofree ) 
	    bmem_free( g_bfs_fd[ f ]->virt_file_data );
	bmem_free( g_bfs_fd[ f ] );
	g_bfs_fd[ f ] = 0;
    }
    return retval;
}

void bfs_rewind( bfs_file f )
{
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	if( g_bfs_fd[ f ]->f && g_bfs_fd[ f ]->type == BFS_FILE_NORMAL )
	{
	    //Standard file:
	    fseek( (FILE*)g_bfs_fd[ f ]->f, 0, 0 );
	}
	else
	{
	    g_bfs_fd[ f ]->virt_file_ptr = 0;
	}
    }
}

int bfs_getc( bfs_file f )
{
    int retval = 0;
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	if( g_bfs_fd[ f ]->f && g_bfs_fd[ f ]->type == BFS_FILE_NORMAL )
	{
	    //Standard file:
	    retval = getc( (FILE*)g_bfs_fd[ f ]->f );
	}
	else
	{
	    if( g_bfs_fd[ f ]->virt_file_ptr < g_bfs_fd[ f ]->virt_file_size )
	    {
		retval = (int)( (uchar)g_bfs_fd[ f ]->virt_file_data[ g_bfs_fd[ f ]->virt_file_ptr ] );
		g_bfs_fd[ f ]->virt_file_ptr++;
	    }
	    else
	    {
		retval = -1;
	    }
	}
    }
    return retval;
}

size_t bfs_tell( bfs_file f )
{
    size_t retval = 0;
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	if( g_bfs_fd[ f ]->f && g_bfs_fd[ f ]->type == BFS_FILE_NORMAL )
	{
	    //Standard file:
	    retval = ftell( (FILE*)g_bfs_fd[ f ]->f );
	}
	else
	{
	    retval = g_bfs_fd[ f ]->virt_file_ptr;
	}
    }
    return retval;
}

int bfs_seek( bfs_file f, long offset, int access )
{
    int retval = 0;
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	if( g_bfs_fd[ f ]->f && g_bfs_fd[ f ]->type == BFS_FILE_NORMAL )
	{
	    //Standard file:
	    retval = fseek( (FILE*)g_bfs_fd[ f ]->f, offset, access );
	}
	else
	{
	    if( access == 0 ) g_bfs_fd[ f ]->virt_file_ptr = offset;
	    if( access == 1 ) g_bfs_fd[ f ]->virt_file_ptr += offset;
	    if( access == 2 ) g_bfs_fd[ f ]->virt_file_ptr = g_bfs_fd[ f ]->virt_file_size + offset;
	}
    }
    return retval;
}

int bfs_eof( bfs_file f )
{
    int retval = 0;
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	if( g_bfs_fd[ f ]->f && g_bfs_fd[ f ]->type == BFS_FILE_NORMAL )
	{
	    //Standard file:
	    retval = feof( (FILE*)g_bfs_fd[ f ]->f );
	}
	else
	{
	    if( g_bfs_fd[ f ]->virt_file_ptr >= g_bfs_fd[ f ]->virt_file_size ) retval = 1;
	}
    }
    return retval;
}

int bfs_flush( bfs_file f )
{
    int retval = 0;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	if( g_bfs_fd[ f ]->f && g_bfs_fd[ f ]->type == BFS_FILE_NORMAL )
	{
	    //Standard file:
	    retval = fflush( (FILE*)g_bfs_fd[ f ]->f );
	}
    }
    return retval;
}

size_t bfs_read( void* ptr, size_t el_size, size_t elements, bfs_file f )
{
    size_t retval = 0;
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	if( g_bfs_fd[ f ]->f && g_bfs_fd[ f ]->type == BFS_FILE_NORMAL )
	{
	    //Standard file:
	    retval = fread( ptr, el_size, elements, (FILE*)g_bfs_fd[ f ]->f );
	}
	else
	{
	    if( g_bfs_fd[ f ]->virt_file_data )
	    {
		size_t size = el_size * elements;
		if( g_bfs_fd[ f ]->virt_file_ptr + size > g_bfs_fd[ f ]->virt_file_size )
		    size = g_bfs_fd[ f ]->virt_file_size - g_bfs_fd[ f ]->virt_file_ptr;
		if( (signed)size < 0 ) size = 0;
		if( (signed)size > 0 )
		    bmem_copy( ptr, g_bfs_fd[ f ]->virt_file_data + g_bfs_fd[ f ]->virt_file_ptr, size );
		g_bfs_fd[ f ]->virt_file_ptr += size;
		retval = size / el_size;
	    }
	}
    }
    return retval;
}

size_t bfs_write( const void* ptr, size_t el_size, size_t elements, bfs_file f )
{
    size_t retval = 0;
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	if( g_bfs_fd[ f ]->f && g_bfs_fd[ f ]->type == BFS_FILE_NORMAL )
	{
	    //Standard file:
	    retval = fwrite( ptr, el_size, elements, (FILE*)g_bfs_fd[ f ]->f );
	}
	else
	{
	    if( g_bfs_fd[ f ]->virt_file_data )
	    {
		size_t size = el_size * elements;
		size_t new_size = g_bfs_fd[ f ]->virt_file_ptr + size;
		if( new_size > g_bfs_fd[ f ]->virt_file_size )
		{
		    if( g_bfs_fd[ f ]->type == BFS_FILE_IN_MEMORY )
		    {
			size_t real_size = bmem_get_size( g_bfs_fd[ f ]->virt_file_data );
			if( new_size > real_size ) g_bfs_fd[ f ]->virt_file_data = (char*)bmem_resize( g_bfs_fd[ f ]->virt_file_data, new_size + 4096 );
			if( g_bfs_fd[ f ]->virt_file_data == 0 )
			    size = 0;
			g_bfs_fd[ f ]->virt_file_size = new_size;
		    }
		    else 
		    {
			size = g_bfs_fd[ f ]->virt_file_size - g_bfs_fd[ f ]->virt_file_ptr;
		    }
		}
		if( (signed)size > 0 )
		    bmem_copy( g_bfs_fd[ f ]->virt_file_data + g_bfs_fd[ f ]->virt_file_ptr, ptr, size );
		g_bfs_fd[ f ]->virt_file_ptr += size;
		retval = size / el_size;
	    }
	}
    }
    return retval;
}

int bfs_putc( int val, bfs_file f )
{
    int rv = -1;
    f--;
    if( (unsigned)f < BFS_MAX_DESCRIPTORS && g_bfs_fd[ f ] )
    {
	if( g_bfs_fd[ f ]->f && g_bfs_fd[ f ]->type == BFS_FILE_NORMAL )
	{
	    //Standard file:
	    rv = fputc( val, (FILE*)g_bfs_fd[ f ]->f );
	}
	else
	{
	    if( g_bfs_fd[ f ]->virt_file_data )
	    {
		if( g_bfs_fd[ f ]->virt_file_ptr < g_bfs_fd[ f ]->virt_file_size )
		{
		    g_bfs_fd[ f ]->virt_file_data[ g_bfs_fd[ f ]->virt_file_ptr ] = (char)val;
		    g_bfs_fd[ f ]->virt_file_ptr++;
		    rv = val;
		}
		else 
		{
		    if( g_bfs_fd[ f ]->type == BFS_FILE_IN_MEMORY )
		    {
			size_t new_size = g_bfs_fd[ f ]->virt_file_ptr + 1;
			size_t real_size = bmem_get_size( g_bfs_fd[ f ]->virt_file_data );
			if( new_size > real_size ) g_bfs_fd[ f ]->virt_file_data = (char*)bmem_resize( g_bfs_fd[ f ]->virt_file_data, new_size + 4096 );
			if( g_bfs_fd[ f ]->virt_file_data )
			{
			    g_bfs_fd[ f ]->virt_file_data[ g_bfs_fd[ f ]->virt_file_ptr ] = (char)val;
			    g_bfs_fd[ f ]->virt_file_ptr++;
			    g_bfs_fd[ f ]->virt_file_size = new_size;
			    rv = val;
			}
		    }
		}
	    }
	}
    }
    return rv;
}

int bfs_remove( const utf8_char* filename )
{
    int retval = 0;
    const utf8_char* name = bfs_make_filename( filename );
    if( name == 0 ) return -1;
#if defined(WIN) || defined(WINCE)
    size_t len = bmem_strlen( name ) + 1;
    WIN_MAKE_LONG_PATH( const utf8_char*, name, len );
    utf16_char* ts = (utf16_char*)bmem_new( ( len + 1 ) * 2 );
    utf8_to_utf16( ts, len, name );
    utf16_unix_slash_to_windows( ts );
    uint attr = GetFileAttributesW( (const WCHAR*)ts );
    if( attr == INVALID_FILE_ATTRIBUTES )
    {
	retval = 1;
    }
    else
    {
	if( attr & FILE_ATTRIBUTE_DIRECTORY )
	{
	    ts[ bmem_strlen_utf16( ts ) + 1 ] = 0; //Double null terminated
	    SHFILEOPSTRUCTW file_op = {
		0,
		FO_DELETE,
		(const WCHAR*)ts,
		0,
		FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
    		0,
    		0,
    		0
	    };
	    retval = SHFileOperationW( &file_op );
	}
	else
	{
	    if( DeleteFileW( (const WCHAR*)ts ) == 0 )
	    {
		retval = 1;
	    }
	}
    }
    bmem_free( ts );
#endif
#if defined(UNIX)
    retval = remove( name );
    if( retval )
    {
	//May be it is a directory with files?
	bfs_find_struct fs;
	bool first;
	utf8_char* dir = (utf8_char*)bmem_new( MAX_DIR_LEN );
	bmem_set( &fs, sizeof( fs ), 0 );
	fs.start_dir = filename;
	fs.mask = 0;
	first = 1;
	while( 1 )
	{
    	    int v;
    	    if( first )
    	    {
        	v = bfs_find_first( &fs );
        	first = 0;
    	    }
    	    else
    	    {
        	v = bfs_find_next( &fs );
    	    }
    	    if( v )
    	    {
        	if( strcmp( fs.name, "." ) && strcmp( fs.name, ".." ) )
        	{
                    sprintf( dir, "%s/%s", fs.start_dir, fs.name );
                    bfs_remove( dir );
                }
    	    }
    	    else break;
	}
	bfs_find_close( &fs );
	bmem_free( dir );
	//... and try it again:
	retval = remove( name );
    }
#endif
    bmem_free( (void*)name );
    return retval;
}

int bfs_rename( const utf8_char* old_name, const utf8_char* new_name )
{
    int retval = 0;

    const utf8_char* name1 = bfs_make_filename( old_name );
    const utf8_char* name2 = bfs_make_filename( new_name );
    if( name1 == 0 ) return -1;
    if( name2 == 0 ) return -1;

#if defined(UNIX)
    retval = rename( name1, name2 );
#endif
#if defined(WIN) || defined(WINCE)
    size_t len1 = bmem_strlen( name1 ) + 1;
    size_t len2 = bmem_strlen( name2 ) + 1;
    WIN_MAKE_LONG_PATH( const utf8_char*, name1, len1 );
    WIN_MAKE_LONG_PATH( const utf8_char*, name2, len2 );
    utf16_char* ts1 = (utf16_char*)bmem_new( len1 * 2 );
    utf16_char* ts2 = (utf16_char*)bmem_new( len2 * 2 );
    if( ts1 && ts2 )
    {
        utf8_to_utf16( ts1, len1, name1 );
        utf8_to_utf16( ts2, len2, name2 );
        utf16_unix_slash_to_windows( ts1 );
        utf16_unix_slash_to_windows( ts2 );
        if( MoveFileW( (const WCHAR*)ts1, (const WCHAR*)ts2 ) != 0 )
            retval = 0; //No errors
        else
            retval = -1;
    }
    else retval = -1;
    bmem_free( ts1 );
    bmem_free( ts2 );
#endif
    
    bmem_free( (void*)name1 );
    bmem_free( (void*)name2 );
    
    return retval;
}

int bfs_mkdir( const utf8_char* pathname, uint mode )
{
    int retval = -1;
    const utf8_char* name = bfs_make_filename( pathname );
    if( name == 0 ) return -1;
#ifdef UNIX
    retval = mkdir( name, mode );
#endif
#if defined(WIN) || defined(WINCE)
    size_t len = bmem_strlen( name ) + 1;
    WIN_MAKE_LONG_PATH( const utf8_char*, name, len );
    utf16_char* ts = (utf16_char*)bmem_new( len * 2 );
    if( ts )
    {
        utf8_to_utf16( ts, len, name );
        utf16_unix_slash_to_windows( ts );
        if( CreateDirectoryW( (const WCHAR*)ts, 0 ) != 0 )
        {
            retval = 0; //No errors
        }
        else
        {
    	    blog( "Failed to create directory %s. Error code %d\n", name, GetLastError() );
            retval = -1;
        }
        bmem_free( ts );
    }
    else retval = -1;
#endif
    bmem_free( (void*)name );    
    return retval;
}

size_t bfs_get_file_size( const utf8_char* filename )
{
    size_t retval = 0;

#ifdef UNIX
    utf8_char* name = bfs_make_filename( filename );
    if( name == 0 ) return 0;
    struct stat file_info;
#ifdef ANDROID
    uchar* raw_stat = (uchar*)&file_info;
    raw_stat[ sizeof( struct stat ) - 1 ] = 98;
    raw_stat[ sizeof( struct stat ) - 2 ] = 76;
    raw_stat[ sizeof( struct stat ) - 3 ] = 54;
#endif	
    if( stat( name, &file_info ) == 0 )
    {	
        retval = (size_t)file_info.st_size;
#ifdef ANDROID
        if( raw_stat[ sizeof( struct stat ) - 1 ] == 98 &&
    	    raw_stat[ sizeof( struct stat ) - 2 ] == 76 &&
	    raw_stat[ sizeof( struct stat ) - 3 ] == 54 )
	{
	    //Wrong stat size. Long Long (64bit) fields are not 8 bytes aligned.
	    uint* raw_stat32 = (uint*)&file_info;
	    retval = (size_t)raw_stat32[ 11 ]; //st_size from the packed (without alignment) structure
	}
#endif	    
    }
    bmem_free( name );
#endif

#if defined(WIN) || defined(WINCE)
    utf8_char* name = bfs_make_filename( filename );
    if( name == 0 ) return 0;
    size_t len = bmem_strlen( name ) + 1;
    WIN_MAKE_LONG_PATH( utf8_char*, name, len );
    utf16_char* ts = (utf16_char*)bmem_new( len * 2 );
    if( ts )
    {
	utf8_to_utf16( ts, len, name );
        utf16_unix_slash_to_windows( ts );
    	WIN32_FILE_ATTRIBUTE_DATA file_info;
	if( GetFileAttributesExW( (LPCWSTR)ts, GetFileExInfoStandard, &file_info ) != 0 )
    	{
	    retval = file_info.nFileSizeLow;
    	}
	bmem_free( ts );
    }
    bmem_free( name );
#endif

    if( retval == 0 )
    {
	bfs_file f = bfs_open( filename, "rb" );
	if( f )
	{
	    bfs_seek( f, 0, 2 );
	    retval = bfs_tell( f );
	    bfs_close( f );
	}
    }
    
    return retval;
}

int bfs_copy_file( const utf8_char* dest, const utf8_char* src )
{
    int rv = -1;
    bfs_file f1 = bfs_open( src, "rb" );
    if( f1 )
    {
	bfs_file f2 = bfs_open( dest, "wb" );
        if( f2 )
        {
    	    int buf_size = 64 * 1024;
    	    void* buf = bmem_new( buf_size );
            if( buf )
            {
                while( 1 )
                {
                    size_t r = bfs_read( buf, 1, buf_size, f1 );
                    if( r == 0 )
                        break;
                    else
                        bfs_write( buf, 1, r, f2 );
                }
                rv = 0;
                bmem_free( buf );
            }
            bfs_close( f2 );
        }
        bfs_close( f1 );
    }
    return rv;
}

//***********************************************************
//***********************************************************
//***********************************************************

//Functions for files searching:

int check_file( utf8_char* our_file, bfs_find_struct* fs )
{
    int p;
    int mp; //mask pointer
    int equal = 0;
    int str_len;
    
    if( fs->mask ) mp = strlen( fs->mask ) - 1; else return 1;
    str_len = (int)strlen( our_file );
    if( str_len > 0 )
	p = str_len - 1;
    else
	return 0;
    
    for( ; p >= 0 ; p--, mp-- )
    {
	if( our_file[ p ] == '.' ) 
	{
	    if( equal ) return 1; //It is our file!
	    else 
	    {
		for(;;) 
		{ //Is there other file types (in mask[]) ?:
		    if( fs->mask[ mp ] == '/' ) break; //There is other type
		    mp--;
		    if( mp < 0 ) return 0; //no... it was the last type in mask[]
		}
	    }
	}
	if( mp < 0 ) return 0;
	if( fs->mask[ mp ] == '/' ) { mp--; p = str_len - 1; }
	utf8_char c = our_file[ p ];
	if( c >= 65 && c <= 90 ) c += 0x20; //Make small leters
	if( c != fs->mask[ mp ] ) 
	{
	    for(;;) 
	    { //Is there other file types (in mask[]) ?:
	        if( fs->mask[ mp ] == '/' ) { p = str_len; mp++; break; } //There is other type
	        mp--;
	        if( mp < 0 ) return 0; //no... it was the last type in mask[]
	    }
	}
	else equal = 1;
    }
    
    return 0;
}

#if defined(WIN) || defined(WINCE)
//Windows:

int bfs_find_first( bfs_find_struct* fs )
{
    int wp = 0, p = 0;
    fs->win_mask[ 0 ] = 0;
    fs->win_start_dir = 0;
    
    fs->start_dir = bfs_make_filename( fs->start_dir );
    if( fs->start_dir == 0 ) return 0;
    
    size_t len = bmem_strlen( fs->start_dir ) + 1;
    WIN_MAKE_LONG_PATH( const utf8_char*, fs->start_dir, len );
    
    //convert start dir from "dir/dir/" to "dir\dir\*.*"
    fs->win_start_dir = (utf8_char*)bmem_new( len + 16 ); 
    bmem_copy( fs->win_start_dir, fs->start_dir, len );
    if( fs->win_start_dir[ len - 2 ] != '/' )
	strcat( fs->win_start_dir, "/" );
    strcat( fs->win_start_dir, "*.*" );
    for( wp = 0; ; wp++ ) 
    {
	if( fs->win_start_dir[ wp ] == 0 ) break;
	if( fs->win_start_dir[ wp ] == '/' ) fs->win_start_dir[ wp ] = 92;
    }
    len = bmem_strlen( fs->win_start_dir ) + 1;
    
    //do it:
    utf16_char* ts = (utf16_char*)bmem_new( len * 2 );
    utf8_to_utf16( ts, len, fs->win_start_dir );
#if WINCE
    fs->find_handle = FindFirstFile( (const WCHAR*)ts, &fs->find_data );
#endif
#ifdef WIN
    fs->find_handle = FindFirstFileW( (const WCHAR*)ts, &fs->find_data );
#endif
    bmem_free( ts );
    if( fs->find_handle == INVALID_HANDLE_VALUE ) return 0; //no files found :(
    
    //save filename:
    fs->name[ 0 ] = 0;
    strcat( fs->name, utf16_to_utf8( fs->temp_name, MAX_DIR_LEN, (const utf16_char*)fs->find_data.cFileName ) );
    if( fs->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) 
    {
	//Dir:
	fs->type = BFS_DIR; 
    }
    else 
    { 
	//File:
	fs->type = BFS_FILE;
	if( !check_file( fs->name, fs ) ) return bfs_find_next( fs );
    }
    
    return 1;
}

int bfs_find_next( bfs_find_struct* fs )
{
    for(;;) 
    {
	if( !FindNextFileW( fs->find_handle, &fs->find_data ) ) return 0; //files not found

	//save filename:
        fs->name[ 0 ] = 0;
	strcat( fs->name, utf16_to_utf8( fs->temp_name, MAX_DIR_LEN, (const utf16_char*)fs->find_data.cFileName ) );
	if( fs->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) 
	{ 
	    //Dir:
	    fs->type = BFS_DIR;
	    return 1; 
	} 
	else 
	{ 
	    //File found. 
	    fs->type = BFS_FILE;
	    //Is it our file?
	    if( check_file( fs->name, fs ) ) 
		return 1;
	}
    }

    return 1;
}

void bfs_find_close( bfs_find_struct* fs )
{
    FindClose( fs->find_handle );
    
    bmem_free( (void*)fs->start_dir );
    bmem_free( fs->win_start_dir );
    fs->start_dir = 0;
    fs->win_start_dir = 0;
}

#endif

#ifdef UNIX
//UNIX:

int bfs_find_first( bfs_find_struct* fs )
{
    DIR* test;
    utf8_char test_dir[ MAX_DIR_LEN ];
    
    fs->start_dir = bfs_make_filename( fs->start_dir );
    if( fs->start_dir == 0 ) return 0;
    
    //convert start dir to unix standard:
    fs->new_start_dir[ 0 ] = 0;
    if( fs->start_dir[ 0 ] == 0 ) 
	strcat( fs->new_start_dir, "./" ); 
    else 
	strcat( fs->new_start_dir, fs->start_dir );
    
    //open dir and read first entry:
    fs->dir = opendir( fs->new_start_dir );
    if( fs->dir == 0 ) return 0; //no such dir :(
    fs->current_file = readdir( fs->dir );
    if( !fs->current_file ) return 0; //no files
    
    //copy file name:
    fs->name[ 0 ] = 0;
    strcpy( fs->name, fs->current_file->d_name );
    
    //is it a dir?
    test_dir[ 0 ] = 0;
    strcat( test_dir, fs->new_start_dir );
    strcat( test_dir, fs->current_file->d_name );
    test = opendir( test_dir );
    if( test ) 
    {
	fs->type = BFS_DIR; 
	closedir( test );
    }
    else 
    {
	fs->type = BFS_FILE;
    }
    if( strcmp( fs->current_file->d_name, "." ) == 0 ) fs->type = BFS_DIR;
    if( strcmp( fs->current_file->d_name, ".." ) == 0 ) fs->type = BFS_DIR;
    
    if( fs->type == BFS_FILE )
	if( !check_file( fs->name, fs ) ) return bfs_find_next( fs );
	
    return 1;
}

int bfs_find_next( bfs_find_struct* fs )
{
    DIR *test;
    utf8_char test_dir[ MAX_DIR_LEN ];
    
    for(;;) 
    {
	//read next entry:
	if( fs->dir == 0 ) return 0; //directory not exist
	fs->current_file = readdir( fs->dir );
	if( !fs->current_file ) return 0; //no files
    
	//copy file name:
	fs->name[ 0 ] = 0;
	strcpy( fs->name, fs->current_file->d_name );
    
	//is it dir?
	test_dir[ 0 ] = 0;
	strcat( test_dir, fs->new_start_dir );
	strcat( test_dir, fs->current_file->d_name );
	test = opendir( test_dir );
	if( test ) 
	{
	    fs->type = BFS_DIR;
	    closedir( test );
	}
	else
	{
	    fs->type = BFS_FILE;
	}
	if( strcmp( fs->current_file->d_name, "." ) == 0 ) fs->type = BFS_DIR;
	if( strcmp( fs->current_file->d_name, ".." ) == 0 ) fs->type = BFS_DIR;
	
	if( fs->type == BFS_FILE )
	{
	    if( check_file( fs->name, fs ) ) return 1;
	}
	else return 1; //Dir found
    }
}

void bfs_find_close( bfs_find_struct* fs )
{
    if( fs->dir )
	closedir( fs->dir );
    
    bmem_free( (void*)fs->start_dir );
    fs->start_dir = 0;
}

#endif
