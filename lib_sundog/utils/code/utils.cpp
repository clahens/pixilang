/*
    utils.cpp. Various functions: string list; random generator; ...
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100% with global init/deinit

#include "core/core.h"
#include "../utils.h"

#if defined(OSX) && !defined(NOFILEUTILS)
    #include "../../various/osx/sundog_bridge.h"
    #include <CoreFoundation/CFBundle.h>
    #include <ApplicationServices/ApplicationServices.h>
#endif

#if defined(IPHONE) && !defined(NOFILEUTILS)
    #include "various/iphone/sundog_bridge.h"
#endif

#if defined(ANDROID) && !defined(NOFILEUTILS)
    #include "various/android/sundog_bridge.h"
#endif

void utils_global_init( void )
{
    profile_new( 0 );
    profile_load( 0, 0 );
    blocale_init();
}

void utils_global_deinit( void )
{
    blocale_deinit();
    profile_close( 0 );
}

//LIST OF STRINGS:

#ifndef NOLIST

void list_init( list_data* data )
{
    data->items = (utf8_char*)bmem_new( 4096 );
    data->items_ptr = (int*)bmem_new( 64 * sizeof( int* ) );
    list_clear( data );
    list_reset_selection( data );
}

void list_close( list_data* data )
{
    if( data->items )
    {
	bmem_free( data->items );
	data->items = 0;
    }
    if( data->items_ptr )
    {
	bmem_free( data->items_ptr );
	data->items_ptr = 0;
    }
}

void list_clear( list_data* data )
{
    data->items_num = 0;
    data->start_item = 0;
}

void list_reset_items( list_data* data )
{
    data->items_num = 0;
}

void list_reset_selection( list_data* data )
{
    data->selected_item = -1;
}

void list_add_item( const utf8_char* item, char attr, list_data* data )
{
    if( data->items == 0 ) return;
    if( data->items_ptr == 0 ) return;
    
    int ptr, p;

    if( data->items_num == 0 )
    {
	data->items_ptr[ 0 ] = 0;
	ptr = 0;
    }
    else
    {
	ptr = data->items_ptr[ data->items_num ];
    }
    
    int len = (int)bmem_strlen( item );
    int old_size = (int)bmem_get_size( data->items );
    if( ptr + len + 2 >= old_size )
    {
	data->items = (utf8_char*)bmem_resize( data->items, old_size * 2 );
	if( data->items == 0 ) return;
    }
    data->items[ ptr++ ] = attr | 128;
    for( p = 0; ; p++, ptr++ )
    {
	data->items[ ptr ] = item[ p ];
	if( item[ p ] == 0 ) break;
    }
    ptr++;
    
    data->items_num++;
    old_size = (int)bmem_get_size( data->items_ptr ) / sizeof( int* );
    if( data->items_num >= old_size )
	data->items_ptr = (int*)bmem_resize( data->items_ptr, old_size * 2 * sizeof( int* ) );
    if( data->items_ptr == 0 ) return;
    data->items_ptr[ data->items_num ] = ptr;
}

void list_delete_item( int item_num, list_data* data )
{
    if( data->items == 0 ) return;
    if( data->items_ptr == 0 ) return;
    
    int ptr, p;

    if( item_num < data->items_num && item_num >= 0 )
    {
	ptr = data->items_ptr[ item_num ]; //Offset of our item (in the "items")
	
	//Get item size (in chars):
	int size;
	for( size = 0; ; size++ )
	{
	    if( data->items[ ptr + size ] == 0 ) break;
	}
	
	//Delete it:
	int items_size = (int)bmem_get_size( data->items );
	int items_ptr_size = (int)bmem_get_size( data->items_ptr ) / sizeof( int* );
	for( p = ptr; p < items_size - size - 1 ; p++ )
	{
	    data->items[ p ] = data->items[ p + size + 1 ];
	}
	for( p = 0; p < data->items_num; p++ )
	{
	    if( data->items_ptr[ p ] > ptr ) data->items_ptr[ p ] -= size + 1;
	}
	for( p = item_num; p < data->items_num; p++ )
	{
	    if( p + 1 < items_ptr_size ) 
		data->items_ptr[ p ] = data->items_ptr[ p + 1 ];
	    else
		data->items_ptr[ p ] = 0;
	}
	data->items_num--;
	if( data->items_num < 0 ) data->items_num = 0;

	if( data->selected_item >= data->items_num ) data->selected_item = data->items_num - 1;
    }
}

void list_move_item_up( int item_num, list_data* data )
{
    if( data->items == 0 ) return;
    if( data->items_ptr == 0 ) return;
    
    if( item_num < data->items_num && item_num >= 0 )
    {
	if( item_num != 0 )
	{
	    int temp = data->items_ptr[ item_num - 1 ];
	    data->items_ptr[ item_num - 1 ] = data->items_ptr[ item_num ];
	    data->items_ptr[ item_num ] = temp;
	    if( item_num == data->selected_item ) data->selected_item--;
	}
    }
}	

void list_move_item_down( int item_num, list_data* data )
{
    if( data->items == 0 ) return;
    if( data->items_ptr == 0 ) return;
    
    if( item_num < data->items_num && item_num >= 0 )
    {
	if( item_num != data->items_num - 1 )
	{
	    int temp = data->items_ptr[ item_num + 1 ];
	    data->items_ptr[ item_num + 1 ] = data->items_ptr[ item_num ];
	    data->items_ptr[ item_num ] = temp;
	    if( item_num == data->selected_item ) data->selected_item++;
	}
    }
}

utf8_char* list_get_item( int item_num, list_data* data )
{
    if( data->items == 0 ) return 0;
    if( data->items_ptr == 0 ) return 0;
    
    if( item_num >= data->items_num ) return 0;
    if( item_num >= 0 )
	return data->items + data->items_ptr[ item_num ] + 1;
    else 
	return 0;
}

char list_get_attr( int item_num, list_data* data )
{
    if( data->items == 0 ) return 0;
    if( data->items_ptr == 0 ) return 0;
    
    if( item_num >= data->items_num ) return 0;
    if( item_num >= 0 )
	return data->items[ data->items_ptr[ item_num ] ] & 127;
    else
	return 0;
}

int list_get_selected_num( list_data* data )
{
    return data->selected_item;
}

void list_set_selected_num( int sel, list_data* data )
{
    data->selected_item = sel;
}

//Return values:
//1 - item1 > item2
//0 - item1 <= item2
int list_compare_items( int item1, int item2, list_data* data )
{
    utf8_char* i1 = data->items + data->items_ptr[ item1 ];
    utf8_char* i2 = data->items + data->items_ptr[ item2 ];
    utf8_char a1 = i1[ 0 ] & 127;
    utf8_char a2 = i2[ 0 ] & 127;
    i1++;
    i2++;
    
    int retval = 0;
    
    //Compare:
    if( a1 != a2 )
    {
	if( a1 == 1 ) retval = 0;
	if( a2 == 1 ) retval = 1;
    }
    else
    {
	for( int a = 0; ; a++ )
	{
	    if( i1[ a ] == 0 ) break;
	    if( i2[ a ] == 0 ) break;
	    if( i1[ a ] < i2[ a ] ) { break; }             //item1 < item2
	    if( i1[ a ] > i2[ a ] ) { retval = 1; break; } //item1 > item2
	}
    }
    
    return retval;
}

void list_sort( list_data* data )
{
    if( data->items == 0 ) return;
    if( data->items_ptr == 0 ) return;
    
    for(;;)
    {
	int s = 0;
	for( int a = 0; a < data->items_num - 1; a++ )
	{
	    if( list_compare_items( a, a + 1, data ) )
	    {
		s = 1;
		int temp = data->items_ptr[ a + 1 ];
		data->items_ptr[ a + 1 ] = data->items_ptr[ a ];
		data->items_ptr[ a ] = temp;
	    }
	}
	if( s == 0 ) break;
    }
}

#endif

//MUTEXES:

int bmutex_init( bmutex* mutex, int attr )
{
    int retval = 0;
    if( mutex == 0 ) return -1;
#ifdef UNIX
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init( &mutexattr );
    pthread_mutexattr_settype( &mutexattr, PTHREAD_MUTEX_RECURSIVE );
    retval = pthread_mutex_init( &mutex->mutex, &mutexattr );
    pthread_mutexattr_destroy( &mutexattr );
#endif
#if defined(WIN) || defined(WINCE)
    InitializeCriticalSection( &mutex->mutex );
#endif
    return retval;
}

int bmutex_destroy( bmutex* mutex )
{
    int retval = 0;
    if( mutex == 0 ) return -1;
#ifdef UNIX
    retval = pthread_mutex_destroy( &mutex->mutex );
#endif
#if defined(WIN) || defined(WINCE)
    DeleteCriticalSection( &mutex->mutex );
#endif
    return retval;
}

//Return 0 if successful
int bmutex_lock( bmutex* mutex )
{
    int retval = 0;
    if( mutex == 0 ) return -1;
#ifdef UNIX
    retval = pthread_mutex_lock( &mutex->mutex );
#endif
#if defined(WIN) || defined(WINCE)
    EnterCriticalSection( &mutex->mutex );
#endif
    return retval;
}

//Return 0 if successful
int bmutex_trylock( bmutex* mutex )
{
    int retval = 0;
    if( mutex == 0 ) return -1;
#ifdef UNIX
    retval = pthread_mutex_trylock( &mutex->mutex );
#endif
#if defined(WIN) || defined(WINCE)
    if( TryEnterCriticalSection( &mutex->mutex ) != 0 )
	retval = 0;
    else 
	retval = 1;
#endif
    return retval;
}

int bmutex_unlock( bmutex* mutex )
{
    int retval = 0;
    if( mutex == 0 ) return -1;
#ifdef UNIX
    retval = pthread_mutex_unlock( &mutex->mutex );
#endif
#if defined(WIN) || defined(WINCE)
    LeaveCriticalSection( &mutex->mutex );
#endif
    return retval;
}

//THREADS:

#if defined(UNIX)
void* bthread_handler( void* arg )
#endif
#if defined(WIN) || defined(WINCE)
DWORD __stdcall bthread_handler( void* arg )
#endif
{
    bthread* th = (bthread*)arg;
    th->proc( th->arg );
    th->finished = true;
#if defined(UNIX)
    pthread_exit( NULL );
#endif
#if defined(WIN) || defined(WINCE)
    ExitThread( 0 );
#endif
    return 0;
}

int bthread_create( bthread* th, void* (*proc)(void*), void* arg, uint flags )
{
    int rv = 0;
    th->arg = arg;
    th->proc = proc;
    th->finished = false;
#if defined(UNIX)
    int err = pthread_create( &th->th, 0, &bthread_handler, (void*)th );
    if( err )
    {
	blog( "pthread_create error %d\n", err );
	return 1;
    }
#endif
#if defined(WIN) || defined(WINCE)
    th->th = CreateThread( NULL, 0, &bthread_handler, (void*)th, 0, NULL );
#endif
    return rv;
}

int bthread_destroy( bthread* th, int timeout )
{
    int rv = 2; //0 - successful; 1 - timeout; 2 - some error

    if( th == 0 ) return 2;
    if( th->proc == 0 ) return 0;
    
    int err;

    bool dont_destroy_after_timeout = 0;
    if( timeout < 0 ) 
    {
	timeout = -timeout;
	dont_destroy_after_timeout = 1;
    }
    
    if( timeout == 0x7FFFFFFF )
    {
	//Infinite:
#ifdef UNIX
	err = pthread_join( th->th, 0 );
	if( err ) 
	{ 
	    blog( "pthread_join() error %d\n", err ); 
	}
	else rv = 0;
#endif
#if defined(WIN) || defined(WINCE)
	err = WaitForSingleObject( th->th, INFINITE );
	if( err == WAIT_OBJECT_0 )
	{
	    //Thread terminated:
	    CloseHandle( th->th );
	    rv = 0;
	}
	else
	{
	    blog( "WaitForSingleObject() error %d\n", err );
	}
#endif
	return rv;
    }

    int timeout_counter = timeout; //Timeout in milliseconds
    int step = 20; //ms
    while( timeout_counter > 0 )
    {
	if( th->finished )
	{
#ifdef UNIX
	    err = pthread_join( th->th, 0 );
	    if( err ) blog( "pthread_join() error %d\n", err );
#endif
#if defined(WIN) || defined(WINCE)
	    CloseHandle( th->th );
#endif
	    rv = 0;
	    break;
	}
#ifdef UNIX
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 1000000 * step;
	nanosleep( &delay, NULL ); //Sleep for delay time
#endif
#if defined(WIN) || defined(WINCE)
	err = WaitForSingleObject( th->th, step );
	if( err == WAIT_OBJECT_0 )
	{
	    //Thread terminated:
	    CloseHandle( th->th );
	    rv = 0;
	    break;
	}
	if( err == WAIT_FAILED )
	{
	    return 2;
	}
#endif
	timeout_counter -= step;
    }
    if( timeout_counter <= 0 )
    {
	if( dont_destroy_after_timeout ) return 1;
#ifdef UNIX
#ifndef ANDROID
	err = pthread_cancel( th->th );
	if( err ) blog( "pthread_cancel() error %d\n", err );
#endif
#endif
#if defined(WIN) || defined(WINCE)
	TerminateThread( th->th, 0 );
	CloseHandle( th->th );
#endif
	rv = 1;
    }

    return rv;
}

int bthread_clean( bthread* th )
{
    if( th == 0 ) return -1;
    bmem_set( th, sizeof( bthread ), 0 );
    return 0;
}

int bthread_is_empty( bthread* th )
{
    if( th == 0 ) return -1;
    if( th->proc == 0 ) return 1;
    return 0;
}

int bthread_is_finished( bthread* th )
{
    if( th == 0 ) return -1;
    if( th->finished )
	return 1;
    return 0;
}

//RING BUFFER:

ring_buf* ring_buf_new( size_t size, uint flags )
{
    ring_buf* rv;
    rv = (ring_buf*)bmem_new( sizeof( ring_buf ) );
    while( rv )
    {
	bmem_zero( rv );
	rv->flags = flags;
	rv->buf_size = round_to_power_of_two( size );
	bmutex_init( &rv->m, 0 );
	break;
    }
    return rv;
}

void ring_buf_remove( ring_buf* b )
{
    if( b == 0 ) return;
    bmem_free( b->buf );
    bmutex_destroy( &b->m );
    bmem_free( b );
}

void ring_buf_lock( ring_buf* b )
{
    if( b == 0 ) return;
    bmutex_lock( &b->m );
}

void ring_buf_unlock( ring_buf* b )
{
    if( b == 0 ) return;
    bmutex_unlock( &b->m );
}

size_t ring_buf_write( ring_buf* b, void* data, size_t size )
{
    if( b == 0 ) return 0;
    size_t size2 = b->buf_size - ( ( b->wp - b->rp ) & ( b->buf_size - 1 ) );
    if( size > size2 ) return 0;
    if( b->buf == 0 )
    {
	b->buf = (uchar*)bmem_new( b->buf_size );
	if( b->buf == 0 ) return 0;
    }
    size_t src_ptr = 0;
    while( size )
    {
        size_t avail = b->buf_size - b->wp;
        if( avail > size )
            avail = size;
        bmem_copy( b->buf + b->wp, (uchar*)data + src_ptr, avail );
        size -= avail;
        src_ptr += avail;
        /*volatile*/ size_t new_wp = ( b->wp + avail ) & ( b->buf_size - 1 );
        //COMPILER_MEMORY_BARRIER();
        b->wp = new_wp;
    }
    return src_ptr;
}

