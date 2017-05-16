#pragma once

void blog_global_init( const utf8_char* filename );
void blog_global_deinit( void );
void blog_disable( void );
void blog_enable( void );
const utf8_char* blog_get_file( void );
utf8_char* blog_get_latest( uint size );
void blog( const utf8_char* str, ... );

void blog_show_error_report( void );
