/*
    memory.cpp. Memory manager (thread safe)
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100% with global init/deinit

#include "core/core.h"
#include "../memory.h"

mem_block_struct* g_mem_start = 0;
mem_block_struct* g_mem_prev_block = 0;  //Previous memory block
size_t g_mem_size = 0;
size_t g_mem_max_size = 0;
bmutex g_mem_mutex;
size_t g_mem_error = 0;

void bmem_global_init( void )
{
    g_mem_start = 0;
    g_mem_prev_block = 0;
    g_mem_size = 0;
    g_mem_max_size = 0;
    g_mem_error = 0;
#ifndef MEM_FAST_MODE
    bmutex_init( &g_mem_mutex, 0 );
#endif
}

void bmem_global_deinit( void )
{
#ifndef MEM_FAST_MODE
    bmutex_destroy( &g_mem_mutex );
#endif
}

int bmem_free_all( void ) //Latest memory function in project before bmem_deinit(). NOT thread safe
{
#ifdef MEM_FAST_MODE
    return 0;
#else
    int rv = 0;
    mem_block_struct* next;
    int mnum = 0;
    int mlimit = 10;

    mem_block_struct* g_mem_start2 = g_mem_start;
    if( g_mem_start2 ) blog( "MEMORY CLEANUP\n" );
    mnum = 0;
    while( g_mem_start2 )
    {
#ifdef USE_NAMES
	utf8_char name[ MEM_MAX_NAME_SIZE ];
	name[ 0 ] = 0;
	bmem_strcat( name, sizeof( name ), (const char*)g_mem_start2->name );
#endif	
	size_t size;
	next = g_mem_start2->next;
	size = g_mem_start2->size;
	g_mem_start2 = next;
	if( mnum < mlimit )
	{
#ifdef USE_NAMES
	    blog( "FREE %d %s\n", (int)size, name );
#else
	    blog( "FREE %d\n", (int)size );
#endif
	}
	mnum++;
    }
    while( g_mem_start )
    {
	size_t size;
	next = g_mem_start->next;
	size = g_mem_start->size;
	simple_bmem_free( g_mem_start );
	g_mem_start = next;
    }

    g_mem_start = 0;
    g_mem_prev_block = 0;
 
    blog( "Max memory used: %d\n", (int)g_mem_max_size );
    if( g_mem_size )
    {
	blog( "Not freed: %d\n", (int)g_mem_size );
	rv = 1;
    }
    
    return rv;
#endif //not MEM_FAST_MODE
}

//Main functions:
void* bmem_new_ext( size_t size, const utf8_char* name )
{
    size_t real_size = size;
    size_t new_size = size + sizeof( mem_block_struct ); //Add structure with info to our memory block
    mem_block_struct* m = (mem_block_struct*)malloc( new_size );

    //Save info about new memory block:
    if( m )
    {
	m->size = size;
#ifdef USE_NAMES
	utf8_char* mname = (utf8_char*)m->name;
	for( int np = 0; np < MEM_MAX_NAME_SIZE - 1; np++ ) { mname[ np ] = name[ np ]; if( name[ np ] == 0 ) break; }
	mname[ MEM_MAX_NAME_SIZE - 1 ] = 0;
#endif

#ifndef MEM_FAST_MODE
	bmutex_lock( &g_mem_mutex );
	
	m->prev = g_mem_prev_block;
	m->next = 0;
	if( g_mem_prev_block == 0 )
	{
	    //It is the first block. Save address:
	    g_mem_start = m;
	    g_mem_prev_block = m;
	}
	else
	{ 
	    //It is not the first block:
	    g_mem_prev_block->next = m;
	    g_mem_prev_block = m;
	}

	g_mem_size += real_size; if( g_mem_size > g_mem_max_size ) g_mem_max_size = g_mem_size;

	bmutex_unlock( &g_mem_mutex );
#endif
    }
    else
    { 
	blog( "MEM ALLOC ERROR %d %s\n", (int)size, name );
	if( g_mem_error == 0 )
	{
	    g_mem_error = size;
	}
	return 0;
    }
    
    char* rv = (char*)m;
    return (void*)( rv + sizeof( mem_block_struct ) );
}

void simple_bmem_free( void* ptr )
{
    free( ptr );
}

void bmem_free( void* ptr )
{
    if( ptr == 0 ) return;
    
    mem_block_struct* m = (mem_block_struct*)( (char*)ptr - sizeof( mem_block_struct ) );
    
#ifndef MEM_FAST_MODE
    bmutex_lock( &g_mem_mutex );
#endif
    
    g_mem_size -= m->size;

#ifndef MEM_FAST_MODE
    mem_block_struct* prev = m->prev;
    mem_block_struct* next = m->next;
    if( prev && next )
    {
	prev->next = next;
	next->prev = prev;
    }
    if( prev && next == 0 )
    {
	prev->next = 0;
	g_mem_prev_block = prev;
    }
    if( prev == 0 && next )
    {
	next->prev = 0;
	g_mem_start = next;
    }
    if( prev == 0 && next == 0 )
    {
	g_mem_prev_block = 0;
	g_mem_start = 0;
    }
    
    bmutex_unlock( &g_mem_mutex );
#endif //not MEM_FAST_MODE

    simple_bmem_free( m );
}

void bmem_zero( void* ptr )
{
    if( ptr == 0 ) return;
    bmem_set( ptr, bmem_get_size( ptr ), 0 );
}

void* bmem_resize( void* ptr, size_t new_size )
{
    if( ptr == 0 ) 
    {
	return bmem_new( new_size );
    }
    
    size_t old_size = bmem_get_size( ptr );

    if( old_size == new_size ) return ptr;
    
    //realloc():
#ifdef MEM_FAST_MODE
    mem_block_struct* m = (mem_block_struct*)( (char*)ptr - sizeof( mem_block_struct ) );
    mem_block_struct* new_m = (mem_block_struct*)realloc( m, new_size + sizeof( mem_block_struct ) );
    void* new_ptr = (void*)( (char*)new_m + sizeof( mem_block_struct ) );
    new_m->size = new_size;
    g_mem_size += new_size - old_size; if( g_mem_size > g_mem_max_size ) g_mem_max_size = g_mem_size;
#else
    bmutex_lock( &g_mem_mutex );
    int change_prev_block = 0;
    mem_block_struct* m = (mem_block_struct*)( (char*)ptr - sizeof( mem_block_struct ) );
    if( g_mem_prev_block == m ) change_prev_block |= 1;
    mem_block_struct* new_m = (mem_block_struct*)realloc( m, new_size + sizeof( mem_block_struct ) );
    void* new_ptr = (void*)( (char*)new_m + sizeof( mem_block_struct ) );
    if( change_prev_block & 1 ) g_mem_prev_block = new_m;
    new_m->size = new_size;
    mem_block_struct* prev = new_m->prev;
    mem_block_struct* next = new_m->next;
    if( prev == 0 )
    {
	g_mem_start = new_m;
    }
    if( prev != 0 )
    {
	prev->next = new_m;
    }
    if( next != 0 )
    {
	next->prev = new_m;
    }
    g_mem_size += new_size - old_size; if( g_mem_size > g_mem_max_size ) g_mem_max_size = g_mem_size;
    bmutex_unlock( &g_mem_mutex );
#endif //not MEM_FAST_MODE
    if( old_size < new_size )
    {
	bmem_set( (char*)new_ptr + old_size, new_size - old_size, 0 );
    }
    
    return new_ptr;
}

void* bmem_clone( void* ptr )
{
    if( ptr == 0 ) return 0;
    void* ptr2 = bmem_new( bmem_get_size( ptr ) );
    if( ptr2 == 0 ) return 0;
    bmem_copy( ptr2, ptr, bmem_get_size( ptr ) );
    return ptr2;
}

int bmem_strcat( char* dest, size_t dest_size, const char* src )
{
    if( dest == 0 || src == 0 || dest_size == 0 ) return 1;
    size_t i;
    for( i = 0; i < dest_size; i++ )
    {
	if( dest[ i ] == 0 ) break;
    }
    if( i == dest_size )
    {
	return 1;
    }
    for( ; i < dest_size; i++ )
    {
	dest[ i ] = *src;
	if( *src == 0 ) break;
	src++;
    }
    if( i == dest_size )
    {
	dest[ dest_size - 1 ] = 0;
	return 1;
    }
    return 0;
}

char* bmem_strcat_d( char* dest, const char* src )
{
    if( dest == 0 ) return 0;
    if( src == 0 ) return dest;
    size_t dest_size = bmem_get_size( dest );
    size_t dest_len = bmem_strlen( dest );
    size_t src_len = bmem_strlen( src );
    if( dest_size == 0 || src_len == 0 ) return dest;
    if( dest_len + src_len + 1 > dest_size )
    {
	dest = (char*)bmem_resize( dest, dest_len + src_len + 64 );
    }
    bmem_copy( dest + dest_len, src, src_len + 1 );
    return dest;
}

size_t bmem_strlen( const char* s )
{
    if( s == 0 ) return 0;
    size_t a;
    for( a = 0 ; ; a++ ) if( s[ a ] == 0 ) break;
    return a;
}

size_t bmem_strlen_utf16( const utf16_char* s )
{
    if( s == 0 ) return 0;
    size_t a;
    for( a = 0 ; ; a++ ) if( s[ a ] == 0 ) break;
    return a;
}

size_t bmem_strlen_utf32( const utf32_char* s )
{
    if( s == 0 ) return 0;
    size_t a;
    for( a = 0 ; ; a++ ) if( s[ a ] == 0 ) break;
    return a;
}

char* bmem_strdup( const char* s1 )
{
    if( s1 == 0 ) return 0;
    size_t len = bmem_strlen( s1 );
    char* newstr = (char*)bmem_new( len + 1 );
    bmem_copy( newstr, s1, len + 1 );
    return newstr;
}

size_t bmem_get_usage( void )
{
    return g_mem_size;
}