size_t ring_buf_read( ring_buf* b, void* data, size_t size )
{
    if( b == 0 ) return 0;
    if( data == 0 ) return 0;
    if( size == 0 ) return 0;
    size_t rp = b->rp;
    size_t wp = b->wp;
    if( rp == wp ) return 0;
    size_t size2 = ( wp - rp ) & ( b->buf_size - 1 );
    if( size > size2 ) return 0;
    size_t dest_ptr = 0;
    while( size )
    {
        size_t avail = b->buf_size - rp;
        if( avail > size )
            avail = size;
        bmem_copy( (uchar*)data + dest_ptr, b->buf + rp, avail );
        rp = ( rp + avail ) & ( b->buf_size - 1 );
        size -= avail;
        dest_ptr += avail;
    }
    return dest_ptr;
}

void ring_buf_next( ring_buf* b, size_t size )
{
    if( b == 0 ) return;
    b->rp = ( b->rp + size ) & ( b->buf_size - 1 );
}

size_t ring_buf_avail( ring_buf* b )
{
    if( b == 0 ) return 0;
    return ( b->wp - b->rp ) & ( b->buf_size - 1 );
}

//PROFILES:

profile_data g_profile;

void profile_new( profile_data* p )
{
    if( p == 0 ) p = &g_profile;
    
    bmem_set( p, sizeof( profile_data ), 0 );

    p->file_num = -1;
    p->num = 0;
    p->keys = (profile_key*)bmem_new( sizeof( profile_key ) * 4 );
    bmem_zero( p->keys );
}

int profile_resize( int new_num, profile_data* p )
{
    if( p == 0 ) p = &g_profile;

    int old_size = bmem_get_size( p->keys ) / sizeof( profile_key );
    int new_size = new_num + 4;
    if( new_num > old_size )
    {
	p->keys = (profile_key*)bmem_resize( p->keys, sizeof( profile_key ) * new_size );
	if( p->keys == 0 ) return -1;
	bmem_set( &p->keys[ old_size ], ( new_size - old_size ) * sizeof( profile_key ), 0 );
    }
    p->num = new_num;
    
    return 0;
}

int profile_add_key( const utf8_char* key, const utf8_char* value, int line_num, profile_data* p )
{
    int rv = -1;
    
    if( p == 0 ) p = &g_profile;
    
    if( key && p->keys )
    {
	for( rv = 0; rv < p->num; rv++ )
	{
	    if( p->keys[ rv ].key == 0 ) break;
	}
	if( rv >= p->num )
	{
	    //Free item not found.
	    if( profile_resize( p->num + 1, p ) ) return -1;
	}
	if( value )
	{
	    p->keys[ rv ].value = (utf8_char*)bmem_new( bmem_strlen( value ) + 1 );
	    p->keys[ rv ].value[ 0 ] = 0;
	    bmem_strcat_resize( p->keys[ rv ].value, value );
	}
	p->keys[ rv ].key = (utf8_char*)bmem_new( bmem_strlen( key ) + 1 );
	if( p->keys[ rv ].key == 0 ) return -1;
	p->keys[ rv ].key[ 0 ] = 0;
	bmem_strcat_resize( p->keys[ rv ].key, key );
	p->keys[ rv ].line_num = line_num;
    }

    return rv;
}

