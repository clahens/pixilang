/*
    pixilang_symtab.cpp
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

int pix_symtab_init( size_t size, pix_symtab* st )
{
    int rv = 0;
    
    if( st == 0 ) return -1;
    
    st->size = size; //53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613, 393241, 786433, 1572869 ...
    st->symtab = (pix_sym**)bmem_new( st->size * sizeof( pix_sym* ) );
    bmem_zero( st->symtab );
    
    return rv;
}

int pix_symtab_hash( const utf8_char* name, size_t size ) //32bit version!
{
    uint h = 0;
    uchar* p = (uchar*)name;
    
    for( ; *p != 0; p++ )
	h = 31 * h + *p;
	
    return (int)( h % (int)size );
}

pix_sym* pix_symtab_lookup( const utf8_char* name, int hash, bool create, pix_sym_type type, PIX_INT ival, PIX_FLOAT fval, bool* created, pix_symtab* st )
{
    pix_sym* s;
    
    if( st == 0 ) return 0;
    if( st->symtab == 0 ) return 0;
    
    if( created ) *created = 0;
    
    if( hash < 0 ) hash = pix_symtab_hash( name, st->size );
    for( s = st->symtab[ hash ]; s != 0; s = s->next )
	if( bmem_strcmp( name, s->name ) == 0 )
	    return s;
	    
    if( create )
    {
	//Create new symbol:
	s = (pix_sym*)bmem_new( sizeof( pix_sym ) );
	size_t slen = bmem_strlen( name ) + 1;
	s->name = (utf8_char*)bmem_new( slen );
	bmem_copy( s->name, name, slen );
	s->type = type;
	if( type == SYMTYPE_NUM_F )
	    s->val.f = fval;
	else
	    s->val.i = ival;
	s->next = st->symtab[ hash ];
	st->symtab[ hash ] = s;
	if( created ) *created = 1;
    }
    
    return s;
}

pix_sym* pix_sym_clone( pix_sym* s )
{
    if( s == 0 ) return 0;
    pix_sym* s2 = (pix_sym*)bmem_new( sizeof( pix_sym ) );
    bmem_copy( s2, s, sizeof( pix_sym ) );
    size_t slen = bmem_get_size( s->name );
    s2->name = (utf8_char*)bmem_new( slen );
    bmem_copy( s2->name, s->name, slen );
    if( s->next )
    {
	s2->next = pix_sym_clone( s->next );
    }
    return s2;
}

int pix_symtab_clone( pix_symtab* dest_st, pix_symtab* src_st )
{
    if( dest_st == 0 ) return -1;
    if( src_st == 0 ) return -1;
    
    if( pix_symtab_init( src_st->size, dest_st ) ) return -1;
    
    for( size_t i = 0; i < src_st->size; i++ )
    {
	pix_sym* s = src_st->symtab[ i ];
	if( s )
	{
	    dest_st->symtab[ i ] = pix_sym_clone( s );
	}
    }
    
    return 0;
}

pix_sym* pix_symtab_get_list( pix_symtab* st )
{
    pix_sym* rv = 0;
    size_t size = 0;
    if( st == 0 ) return 0;
    if( st->symtab == 0 ) return 0;

    for( size_t i = 0; i < st->size; i++ )
    {
	pix_sym* s = st->symtab[ i ];
	while( s )
	{
	    if( s->name )
	    {
		if( size == 0 )
		    rv = (pix_sym*)bmem_new( sizeof( pix_sym ) * 8 );
		else
		{
		    if( size >= bmem_get_size( rv ) / sizeof( pix_sym ) )
			rv = (pix_sym*)bmem_resize( rv, ( size + 8 ) * sizeof( pix_sym ) );
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
	rv = (pix_sym*)bmem_resize( rv, size * sizeof( pix_sym ) );
    }
    
    return rv;
}

int pix_symtab_deinit( pix_symtab* st )
{
    int rv = 0;
    
    if( st == 0 ) return -1;
    if( st->symtab == 0 ) return -1;
    
    for( size_t i = 0; i < st->size; i++ )
    {
	pix_sym* s = st->symtab[ i ];
	while( s )
	{
	    pix_sym* next = s->next;
	    bmem_free( s->name );
	    bmem_free( s );
	    s = next;
	}
    }
    bmem_free( st->symtab );
    st->symtab = 0;
    
    return rv;
}

