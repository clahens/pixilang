/*
    pixilang_vm_font.cpp
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

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
    #define DPRINT( fmt, ARGS... ) blog( fmt, ## ARGS )
#else
    #define DPRINT( fmt, ARGS... ) {}
#endif

pix_vm_font* pix_vm_get_font_for_char( utf32_char c, pix_vm* vm )
{
    int fnum;
    int fonts = sizeof( vm->fonts ) / sizeof( pix_vm_font );
    pix_vm_font* font;
    for( fnum = 0; fnum < fonts; fnum++ )
    {
	font = &vm->fonts[ fnum ];
	if( (unsigned)font->font < (unsigned)vm->c_num )
	{
	    if( c >= font->first && c <= font->last )
	    {
		break;
	    }
	}
    }
    if( fnum == fonts ) return 0; //Font not found for selected character.
	return font;
}

int pix_vm_set_font( utf32_char first_char, PIX_CID cnum, int xchars, int ychars, pix_vm* vm )
{
    pix_vm_font* font = 0;
    int fonts = sizeof( vm->fonts ) / sizeof( pix_vm_font );
    for( int fnum = 0; fnum < fonts; fnum++ )
    {
	font = &vm->fonts[ fnum ];
	if( font->first == first_char ) break;
	font = 0;
    }
    if( font == 0 )
    {
	//No font for selected first_char. Create it:
	for( int fnum = 0; fnum < fonts; fnum++ )
	{
	    font = &vm->fonts[ fnum ];
	    if( font->font == -1 ) 
	    {
		font->xchars = 16;
		font->ychars = 16;
		break;
	    }
	    font = 0;
	}
    }
    if( font == 0 )
    {
	return 1;
    }
    if( xchars > 0 ) font->xchars = xchars;
    if( ychars > 0 ) font->ychars = ychars;
    font->first = first_char;
    font->last = first_char + font->xchars * font->ychars - 1;
    font->font = cnum;
    return 0;
}