void profile_remove_key( const utf8_char* key, profile_data* p )
{
    if( p == 0 ) p = &g_profile;
    
    if( key && p->keys )
    {
	int i;
	for( i = 0; i < p->num; i++ )
	{
	    if( p->keys[ i ].key )
	    {
		if( bmem_strcmp( p->keys[ i ].key, key ) == 0 ) 
		    break;
	    }
	}
	if( i < p->num )
	{
	    bmem_free( p->keys[ i ].value );
	    p->keys[ i ].value = 0;
	    p->keys[ i ].deleted = 1;
	}
    }
}

void profile_set_str_value( const utf8_char* key, const utf8_char* value, profile_data* p )
{
    if( p == 0 ) p = &g_profile;
    
    if( key && p->keys )
    {
	int i;
	for( i = 0; i < p->num; i++ )
	{
	    if( p->keys[ i ].key )
	    {
		if( bmem_strcmp( p->keys[ i ].key, key ) == 0 ) 
		    break;
	    }
	}
	if( i < p->num )
	{
	    //Already exists:
	    utf8_char* value_copy = 0;
	    if( value )
	    {
		value_copy = (utf8_char*)bmem_new( bmem_strlen( value ) + 1 );
		value_copy[ 0 ] = 0;
		bmem_strcat_resize( value_copy, value );
	    }
	    bmem_free( p->keys[ i ].value );
	    p->keys[ i ].value = value_copy;
	    p->keys[ i ].deleted = 0;
	}
	else 
	{
	    //Not exists:
	    profile_add_key( key, value, -1, p );
	}
    }
}

void profile_set_int_value( const utf8_char* key, int value, profile_data* p )
{
    utf8_char ts[ 16 ];
    sprintf( ts, "%d", value );
    profile_set_str_value( key, ts, p );
}

int profile_get_int_value( const utf8_char* key, int default_value, profile_data* p )
{
    int rv = default_value;

    if( p == 0 ) p = &g_profile;

    if( key && p->keys )
    {
	int i;
	for( i = 0; i < p->num; i++ )
	{
	    if( p->keys[ i ].key )
	    {
		if( bmem_strcmp( p->keys[ i ].key, key ) == 0 ) 
		    break;
	    }
	}
	if( i < p->num && p->keys[ i ].value )
	{
	    rv = string_to_int( p->keys[ i ].value );
	}
    }
    return rv;
}

utf8_char* profile_get_str_value( const utf8_char* key, const utf8_char* default_value, profile_data* p )
{
    utf8_char* rv = (utf8_char*)default_value;

    if( p == 0 ) p = &g_profile;

    if( key && p->keys )
    {
	int i;
	for( i = 0; i < p->num; i++ )
	{
	    if( p->keys[ i ].key )
		if( bmem_strcmp( p->keys[ i ].key, key ) == 0 ) 
		    break;
	}
	if( i < p->num && p->keys[ i ].value )
	{
	    rv = p->keys[ i ].value;
	}
    }
    return rv;
}

void profile_close( profile_data* p )
{
    if( p == 0 ) p = &g_profile;

    bmem_free( p->file_name );
    bmem_free( p->source );
    p->source = 0;
    p->file_name = 0;
    
    if( p->num && p->keys )
    {
	for( int i = 0; i < p->num; i++ )
	{
	    if( p->keys[ i ].key ) bmem_free( p->keys[ i ].key );
	    if( p->keys[ i ].value ) bmem_free( p->keys[ i ].value );
	}
    }
    bmem_free( p->keys );
    p->keys = 0;
    p->num = 0;
}

#define PROFILE_KEY_CHAR( cc ) ( !( cc < 0x21 || ptr >= size ) )

void profile_load( const utf8_char* filename, profile_data* p )
{
    utf8_char* str1 = (utf8_char*)bmem_new( 1025 );
    if( str1 == 0 ) return;
    utf8_char* str2 = (utf8_char*)bmem_new( 1025 );
    if( str2 == 0 ) return;
    str1[ 1024 ] = 0;
    str2[ 1024 ] = 0;
    str1[ 0 ] = 0;
    str2[ 0 ] = 0;
    int i = 0;
    
    int size = 0;
    utf8_char* f = 0;
    bfs_file fp = 0;
    
    int ptr = 0;
    int c = 0;
    char comment_mode = 0;
    char key_mode = 0;
    int line_num = 0;
    
    if( p == 0 ) p = &g_profile;

    int pn = -1;
    if( filename == 0 )
    {
	pn = 0;
	while( user_profile_names[ pn ] != 0 )
	{
	    bfs_file pf = bfs_open( user_profile_names[ pn ], "rb" );
	    if( pf )
	    {
		bfs_close( pf );
		filename = user_profile_names[ pn ];
		break;
	    }
	    else 
	    {
		filename = 0;
	    }
	    pn++;
	}
	if( filename == 0 ) pn = -1;
    }

    profile_close( p );
    profile_new( p );
    
    p->file_num = pn;
    p->file_name = (utf8_char*)bmem_new( bmem_strlen( filename ) + 1 );
    p->file_name[ 0 ] = 0;
    bmem_strcat_resize( p->file_name, filename );
    
    size = bfs_get_file_size( filename );
    if( size == 0 ) goto load_end;
    f = (utf8_char*)bmem_new( size );
    if( f == 0 ) goto load_end;
    fp = bfs_open( filename, "rb" );
    if( fp )
    {
	bfs_read( f, 1, size, fp );
	bfs_close( fp );
	if( f[ size - 1 ] >= 0x20 && f[ size - 1 ] < 0x7E )
	{
	    //Wrong symbol at the end of profile. Must be 0xA or 0xD
	    f = (utf8_char*)bmem_resize( f, size + 1 );
	    if( f == 0 ) goto load_end;
	    f[ size ] = 0xA;
	    size++;
	}
    }
    else
    {
	bmem_free( f );
	goto load_end;
    }
    
    while( ptr < size )
    {
        c = f[ ptr ];
        if( c == 0xD || c == 0xA )
        {
    	    comment_mode = 0; //Reset comment mode at the end of line
	    if( key_mode > 0 )
	    {
		profile_add_key( str1, str2, line_num, p );
	    }
	    key_mode = 0;
	    line_num++;
	    if( ptr + 1 < size )
	    {
		if( c == 0xD && f[ ptr + 1 ] == 0xA )
		{
		    ptr++;
		}
		else
		{
		    if( c == 0xA && f[ ptr + 1 ] == 0xD ) ptr++;
		}
	    }
	}
	if( comment_mode == 0 )
	{
	    if( f[ ptr ] == '/' && f[ ptr + 1 ] == '/' )
	    {
	        comment_mode = 1; //Comments
		ptr += 2;
	        continue;
	    }
	    if( PROFILE_KEY_CHAR( c ) )
	    {
	        if( key_mode == 0 )
	        {
	    	    //Get key name:
		    str2[ 0 ] = 0;
		    for( i = 0; i < 1024; i++ )
		    {
			if( !PROFILE_KEY_CHAR( f[ ptr ] ) ) 
			{ 
			    str1[ i ] = 0;
			    ptr--;
			    break; 
			}
		        str1[ i ] = f[ ptr ];
			ptr++;
		    }
		    key_mode = 1;
		}
		else if( key_mode == 1 )
		{
		    //Get value:
		    str2[ 0 ] = 0;
		    if( f[ ptr ] == '"' )
		    {
			ptr++;
			for( i = 0; i < 1024; i++ )
			{
			    if( ptr >= size || f[ ptr ] == '"' ) 
			    { 
			        str2[ i ] = 0;
			        break; 
			    }
			    str2[ i ] = f[ ptr ];
			    ptr++;
			}
		    }
		    else
		    {
			for( i = 0; i < 1024; i++ )
			{
			    if( ptr >= size || f[ ptr ] < 0x21 ) 
			    { 
			        str2[ i ] = 0;
				ptr--;
			        break; 
			    }
			    str2[ i ] = f[ ptr ];
			    ptr++;
			}
		    }
		    key_mode = 2;
		}
	    }
	}
	ptr++;
    }
    if( key_mode > 0 )
    {
	profile_add_key( str1, str2, line_num, p );
    }

    p->source = f;
    
load_end:

    bmem_free( str1 );
    bmem_free( str2 );
}

void profile_save_key( int key, bfs_file f, profile_data* p )
{
    if( ( p->keys[ key ].key ) && ( p->keys[ key ].deleted == 0 ) )
    {
	bfs_write( p->keys[ key ].key, 1, bmem_strlen( p->keys[ key ].key ), f );
	if( p->keys[ key ].value )
	{
	    int vsize = bmem_strlen( p->keys[ key ].value );
	    bool q = 0;
	    for( int i2 = 0; i2 < vsize; i2++ )
	    {
		int cc = p->keys[ key ].value[ i2 ];
		if( cc < 0x21 || cc == '/' ) 
		{
		    q = 1;
		    break;
		}
	    }
	    bfs_putc( ' ', f );
	    if( q ) bfs_putc( '"', f );
	    bfs_write( p->keys[ key ].value, 1, vsize, f );
	    if( q ) bfs_putc( '"', f );
	    bfs_putc( '\n', f );
	}
    }
}

