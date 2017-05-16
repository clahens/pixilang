#pragma once

#ifndef MEM_FAST_MODE
    #define USE_NAMES
#endif

#define MEM_MAX_NAME_SIZE 16

struct mem_block_struct 
{
    size_t size;
#ifdef USE_NAMES
    utf8_char name[ MEM_MAX_NAME_SIZE ];
#endif
#ifndef MEM_FAST_MODE
    mem_block_struct* next;
    mem_block_struct* prev;
#endif
};

#define bmem_new( size ) bmem_new_ext( size, (utf8_char*)__FUNCTION__ )
#define bmem_clear_struct( s ) bmem_set( &( s ), sizeof( s ), 0 )

extern size_t g_mem_error;

void bmem_global_init( void );
void bmem_global_deinit( void );
int bmem_free_all( void );
void* bmem_new_ext( size_t size, const utf8_char* name ); //Each memory block has its own name
void simple_bmem_free( void* ptr );
void bmem_free( void* ptr );
inline void bmem_set( void* ptr, size_t size, unsigned char value )
{
    if( ptr == 0 ) return;
    memset( ptr, value, size );
}
void bmem_zero( void* ptr );
void* bmem_resize( void* ptr, size_t size );
inline void bmem_copy( void* dest, const void* src, size_t size ) //Overlapping allowed
{
    if( dest == 0 || src == 0 ) return;
    memmove( dest, src, size ); //Overlapping allowed
}
inline int bmem_cmp( const char* p1, const char* p2, size_t size )
{
    if( p1 == 0 || p2 == 0 ) return 0;
    return memcmp( p1, p2, size );
}
void* bmem_clone( void* ptr );
int bmem_strcat( char* dest, size_t dest_size, const char* src );
char* bmem_strcat_d( char* dest, const char* src ); //Dynamic version with bmem_resize(). Use it with the SunDog (bmem_new) memory blocks only!
#define bmem_strcat_resize( dest, src ) dest = bmem_strcat_d( dest, src )
inline int bmem_strcmp( const char* s1, const char* s2 )
{
    if( s1 == 0 || s2 == 0 ) return 0;
    return strcmp( s1, s2 );
}
inline const char* bmem_strstr( const char* s1, const char* s2 )
{
    if( s1 == 0 || s2 == 0 ) return 0;
    return strstr( s1, s2 );
}
inline char* bmem_strstr( char* s1, const char* s2 )
{
    if( s1 == 0 || s2 == 0 ) return 0;
    return strstr( s1, s2 );
}
size_t bmem_strlen( const char* s );
size_t bmem_strlen_utf16( const utf16_char* s );
size_t bmem_strlen_utf32( const utf32_char* s );
char* bmem_strdup( const char* s1 );

//Get info:
inline size_t bmem_get_size( void* ptr )
{
    if( ptr == 0 ) return 0;
    mem_block_struct* m = (mem_block_struct*)( (char*)ptr - sizeof( mem_block_struct ) );
    return m->size;
}
inline utf8_char* bmem_get_name( void* ptr )
{
#ifdef USE_NAMES
    if( ptr == 0 ) return 0;
    mem_block_struct* m = (mem_block_struct*)( (char*)ptr - sizeof( mem_block_struct ) );
    return m->name;
#else
    return 0;
#endif
}
size_t bmem_get_usage( void );
