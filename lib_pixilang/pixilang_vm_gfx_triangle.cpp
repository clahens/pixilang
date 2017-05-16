/*
    pixilang_vm_gfx_triangle.cpp
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

//y - int
//x1, x2 - fixed point PIX_FIXED_MATH_PREC
inline static void pix_vm_gfx_draw_hline( int y, PIX_INT x1, PIX_INT x2, COLOR color, pix_vm* vm )
{
    //Correction:
    if( x1 > x2 ) { PIX_INT temp = x1; x1 = x2; x2 = temp; }
    x1 >>= PIX_FIXED_MATH_PREC;
    x2 >>= PIX_FIXED_MATH_PREC;
    if( x1 >= vm->screen_xsize ) return;
    if( x2 < 0 ) return;
    
    //Crop:
    if( x1 < 0 ) 
    {
	x1 = 0;
    }
    if( x2 > vm->screen_xsize )
    {
	x2 = vm->screen_xsize;
    }
    
    //Draw:
    COLORPTR ptr = vm->screen_ptr + y * vm->screen_xsize + x1;
    COLORPTR ptr2;
    uchar transp = vm->transp;
    if( transp == 255 )
    {
	ptr2 = ptr + ( x2 - x1 );
	while( ptr < ptr2 )
	{
	    *ptr++ = color;
	}
    }
    else
    {
	ptr2 = ptr + ( x2 - x1 );
	while( ptr < ptr2 )
	{
	    *ptr = fast_blend( *ptr, color, transp );
	    ptr++;
	}
    }
}

//y - int
//x1, z1, x2, z2 - fixed point PIX_FIXED_MATH_PREC
inline static void pix_vm_gfx_draw_hline_zbuf( int y, PIX_INT x1, PIX_INT z1, PIX_INT x2, PIX_INT z2, COLOR color, COLORPTR scr, int* zbuf, pix_vm* vm )
{
    //Correction:
    if( x1 > x2 ) { PIX_INT temp = x1; x1 = x2; x2 = temp; temp = z1; z1 = z2; z2 = temp; }
    x1 >>= PIX_FIXED_MATH_PREC;
    x2 >>= PIX_FIXED_MATH_PREC;
    if( x1 >= vm->screen_xsize ) return;
    if( x2 < 0 ) return;
    
    PIX_INT size = x2 - x1;
    if( size == 0 ) return;
    PIX_INT z_delta = ( z2 - z1 ) / size;
    
    //Crop:
    if( x1 < 0 ) 
    {
	z1 += z_delta * -x1;
	x1 = 0;
    }
    if( x2 > vm->screen_xsize )
    {
	x2 = vm->screen_xsize;
    }
    
    //Draw:
    size = x2 - x1;
    int scr_off = y * vm->screen_xsize + x1;
    uchar transp = vm->transp;
    if( transp == 255 )
    {
	while( size-- )
	{
	    if( zbuf[ scr_off ] < z1 )
	    {
		scr[ scr_off ] = color;
		zbuf[ scr_off ] = z1;
	    }
	    scr_off++;
	    z1 += z_delta;
	}
    }
    else
    {
	while( size-- )
	{
	    if( zbuf[ scr_off ] < z1 )
	    {
		scr[ scr_off ] = fast_blend( scr[ scr_off ], color, transp );
		zbuf[ scr_off ] = z1;
	    }
	    scr_off++;
	    z1 += z_delta;
	}
    }
}

#define ADD_ZBUF \
    scr_off++; \
    z1 += z_delta; \
    tx1 += tx_delta; \
    ty1 += ty_delta;

#define ADD_WITHOUT_ZBUF \
    scr_off++; \
    tx1 += tx_delta; \
    ty1 += ty_delta;

#define GET_TEXTURE_PIXEL \
    int tx = tx1 >> PIX_TEX_FIXED_MATH_PREC; \
    int ty = ty1 >> PIX_TEX_FIXED_MATH_PREC; \
    if( tx < 0 ) tx = 0; \
    if( tx >= txt_xsize ) tx = txt_xsize - 1; \
    if( ty < 0 ) ty = 0; \
    if( ty >= txt_ysize ) ty = txt_ysize - 1; \
    COLOR pixel = txt[ txt_xsize * ty + tx ];
    
#define GET_TEXTURE_PIXEL_INTERP \
    COLOR p1, p2, p3, p4; \
    int ttx, tty; \
    int tx = tx1 >> PIX_TEX_FIXED_MATH_PREC; \
    int ty = ty1 >> PIX_TEX_FIXED_MATH_PREC; \
    ttx = tx; tty = ty; \
    if( ttx < 0 ) ttx = 0; if( ttx >= txt_xsize ) ttx = txt_xsize - 1; if( tty < 0 ) tty = 0; if( tty >= txt_ysize ) tty = txt_ysize - 1; \
    p1 = txt[ txt_xsize * tty + ttx ]; \
    tty = ty + 1; \
    if( tty < 0 ) tty = 0; if( tty >= txt_ysize ) tty = txt_ysize - 1; \
    p3 = txt[ txt_xsize * tty + ttx ]; \
    ttx = tx + 1; \
    if( ttx < 0 ) ttx = 0; if( ttx >= txt_xsize ) ttx = txt_xsize - 1; \
    p4 = txt[ txt_xsize * tty + ttx ]; \
    tty = ty; \
    if( tty < 0 ) tty = 0; if( tty >= txt_ysize ) tty = txt_ysize - 1; \
    p2 = txt[ txt_xsize * tty + ttx ]; \
    int xc = ( tx1 >> ( PIX_TEX_FIXED_MATH_PREC - 8 ) ) & 255; \
    int yc = ( ty1 >> ( PIX_TEX_FIXED_MATH_PREC - 8 ) ) & 255; \
    COLOR pixel = blend( p1, p2, xc ); \
    COLOR pixel2 = blend( p3, p4, xc ); \
    pixel = blend( pixel, pixel2, yc );

#define GET_TEXTURE_PIXEL_WITH_ALPHA \
    int tx = tx1 >> PIX_TEX_FIXED_MATH_PREC; \
    int ty = ty1 >> PIX_TEX_FIXED_MATH_PREC; \
    if( tx < 0 ) tx = 0; \
    if( tx >= txt_xsize ) tx = txt_xsize - 1; \
    if( ty < 0 ) ty = 0; \
    if( ty >= txt_ysize ) ty = txt_ysize - 1; \
    tx = txt_xsize * ty + tx; \
    uchar pixel_alpha = txt_alpha[ tx ]; \
    COLOR pixel; \
    if( pixel_alpha ) pixel = txt[ tx ];

#define GET_TEXTURE_PIXEL_WITH_ALPHA_INTERP \
    COLOR p1, p2, p3, p4; \
    COLOR pixel, pixel2; \
    uchar a1, a2, a3, a4; \
    int ttx, tty, toff; \
    int tx = tx1 >> PIX_TEX_FIXED_MATH_PREC; \
    int ty = ty1 >> PIX_TEX_FIXED_MATH_PREC; \
    ttx = tx; tty = ty; \
    if( ttx < 0 ) ttx = 0; if( ttx >= txt_xsize ) ttx = txt_xsize - 1; if( tty < 0 ) tty = 0; if( tty >= txt_ysize ) tty = txt_ysize - 1; \
    toff = txt_xsize * tty + ttx; p1 = txt[ toff ]; a1 = txt_alpha[ toff ]; \
    tty = ty + 1; \
    if( tty < 0 ) tty = 0; if( tty >= txt_ysize ) tty = txt_ysize - 1; \
    toff = txt_xsize * tty + ttx; p3 = txt[ toff ]; a3 = txt_alpha[ toff ]; \
    ttx = tx + 1; \
    if( ttx < 0 ) ttx = 0; if( ttx >= txt_xsize ) ttx = txt_xsize - 1; \
    toff = txt_xsize * tty + ttx; p4 = txt[ toff ]; a4 = txt_alpha[ toff ]; \
    tty = ty; \
    if( tty < 0 ) tty = 0; if( tty >= txt_ysize ) tty = txt_ysize - 1; \
    toff = txt_xsize * tty + ttx; p2 = txt[ toff ]; a2 = txt_alpha[ toff ]; \
    int xc = ( tx1 >> ( PIX_TEX_FIXED_MATH_PREC - 8 ) ) & 255; \
    int yc = ( ty1 >> ( PIX_TEX_FIXED_MATH_PREC - 8 ) ) & 255; \
    uchar pixel_alpha = (uchar)( (int)( (int)a1 * ( 255 - xc ) + (int)a2 * xc ) >> 8 ); \
    uchar pixel_alpha2 = (uchar)( (int)( (int)a3 * ( 255 - xc ) + (int)a4 * xc ) >> 8 ); \
    pixel_alpha = (uchar)( (int)( (int)pixel_alpha * ( 255 - yc ) + (int)pixel_alpha2 * yc ) >> 8 ); \
    if( pixel_alpha ) \
    { \
	pixel = blend( p1, p2, xc ); \
	pixel2 = blend( p3, p4, xc ); \
	pixel = blend( pixel, pixel2, yc ); \
    }

#define COLORIZE \
    if( color != COLORMASK ) \
    { \
	int pixel_r = ( red( pixel ) * red( color ) ) / 256; \
	int pixel_g = ( green( pixel ) * green( color ) ) / 256; \
	int pixel_b = ( blue( pixel ) * blue( color ) ) / 256; \
	pixel = get_color( pixel_r, pixel_g, pixel_b ); \
    }

//y - int
//other - fixed point PIX_FIXED_MATH_PREC
inline static void pix_vm_gfx_draw_hline_t( 
    int y, 
    PIX_INT x1, 
    PIX_INT z1, 
    PIX_INT tx1,
    PIX_INT ty1,
    PIX_INT x2, 
    PIX_INT z2, 
    PIX_INT tx2,
    PIX_INT ty2,
    COLOR color,
    COLORPTR txt,
    int txt_xsize,
    int txt_ysize,
    uchar* txt_alpha,
    bool uses_key,
    COLOR key,
    COLORPTR scr, 
    int* zbuf, 
    pix_vm* vm )
{
    //Correction:
    if( x1 > x2 ) 
    { 
	PIX_INT temp = x1; x1 = x2; x2 = temp; 
	temp = z1; z1 = z2; z2 = temp; 
	temp = tx1; tx1 = tx2; tx2 = temp; 
	temp = ty1; ty1 = ty2; ty2 = temp;
    }
    x1 >>= PIX_FIXED_MATH_PREC;
    x2 >>= PIX_FIXED_MATH_PREC;
    if( x1 >= vm->screen_xsize ) return;
    if( x2 < 0 ) return;
    
    PIX_INT size = x2 - x1;
    if( size == 0 ) return;

    PIX_INT z_delta;
    if( zbuf )
    {
	z_delta = ( z2 - z1 ) / size;
    }
    
    PIX_INT tx_delta = ( tx2 - tx1 ) / size;
    PIX_INT ty_delta = ( ty2 - ty1 ) / size;
    
    //Crop:
    if( x1 < 0 ) 
    {
	if( zbuf ) z1 += z_delta * -x1;
	tx1 += tx_delta * -x1;
	ty1 += ty_delta * -x1;
	x1 = 0;
    }
    if( x2 > vm->screen_xsize )
    {
	x2 = vm->screen_xsize;
    }
    
    //Draw:
    size = x2 - x1;
    int scr_off = y * vm->screen_xsize + x1;
    uchar transp = vm->transp;
    if( txt_alpha )
    {
	//With alpha-channel:
	if( transp == 255 )
	{
	    //Not transparent:
	    if( zbuf )
	    {
		//With Z-Buffer:
		while( size-- )
		{
		    if( zbuf[ scr_off ] < z1 )
		    {
			GET_TEXTURE_PIXEL_WITH_ALPHA;
			if( pixel_alpha && !( uses_key && key == pixel ) )
			{
			    COLORIZE;
			    scr[ scr_off ] = blend( scr[ scr_off ], pixel, pixel_alpha );
			    zbuf[ scr_off ] = z1;
			}
		    }
		    ADD_ZBUF;
		}
	    }
	    else 
	    {
		//Without Z-Buffer:
		while( size-- )
		{
		    GET_TEXTURE_PIXEL_WITH_ALPHA;
		    if( pixel_alpha && !( uses_key && key == pixel ) )
		    {
			COLORIZE;
			scr[ scr_off ] = blend( scr[ scr_off ], pixel, pixel_alpha );
		    }
		    ADD_WITHOUT_ZBUF;
		}
	    }
	}
	else 
	{
	    //Transparent:
	    if( zbuf )
	    {
		//With Z-Buffer:
		while( size-- )
		{
		    if( zbuf[ scr_off ] < z1 )
		    {
			GET_TEXTURE_PIXEL_WITH_ALPHA;
			if( pixel_alpha && !( uses_key && key == pixel ) )
			{
			    COLORIZE;
			    pixel_alpha = (uchar)( (int)pixel_alpha * (int)transp / 256 );
			    scr[ scr_off ] = blend( scr[ scr_off ], pixel, pixel_alpha );
			    zbuf[ scr_off ] = z1;
			}
		    }
		    ADD_ZBUF;
		}
	    }
	    else 
	    {
		//Without Z-Buffer:
		while( size-- )
		{
		    GET_TEXTURE_PIXEL_WITH_ALPHA;
		    if( pixel_alpha && !( uses_key && key == pixel ) )
		    {
			COLORIZE;
			pixel_alpha = (uchar)( (int)pixel_alpha * (int)transp / 256 );
			scr[ scr_off ] = blend( scr[ scr_off ], pixel, pixel_alpha );
		    }
		    ADD_WITHOUT_ZBUF;
		}
	    }
	}
    }
    else 
    {
	//Without alpha-channel:
	if( transp == 255 )
	{
	    //Not transparent:
	    if( zbuf )
	    {
		//With Z-Buffer:
		while( size-- )
		{
		    if( zbuf[ scr_off ] < z1 )
		    {
			GET_TEXTURE_PIXEL;
			if( !( uses_key && key == pixel ) )
			{
			    COLORIZE;
			    scr[ scr_off ] = pixel;
			    zbuf[ scr_off ] = z1;
			}
		    }
		    ADD_ZBUF;
		}
	    }
	    else 
	    {
		//Without Z-Buffer:
		while( size-- )
		{
		    GET_TEXTURE_PIXEL;
		    if( !( uses_key && key == pixel ) ) 
		    {
			COLORIZE;
			scr[ scr_off ] = pixel;
		    }
		    ADD_WITHOUT_ZBUF;
		}
	    }
	}
	else 
	{
	    //Transparent:
	    if( zbuf )
	    {
		//With Z-Buffer:
		while( size-- )
		{
		    if( zbuf[ scr_off ] < z1 )
		    {
			GET_TEXTURE_PIXEL;
			if( !( uses_key && key == pixel ) )
			{
			    COLORIZE;
			    scr[ scr_off ] = fast_blend( scr[ scr_off ], pixel, transp );
			    zbuf[ scr_off ] = z1;
			}
		    }
		    ADD_ZBUF;
		}
	    }
	    else 
	    {
		//Without Z-Buffer:
		while( size-- )
		{
		    GET_TEXTURE_PIXEL;
		    if( !( uses_key && key == pixel ) )
		    {
			COLORIZE;
			scr[ scr_off ] = fast_blend( scr[ scr_off ], pixel, transp );
		    }
		    ADD_WITHOUT_ZBUF;
		}
	    }
	}
    }
}

//y - int
//other - fixed point PIX_FIXED_MATH_PREC
inline static void pix_vm_gfx_draw_hline_t_interp( 
    int y, 
    PIX_INT x1, 
    PIX_INT z1, 
    PIX_INT tx1,
    PIX_INT ty1,
    PIX_INT x2, 
    PIX_INT z2, 
    PIX_INT tx2,
    PIX_INT ty2,
    COLOR color,
    COLORPTR txt,
    int txt_xsize,
    int txt_ysize,
    uchar* txt_alpha,
    bool uses_key,
    COLOR key,
    COLORPTR scr, 
    int* zbuf, 
    pix_vm* vm )
{
    //Correction:
    if( x1 > x2 ) 
    { 
	PIX_INT temp = x1; x1 = x2; x2 = temp; 
	temp = z1; z1 = z2; z2 = temp; 
	temp = tx1; tx1 = tx2; tx2 = temp; 
	temp = ty1; ty1 = ty2; ty2 = temp;
    }
    x1 >>= PIX_FIXED_MATH_PREC;
    x2 >>= PIX_FIXED_MATH_PREC;
    if( x1 >= vm->screen_xsize ) return;
    if( x2 < 0 ) return;
    
    PIX_INT size = x2 - x1;
    if( size == 0 ) return;

    PIX_INT z_delta;
    if( zbuf )
    {
	z_delta = ( z2 - z1 ) / size;
    }
    
    PIX_INT tx_delta = ( tx2 - tx1 ) / size;
    PIX_INT ty_delta = ( ty2 - ty1 ) / size;
    
    //Crop:
    if( x1 < 0 ) 
    {
	if( zbuf ) z1 += z_delta * -x1;
	tx1 += tx_delta * -x1;
	ty1 += ty_delta * -x1;
	x1 = 0;
    }
    if( x2 > vm->screen_xsize )
    {
	x2 = vm->screen_xsize;
    }
    
    tx1 -= 1 << ( PIX_TEX_FIXED_MATH_PREC - 1 );
    ty1 -= 1 << ( PIX_TEX_FIXED_MATH_PREC - 1 );
    tx1 += tx_delta / 2;
    ty1 += ty_delta / 2;
    
    //Draw:
    size = x2 - x1;
    int scr_off = y * vm->screen_xsize + x1;
    uchar transp = vm->transp;
    if( txt_alpha )
    {
	//With alpha-channel:
	if( transp == 255 )
	{
	    //Not transparent:
	    if( zbuf )
	    {
		//With Z-Buffer:
		while( size-- )
		{
		    if( zbuf[ scr_off ] < z1 )
		    {
			GET_TEXTURE_PIXEL_WITH_ALPHA_INTERP;
			if( pixel_alpha && !( uses_key && key == pixel ) )
			{
			    COLORIZE;
			    scr[ scr_off ] = blend( scr[ scr_off ], pixel, pixel_alpha );
			    zbuf[ scr_off ] = z1;
			}
		    }
		    ADD_ZBUF;
		}
	    }
	    else 
	    {
		//Without Z-Buffer:
		while( size-- )
		{
		    GET_TEXTURE_PIXEL_WITH_ALPHA_INTERP;
		    if( pixel_alpha && !( uses_key && key == pixel ) )
		    {
			COLORIZE;
			scr[ scr_off ] = blend( scr[ scr_off ], pixel, pixel_alpha );
		    }
		    ADD_WITHOUT_ZBUF;
		}
	    }
	}
	else 
	{
	    //Transparent:
	    if( zbuf )
	    {
		//With Z-Buffer:
		while( size-- )
		{
		    if( zbuf[ scr_off ] < z1 )
		    {
			GET_TEXTURE_PIXEL_WITH_ALPHA_INTERP;
			if( pixel_alpha && !( uses_key && key == pixel ) )
			{
			    COLORIZE;
			    pixel_alpha = (uchar)( (int)pixel_alpha * (int)transp / 256 );
			    scr[ scr_off ] = blend( scr[ scr_off ], pixel, pixel_alpha );
			    zbuf[ scr_off ] = z1;
			}
		    }
		    ADD_ZBUF;
		}
	    }
	    else 
	    {
		//Without Z-Buffer:
		while( size-- )
		{
		    GET_TEXTURE_PIXEL_WITH_ALPHA_INTERP;
		    if( pixel_alpha && !( uses_key && key == pixel ) )
		    {
			COLORIZE;
			pixel_alpha = (uchar)( (int)pixel_alpha * (int)transp / 256 );
			scr[ scr_off ] = blend( scr[ scr_off ], pixel, pixel_alpha );
		    }
		    ADD_WITHOUT_ZBUF;
		}
	    }
	}
    }
    else 
    {
	//Without alpha-channel:
	if( transp == 255 )
	{
	    //Not transparent:
	    if( zbuf )
	    {
		//With Z-Buffer:
		while( size-- )
		{
		    if( zbuf[ scr_off ] < z1 )
		    {
			GET_TEXTURE_PIXEL_INTERP;
			if( !( uses_key && key == pixel ) )
			{
			    COLORIZE;
			    scr[ scr_off ] = pixel;
			    zbuf[ scr_off ] = z1;
			}
		    }
		    ADD_ZBUF;
		}
	    }
	    else 
	    {
		//Without Z-Buffer:
		while( size-- )
		{
		    GET_TEXTURE_PIXEL_INTERP;
		    if( !( uses_key && key == pixel ) ) 
		    {
			COLORIZE;
			scr[ scr_off ] = pixel;
		    }
		    ADD_WITHOUT_ZBUF;
		}
	    }
	}
	else 
	{
	    //Transparent:
	    if( zbuf )
	    {
		//With Z-Buffer:
		while( size-- )
		{
		    if( zbuf[ scr_off ] < z1 )
		    {
			GET_TEXTURE_PIXEL_INTERP;
			if( !( uses_key && key == pixel ) )
			{
			    COLORIZE;
			    scr[ scr_off ] = fast_blend( scr[ scr_off ], pixel, transp );
			    zbuf[ scr_off ] = z1;
			}
		    }
		    ADD_ZBUF;
		}
	    }
	    else 
	    {
		//Without Z-Buffer:
		while( size-- )
		{
		    GET_TEXTURE_PIXEL_INTERP;
		    if( !( uses_key && key == pixel ) )
		    {
			COLORIZE;
			scr[ scr_off ] = fast_blend( scr[ scr_off ], pixel, transp );
		    }
		    ADD_WITHOUT_ZBUF;
		}
	    }
	}
    }
}

void pix_vm_gfx_draw_triangle( pix_vm_ivertex* v1, pix_vm_ivertex* v2, pix_vm_ivertex* v3, COLOR color, pix_vm* vm )
{
    if( vm->transp == 0 ) return;
    
    //Sort vertexes:
    pix_vm_ivertex* temp;
    if( v1->y > v2->y ) { temp = v1; v1 = v2; v2 = temp; }
    if( v1->y > v3->y ) { temp = v1; v1 = v3; v3 = temp; }
    if( v2->y > v3->y ) { temp = v2; v2 = v3; v3 = temp; }
        
    //Screen bounds control:
    if( v1->y == v3->y ) return;
    if( v3->y < 0 ) return;
    if( v1->y >> PIX_FIXED_MATH_PREC >= vm->screen_ysize ) return;
    if( v1->x < 0 && v2->x < 0 && v3->x < 0 ) return;
    if( v1->x >> PIX_FIXED_MATH_PREC >= vm->screen_xsize && v2->x >> PIX_FIXED_MATH_PREC >= vm->screen_xsize && v3->x >> PIX_FIXED_MATH_PREC >= vm->screen_xsize ) return;
    
    //Draw:
    PIX_INT y, y2;
    bool ret_request = 0;
    pix_vm_ivertex d1, d2; //deltas
    pix_vm_ivertex c1, c2; //current vertexes
    //Calc deltas:
    PIX_INT delta = ( v2->y - v1->y ) >> PIX_FIXED_MATH_PREC;
    if( delta ) d1.x = ( v2->x - v1->x ) / delta; else d1.x = 0;
    delta = ( v3->y - v1->y ) >> PIX_FIXED_MATH_PREC;
    if( delta ) d2.x = ( v3->x - v1->x ) / delta; else d2.x = 0;
    //Set start values:
    c1.x = v1->x;
    c2.x = v1->x;
    y = v1->y >> PIX_FIXED_MATH_PREC;
    y2 = v2->y >> PIX_FIXED_MATH_PREC;
    if( y < 0 )
    {
	PIX_INT ysize;
	if( y2 >= 0 ) 
	    ysize = -y; 
	else 
	    ysize = y2 - y;
	c1.x += d1.x * ysize;
	c2.x += d2.x * ysize;
	y += ysize;
    }
    if( y2 > vm->screen_ysize ) { y2 = vm->screen_ysize; ret_request = 1; }
    for( ; y < y2; y++ )
    {
	pix_vm_gfx_draw_hline( y, c1.x, c2.x, color, vm );
	c1.x += d1.x;
	c2.x += d2.x;
    }
    if( ret_request ) return;
    //Calc deltas:
    delta = ( v3->y - v2->y ) >> PIX_FIXED_MATH_PREC;
    if( delta ) d1.x = ( v3->x - v2->x ) / delta; else d1.x = 0;
    //Set start values:
    c1.x = v2->x;
    y = v2->y >> PIX_FIXED_MATH_PREC;
    y2 = v3->y >> PIX_FIXED_MATH_PREC;
    if( y < 0 )
    {
	PIX_INT ysize;
	if( y2 >= 0 ) 
	    ysize = -y; 
	else 
	    ysize = y2 - y;
	c1.x += d1.x * ysize;
	c2.x += d2.x * ysize;
	y += ysize;
    }
    if( y2 > vm->screen_ysize ) y2 = vm->screen_ysize;
    for( ; y < y2; y++ )
    {
	pix_vm_gfx_draw_hline( y, c1.x, c2.x, color, vm );
	c1.x += d1.x;
	c2.x += d2.x;
    }
}

void pix_vm_gfx_draw_triangle_zbuf( pix_vm_ivertex* v1, pix_vm_ivertex* v2, pix_vm_ivertex* v3, COLOR color, pix_vm* vm )
{
    if( vm->transp == 0 ) return;
    
    //Sort vertexes:
    pix_vm_ivertex* temp;
    if( v1->y > v2->y ) { temp = v1; v1 = v2; v2 = temp; }
    if( v1->y > v3->y ) { temp = v1; v1 = v3; v3 = temp; }
    if( v2->y > v3->y ) { temp = v2; v2 = v3; v3 = temp; }
    
    //Screen bounds control:
    if( v1->y == v3->y ) return;
    if( v3->y < 0 ) return;
    if( v1->y >> PIX_FIXED_MATH_PREC >= vm->screen_ysize ) return;
    if( v1->x < 0 && v2->x < 0 && v3->x < 0 ) return;
    if( v1->x >> PIX_FIXED_MATH_PREC >= vm->screen_xsize && v2->x >> PIX_FIXED_MATH_PREC >= vm->screen_xsize && v3->x >> PIX_FIXED_MATH_PREC >= vm->screen_xsize ) return;
    
    COLORPTR scr = vm->screen_ptr;
    if( scr == 0 ) return;
    int* zbuf = 0;
    if( (unsigned)vm->zbuf >= (unsigned)vm->c_num ) return;
    {
	pix_vm_container* zbuf_cont = vm->c[ vm->zbuf ];
	if( zbuf_cont == 0 ) return;
	if( zbuf_cont->data == 0 ) return;
	if( zbuf_cont->type != PIX_CONTAINER_TYPE_INT32 )
	{
	    PIX_VM_LOG( "ZBuffer must be INT32.\n" );
	    return;
	}
	zbuf = (int*)zbuf_cont->data;
    }
    
    //Draw:
    PIX_INT y, y2;
    bool ret_request = 0;
    pix_vm_ivertex d1, d2; //deltas
    pix_vm_ivertex c1, c2; //current vertexes
    //Calc deltas:
    PIX_INT delta = ( v2->y - v1->y ) >> PIX_FIXED_MATH_PREC;
    if( delta ) 
    {
	d1.x = ( v2->x - v1->x ) / delta; 
	d1.z = ( v2->z - v1->z ) / delta;
    }
    else 
    {
	d1.x = 0;
	d1.z = 0;
    }
    delta = ( v3->y - v1->y ) >> PIX_FIXED_MATH_PREC;
    if( delta ) 
    {
	d2.x = ( v3->x - v1->x ) / delta;
	d2.z = ( v3->z - v1->z ) / delta;
    }
    else 
    {
	d2.x = 0;
	d2.z = 0;
    }
    //Set start values:
    c1.x = v1->x;
    c1.z = v1->z;
    c2.x = v1->x;
    c2.z = v1->z;
    y = v1->y >> PIX_FIXED_MATH_PREC;
    y2 = v2->y >> PIX_FIXED_MATH_PREC;
    if( y < 0 )
    {
	PIX_INT ysize;
	if( y2 >= 0 ) 
	    ysize = -y; 
	else 
	    ysize = y2 - y;
	c1.x += d1.x * ysize;
	c1.z += d1.z * ysize;
	c2.x += d2.x * ysize;
	c2.z += d2.z * ysize;
	y += ysize;
    }
    if( y2 > vm->screen_ysize ) { y2 = vm->screen_ysize; ret_request = 1; }
    for( ; y < y2; y++ )
    {
	pix_vm_gfx_draw_hline_zbuf( 
				   y, 
				   c1.x, 
				   c1.z, 
				   c2.x,
				   c2.z,
				   color, 
				   scr,
				   zbuf,
				   vm );
	c1.x += d1.x;
	c1.z += d1.z;
	c2.x += d2.x;
	c2.z += d2.z;
    }
    if( ret_request ) return;
    //Calc deltas:
    delta = ( v3->y - v2->y ) >> PIX_FIXED_MATH_PREC;
    if( delta ) 
    {
	d1.x = ( v3->x - v2->x ) / delta; 
	d1.z = ( v3->z - v2->z ) / delta;
    }
    else 
    {
	d1.x = 0;
	d1.z = 0;
    }
    //Set start values:
    c1.x = v2->x;
    c1.z = v2->z;
    y = v2->y >> PIX_FIXED_MATH_PREC;
    y2 = v3->y >> PIX_FIXED_MATH_PREC;
    if( y < 0 )
    {
	PIX_INT ysize;
	if( y2 >= 0 ) 
	    ysize = -y; 
	else 
	    ysize = y2 - y;
	c1.x += d1.x * ysize;
	c1.z += d1.z * ysize;
	c2.x += d2.x * ysize;
	c2.z += d2.z * ysize;
	y += ysize;
    }
    if( y2 > vm->screen_ysize ) y2 = vm->screen_ysize;
    for( ; y < y2; y++ )
    {
	pix_vm_gfx_draw_hline_zbuf( 
				   y, 
				   c1.x, 
				   c1.z,
				   c2.x, 
				   c2.z,
				   color, 
				   scr,
				   zbuf,
				   vm );
	c1.x += d1.x;
	c1.z += d1.z;
	c2.x += d2.x;
	c2.z += d2.z;
    }
}

void pix_vm_gfx_draw_triangle_t( PIX_FLOAT* v1f, PIX_FLOAT* v2f, PIX_FLOAT* v3f, PIX_CID cnum, COLOR color, pix_vm* vm )
{
    if( vm->transp == 0 ) return;
    
    //Sort vertexes:
    {
	PIX_FLOAT* temp;
	if( v1f[ 1 ] > v2f[ 1 ] ) { temp = v1f; v1f = v2f; v2f = temp; }
	if( v1f[ 1 ] > v3f[ 1 ] ) { temp = v1f; v1f = v3f; v3f = temp; }
	if( v2f[ 1 ] > v3f[ 1 ] ) { temp = v2f; v2f = v3f; v3f = temp; }
    }

    //Screen bounds control #1:
    if( v1f[ 1 ] == v3f[ 1 ] ) return;
    if( v3f[ 1 ] < 0 ) return;
    if( v1f[ 0 ] < 0 && v2f[ 0 ] < 0 && v3f[ 0 ] < 0 ) return;

    pix_vm_ivertex_t v1;
    pix_vm_ivertex_t v2;
    pix_vm_ivertex_t v3;
    if( v1f[ 0 ] >= PIX_MAX_FIXED_MATH || v1f[ 0 ] <= -PIX_MAX_FIXED_MATH ) return; else
	v1.x = (PIX_INT)( v1f[ 0 ] * ( 1 << PIX_FIXED_MATH_PREC ) );
    if( v1f[ 1 ] >= PIX_MAX_FIXED_MATH || v1f[ 1 ] <= -PIX_MAX_FIXED_MATH ) return; else
	v1.y = (PIX_INT)( v1f[ 1 ] * ( 1 << PIX_FIXED_MATH_PREC ) );
    v1.z = (PIX_INT)( v1f[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC ) );
    v1.tx = (PIX_INT)( v1f[ 3 ] * ( 1 << PIX_TEX_FIXED_MATH_PREC ) );
    v1.ty = (PIX_INT)( v1f[ 4 ] * ( 1 << PIX_TEX_FIXED_MATH_PREC ) );
    if( v2f[ 0 ] >= PIX_MAX_FIXED_MATH || v2f[ 0 ] <= -PIX_MAX_FIXED_MATH ) return; else
	v2.x = (PIX_INT)( v2f[ 0 ] * ( 1 << PIX_FIXED_MATH_PREC ) );
    if( v2f[ 1 ] >= PIX_MAX_FIXED_MATH || v2f[ 1 ] <= -PIX_MAX_FIXED_MATH ) return; else
	v2.y = (PIX_INT)( v2f[ 1 ] * ( 1 << PIX_FIXED_MATH_PREC ) );
    v2.z = (PIX_INT)( v2f[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC ) );
    v2.tx = (PIX_INT)( v2f[ 3 ] * ( 1 << PIX_TEX_FIXED_MATH_PREC ) );
    v2.ty = (PIX_INT)( v2f[ 4 ] * ( 1 << PIX_TEX_FIXED_MATH_PREC ) );
    if( v3f[ 0 ] >= PIX_MAX_FIXED_MATH || v3f[ 0 ] <= -PIX_MAX_FIXED_MATH ) return; else
	v3.x = (PIX_INT)( v3f[ 0 ] * ( 1 << PIX_FIXED_MATH_PREC ) );
    if( v3f[ 1 ] >= PIX_MAX_FIXED_MATH || v3f[ 1 ] <= -PIX_MAX_FIXED_MATH ) return; else
	v3.y = (PIX_INT)( v3f[ 1 ] * ( 1 << PIX_FIXED_MATH_PREC ) );
    v3.z = (PIX_INT)( v3f[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC ) );
    v3.tx = (PIX_INT)( v3f[ 3 ] * ( 1 << PIX_TEX_FIXED_MATH_PREC ) );
    v3.ty = (PIX_INT)( v3f[ 4 ] * ( 1 << PIX_TEX_FIXED_MATH_PREC ) );
        
    //Screen bounds control #2:
    if( v1.y >> PIX_FIXED_MATH_PREC >= vm->screen_ysize ) return;
    if( v1.x >> PIX_FIXED_MATH_PREC >= vm->screen_xsize && v2.x >> PIX_FIXED_MATH_PREC >= vm->screen_xsize && v3.x >> PIX_FIXED_MATH_PREC >= vm->screen_xsize ) return;
    
    //Get texture:
    if( (unsigned)cnum >= (unsigned)vm->c_num ) return;
    pix_vm_container* c = vm->c[ cnum ];
    if( c == 0 ) return;
    if( g_pix_container_type_sizes[ c->type ] != COLORLEN ) return;
    COLORPTR txt = (COLORPTR)c->data;
    int txt_xsize = c->xsize;
    int txt_ysize = c->ysize;
    
    //Get alpha-channel:
    uchar* txt_alpha = (uchar*)pix_vm_get_container_alpha_data( cnum, vm );
    
    //Get key color:
    bool uses_key = 0;
    COLOR key;
    if( c->flags & PIX_CONTAINER_FLAG_USES_KEY )
    {
	uses_key = 1;
	key = c->key;
    }
    
    //Get screen and z-buffer:
    COLORPTR scr = vm->screen_ptr;
    if( scr == 0 ) return;
    int* zbuf = pix_vm_gfx_get_zbuf( vm );
    
    //Draw:
    PIX_INT y, y2;
    bool ret_request = 0;
    pix_vm_ivertex_t d1, d2; //deltas
    pix_vm_ivertex_t c1, c2; //current vertexes
    //Calc deltas:
    PIX_INT delta = ( v2.y - v1.y ) >> PIX_FIXED_MATH_PREC;
    if( delta ) 
    {
	d1.x = ( v2.x - v1.x ) / delta;
	d1.z = ( v2.z - v1.z ) / delta;
	d1.tx = ( v2.tx - v1.tx ) / delta;
	d1.ty = ( v2.ty - v1.ty ) / delta;
    }
    else 
    {
	d1.x = 0;
	d1.z = 0;
	d1.tx = 0;
	d1.ty = 0;
    }
    delta = ( v3.y - v1.y ) >> PIX_FIXED_MATH_PREC;
    if( delta ) 
    {
	d2.x = ( v3.x - v1.x ) / delta;
	d2.z = ( v3.z - v1.z ) / delta;
	d2.tx = ( v3.tx - v1.tx ) / delta;
	d2.ty = ( v3.ty - v1.ty ) / delta;
    }
    else 
    {
	d2.x = 0;
	d2.z = 0;
	d2.tx = 0;
	d2.ty = 0;
    }
    //Set start values:
    c1.x = v1.x;
    c1.z = v1.z;
    c1.tx = v1.tx;
    c1.ty = v1.ty;
    c2.x = v1.x;
    c2.z = v1.z;
    c2.tx = v1.tx;
    c2.ty = v1.ty;
    y = v1.y >> PIX_FIXED_MATH_PREC;
    y2 = v2.y >> PIX_FIXED_MATH_PREC;
    if( y < 0 )
    {
	PIX_INT ysize;
	if( y2 >= 0 ) 
	    ysize = -y; 
	else 
	    ysize = y2 - y;
	c1.x += d1.x * ysize;
	c1.z += d1.z * ysize;
	c1.tx += d1.tx * ysize;
	c1.ty += d1.ty * ysize;
	c2.x += d2.x * ysize;
	c2.z += d2.z * ysize;
	c2.tx += d2.tx * ysize;
	c2.ty += d2.ty * ysize;
	y += ysize;
    }
    if( y2 > vm->screen_ysize ) { y2 = vm->screen_ysize; ret_request = 1; }
    if( c->flags & PIX_CONTAINER_FLAG_INTERP )
	for( ; y < y2; y++ )
	{
	    pix_vm_gfx_draw_hline_t_interp( 
		y,
		c1.x,
		c1.z,
		c1.tx,
		c1.ty,
		c2.x,
		c2.z,
		c2.tx,
		c2.ty,
		color,
		txt,
		txt_xsize,
		txt_ysize,
		txt_alpha,
		uses_key,
		key,
		scr,
		zbuf,
		vm );
	    c1.x += d1.x;
	    c1.z += d1.z;
	    c1.tx += d1.tx;
	    c1.ty += d1.ty;
	    c2.x += d2.x;
	    c2.z += d2.z;
	    c2.tx += d2.tx;
	    c2.ty += d2.ty;
	}
    else
	for( ; y < y2; y++ )
	{
	    pix_vm_gfx_draw_hline_t( 
		y,
		c1.x,
		c1.z,
		c1.tx,
		c1.ty,
		c2.x,
		c2.z,
		c2.tx,
		c2.ty,
		color,
		txt,
		txt_xsize,
		txt_ysize,
		txt_alpha,
		uses_key,
		key,
		scr,
		zbuf,
		vm );
	    c1.x += d1.x;
	    c1.z += d1.z;
	    c1.tx += d1.tx;
	    c1.ty += d1.ty;
	    c2.x += d2.x;
	    c2.z += d2.z;
	    c2.tx += d2.tx;
	    c2.ty += d2.ty;
	}
    if( ret_request ) return;
    //Calc deltas:
    delta = ( v3.y - v2.y ) >> PIX_FIXED_MATH_PREC;
    if( delta ) 
    {
	d1.x = ( v3.x - v2.x ) / delta; 
	d1.z = ( v3.z - v2.z ) / delta;
	d1.tx = ( v3.tx - v2.tx ) / delta;
	d1.ty = ( v3.ty - v2.ty ) / delta;
    }
    else 
    {
	d1.x = 0;
	d1.z = 0;
	d1.tx = 0;
	d1.ty = 0;
    }
    //Set start values:
    c1.x = v2.x;
    c1.z = v2.z;
    c1.tx = v2.tx;
    c1.ty = v2.ty;
    y = v2.y >> PIX_FIXED_MATH_PREC;
    y2 = v3.y >> PIX_FIXED_MATH_PREC;
    if( y < 0 )
    {
	PIX_INT ysize;
	if( y2 >= 0 ) 
	    ysize = -y; 
	else 
	    ysize = y2 - y;
	c1.x += d1.x * ysize;
	c1.z += d1.z * ysize;
	c1.tx += d1.tx * ysize;
	c1.ty += d1.ty * ysize;
	c2.x += d2.x * ysize;
	c2.z += d2.z * ysize;
	c2.tx += d2.tx * ysize;
	c2.ty += d2.ty * ysize;
	y += ysize;
    }
    if( y2 > vm->screen_ysize ) y2 = vm->screen_ysize;
    if( c->flags & PIX_CONTAINER_FLAG_INTERP )
	for( ; y < y2; y++ )
	{
	    pix_vm_gfx_draw_hline_t_interp( 
		y,
		c1.x,
		c1.z,
		c1.tx,
		c1.ty,
		c2.x,
		c2.z,
		c2.tx,
		c2.ty,
		color,
		txt,
		txt_xsize,
		txt_ysize,
		txt_alpha,
		uses_key,
		key,
		scr,
		zbuf,
		vm );
	    c1.x += d1.x;
	    c1.z += d1.z;
	    c1.tx += d1.tx;
	    c1.ty += d1.ty;
	    c2.x += d2.x;
	    c2.z += d2.z;
	    c2.tx += d2.tx;
	    c2.ty += d2.ty;
	}
    else
	for( ; y < y2; y++ )
	{
	    pix_vm_gfx_draw_hline_t( 
		y,
		c1.x,
		c1.z,
		c1.tx,
		c1.ty,
		c2.x,
		c2.z,
		c2.tx,
		c2.ty,
		color,
		txt,
		txt_xsize,
		txt_ysize,
		txt_alpha,
		uses_key,
		key,
		scr,
		zbuf,
		vm );
	    c1.x += d1.x;
	    c1.z += d1.z;
	    c1.tx += d1.tx;
	    c1.ty += d1.ty;
	    c2.x += d2.x;
	    c2.z += d2.z;
	    c2.tx += d2.tx;
	    c2.ty += d2.ty;
	}
}