int profile_save( profile_data* p )
{
    int rv = 0;
    
    if( p == 0 ) p = &g_profile;
    
    //Get file name for writing:
    bfs_file f = 0;
    if( p->file_name )
	f = bfs_open( p->file_name, "wb" );
    if( f )
    {
	bfs_close( f );
    }
    else 
    {
	if( p->file_num < 0 )
	{
	    for( p->file_num = 0; ; p->file_num++ )
	    {
		if( user_profile_names[ p->file_num ] == 0 ) break;
	    }
	    p->file_num--;
	}
	else 
	{
	    p->file_num--;
	}
	if( p->file_num < 0 ) return 1;
	while( 1 )
	{
	    bmem_free( p->file_name );
	    p->file_name = 0;
	    p->file_name = (utf8_char*)bmem_new( bmem_strlen( user_profile_names[ p->file_num ] ) + 1 );
	    p->file_name[ 0 ] = 0;
	    bmem_strcat_resize( p->file_name, user_profile_names[ p->file_num ] );
	    f = bfs_open( p->file_name, "wb" );
	    if( f )
	    {
		bfs_close( f );
		break;
	    }
	    p->file_num--;
	    if( p->file_num < 0 ) return 1;
	}
    }
    
    //Save profile:
    f = bfs_open( p->file_name, "wb" );
    if( f == 0 ) return 1;
    int line_num = 0;
    bool new_line = 1;
    int size = bmem_get_size( p->source );
    for( int i = 0; i < size; i++ )
    {
	int c = p->source[ i ];
	if( c == 0xA || c == 0xD )
	{
	    //New line:
	    line_num++;
	    new_line = 1;
	    bfs_putc( c, f );
	}
	else 
	{
	    int key = -1;
	    if( new_line )
	    {
		new_line = 0;
		for( int k = 0; k < p->num; k++ )
		{
		    if( p->keys[ k ].line_num == line_num )
		    {
			//Key found for this line:
			key = k;
			break;
		    }
		}
	    }
	    if( key >= 0 )
	    {
		//Ignore this line:
		while( i < size )
		{
		    c = p->source[ i ];
		    if( c == 0xA || c == 0xD )
		    {
			if( i + 1 < size )
			{
			    if( c == 0xA && p->source[ i + 1 ] == 0xD ) 
			    {
				i++;
			    }
			    else
			    {
				if( c == 0xD && p->source[ i + 1 ] == 0xA ) i++;
			    }
			}
			line_num++;
			new_line = 1;
			break;
		    }
		    i++;
		}
		//Save value:
		profile_save_key( key, f, p );
	    }
	    else 
	    {
		bfs_putc( c, f );
	    }
	}
    }
    for( int k = 0; k < p->num; k++ )
    {
	if( p->keys[ k ].line_num == -1 )
	{
	    profile_save_key( k, f, p );
	}
    }
    bfs_close( f );
    
    return rv;
}

void remove_all_profile_files( void )
{
    //Remove the profile:
    blog( "Removing the profile files...\n" );
    for( int p = 0;; p++ )
    {
        if( user_profile_names[ p ] == 0 ) break;
        bfs_remove( user_profile_names[ p ] );
    }
}

//WORKING WITH STRINGS:

void int_to_string( int value, utf8_char* str )
{
    int n;
    utf8_char ts[ 10 ];
    int ts_ptr = 0;
    
    if( value < 0 )
    {
    	*str = '-';
	str++;
	value = -value;
    }

    n = value % 10; ts[ ts_ptr++ ] = (utf8_char) n + 48; value /= 10; if( !value ) goto int_finish;
    n = value % 10; ts[ ts_ptr++ ] = (utf8_char) n + 48; value /= 10; if( !value ) goto int_finish;
    n = value % 10; ts[ ts_ptr++ ] = (utf8_char) n + 48; value /= 10; if( !value ) goto int_finish;
    n = value % 10; ts[ ts_ptr++ ] = (utf8_char) n + 48; value /= 10; if( !value ) goto int_finish;
    n = value % 10; ts[ ts_ptr++ ] = (utf8_char) n + 48; value /= 10; if( !value ) goto int_finish;
    n = value % 10; ts[ ts_ptr++ ] = (utf8_char) n + 48; value /= 10; if( !value ) goto int_finish;
    n = value % 10; ts[ ts_ptr++ ] = (utf8_char) n + 48; value /= 10; if( !value ) goto int_finish;
    n = value % 10; ts[ ts_ptr++ ] = (utf8_char) n + 48; value /= 10; if( !value ) goto int_finish;
    n = value % 10; ts[ ts_ptr++ ] = (utf8_char) n + 48; value /= 10; if( !value ) goto int_finish;
    n = value; ts[ ts_ptr++ ] = (utf8_char) n + 48;
int_finish:

    while( ts_ptr )
    {
	ts_ptr--;
	*str = ts[ ts_ptr ];
	str++;
    }
    
    *str = 0;
}
 
void hex_int_to_string( uint value, utf8_char* str )
{
    int n;
    utf8_char ts[ 8 ];
    int ts_ptr = 0;

    n = value & 15; if( n > 9 ) ts[ ts_ptr++ ] = (utf8_char) ( n - 10 ) + 'A'; else ts[ ts_ptr++ ] = (utf8_char) n + '0'; value >>= 4; if( !value ) goto hex_int_finish;
    n = value & 15; if( n > 9 ) ts[ ts_ptr++ ] = (utf8_char) ( n - 10 ) + 'A'; else ts[ ts_ptr++ ] = (utf8_char) n + '0'; value >>= 4; if( !value ) goto hex_int_finish;
    n = value & 15; if( n > 9 ) ts[ ts_ptr++ ] = (utf8_char) ( n - 10 ) + 'A'; else ts[ ts_ptr++ ] = (utf8_char) n + '0'; value >>= 4; if( !value ) goto hex_int_finish;
    n = value & 15; if( n > 9 ) ts[ ts_ptr++ ] = (utf8_char) ( n - 10 ) + 'A'; else ts[ ts_ptr++ ] = (utf8_char) n + '0'; value >>= 4; if( !value ) goto hex_int_finish;
    n = value & 15; if( n > 9 ) ts[ ts_ptr++ ] = (utf8_char) ( n - 10 ) + 'A'; else ts[ ts_ptr++ ] = (utf8_char) n + '0'; value >>= 4; if( !value ) goto hex_int_finish;
    n = value & 15; if( n > 9 ) ts[ ts_ptr++ ] = (utf8_char) ( n - 10 ) + 'A'; else ts[ ts_ptr++ ] = (utf8_char) n + '0'; value >>= 4; if( !value ) goto hex_int_finish;
    n = value & 15; if( n > 9 ) ts[ ts_ptr++ ] = (utf8_char) ( n - 10 ) + 'A'; else ts[ ts_ptr++ ] = (utf8_char) n + '0'; value >>= 4; if( !value ) goto hex_int_finish;
    n = value & 15; if( n > 9 ) ts[ ts_ptr++ ] = (utf8_char) ( n - 10 ) + 'A'; else ts[ ts_ptr++ ] = (utf8_char) n + '0';
hex_int_finish:

    while( ts_ptr )
    {
	ts_ptr--;
	*str = ts[ ts_ptr ];
	str++;
    }
    
    *str = 0;
}

int string_to_int( const utf8_char* str )
{
    int res = 0;
    int d = 1;
    int minus = 1;
    for( int a = bmem_strlen( str ) - 1; a >= 0; a-- )
    {
	int v = str[ a ];
	if( v >= '0' && v <= '9' )
	{
	    v -= '0';
	    res += v * d;
	    d *= 10;
	}
	else
	if( v == '-' ) minus = -1;
    }
    return res * minus;
}

int hex_string_to_int( const utf8_char* str )
{
    int res = 0;
    int d = 1;
    int minus = 1;
    for( int a = bmem_strlen( str ) - 1; a >= 0; a-- )
    {
	int v = str[ a ];
	if( ( v >= '0' && v <= '9' ) || ( v >= 'A' && v <= 'F' ) || ( v >= 'a' && v <= 'f' ) )
	{
	    if( v >= '0' && v <= '9' ) v -= '0';
	    else
	    if( v >= 'A' && v <= 'F' ) { v -= 'A'; v += 10; }
	    else
	    if( v >= 'a' && v <= 'f' ) { v -= 'a'; v += 10; }
	    res += v * d;
	    d *= 16;
	}
	else
	if( v == '-' ) minus = -1;
    }
    return res * minus;
}

char int_to_hchar( int value )
{
    if( value < 10 ) return value + '0';
	else return ( value - 10 ) + 'A';
}

