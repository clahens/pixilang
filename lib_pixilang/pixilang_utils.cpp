/*
    pixilang_utils.cpp
    This file is part of the Pixilang programming language.

    [ MIT license ]

    Copyright (c) 2006 - 2016, Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to 
    deal in the Software without restriction, including without limitation the 
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is 
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in 
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

//Modularity: 100%

#include "core/core.h"
#include "pixilang.h"

utf8_char* pix_get_base_path( const utf8_char* src_name )
{
    int i;
    for( i = (int)bmem_strlen( src_name ) - 1; i >= 0; i-- )
    {
	if( src_name[ i ] == '/' ) break;
    }
    if( i <= 0 )
    {
	utf8_char* rv = (utf8_char*)bmem_new( 1 * sizeof( utf8_char ) );
	rv[ 0 ] = 0;
	return rv;
    }
    else 
    {
	utf8_char* rv = (utf8_char*)bmem_new( ( i + 2 ) * sizeof( utf8_char ) );
	bmem_copy( rv, src_name, ( i + 1 ) * sizeof( utf8_char ) );
	rv[ i + 1 ] = 0;
	return rv;
    }
}

utf8_char* pix_compose_full_path( utf8_char* base_path, utf8_char* file_name, pix_vm* vm )
{
    utf8_char* new_name;
    int name_len = (int)bmem_strlen( file_name );
    if( vm && vm->virt_disk0 && name_len > 3 )
    {
	if( file_name[ 0 ] == '0' && file_name[ 1 ] == ':' && file_name[ 2 ] == '/' )
	{
	    //Virtual disk0:
	    new_name = (utf8_char*)bmem_new( name_len + 16 );
	    sprintf( new_name, "vfs%d:/%s", vm->virt_disk0, file_name + 3 );
	    return new_name;
	}
    }
    if( ( name_len > 1 && file_name[ 0 ] == '/' ) ||
        ( name_len > 2 && file_name[ 1 ] == ':' ) )
    {
	//File name with absolute path:
	new_name = (utf8_char*)bmem_new( name_len + 1 );
	bmem_copy( new_name, file_name, name_len + 1 );
    }
    else 
    {
	if( base_path == 0 || base_path[ 0 ] == 0 )
	{
	    //Base path is empty:
	    new_name = (utf8_char*)bmem_new( name_len + 1 );
	    bmem_copy( new_name, file_name, name_len + 1 );
	}
	else 
	{
	    size_t len = bmem_strlen( base_path );
	    new_name = (utf8_char*)bmem_new( len + 1 + name_len + 1 );
	    new_name[ 0 ] = 0;
	    bmem_strcat_resize( new_name, base_path );
	    if( base_path[ len - 1 ] != '/' )
		bmem_strcat_resize( new_name, "/" );
	    bmem_strcat_resize( new_name, file_name );
	}
    }
    return new_name;
}