utf16_char* utf8_to_utf16( utf16_char* dest, int dest_chars, const utf8_char* s )
{
    unsigned char* src = (unsigned char*)s;
    if( dest == 0 )
    {
	dest = (utf16_char*)bmem_new( sizeof( utf16_char ) * 1024 );
	dest_chars = 1024;
	if( dest == 0 ) return 0;
    }
    utf16_char* dest_begin = dest;
    utf16_char* dest_end = dest + dest_chars;
    while( *src != 0 )
    {
	if( *src < 128 ) 
	{
	    *dest = (utf16_char)(*src);
	    src++;
	    dest++;
	}
	else
	{
	    if( *src & 64 )
	    {
		int res;
		if( ( *src & 32 ) == 0 )
		{
		    //Two bytes:
		    res = ( *src & 31 ) << 6;
		    src++;
		    res |= ( *src & 63 );
		    src++;
		    *dest = (utf16_char)res;
		    dest++;
		}
		else if( ( *src & 16 ) == 0 )
		{
		    //Three bytes:
		    res = ( *src & 15 ) << 12;
		    src++;
		    res |= ( *src & 63 ) << 6;
		    src++;
		    res |= ( *src & 63 );
		    src++;
		    *dest = (utf16_char)res;
		    dest++;
		}
		else if( ( *src & 8 ) == 0 )
		{
		    //Four bytes:
		    res = ( *src & 7 ) << 18;
		    src++;
		    res |= ( *src & 63 ) << 12;
		    src++;
		    res |= ( *src & 63 ) << 6;
		    src++;
		    res |= ( *src & 63 );
		    src++;
		    *dest = 0xD800 | (utf16_char)( res & 1023 );
		    dest++;
		    if( dest >= dest_end )
		    {
			dest--;
			break;
		    }
		    *dest = 0xDC00 | (utf16_char)( ( res >> 10 ) & 1023 );
		    dest++;
		}
		else
		{
		    //Unknown byte:
		    src++;
		    continue;
		}
	    }
	    else
	    {
		//Unknown byte:
		src++;
		continue;
	    }
	}
	if( dest >= dest_end )
	{
	    dest--;
	    break;
	}
    }
    *dest = 0;
    return dest_begin;
}

utf32_char* utf8_to_utf32( utf32_char* dest, int dest_chars, const utf8_char* s )
{
    unsigned char* src = (unsigned char*)s;
    if( dest == 0 )
    {
	dest = (utf32_char*)bmem_new( sizeof( utf32_char ) * 1024 );
	dest_chars = 1024;
	if( dest == 0 ) return 0;
    }
    utf32_char* dest_begin = dest;
    utf32_char* dest_end = dest + dest_chars;
    while( *src != 0 )
    {
	if( *src < 128 ) 
	{
	    *dest = *src;
	    src++;
	    dest++;
	}
	else
	{
	    if( *src & 64 )
	    {
		int res;
		if( ( *src & 32 ) == 0 )
		{
		    //Two bytes:
		    res = ( *src & 31 ) << 6;
		    src++;
		    res |= ( *src & 63 );
		    src++;
		    *dest = (utf32_char)res;
		    dest++;
		}
		else if( ( *src & 16 ) == 0 )
		{
		    //Three bytes:
		    res = ( *src & 15 ) << 12;
		    src++;
		    res |= ( *src & 63 ) << 6;
		    src++;
		    res |= ( *src & 63 );
		    src++;
		    *dest = (utf32_char)res;
		    dest++;
		}
		else if( ( *src & 8 ) == 0 )
		{
		    //Four bytes:
		    res = ( *src & 7 ) << 18;
		    src++;
		    res |= ( *src & 63 ) << 12;
		    src++;
		    res |= ( *src & 63 ) << 6;
		    src++;
		    res |= ( *src & 63 );
		    src++;
		    *dest = (utf32_char)res;
		    dest++;
		}
		else
		{
		    //Unknown byte:
		    src++;
		    continue;
		}
	    }
	    else
	    {
		//Unknown byte:
		src++;
		continue;
	    }
	}
	if( dest >= dest_end )
	{
	    dest--;
	    break;
	}
    }
    *dest = 0;
    return dest_begin;
}

utf8_char* utf16_to_utf8( utf8_char* dst, int dest_chars, const utf16_char* src )
{
    unsigned char* dest = (unsigned char*)dst;
    if( dest == 0 )
    {
	dest = (unsigned char*)bmem_new( sizeof( char ) * 1024 );
	dest_chars = 1024;
	if( dest == 0 ) return 0;
    }
    unsigned char* dest_begin = dest;
    unsigned char* dest_end = dest + dest_chars;
    while( *src != 0 )
    {
	int res;
	if( ( *src & ~1023 ) != 0xD800 )
	{
	    res = *src;
	    src++;
	}
	else
	{
	    res = *src & 1023;
	    src++;
	    res |= ( *src & 1023 ) << 10;
	    src++;
	}
	if( res < 128 )
	{
	    *dest = (unsigned char)res;
	    dest++;
	}
	else if( res < 0x800 )
	{
	    if( dest >= dest_end - 2 ) break;
	    *dest = 0xC0 | ( ( res >> 6 ) & 31 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 0 ) & 63 );
	    dest++;
	}
	else if( res < 0x10000 )
	{
	    if( dest >= dest_end - 3 ) break;
	    *dest = 0xE0 | ( ( res >> 12 ) & 15 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 6 ) & 63 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 0 ) & 63 );
	    dest++;
	}
	else
	{
	    if( dest >= dest_end - 4 ) break;
	    *dest = 0xF0 | ( ( res >> 18 ) & 7 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 12 ) & 63 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 6 ) & 63 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 0 ) & 63 );
	    dest++;
	}
	if( dest >= dest_end )
	{
	    dest--;
	    break;
	}
    }
    *dest = 0;
    return (utf8_char*)dest_begin;
}

utf8_char* utf32_to_utf8( utf8_char* dst, int dest_chars, const utf32_char* src )
{
    unsigned char* dest = (unsigned char*)dst;
    if( dest == 0 )
    {
	dest = (unsigned char*)bmem_new( sizeof( char ) * 1024 );
	dest_chars = 1024;
	if( dest == 0 ) return 0;
    }
    unsigned char* dest_begin = dest;
    unsigned char* dest_end = dest + dest_chars;
    while( *src != 0 )
    {
	int res = (int)*src;
	src++;
	if( res < 128 )
	{
	    *dest = (unsigned char)res;
	    dest++;
	}
	else if( res < 0x800 )
	{
	    if( dest >= dest_end - 2 ) break;
	    *dest = 0xC0 | ( ( res >> 6 ) & 31 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 0 ) & 63 );
	    dest++;
	}
	else if( res < 0x10000 )
	{
	    if( dest >= dest_end - 3 ) break;
	    *dest = 0xE0 | ( ( res >> 12 ) & 15 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 6 ) & 63 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 0 ) & 63 );
	    dest++;
	}
	else
	{
	    if( dest >= dest_end - 4 ) break;
	    *dest = 0xF0 | ( ( res >> 18 ) & 7 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 12 ) & 63 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 6 ) & 63 );
	    dest++;
	    *dest = 0x80 | ( ( res >> 0 ) & 63 );
	    dest++;
	}
	if( dest >= dest_end )
	{
	    dest--;
	    break;
	}
    }
    *dest = 0;
    return (utf8_char*)dest_begin;
}

int utf8_to_utf32_char( const utf8_char* str, utf32_char* res )
{
    *res = 0;
    unsigned char* src = (unsigned char*)str;
    while( *src != 0 )
    {
	if( *src < 128 ) 
	{
	    *res = (utf32_char)*src;
	    return 1;
	}
	else
	{
	    if( *src & 64 )
	    {
		if( ( *src & 32 ) == 0 )
		{
		    //Two bytes:
		    *res = ( *src & 31 ) << 6;
		    src++;
		    *res |= ( *src & 63 );
		    return 2;
		}
		else if( ( *src & 16 ) == 0 )
		{
		    //Three bytes:
		    *res = ( *src & 15 ) << 12;
		    src++;
		    *res |= ( *src & 63 ) << 6;
		    src++;
		    *res |= ( *src & 63 );
		    return 3;
		}
		else if( ( *src & 8 ) == 0 )
		{
		    //Four bytes:
		    *res = ( *src & 7 ) << 18;
		    src++;
		    *res |= ( *src & 63 ) << 12;
		    src++;
		    *res |= ( *src & 63 ) << 6;
		    src++;
		    *res |= ( *src & 63 );
		    return 4;
		}
		else
		{
		    //Unknown byte:
		    *res = '?';
		    return 1;
		}
	    }
	    else
	    {
		//Unknown byte:
		*res = '?';
		return 1;
	    }
	}
    }
    return 0;
}

int utf8_to_utf32_char_safe( utf8_char* str, size_t str_size, utf32_char* res )
{
    *res = 0;
    uchar* src = (uchar*)str;
    uchar* src_end = src + str_size;
    while( *src != 0 && src != src_end )
    {
	if( *src < 128 ) 
	{
	    *res = (utf32_char)*src;
	    return 1;
	}
	else
	{
	    if( *src & 64 )
	    {
		if( ( *src & 32 ) == 0 )
		{
		    //Two bytes:
		    *res = ( *src & 31 ) << 6;
		    if( src == src_end ) return 1;
		    src++;
		    *res |= ( *src & 63 );
		    return 2;
		}
		else if( ( *src & 16 ) == 0 )
		{
		    //Three bytes:
		    *res = ( *src & 15 ) << 12;
		    if( src == src_end ) return 1;
		    src++;
		    *res |= ( *src & 63 ) << 6;
		    if( src == src_end ) return 2;
		    src++;
		    *res |= ( *src & 63 );
		    return 3;
		}
		else if( ( *src & 8 ) == 0 )
		{
		    //Four bytes:
		    *res = ( *src & 7 ) << 18;
		    if( src == src_end ) return 1;
		    src++;
		    *res |= ( *src & 63 ) << 12;
		    if( src == src_end ) return 2;
		    src++;
		    *res |= ( *src & 63 ) << 6;
		    if( src == src_end ) return 3;
		    src++;
		    *res |= ( *src & 63 );
		    return 4;
		}
		else
		{
		    //Unknown byte:
		    *res = '?';
		    return 1;
		}
	    }
	    else
	    {
		//Unknown byte:
		*res = '?';
		return 1;
	    }
	}
    }
    return 0;
}

void utf8_unix_slash_to_windows( utf8_char* str )
{
    while( *str != 0 )
    {
	if( *str == 0x2F ) *str = 0x5C;	    
	str++;
    }
}

void utf16_unix_slash_to_windows( utf16_char* str )
{
    while( *str != 0 )
    {
	if( *str == 0x2F ) *str = 0x5C;	    
	str++;
    }
}

void utf32_unix_slash_to_windows( utf32_char* str )
{
    while( *str != 0 )
    {
	if( *str == 0x2F ) *str = 0x5C;	    
	str++;
    }
}

int make_string_lower_upper( utf8_char* dest, size_t dest_size, utf8_char* src, int low_up )
{
    if( src == 0 ) return -1;
    size_t src_size = strlen( src ) + 1;
    if( src_size <= 1 ) return -1;
    utf32_char* str32 = 0;
    utf32_char temp[ 64 ];
    if( src_size <= 64 )
	str32 = temp;
    else
	str32 = (utf32_char*)bmem_new( src_size * 4 );
    if( str32 == 0 ) return -1;
    utf8_to_utf32( str32, src_size, src );
    for( size_t i = 0; i < src_size; i++ )
    {
	utf32_char c = str32[ i ];
	if( c == 0 ) break;
	while( 1 )
	{
	    if( low_up == 0 )
	    {
		//Lowercase:
    		if( c >= 65 && c <= 90 )
    		{
    		    //English ASCII:
    		    c += 32;
    		    break;
    		}
    		if( c >= 0x410 && c <= 0x42F )
    		{
    		    //Russian:
    		    c += 32;
    		}
    	    }
    	    else
    	    {
    		//Uppercase:
    		if( c >= 97 && c <= 122 )
    		{
    		    //English ASCII:
    		    c -= 32;
    	    	    break;
    		}
    		if( c >= 0x430 && c <= 0x44F )
    	        {
    	    	    //Russian:
    		    c -= 32;
    		}
    	    }
    	    break;
    	}
        str32[ i ] = c;
    }
    utf32_to_utf8( dest, dest_size, str32 );
    if( str32 != temp )
	bmem_free( str32 );
    return 0;
}

int make_string_lowercase( utf8_char* dest, size_t dest_size, utf8_char* src )
{
    return make_string_lower_upper( dest, dest_size, src, 0 );
}

int make_string_uppercase( utf8_char* dest, size_t dest_size, utf8_char* src )
{
    return make_string_lower_upper( dest, dest_size, src, 1 );
}

//LOCALE:

static utf8_char* g_blocale_lang = 0;

int blocale_init( void )
{
    int rv = 0;
    utf8_char* l = profile_get_str_value( KEY_LANG, 0, 0 );
    if( l )
    {
	g_blocale_lang = bmem_strdup( l );
    }
    else
    {
	while( 1 )
	{
#ifdef NOWM
	    g_blocale_lang = bmem_strdup( "en_US" );
#else
#ifdef UNIX
#ifdef ANDROID
	    g_blocale_lang = bmem_strdup( g_android_lang );
	    break;
#else
#if ( defined(OSX) || defined(IPHONE) ) && !defined(NOMAIN)
	    if( bmem_strstr( g_ui_lang, "ru" ) )
		g_blocale_lang = bmem_strdup( "ru_RU" );
	    break;
#else
	    g_blocale_lang = bmem_strdup( getenv( "LANG" ) );
	    break;
#endif
#endif
#else
#if defined(WIN) || defined(WINCE)
	    LCID lid = GetUserDefaultLCID();
	    const utf8_char* lname = 0;
	    switch( lid )
	    {
		case 1033: lname = "en_US"; break;
		case 2057: lname = "en_GB"; break;
		case 3081: lname = "en_AU"; break;
		case 4105: lname = "en_CA"; break;
		case 5129: lname = "en_NZ"; break;
		case 6153: lname = "en_IE"; break;
		case 7177: lname = "en_ZA"; break;
		case 1049: lname = "ru_RU"; break;
		case 1058: lname = "uk_UA"; break;
		case 1059: lname = "be_BY"; break;
	    }
	    if( lname ) g_blocale_lang = bmem_strdup( lname );
	    break;
#endif
#endif
#endif //!NOWM
	    break;
	}
    }
    if( g_blocale_lang == 0 ) g_blocale_lang = bmem_strdup( "en_US" ); //Default language
    blog( "Language: %s\n", g_blocale_lang );
    return rv;
}

void blocale_deinit( void )
{
    bmem_free( g_blocale_lang );
    g_blocale_lang = 0;
}

const utf8_char* blocale_get_lang( void )
{
    return g_blocale_lang;
}

//UNDO MANAGER:

#define UNDO_DEBUG

void undo_init( size_t size_limit, int (*action_handler)( UNDO_HANDLER_PARS ), void* user_data, undo_data* u )
{
    bmem_set( u, sizeof( undo_data ), 0 );
    u->data_size_limit = size_limit / 2;
    u->actions_num_limit = round_to_power_of_two( ( size_limit / 2 ) / sizeof( undo_action ) );
    u->action_handler = action_handler;
    u->user_data = user_data;
}

void undo_remove_action( size_t n, undo_data* u )
{
    undo_action* a = &u->actions[ ( u->first_action + n ) & ( u->actions_num_limit - 1 ) ];
    u->data_size -= bmem_get_size( a->data );
    //printf( "remove %d. size: %d. limit: %d\n", bmem_get_size( a->data ), u->data_size, u->data_size_limit );
    bmem_free( a->data );
    a->data = 0;
}

void undo_deinit( undo_data* u )
{
    if( u->actions )
    {
	for( size_t i = 0; i < u->actions_num; i++ )
	{
	    undo_remove_action( i, u );
	}
	bmem_free( u->actions );
    }
}

void undo_reset( undo_data* u )
{
#ifdef UNDO_DEBUG
    blog( "undo_reset(). data_size: %d\n", u->data_size );
#endif
    if( u->actions )
    {
	for( size_t i = 0; i < u->actions_num; i++ )
	{
	    undo_remove_action( i, u );
	}
	bmem_free( u->actions );
	u->actions = 0;
    }
    u->status = 0;
    u->data_size = 0;
    u->level = 0;
    u->first_action = 0;
    u->cur_action = 0;
    u->actions_num = 0;
}

void undo_remove_first_actions( size_t level_bound, bool use_level_bound, undo_data* u )
{
    while( ( u->data_size > u->data_size_limit ) && ( u->actions_num > 0 ) )
    {
	//Not enough memory.
	//Remove the first action.
	undo_action* a = &u->actions[ ( u->first_action + 0 ) & ( u->actions_num_limit - 1 ) ];
	size_t first_level = a->level;
	if( use_level_bound )
	{
	    if( first_level == level_bound ) break; //Can't remove current level!
	}
	undo_remove_action( 0, u );
	u->first_action++;
	u->first_action &= u->actions_num_limit - 1;
	u->cur_action--;
	u->actions_num--;
	
	while( u->actions_num > 0 )
	{
	    a = &u->actions[ ( u->first_action + 0 ) & ( u->actions_num_limit - 1 ) ];
	    size_t next_level = a->level;
	    if( next_level == first_level )
	    {
		//Remove all actions with this level:
		undo_remove_action( 0, u );
		u->first_action++;
		u->first_action &= u->actions_num_limit - 1;
		u->cur_action--;
		u->actions_num--;
	    }
	    else
	    {
		break;
	    }
	}
    }
}

int undo_add_action( undo_action* action, undo_data* u )
{
    //Execute action:
    action->level = u->level;
    u->status = UNDO_STATUS_ADD_ACTION;
    int action_error = u->action_handler( 1, action, u );
    u->status = UNDO_STATUS_NONE;
    if( action_error )
	return action_error;
    
    if( u->actions == 0 )
    {
	u->actions = (undo_action*)bmem_new( sizeof( undo_action ) * u->actions_num_limit );
    }
    
    if( u->cur_action >= u->actions_num_limit )
    {
	//Overflow in the table with actions.
	//Remove the first action.
	undo_remove_action( 0, u );
	u->first_action++;
	u->first_action &= u->actions_num_limit - 1;
	u->cur_action--;
	u->actions_num--;
    }
    
    //Remove previous history.
    for( size_t i = u->cur_action; i < u->actions_num; i++ )
    {
	undo_remove_action( i, u );
    }
    u->actions_num -= u->actions_num - u->cur_action;
    
    //Copy action:
    undo_action* a = &u->actions[ ( u->first_action + u->cur_action ) & ( u->actions_num_limit - 1 ) ];
    bmem_copy( a, action, sizeof( undo_action ) );

    //Increase number of actions:
    u->actions_num++;
    
    //Go to the next action:
    u->cur_action++;
    
    //Get data size:
    u->data_size += bmem_get_size( a->data );
    
    undo_remove_first_actions( 0, 0, u );

    return 0;
}

void undo_next_level( undo_data* u )
{
    u->level++;
}

void execute_undo( undo_data* u )
{
    bool l = 0;
    size_t level = 0;
    
    while( u->cur_action > 0 )
    {
	u->cur_action--;
	undo_action* a = &u->actions[ ( u->first_action + u->cur_action ) & ( u->actions_num_limit - 1 ) ];
	
	if( l == 0 )
	{
	    level = a->level;
	    l = 1;
	}
	else 
	{
	    if( a->level != level )
	    {
		u->cur_action++;
		break;
	    }
	}
	
	size_t old_size = bmem_get_size( a->data );
	if( u->action_handler( 0, a, u ) == 0 )
	{
	    size_t new_size = bmem_get_size( a->data );
	    u->data_size -= old_size - new_size;
	    undo_remove_first_actions( level, 1, u );
	    if( u->data_size > u->data_size_limit )
	    {
		//Error:
#ifdef UNDO_DEBUG
		blog( "execute_undo(). mem.error. data_size: %d\n", (int)u->data_size );
#endif
		undo_reset( u );
		break;
	    }
	}
	else 
	{
	    //Error:
#ifdef UNDO_DEBUG
	    blog( "execute_undo(). action error. data_size: %d\n", (int)u->data_size );
#endif
	    undo_reset( u );
	    break;
	}
    }
}

void execute_redo( undo_data* u )
{
    bool l = 0;
    size_t level = 0;
    
    while( u->cur_action < u->actions_num )
    {
	undo_action* a = &u->actions[ ( u->first_action + u->cur_action ) & ( u->actions_num_limit - 1 ) ];
	
	if( l == 0 )
	{
	    level = a->level;
	    l = 1;
	}
	else 
	{
	    if( a->level != level ) break;
	}
	
	size_t old_size = bmem_get_size( a->data );
	if( u->action_handler( 1, a, u ) == 0 )
	{
	    size_t new_size = bmem_get_size( a->data );
	    u->data_size -= old_size - new_size;
	    undo_remove_first_actions( level, 1, u );
	    if( u->data_size > u->data_size_limit )
	    {
		//Error:
#ifdef UNDO_DEBUG
		blog( "execute_redo(). mem.error. data_size: %d\n", (int)u->data_size );
#endif
		undo_reset( u );
		break;
	    }
	}
	else 
	{
	    //Error:
#ifdef UNDO_DEBUG
	    blog( "execute_redo(). action error. data_size: %d\n", (int)u->data_size );
#endif
	    undo_reset( u );
	    break;
	}
	
	u->cur_action++;
    }
}

//SYMBOL TABLE:

bsymtab* bsymtab_new( size_t size )
{               
    bsymtab* st = (bsymtab*)bmem_new( sizeof( bsymtab ) );
    if( st == 0 ) return 0;
    bmem_zero( st );
    
    while( 1 )
    {
	if( size >= 1572869 ) { size = 1572869; break; }
	if( size >= 786433 ) { size = 786433; break; }
	if( size >= 393241 ) { size = 393241; break; }
	if( size >= 196613 ) { size = 196613; break; }
	if( size >= 98317 ) { size = 98317; break; }
	if( size >= 49157 ) { size = 49157; break; }
	if( size >= 24593 ) { size = 24593; break; }
	if( size >= 12289 ) { size = 12289; break; }
	if( size >= 6151 ) { size = 6151; break; }
	if( size >= 3079 ) { size = 3079; break; }
	if( size >= 1543 ) { size = 1543; break; }
	if( size >= 769 ) { size = 769; break; }
	if( size >= 389 ) { size = 389; break; }
	if( size >= 193 ) { size = 193; break; }
	if( size >= 97 ) { size = 97; break; }
	size = 53;
	break;
    }
    
    st->size = size;
    st->symtab = (bsymtab_item**)bmem_new( st->size * sizeof( void* ) );
    bmem_zero( st->symtab );

    return st;
}

int bsymtab_remove( bsymtab* st )
{
    int rv = 0;

    if( st == 0 ) return -1;
    if( st->symtab == 0 ) return -1;
    
    for( size_t i = 0; i < st->size; i++ )
    {
        bsymtab_item* s = st->symtab[ i ];
        while( s )
        {
            bsymtab_item* next = s->next;
            bmem_free( s->name );
            bmem_free( s );
            s = next;
        }
    }
    bmem_free( st->symtab );
    bmem_free( st );
    
    return rv;
}

int bsymtab_hash( const utf8_char* name, size_t size ) //32bit version!
{
    uint h = 0;
    uchar* p = (uchar*)name;

    for( ; *p != 0; p++ )
        h = 31 * h + *p;

    return (int)( h % (int)size );
}

bsymtab_item* bsymtab_lookup( const utf8_char* name, int hash, bool create, int initial_type, BSYMTAB_VAL initial_val, bool* created, bsymtab* st )
{
    bsymtab_item* s;

    if( st == 0 ) return 0;
    if( st->symtab == 0 ) return 0;

    if( created ) *created = 0;

    if( hash < 0 ) hash = bsymtab_hash( name, st->size );
    for( s = st->symtab[ hash ]; s != 0; s = s->next )
        if( bmem_strcmp( name, s->name ) == 0 )
            return s;

    if( create )
    {
        //Create new symbol:
        s = (bsymtab_item*)bmem_new( sizeof( bsymtab_item ) );
        size_t slen = bmem_strlen( name ) + 1;
        s->name = (utf8_char*)bmem_new( slen );
        bmem_copy( s->name, name, slen );
        s->type = initial_type;
	s->val = initial_val;
        s->next = st->symtab[ hash ];
        st->symtab[ hash ] = s;
        if( created ) *created = 1;
    }

    return s;
}

bsymtab_item* bsymtab_get_list( bsymtab* st )
{
    bsymtab_item* rv = 0;
    size_t size = 0;
    if( st == 0 ) return 0;
    if( st->symtab == 0 ) return 0;
    
    for( size_t i = 0; i < st->size; i++ )
    {
        bsymtab_item* s = st->symtab[ i ];
        while( s )
        {
            if( s->name )
            {
                if( size == 0 )
                    rv = (bsymtab_item*)bmem_new( sizeof( bsymtab_item ) * 8 );
                else
                {
                    if( size >= bmem_get_size( rv ) / sizeof( bsymtab_item ) )
                        rv = (bsymtab_item*)bmem_resize( rv, ( size + 8 ) * sizeof( bsymtab_item ) );
                }
                rv[ size ].name = s->name;
                rv[ size ].type = s->type;
                rv[ size ].val = s->val;
                size++;
            }
            s = s->next;
        }
    }

    if( size > 0 )
    {
        rv = (bsymtab_item*)bmem_resize( rv, size * sizeof( bsymtab_item ) );
    }

    return rv;
}

int bsymtab_iset( const utf8_char* sym_name, int val, bsymtab* st )
{
    int rv = 0;
    if( st == 0 ) return -1;
    if( st->symtab == 0 ) return -1;
    
    BSYMTAB_VAL v;
    v.i = val;
    bsymtab_item* s = bsymtab_lookup( sym_name, -1, true, 0, v, 0, st );
    if( s )
    {
	s->type = 0;
	s->val.i = val;
    }    
        
    return rv;
}

int bsymtab_iset( uint sym_name, int val, bsymtab* st )
{
    utf8_char name[ 16 ];
    hex_int_to_string( sym_name, name );
    return bsymtab_iset( name, val, st );
}

int bsymtab_iget( const utf8_char* sym_name, int notfound_val, bsymtab* st )
{
    int rv = notfound_val;
    if( st == 0 ) return rv;
    if( st->symtab == 0 ) return rv;
    
    BSYMTAB_VAL v;
    v.i = 0;
    bsymtab_item* s = bsymtab_lookup( sym_name, -1, false, 0, v, 0, st );
    if( s )
    {
	rv = s->val.i;
    }    
        
    return rv;
}

int bsymtab_iget( uint sym_name, int notfound_val, bsymtab* st )
{
    utf8_char name[ 16 ];
    hex_int_to_string( sym_name, name );
    return bsymtab_iget( name, notfound_val, st );
}

//COPY / PASTE:

int system_copy( const utf8_char* filename )
{
    int rv = 0;
#ifndef NOFILEUTILS
    utf8_char* new_filename = bfs_make_filename( filename );
#ifdef IPHONE
    rv = iphone_sundog_copy( new_filename );
#endif
    bmem_free( new_filename );
#endif
    return rv;
}

utf8_char* system_paste( uint flags )
{
    utf8_char* rv = 0;
#ifndef NOFILEUTILS
#ifdef IPHONE
    rv = iphone_sundog_paste();
    if( rv )
    {
	utf8_char* name = bmem_strdup( rv );
	free( rv );
	rv = name;
    }
#endif
#endif
    return rv;
}

//URL:

void open_url( const utf8_char* url )
{
#ifndef NOFILEUTILS
#ifdef LINUX
    utf8_char* ts = (utf8_char*)bmem_new( bmem_strlen( url ) + 256 );
    if( ts )
    {
	sprintf( ts, "xdg-open \"%s\" &", url );
	if( system( ts ) != 0 )
	{
	    sprintf( ts, "sensible-browser \"%s\" &", url );
	    system( ts );
	}
	bmem_free( ts );
    }
#endif
#ifdef WIN
    ShellExecute( NULL, "open", url, NULL, NULL, SW_SHOWNORMAL );
#endif
#ifdef OSX
    CFURLRef u = CFURLCreateWithBytes (
	NULL,                   // allocator
        (UInt8*)url,     	// URLBytes
        bmem_strlen( url ),      // length
        kCFStringEncodingASCII, // encoding
        NULL                    // baseURL
    );
    LSOpenCFURLRef( u, 0 );
    CFRelease( u );
#endif
#ifdef IPHONE
    iphone_sundog_open_url( url );
#endif
#ifdef ANDROID
#ifndef NOMAIN
    android_sundog_open_url( url );
#endif
#endif
#endif
}

void send_mail( const utf8_char* email, const utf8_char* subj, const utf8_char* body )
{
#ifndef NOFILEUTILS
#ifdef IPHONE
    iphone_sundog_send_mail( email, subj, body );
#endif
#endif
}

//SEND FILE:

int send_file_to_email( const utf8_char* filename )
{
    int rv = -1;
#ifndef NOFILEUTILS
    utf8_char* new_filename = bfs_make_filename( filename );
#ifdef IPHONE
    iphone_sundog_send_file_to_email( "App File", new_filename );
    rv = 0;
#endif
    bmem_free( new_filename );
#endif
    return rv;
}

int send_file_to_gallery( const utf8_char* filename )
{
    int rv = -1;
#ifndef NOFILEUTILS
    utf8_char* real_filename = bfs_make_filename( filename );
#ifdef ANDROID
    const utf8_char* dest_dir = 0;
    switch( bfs_get_file_type( real_filename, 0 ) )
    {
	case BFS_FILE_TYPE_JPEG:
	case BFS_FILE_TYPE_PNG:
	case BFS_FILE_TYPE_GIF:
	    dest_dir = g_android_pics_ext_path;
	    break;
	case BFS_FILE_TYPE_AVI:
	case BFS_FILE_TYPE_MP4:
	    dest_dir = g_android_movies_ext_path;
	    break;
    }
    if( dest_dir )
    {
        utf8_char* new_name = (utf8_char*)bmem_new( bmem_strlen( bfs_get_filename_without_dir( real_filename ) ) + bmem_strlen( g_android_pics_ext_path ) + 1024 );
        sprintf( new_name, "%s/%s", dest_dir, user_window_name_short );
        mkdir( new_name, 0770 );
        sprintf( new_name, "%s/%s/%04d.%02d.%02d %02d.%02d.%02d.%s", 
    	    dest_dir, user_window_name_short, 
	    time_year(), time_month() + 1, time_day() + 1, 
	    time_hours(), time_minutes(), time_seconds(), 
	    bfs_get_filename_extension( filename ) );
	if( bfs_copy_file( new_name, filename ) == 0 ) rv = 0;
        android_sundog_scan_media( new_name );
        bmem_free( new_name );
    }
#endif
#ifdef IPHONE
    iphone_sundog_send_file_to_gallery( real_filename );
    rv = 0;
#endif
    bmem_free( real_filename );
#endif
    return rv;
}

//RANDOM GENERATOR:

unsigned int g_rand_next = 1;

void set_seed( unsigned int seed )
{
    g_rand_next = seed;
}

unsigned int pseudo_random( void )
{
    g_rand_next = g_rand_next * 1103515245 + 12345;
    return ( (unsigned int)( g_rand_next / 65536 ) % 32768 );
}

unsigned int pseudo_random_with_seed( unsigned int* s )
{
    *s = *s * 1103515245 + 12345;
    return ( (unsigned int)( *s / 65536 ) % 32768 );
}

//3D TRANSFORMATION MATRIX OPERATIONS:

void matrix_4x4_reset( float* m )
{
    bmem_set( m, sizeof( float ) * 4 * 4, 0 );
    m[ 0 ] = 1;
    m[ 4 + 1 ] = 1;
    m[ 8 + 2 ] = 1;
    m[ 12 + 3 ] = 1;
}

void matrix_4x4_mul( float* res, float* m1, float* m2 )
{
    int res_ptr = 0;
    for( int x = 0; x < 4; x++ )
    {
        for( int y = 0; y < 4; y++ )
        {
            res[ res_ptr ] = 0;
            for( int k = 0; k < 4; k++ )
            {
                res[ res_ptr ] += m1[ y + k * 4 ] * m2[ x * 4 + k ];
            }
            res_ptr++;
        }
    }
}

void matrix_4x4_rotate( float angle, float x, float y, float z, float* m )
{
    angle /= 180;
    angle *= M_PI;

    //Normalize vector:
    float inv_length = 1.0f / sqrt( x * x + y * y + z * z );
    x *= inv_length;
    y *= inv_length;
    z *= inv_length;

    //Create the matrix:
    float c = cos( angle );
    float s = sin( angle );
    float cc = 1 - c;
    float r[ 4 * 4 ];
    float res[ 4 * 4 ];
    r[ 0 + 0 ] = x * x * cc + c;
    r[ 4 + 0 ] = x * y * cc - z * s;
    r[ 8 + 0 ] = x * z * cc + y * s;
    r[ 12 + 0 ] = 0;
    r[ 0 + 1 ] = y * x * cc + z * s;
    r[ 4 + 1 ] = y * y * cc + c;
    r[ 8 + 1 ] = y * z * cc - x * s;
    r[ 12 + 1 ] = 0;
    r[ 0 + 2 ] = x * z * cc - y * s;
    r[ 4 + 2 ] = y * z * cc + x * s;
    r[ 8 + 2 ] = z * z * cc + c;
    r[ 12 + 2 ] = 0;
    r[ 0 + 3 ] = 0;
    r[ 4 + 3 ] = 0;
    r[ 8 + 3 ] = 0;
    r[ 12 + 3 ] = 1;
    matrix_4x4_mul( res, m, r );
    bmem_copy( m, res, sizeof( float ) * 4 * 4 );
}

void matrix_4x4_translate( float x, float y, float z, float* m )
{
    float m2[ 4 * 4 ];
    float res[ 4 * 4 ];
    bmem_set( m2, sizeof( float ) * 4 * 4, 0 );
    m2[ 0 ] = 1;
    m2[ 4 + 1 ] = 1;
    m2[ 8 + 2 ] = 1;
    m2[ 12 + 0 ] = x;
    m2[ 12 + 1 ] = y;
    m2[ 12 + 2 ] = z;
    m2[ 12 + 3 ] = 1;

    matrix_4x4_mul( res, m, m2 );
    bmem_copy( m, res, sizeof( float ) * 4 * 4 );
}

void matrix_4x4_scale( float x, float y, float z, float* m )
{
    float m2[ 4 * 4 ];
    float res[ 4 * 4 ];
    bmem_set( m2, sizeof( float ) * 4 * 4, 0 );
    m2[ 0 ] = x;
    m2[ 4 + 1 ] = y;
    m2[ 8 + 2 ] = z;
    m2[ 12 + 3 ] = 1;

    matrix_4x4_mul( res, m, m2 );
    bmem_copy( m, res, sizeof( float ) * 4 * 4 );
}

void matrix_4x4_ortho( float left, float right, float bottom, float top, float z_near, float z_far, float* m )
{
    float m2[ 4 * 4 ];
    float res[ 4 * 4 ];
    bmem_set( m2, sizeof( float ) * 4 * 4, 0 );
    m2[ 0 ] = 2 / ( right - left );
    m2[ 4 + 1 ] = 2 / ( top - bottom );
    m2[ 8 + 2 ] = -2 / ( z_far - z_near );
    m2[ 12 + 0 ] = - ( right + left ) / ( right - left );
    m2[ 12 + 1 ] = - ( top + bottom ) / ( top - bottom );
    m2[ 12 + 2 ] = - ( z_far + z_near ) / ( z_far - z_near );
    m2[ 12 + 3 ] = 1;

    matrix_4x4_mul( res, m, m2 );
    bmem_copy( m, res, sizeof( float ) * 4 * 4 );
}

//MISC:

size_t round_to_power_of_two( size_t v )
{
    size_t b = 1;
    while( 1 )
    {
        if( b >= v )
        {
	    break;
	}
	b <<= 1;
    }
    return b;
}

uint sqrt_newton( uint l )
{
    uint temp, div; 
    uint rslt = l; 
    
    if( l <= 0 ) return 0;
    
    if( l & 0xFFFF0000 )
    {
	if( l & 0xFF000000L )
	    div = 0x3FFF; 
	else
	    div = 0x3FF; 
    }
    else 
    {
    	if( l & 0x0FF00 ) 
    	    div = 0x3F; 
	else
	    div = ( l > 4 ) ? 0x7 : l; 
    }
    
    while( l )
    {
	temp = l / div + div; 
	div = temp >> 1; 
	div += temp & 1; 
	if( rslt > div ) 
	{
	    rslt = (unsigned)div; 
	}
	else 
	{
	    if( l / rslt == rslt - 1 && l % rslt == 0 )
		rslt--; 
            return rslt; 
        }
    }
    
    return 0;
}
