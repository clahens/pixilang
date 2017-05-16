/*
    pixilang_vm_gfx.cpp
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

void pix_vm_gfx_set_screen( PIX_CID cnum, pix_vm* vm )
{
    vm->screen = cnum;
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( c && c->data )
	{
	    vm->screen_xsize = c->xsize;
	    vm->screen_ysize = c->ysize;
	    vm->screen_ptr = (COLORPTR)c->data;
	    return;
	}
    }
    if( vm->screen == PIX_GL_SCREEN )
    {
#ifdef OPENGL
	vm->screen_ptr = &vm->gl_temp_screen;
	vm->screen_xsize = 1;
	vm->screen_ysize = 1;
	pix_vm_gl_matrix_set( vm );
#endif
    }
    else
    {
	vm->screen = -1;
	vm->screen_ptr = 0;
    }
}

void pix_vm_gfx_matrix_reset( pix_vm* vm )
{
    vm->t_enabled = 0;
    PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
    bmem_set( m, sizeof( PIX_FLOAT ) * 4 * 4, 0 );
    m[ 0 ] = 1;
    m[ 4 + 1 ] = 1;
    m[ 8 + 2 ] = 1;
    m[ 12 + 3 ] = 1;
}

void pix_vm_gfx_matrix_mul( PIX_FLOAT* res, PIX_FLOAT* m1, PIX_FLOAT* m2 ) 
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

void pix_vm_gfx_vertex_transform( PIX_FLOAT* v, PIX_FLOAT* m )
{
    PIX_FLOAT rx, ry, rz, rw;
    rx = 0; 
    ry = 0; 
    rz = 0, 
    rw = 0;
    
    if( m[ 0 ] ) rx += v[ 0 ] * m[ 0 ];
    if( m[ 4 ] ) rx += v[ 1 ] * m[ 4 ];
    if( m[ 8 ] ) rx += v[ 2 ] * m[ 8 ];
    rx += m[ 12 ];
    if( m[ 0 + 1 ] ) ry += v[ 0 ] * m[ 0 + 1 ];
    if( m[ 4 + 1 ] ) ry += v[ 1 ] * m[ 4 + 1 ];
    if( m[ 8 + 1 ] ) ry += v[ 2 ] * m[ 8 + 1 ];
    ry += m[ 12 + 1 ];
    if( m[ 0 + 2 ] ) rz += v[ 0 ] * m[ 0 + 2 ];
    if( m[ 4 + 2 ] ) rz += v[ 1 ] * m[ 4 + 2 ];
    if( m[ 8 + 2 ] ) rz += v[ 2 ] * m[ 8 + 2 ];
    rz += m[ 12 + 2 ];
    if( m[ 0 + 3 ] ) rw += v[ 0 ] * m[ 0 + 3 ];
    if( m[ 4 + 3 ] ) rw += v[ 1 ] * m[ 4 + 3 ];
    if( m[ 8 + 3 ] ) rw += v[ 2 ] * m[ 8 + 3 ];
    rw += m[ 12 + 3 ];
    
    v[ 0 ] = rx / rw;
    v[ 1 ] = ry / rw;
    v[ 2 ] = rz / rw;
}

#define bottom 1
#define top 2
#define left 4
#define right 8
static int pix_vm_gfx_make_line_code( PIX_INT x, PIX_INT y, int clip_x, int clip_y )
{
    int code = 0;
    if( y >= clip_y ) code = bottom;
    else if( y < 0 ) code = top;
    if( x >= clip_x ) code += right;
    else if( x < 0 ) code += left;
    return code;
}

void pix_vm_gfx_draw_line( PIX_INT x1, PIX_INT y1, PIX_INT x2, PIX_INT y2, COLOR color, pix_vm* vm )
{
    uchar transp = vm->transp;
    if( transp == 0 ) return;

    if( x1 == x2 )
    {
	if( (unsigned)x1 < (unsigned)vm->screen_xsize )
	{
	    if( y1 > y2 ) { PIX_INT temp = y1; y1 = y2; y2 = temp; }
	    if( y1 >= vm->screen_ysize ) return;
	    if( y2 < 0 ) return;
	    if( y1 < 0 ) y1 = 0;
	    if( y2 >= vm->screen_ysize ) y2 = vm->screen_ysize - 1;
	    COLORPTR pp = vm->screen_ptr + y1 * vm->screen_xsize + x1;
	    COLORPTR pp_end = vm->screen_ptr + ( y2 + 1 ) * vm->screen_xsize + x1;
	    if( transp == 255 )
	    {
		while( pp < pp_end )
		{
		    *pp = color;
		    pp += vm->screen_xsize;
		}
	    }
	    else
	    {
		while( pp < pp_end )
		{
		    *pp = fast_blend( *pp, color, transp );
		    pp += vm->screen_xsize;
		}
	    }
	}
	return;
    }
    if( y1 == y2 )
    {
	if( (unsigned)y1 < (unsigned)vm->screen_ysize )
	{
	    if( x1 > x2 ) { PIX_INT temp = x1; x1 = x2; x2 = temp; }
	    if( x1 >= vm->screen_xsize ) return;
	    if( x2 < 0 ) return;
	    if( x1 < 0 ) x1 = 0;
	    if( x2 >= vm->screen_xsize ) x2 = vm->screen_xsize - 1;
	    COLORPTR pp = vm->screen_ptr + y1 * vm->screen_xsize + x1;
	    COLORPTR pp_end = pp + ( x2 - x1 + 1 );
	    if( transp == 255 )
	    {
		while( pp < pp_end )
		{
		    *pp++ = color;
		}
	    }
	    else
	    {
		while( pp < pp_end )
		{
		    *pp = fast_blend( *pp, color, transp );
		    pp++;
		}
	    }
	}
	return;
    }
    
    //Cohen-Sutherland line clipping algorithm:
    int code0;
    int code1;
    int out_code;
    PIX_INT x, y;
    code0 = pix_vm_gfx_make_line_code( x1, y1, vm->screen_xsize, vm->screen_ysize );
    code1 = pix_vm_gfx_make_line_code( x2, y2, vm->screen_xsize, vm->screen_ysize );
    while( code0 || code1 )
    {
	if( code0 & code1 ) 
	{
	    //Trivial reject
	    return; 
	}
	else
	{
	    //Failed both tests, so calculate the line segment to clip
	    if( code0 )
		out_code = code0; //Clip the first point
	    else
		out_code = code1; //Clip the last point
	    
	    if( out_code & bottom )
	    {
		//Clip the line to the bottom of the viewport
		y = vm->screen_ysize - 1;
		x = x1 + ( x2 - x1 ) * ( y - y1 ) / ( y2 - y1 );
	    }
	    else 
		if( out_code & top )
		{
		    y = 0;
		    x = x1 + ( x2 - x1 ) * ( y - y1 ) / ( y2 - y1 );
		}
		else
		    if( out_code & right )
		    {
			x = vm->screen_xsize - 1;
			y = y1 + ( y2 - y1 ) * ( x - x1 ) / ( x2 - x1 );
		    }
		    else
			if( out_code & left )
			{
			    x = 0;
			    y = y1 + ( y2 - y1 ) * ( x - x1 ) / ( x2 - x1 );
			}
	    
	    if( out_code == code0 )
	    { //Modify the first coordinate 
		x1 = x; y1 = y;
		code0 = pix_vm_gfx_make_line_code( x1, y1, vm->screen_xsize, vm->screen_ysize );
	    }
	    else
	    { //Modify the second coordinate
		x2 = x; y2 = y;
		code1 = pix_vm_gfx_make_line_code( x2, y2, vm->screen_xsize, vm->screen_ysize );
	    }
	}
    }
    
    //Draw line:
    int len_x = x2 - x1; if( len_x < 0 ) len_x = -len_x;
    int len_y = y2 - y1; if( len_y < 0 ) len_y = -len_y;
    int ptr = y1 * vm->screen_xsize + x1;
    int delta;
    int v = 0, old_v = 0;
    if( len_x > len_y )
    {
	//Horisontal:
	if( len_x != 0 )
	    delta = ( len_y << 10 ) / len_x;
	else
	    delta = 0;
	if( transp == 255 )
	{
	    for( int a = 0; a <= len_x; a++ )
	    {
		vm->screen_ptr[ ptr ] = color;
		old_v = v;
		v += delta;
		if( x2 - x1 > 0 ) ptr++; else ptr--;
		if( ( old_v >> 10 ) != ( v >> 10 ) ) 
		{
		    if( y2 - y1 > 0 )
			ptr += vm->screen_xsize;
		    else
			ptr -= vm->screen_xsize;
		}
	    }
	}
	else
	{
	    for( int a = 0; a <= len_x; a++ )
	    {
		vm->screen_ptr[ ptr ] = fast_blend( vm->screen_ptr[ ptr ], color, transp );
		old_v = v;
		v += delta;
		if( x2 - x1 > 0 ) ptr++; else ptr--;
		if( ( old_v >> 10 ) != ( v >> 10 ) ) 
		{
		    if( y2 - y1 > 0 )
			ptr += vm->screen_xsize;
		    else
			ptr -= vm->screen_xsize;
		}
	    }
	}
    }
    else
    {
	//Vertical:
	if( len_y != 0 ) 
	    delta = ( len_x << 10 ) / len_y;
	else
	    delta = 0;
	if( transp == 255 )
	{
	    for( int a = 0; a <= len_y; a++ )
	    {
		vm->screen_ptr[ ptr ] = color;
		old_v = v;
		v += delta;
		if( y2 - y1 > 0 ) 
		    ptr += vm->screen_xsize;
		else
		    ptr -= vm->screen_xsize;
		if( ( old_v >> 10 ) != ( v >> 10 ) ) 
		{
		    if( x2 - x1 > 0 ) ptr++; else ptr--;
		}
	    }
	}
	else
	{
	    for( int a = 0; a <= len_y; a++ )
	    {
		vm->screen_ptr[ ptr ] = fast_blend( vm->screen_ptr[ ptr ], color, transp );
		old_v = v;
		v += delta;
		if( y2 - y1 > 0 ) 
		    ptr += vm->screen_xsize;
		else
		    ptr -= vm->screen_xsize;
		if( ( old_v >> 10 ) != ( v >> 10 ) ) 
		{
		    if( x2 - x1 > 0 ) ptr++; else ptr--;
		}
	    }
	}
    }
}

//z1, z2 - fixed point PIX_FIXED_MATH_PREC
void pix_vm_gfx_draw_line_zbuf( PIX_INT x1, PIX_INT y1, PIX_INT z1, PIX_INT x2, PIX_INT y2, PIX_INT z2, COLOR color, int* zbuf, pix_vm* vm )
{
    uchar transp = vm->transp;
    if( transp == 0 ) return;
        
    //Cohen-Sutherland line clipping algorithm:
    int code0;
    int code1;
    int out_code;
    PIX_INT x, y, z;
    code0 = pix_vm_gfx_make_line_code( x1, y1, vm->screen_xsize, vm->screen_ysize );
    code1 = pix_vm_gfx_make_line_code( x2, y2, vm->screen_xsize, vm->screen_ysize );
    while( code0 || code1 )
    {
	if( code0 & code1 ) return; //Trivial reject
	else
	{
	    //Failed both tests, so calculate the line segment to clip
	    if( code0 )
		out_code = code0; //Clip the first point
	    else
		out_code = code1; //Clip the last point
	    
	    if( out_code & bottom )
	    {
		//Clip the line to the bottom of the viewport
		y = vm->screen_ysize - 1;
		PIX_INT ylen = y2 - y1;
		PIX_INT ylen2 = y - y1;
		x = x1 + ( x2 - x1 ) * ylen2 / ylen;
		z = z1 + ( z2 - z1 ) * ylen2 / ylen;
	    }
	    else 
	    {
		if( out_code & top )
		{
		    y = 0;
		    PIX_INT ylen = y2 - y1;
		    PIX_INT ylen2 = y - y1;
		    x = x1 + ( x2 - x1 ) * ylen2 / ylen;
		    z = z1 + ( z2 - z1 ) * ylen2 / ylen;
		}
		else
		{
		    if( out_code & right )
		    {
			x = vm->screen_xsize - 1;
			PIX_INT xlen = x2 - x1;
			PIX_INT xlen2 = x - x1;
			y = y1 + ( y2 - y1 ) * xlen2 / xlen;
			z = z1 + ( z2 - z1 ) * xlen2 / xlen;
		    }
		    else
		    {
			if( out_code & left )
			{
			    x = 0;
			    PIX_INT xlen = x2 - x1;
			    PIX_INT xlen2 = x - x1;
			    y = y1 + ( y2 - y1 ) * xlen2 / xlen;
			    z = z1 + ( z2 - z1 ) * xlen2 / xlen;
			}
		    }
		}
	    }
	    
	    if( out_code == code0 )
	    { //Modify the first coordinate 
		x1 = x; y1 = y; z1 = z;
		code0 = pix_vm_gfx_make_line_code( x1, y1, vm->screen_xsize, vm->screen_ysize );
	    }
	    else
	    { //Modify the second coordinate
		x2 = x; y2 = y; z2 = z;
		code1 = pix_vm_gfx_make_line_code( x2, y2, vm->screen_xsize, vm->screen_ysize );
	    }
	}
    }
    
    //Draw line:
    int len_x = x2 - x1; if( len_x < 0 ) len_x = -len_x;
    int len_y = y2 - y1; if( len_y < 0 ) len_y = -len_y;
    int len_z = z2 - z1;
    int ptr = y1 * vm->screen_xsize + x1;
    int delta;
    int delta_z;
    int v = 0, old_v = 0;
    int cur_z = z1;
    if( len_x > len_y )
    {
	//Horisontal:
	if( len_x != 0 )
	{
	    delta = ( len_y << 10 ) / len_x;
	    delta_z = len_z / len_x;
	}
	else
	{
	    delta = 0;
	    delta_z = 0;
	}
	if( transp == 255 )
	{
	    for( int a = 0; a <= len_x; a++ )
	    {
		if( cur_z > zbuf[ ptr ] )
		{
		    vm->screen_ptr[ ptr ] = color;
		    zbuf[ ptr ] = cur_z;
		}
		old_v = v;
		v += delta;
		if( x2 - x1 > 0 ) ptr++; else ptr--;
		if( ( old_v >> 10 ) != ( v >> 10 ) ) 
		{
		    if( y2 - y1 > 0 )
			ptr += vm->screen_xsize;
		    else
			ptr -= vm->screen_xsize;
		}
		cur_z += delta_z;
	    }
	}
	else
	{
	    for( int a = 0; a <= len_x; a++ )
	    {
		if( cur_z > zbuf[ ptr ] )
		{
		    vm->screen_ptr[ ptr ] = fast_blend( vm->screen_ptr[ ptr ], color, transp );
		    zbuf[ ptr ] = cur_z;
		}
		old_v = v;
		v += delta;
		if( x2 - x1 > 0 ) ptr++; else ptr--;
		if( ( old_v >> 10 ) != ( v >> 10 ) ) 
		{
		    if( y2 - y1 > 0 )
			ptr += vm->screen_xsize;
		    else
			ptr -= vm->screen_xsize;
		}
		cur_z += delta_z;
	    }
	}
    }
    else
    {
	//Vertical:
	if( len_y != 0 ) 
	{
	    delta = ( len_x << 10 ) / len_y;
	    delta_z = len_z / len_y;
	}
	else
	{
	    delta = 0;
	    delta_z = 0;
	}
	if( transp == 255 )
	{
	    for( int a = 0; a <= len_y; a++ )
	    {
		if( cur_z > zbuf[ ptr ] )
		{
		    vm->screen_ptr[ ptr ] = color;
		    zbuf[ ptr ] = cur_z;
		}
		old_v = v;
		v += delta;
		if( y2 - y1 > 0 ) 
		    ptr += vm->screen_xsize;
		else
		    ptr -= vm->screen_xsize;
		if( ( old_v >> 10 ) != ( v >> 10 ) ) 
		{
		    if( x2 - x1 > 0 ) ptr++; else ptr--;
		}
		cur_z += delta_z;
	    }
	}
	else
	{
	    for( int a = 0; a <= len_y; a++ )
	    {
		if( cur_z > zbuf[ ptr ] )
		{
		    vm->screen_ptr[ ptr ] = fast_blend( vm->screen_ptr[ ptr ], color, transp );
		    zbuf[ ptr ] = cur_z;
		}
		old_v = v;
		v += delta;
		if( y2 - y1 > 0 ) 
		    ptr += vm->screen_xsize;
		else
		    ptr -= vm->screen_xsize;
		if( ( old_v >> 10 ) != ( v >> 10 ) ) 
		{
		    if( x2 - x1 > 0 ) ptr++; else ptr--;
		}
		cur_z += delta_z;
	    }
	}
    }
}

void pix_vm_gfx_draw_box( PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, COLOR color, pix_vm* vm )
{
    pix_vm_gfx_draw_line( x, y, x + xsize, y, color, vm );
    pix_vm_gfx_draw_line( x + xsize, y, x + xsize, y + ysize, color, vm );
    pix_vm_gfx_draw_line( x + xsize, y + ysize, x, y + ysize, color, vm );
    pix_vm_gfx_draw_line( x, y + ysize, x, y, color, vm );
}

void pix_vm_gfx_draw_fbox( PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, COLOR color, pix_vm* vm )
{
    uchar transp = vm->transp;
    if( transp == 0 ) return;

    if( x < 0 ) { xsize += x; x = 0; }
    if( y < 0 ) { ysize += y; y = 0; }
    if( x + xsize <= 0 ) return;
    if( y + ysize <= 0 ) return;
    if( x + xsize > vm->screen_xsize ) xsize -= x + xsize - vm->screen_xsize;
    if( y + ysize > vm->screen_ysize ) ysize -= y + ysize - vm->screen_ysize;
    if( x >= vm->screen_xsize ) return;
    if( y >= vm->screen_ysize ) return;
    if( xsize < 0 ) return;
    if( ysize < 0 ) return;
    
    COLORPTR ptr = vm->screen_ptr + y * vm->screen_xsize + x;
    int add = vm->screen_xsize - xsize;
    if( transp == 255 )
    {
	for( int cy = 0; cy < ysize; cy++ )
	{
	    COLORPTR size = ptr + xsize;
	    while( ptr < size ) { *ptr = color; ptr++; }
	    ptr += add;
	}
    }
    else
    {
	for( int cy = 0; cy < ysize; cy++ )
	{
	    COLORPTR size = ptr + xsize;
	    while( ptr < size ) { *ptr = fast_blend( *ptr, color, transp ); ptr++; }
	    ptr += add;
	}
    }
}

#define GET_TEXTURE_PIXEL \
    int ttx = ctx >> PIX_TEX_FIXED_MATH_PREC; \
    int tty = ty >> PIX_TEX_FIXED_MATH_PREC; \
    if( ttx < 0 ) ttx = 0; \
    if( ttx >= txt_xsize ) ttx = txt_xsize - 1; \
    if( tty < 0 ) tty = 0; \
    if( tty >= txt_ysize ) tty = txt_ysize - 1; \
    COLOR pixel = txt[ txt_xsize * tty + ttx ];

#define GET_TEXTURE_PIXEL_INTERP \
    COLOR p1, p2, p3, p4; \
    int ttx2, tty2; \
    int ttx = ctx >> PIX_TEX_FIXED_MATH_PREC; \
    int tty = ty >> PIX_TEX_FIXED_MATH_PREC; \
    ttx2 = ttx; tty2 = tty; \
    if( ttx2 < 0 ) ttx2 = 0; if( ttx2 >= txt_xsize ) ttx2 = txt_xsize - 1; if( tty2 < 0 ) tty2 = 0; if( tty2 >= txt_ysize ) tty2 = txt_ysize - 1; \
    p1 = txt[ txt_xsize * tty2 + ttx2 ]; \
    tty2 = tty + 1; \
    if( tty2 < 0 ) tty2 = 0; if( tty2 >= txt_ysize ) tty2 = txt_ysize - 1; \
    p3 = txt[ txt_xsize * tty2 + ttx2 ]; \
    ttx2 = ttx + 1; \
    if( ttx2 < 0 ) ttx2 = 0; if( ttx2 >= txt_xsize ) ttx2 = txt_xsize - 1; \
    p4 = txt[ txt_xsize * tty2 + ttx2 ]; \
    tty2 = tty; \
    if( tty2 < 0 ) tty2 = 0; if( tty2 >= txt_ysize ) tty2 = txt_ysize - 1; \
    p2 = txt[ txt_xsize * tty2 + ttx2 ]; \
    int xc = ( ctx >> ( PIX_TEX_FIXED_MATH_PREC - 8 ) ) & 255; \
    int yc = ( ty >> ( PIX_TEX_FIXED_MATH_PREC - 8 ) ) & 255; \
    COLOR pixel = blend( p1, p2, xc ); \
    COLOR pixel2 = blend( p3, p4, xc ); \
    pixel = blend( pixel, pixel2, yc );
    
#define GET_TEXTURE_PIXEL_WITH_ALPHA \
    int ttx = ctx >> PIX_TEX_FIXED_MATH_PREC; \
    int tty = ty >> PIX_TEX_FIXED_MATH_PREC; \
    if( ttx < 0 ) ttx = 0; \
    if( ttx >= txt_xsize ) ttx = txt_xsize - 1; \
    if( tty < 0 ) tty = 0; \
    if( tty >= txt_ysize ) tty = txt_ysize - 1; \
    ttx = txt_xsize * tty + ttx; \
    uchar pixel_alpha = txt_alpha[ ttx ]; \
    COLOR pixel; \
    if( pixel_alpha ) pixel = txt[ ttx ];

#define GET_TEXTURE_PIXEL_WITH_ALPHA_INTERP \
    COLOR p1, p2, p3, p4; \
    COLOR pixel, pixel2; \
    uchar a1, a2, a3, a4; \
    int ttx2, tty2, toff; \
    int ttx = ctx >> PIX_TEX_FIXED_MATH_PREC; \
    int tty = ty >> PIX_TEX_FIXED_MATH_PREC; \
    ttx2 = ttx; tty2 = tty; \
    if( ttx2 < 0 ) ttx2 = 0; if( ttx2 >= txt_xsize ) ttx2 = txt_xsize - 1; if( tty2 < 0 ) tty2 = 0; if( tty2 >= txt_ysize ) tty2 = txt_ysize - 1; \
    toff = txt_xsize * tty2 + ttx2; p1 = txt[ toff ]; a1 = txt_alpha[ toff ]; \
    tty2 = tty + 1; \
    if( tty2 < 0 ) tty2 = 0; if( tty2 >= txt_ysize ) tty2 = txt_ysize - 1; \
    toff = txt_xsize * tty2 + ttx2; p3 = txt[ toff ]; a3 = txt_alpha[ toff ]; \
    ttx2 = ttx + 1; \
    if( ttx2 < 0 ) ttx2 = 0; if( ttx2 >= txt_xsize ) ttx2 = txt_xsize - 1; \
    toff = txt_xsize * tty2 + ttx2; p4 = txt[ toff ]; a4 = txt_alpha[ toff ]; \
    tty2 = tty; \
    if( tty2 < 0 ) tty2 = 0; if( tty2 >= txt_ysize ) tty2 = txt_ysize - 1; \
    toff = txt_xsize * tty2 + ttx2; p2 = txt[ toff ]; a2 = txt_alpha[ toff ]; \
    int xc = ( ctx >> ( PIX_TEX_FIXED_MATH_PREC - 8 ) ) & 255; \
    int yc = ( ty >> ( PIX_TEX_FIXED_MATH_PREC - 8 ) ) & 255; \
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

void pix_vm_gfx_draw_container(
    PIX_CID cnum,
    PIX_FLOAT x,
    PIX_FLOAT y,
    PIX_FLOAT z,
    PIX_FLOAT xsize, 
    PIX_FLOAT ysize, 
    PIX_INT tx, 
    PIX_INT ty, 
    PIX_INT txsize, 
    PIX_INT tysize, 
    COLOR color, 
    pix_vm* vm )
{
    uchar transp = vm->transp;
    if( transp == 0 ) return;
    
#ifdef OPENGL
    if( vm->screen == PIX_GL_SCREEN )
    {
	pix_vm_container_gl_data* gl = pix_vm_create_container_gl_data( cnum, vm );
	if( gl == 0 ) return;
	
	float v[ 3 * 4 ];
	float t[ 2 * 4 ];
	v[ 0 ] = x; v[ 1 ] = y; v[ 2 ] = z;
	v[ 3 ] = x + xsize; v[ 4 ] = y; v[ 5 ] = z;
	v[ 6 ] = x; v[ 7 ] = y + ysize; v[ 8 ] = z;
	v[ 9 ] = x + xsize; v[ 10 ] = y + ysize; v[ 11 ] = z;
	t[ 0 ] = (float)tx / (float)gl->xsize; t[ 1 ] = (float)ty / (float)gl->ysize;
	t[ 2 ] = (float)( tx + txsize ) / (float)gl->xsize; t[ 3 ] = (float)ty / (float)gl->ysize;
	t[ 4 ] = (float)tx / (float)gl->xsize; t[ 5 ] = (float)( ty + tysize ) / (float)gl->ysize;
	t[ 6 ] = (float)( tx + txsize ) / (float)gl->xsize; t[ 7 ] = (float)( ty + tysize ) / (float)gl->ysize;

        gl_program_struct* p;
        if( gl->texture_format == GL_ALPHA )
    	    p = vm->gl_prog_tex_alpha_solid;
	else
    	    p = vm->gl_prog_tex_rgb_solid;
    	if( vm->gl_user_defined_prog ) p = vm->gl_user_defined_prog;
        if( vm->gl_current_prog != p )
        {
            pix_vm_gl_use_prog( p, vm );
            gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) | ( 1 << GL_PROG_ATT_TEX_COORD ) );
            glActiveTexture( GL_TEXTURE0 );
	    glUniform1i( p->uniforms[ GL_PROG_UNI_TEXTURE ], 0 );
        }
        glBindTexture( GL_TEXTURE_2D, gl->texture_id );
        glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( color ) / 255, (float)green( color ) / 255, (float)blue( color ) / 255, (float)transp / 255 );
        glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, v );
        glVertexAttribPointer( p->attributes[ GL_PROG_ATT_TEX_COORD ], 2, GL_FLOAT, false, 0, t );
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	
	return;
    }
#endif
    
    int screen_hxsize = vm->screen_xsize / 2;
    int screen_hysize = vm->screen_ysize / 2;
    
    PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
    
    PIX_FLOAT f1[ 5 ];
    PIX_FLOAT f2[ 5 ];
    PIX_FLOAT f3[ 5 ];
    PIX_FLOAT f4[ 5 ];
    
    f1[ 0 ] = x;
    f1[ 1 ] = y;
    f1[ 2 ] = 0;
    if( vm->t_enabled ) pix_vm_gfx_vertex_transform( f1, m );
    f1[ 0 ] += screen_hxsize;
    f1[ 1 ] += screen_hysize;
    f1[ 3 ] = tx;
    f1[ 4 ] = ty;

    f2[ 0 ] = x + xsize;
    f2[ 1 ] = y;
    f2[ 2 ] = 0;
    if( vm->t_enabled ) pix_vm_gfx_vertex_transform( f2, m );
    f2[ 0 ] += screen_hxsize;
    f2[ 1 ] += screen_hysize;
    f2[ 3 ] = tx + txsize;
    f2[ 4 ] = ty;

    f3[ 0 ] = x + xsize;
    f3[ 1 ] = y + ysize;
    f3[ 2 ] = 0;
    if( vm->t_enabled ) pix_vm_gfx_vertex_transform( f3, m );
    f3[ 0 ] += screen_hxsize;
    f3[ 1 ] += screen_hysize;
    f3[ 3 ] = tx + txsize;
    f3[ 4 ] = ty + tysize;
    
    f4[ 0 ] = x;
    f4[ 1 ] = y + ysize;
    f4[ 2 ] = 0;
    if( vm->t_enabled ) pix_vm_gfx_vertex_transform( f4, m );
    f4[ 0 ] += screen_hxsize;
    f4[ 1 ] += screen_hysize;
    f4[ 3 ] = tx;
    f4[ 4 ] = ty + tysize;
    
    bool rect;
    while( 1 )
    {
	if( vm->t_enabled == 0 )
	{
	    rect = 1;
	    break;
	}
	else
	{
	    if( f1[ 2 ] != f2[ 2 ] || f1[ 2 ] != f3[ 2 ] || f1[ 2 ] != f4[ 2 ] )
	    {
		rect = 0;
		break;
	    }
	    // 12 43 21 34
	    // 43 12 34 21
	    if( f1[ 1 ] == f2[ 1 ] )
	    {
		if( f4[ 1 ] == f3[ 1 ] && f1[ 0 ] == f4[ 0 ] && f2[ 0 ] == f3[ 0 ] )
		{
		    rect = 1;
		    break;
		}
	    }
	}
	rect = 0;
	break;
    }

    if( rect == 0 )
    {
	pix_vm_gfx_draw_triangle_t( f4, f2, f1, cnum, color, vm );
	pix_vm_gfx_draw_triangle_t( f4, f2, f3, cnum, color, vm );
    }
    else 
    {
	PIX_INT ix;
	PIX_INT iy;
	PIX_INT iz;
	PIX_INT ixsize;
	PIX_INT iysize;
	
	bool ff;
	if( f1[ 0 ] < f3[ 0 ] )
	{
	    ff = 1;
	    ix = f1[ 0 ];
	    ixsize = (PIX_INT)f3[ 0 ] - ix;
	}
	else 
	{
	    ff = 0;
	    ix = f3[ 0 ];
	    ixsize = (PIX_INT)f1[ 0 ] - ix;
	}
	if( f1[ 1 ] < f3[ 1 ] )
	{
	    iy = f1[ 1 ];
	    iysize = (PIX_INT)f3[ 1 ] - iy;
	    if( ff )
	    {
		tx = tx;
		ty = ty;
		txsize = txsize;
		tysize = tysize;
	    }
	    else 
	    {
		tx = tx + txsize;
		ty = ty;
		txsize = -txsize;
		tysize = tysize;
	    }
	}
	else 
	{
	    iy = f3[ 1 ];
	    iysize = (PIX_INT)f1[ 1 ] - iy;
	    if( ff )
	    {
		tx = tx;
		ty = ty + tysize;
		txsize = txsize;
		tysize = -tysize;
	    }
	    else 
	    {
		tx = tx + txsize;
		ty = ty + tysize;
		txsize = -txsize;
		tysize = -tysize;
	    }
	}
	
	iz = (PIX_INT)( f1[ 2 ] * (PIX_FLOAT)( 1 << PIX_FIXED_MATH_PREC ) );
	
	tx <<= PIX_TEX_FIXED_MATH_PREC;
	ty <<= PIX_TEX_FIXED_MATH_PREC;
	
	PIX_INT dtx;
	PIX_INT dty;
	if( ixsize )
	    dtx = ( txsize << PIX_TEX_FIXED_MATH_PREC ) / ixsize;
	else
	    dtx = 0;
	if( iysize )
	    dty = ( tysize << PIX_TEX_FIXED_MATH_PREC ) / iysize;
	else
	    dty = 0;
	if( ix < 0 )
	{
	    ixsize += ix;
	    tx += dtx * -ix;
	    ix = 0;
	}
	if( iy < 0 )
	{
	    iysize += iy;
	    ty += dty * -iy;
	    iy = 0;
	}
	if( ix + ixsize > vm->screen_xsize )
	{
	    ixsize = vm->screen_xsize - ix;
	}
	if( iy + iysize > vm->screen_ysize )
	{
	    iysize = vm->screen_ysize - iy;
	}
	if( ixsize <= 0 || iysize <= 0 ) return;
	
	//Get texture:
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c == 0 ) return;
	if( g_pix_container_type_sizes[ c->type ] != COLORLEN ) return;
	COLORPTR txt = (COLORPTR)c->data;
	int txt_xsize = c->xsize;
	int txt_ysize = c->ysize;
	uchar* txt_alpha = 0;
	if( (unsigned)c->alpha < (unsigned)vm->c_num )
        {
            pix_vm_container* alpha_cont = vm->c[ c->alpha ];
            txt_alpha = (uchar*)alpha_cont->data;
        }
	
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
	scr += iy * vm->screen_xsize + ix;
	int* zbuf = pix_vm_gfx_get_zbuf( vm );
	if( zbuf ) zbuf += iy * vm->screen_xsize + ix;
	
	//Draw:
	int line_add = vm->screen_xsize - ixsize;
	if( c->flags & PIX_CONTAINER_FLAG_INTERP )
	{
	    //With interpolation:
	    tx -= 1 << ( PIX_TEX_FIXED_MATH_PREC - 1 );
	    ty -= 1 << ( PIX_TEX_FIXED_MATH_PREC - 1 );
	    tx += dtx / 2;
	    ty += dty / 2;
	    if( txt_alpha )
	    {
		//With alpha channel:
		if( transp == 255 )
		{
		    //Not transparent:
		    if( zbuf )
		    {
			//With Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				if( *zbuf < iz )
				{
				    GET_TEXTURE_PIXEL_WITH_ALPHA_INTERP;
				    if( !( uses_key && key == pixel ) ) 
				    {
					COLORIZE;
					*scr = blend( *scr, pixel, pixel_alpha );
					*zbuf = iz;
				    }
				}
				scr++;
				zbuf++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			    zbuf += line_add;
			}
		    }
		    else
		    {
			//Without Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				GET_TEXTURE_PIXEL_WITH_ALPHA_INTERP;
				if( !( uses_key && key == pixel ) ) 
				{
				    COLORIZE;
				    *scr = blend( *scr, pixel, pixel_alpha );
				}
				scr++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			}
		    }
		}
		else
		{
		    //Transparent:
		    if( zbuf )
		    {
			//With Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				if( *zbuf < iz )
				{
				    GET_TEXTURE_PIXEL_WITH_ALPHA_INTERP;
				    if( !( uses_key && key == pixel ) ) 
				    {
					COLORIZE;
					pixel_alpha = (uchar)( (int)pixel_alpha * (int)transp / 256 );
					*scr = blend( *scr, pixel, pixel_alpha );
					*zbuf = iz;
				    }
				}
				scr++;
				zbuf++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			    zbuf += line_add;
			}
		    }
		    else
		    {
			//Without Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				GET_TEXTURE_PIXEL_WITH_ALPHA_INTERP;
				if( !( uses_key && key == pixel ) ) 
				{
				    COLORIZE;
				    pixel_alpha = (uchar)( (int)pixel_alpha * (int)transp / 256 );
				    *scr = blend( *scr, pixel, pixel_alpha );
				}
				scr++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			}
		    }
		}
	    }
	    else 
	    {
		//Without alpha channel:
		if( transp == 255 )
		{
		    //Not transparent:
		    if( zbuf )
		    {
			//With Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				if( *zbuf < iz )
				{
				    GET_TEXTURE_PIXEL_INTERP;
				    if( !( uses_key && key == pixel ) ) 
				    {
					COLORIZE;
					*scr = pixel;
				    }
				}
				scr++;
				zbuf++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			    zbuf += line_add;
			}
		    }
		    else
		    {
			//Without Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				GET_TEXTURE_PIXEL_INTERP;
				if( !( uses_key && key == pixel ) ) 
				{
				    COLORIZE;
				    *scr = pixel;
				}
				scr++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			}
		    }
		}
		else
		{
		    //Transparent:
		    if( zbuf )
		    {
			//With Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				if( *zbuf < iz )
				{
				    GET_TEXTURE_PIXEL_INTERP;
				    if( !( uses_key && key == pixel ) ) 
				    {
					COLORIZE;
					*scr = fast_blend( *scr, pixel, transp );
					*zbuf = iz;
				    }
				}
				scr++;
				zbuf++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			    zbuf += line_add;
			}
		    }
		    else
		    {
			//Without Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				GET_TEXTURE_PIXEL_INTERP;
				if( !( uses_key && key == pixel ) ) 
				{
				    COLORIZE;
				    *scr = fast_blend( *scr, pixel, transp );
				}
				scr++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			}
		    }
		}
	    }
	}
	else
	{
	    //Without interpolation:
	    if( txt_alpha )
	    {
		//With alpha channel:
		if( transp == 255 )
		{
		    //Not transparent:
		    if( zbuf )
		    {
			//With Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				if( *zbuf < iz )
				{
				    GET_TEXTURE_PIXEL_WITH_ALPHA;
				    if( !( uses_key && key == pixel ) ) 
				    {
					COLORIZE;
					*scr = blend( *scr, pixel, pixel_alpha );
					*zbuf = iz;
				    }
				}
				scr++;
				zbuf++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			    zbuf += line_add;
			}
		    }
		    else
		    {
			//Without Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				GET_TEXTURE_PIXEL_WITH_ALPHA;
				if( !( uses_key && key == pixel ) ) 
				{
				    COLORIZE;
				    *scr = blend( *scr, pixel, pixel_alpha );
				}
				scr++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			}
		    }
		}
		else
		{
		    //Transparent:
		    if( zbuf )
		    {
			//With Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				if( *zbuf < iz )
				{
				    GET_TEXTURE_PIXEL_WITH_ALPHA;
				    if( !( uses_key && key == pixel ) ) 
				    {
					COLORIZE;
					pixel_alpha = (uchar)( (int)pixel_alpha * (int)transp / 256 );
					*scr = blend( *scr, pixel, pixel_alpha );
					*zbuf = iz;
				    }
				}
				scr++;
				zbuf++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			    zbuf += line_add;
			}
		    }
		    else
		    {
			//Without Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				GET_TEXTURE_PIXEL_WITH_ALPHA;
				if( !( uses_key && key == pixel ) ) 
				{
				    COLORIZE;
				    pixel_alpha = (uchar)( (int)pixel_alpha * (int)transp / 256 );
				    *scr = blend( *scr, pixel, pixel_alpha );
				}
				scr++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			}
		    }
		}
	    }
	    else 
	    {
		//Without alpha channel:
		if( transp == 255 )
		{
		    //Not transparent:
		    if( zbuf )
		    {
			//With Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				if( *zbuf < iz )
				{
				    GET_TEXTURE_PIXEL;
				    if( !( uses_key && key == pixel ) ) 
				    {
					COLORIZE;
					*scr = pixel;
				    }
				}
				scr++;
				zbuf++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			    zbuf += line_add;
			}
		    }
		    else
		    {
			//Without Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				GET_TEXTURE_PIXEL;
				if( !( uses_key && key == pixel ) ) 
				{
				    COLORIZE;
				    *scr = pixel;
				}
				scr++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			}
		    }
		}
		else
		{
		    //Transparent:
		    if( zbuf )
		    {
			//With Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				if( *zbuf < iz )
				{
				    GET_TEXTURE_PIXEL;
				    if( !( uses_key && key == pixel ) ) 
				    {
					COLORIZE;
					*scr = fast_blend( *scr, pixel, transp );
					*zbuf = iz;
				    }
				}
				scr++;
				zbuf++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			    zbuf += line_add;
			}
		    }
		    else
		    {
			//Without Z-Buffer:
			for( PIX_INT cy = iy; cy < iy + iysize; cy++ )
			{
			    PIX_INT ctx = tx;
			    for( PIX_INT cx = ix; cx < ix + ixsize; cx++ )
			    {
				GET_TEXTURE_PIXEL;
				if( !( uses_key && key == pixel ) ) 
				{
				    COLORIZE;
				    *scr = fast_blend( *scr, pixel, transp );
				}
				scr++;
				ctx += dtx;
			    }
			    ty += dty;
			    scr += line_add;
			}
		    }
		}
	    }
	}
    }
}

int* pix_vm_gfx_get_zbuf( pix_vm* vm )
{
    int* zbuf = 0;
    if( (unsigned)vm->zbuf < (unsigned)vm->c_num )
    {
	pix_vm_container* zbuf_cont = vm->c[ vm->zbuf ];
	if( zbuf_cont )
	{
	    if( zbuf_cont->data )
	    {
		if( zbuf_cont->type == PIX_CONTAINER_TYPE_INT32 )
		{
		    zbuf = (int*)zbuf_cont->data;
		}
		else 
		{
		    //blog( "ZBuffer must be INT32.\n" );
		}
	    }
	}
    }
    return zbuf;
}

// 0 - empty (this char can be ignored); 1 - new line possible (#$%); 2 - new line impossible (abcdef,.:)
const static char g_break_char_action[ 96 ] = 
{
    0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 2, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1
};

void pix_vm_gfx_draw_text( 
    utf8_char* str, size_t str_size, 
    PIX_FLOAT x, PIX_FLOAT y, PIX_FLOAT z, 
    int align, 
    COLOR color, 
    int max_xsize,
    int* out_xsize, int* out_ysize, 
    bool dont_draw, 
    pix_vm* vm )
{
    uchar transp = vm->transp;
    
    if( out_xsize || out_ysize )
    {
	if( out_xsize ) *out_xsize = 0;
	if( out_ysize ) *out_ysize = 0;
    }
    else
    {
	if( transp == 0 ) return;
    }
    
    //UTF8 -> UTF32:
    size_t len = 0;
    utf32_char c32;
    while( *str )
    {
	int size = utf8_to_utf32_char_safe( str, str_size, &c32 );
	if( size == 0 ) break;
	if( c32 == 0 ) break;
	str += size;
	str_size -= size;
	len++;
	if( vm->text == 0 ) 
	{
	    vm->text = (utf32_char*)bmem_new( sizeof( utf32_char ) );
	}
	else 
	{
	    size_t old_size = bmem_get_size( vm->text ) / sizeof( utf32_char );
	    if( old_size < len )
	    {
		//Resize the buffer:
		vm->text = (utf32_char*)bmem_resize( vm->text, ( old_size * 2 ) * sizeof( utf32_char ) );
	    }
	}
	if( vm->text == 0 ) return;
	vm->text[ len - 1 ] = c32;
	if( str_size == 0 ) break;
    }

    //Get the lines:
    int space_char_ysize = 0;
    size_t lines = 0;
    int line_xsize = 0;
    int line_ysize = 0;
    int lines_xsize = 0;
    int lines_ysize = 0;
    int word_end_xsize;
    int word_end_ysize;
    size_t word_end_i = 0;
    if( vm->text_lines == 0 ) vm->text_lines = (pix_vm_text_line*)bmem_new( sizeof( pix_vm_text_line ) );
    vm->text_lines[ lines ].offset = 0;
    for( size_t i = 0; i <= len; i++ )
    {
	if( i == len ) 
	    c32 = 0xA;
	else 
	    c32 = vm->text[ i ];
check_again:
	if( c32 == 0xA )
	{
	    //New line:
	    if( space_char_ysize == 0 )
	    {
		pix_vm_font* font = pix_vm_get_font_for_char( ' ', vm );
		if( font )
		{
		    pix_vm_container* cont = vm->c[ font->font ];
		    if( cont )
		    {
			space_char_ysize = cont->ysize / font->ychars;
		    }
		}
	    }
	    if( space_char_ysize > line_ysize ) line_ysize = space_char_ysize;
	    vm->text_lines[ lines ].xsize = line_xsize;
	    vm->text_lines[ lines ].ysize = line_ysize;
	    vm->text_lines[ lines ].end = i;
	    if( line_xsize > lines_xsize ) lines_xsize = line_xsize;
	    lines_ysize += line_ysize;
	    lines++;
	    size_t old_size = bmem_get_size( vm->text_lines ) / sizeof( pix_vm_text_line );
	    if( lines >= old_size )
		vm->text_lines = (pix_vm_text_line*)bmem_resize( vm->text_lines, ( old_size * 2 ) * sizeof( pix_vm_text_line ) );
	    vm->text_lines[ lines ].offset = i;
	    line_xsize = 0;
	    line_ysize = 0;
	    word_end_i = 0;
	}
	else 
	{
	    pix_vm_font* font = pix_vm_get_font_for_char( c32, vm );
	    if( max_xsize > 0 )
	    {
		if( c32 < '/' || ( c32 >= ':' && c32 <= '@' ) || ( c32 >= '[' && c32 <= ' ' ) )
		{
		    word_end_i = i + 1;
		    word_end_xsize = line_xsize;
		    word_end_ysize = line_ysize;
		}
	    }
	    int char_xsize = 0;
	    int char_ysize = 0;
	    if( font )
	    {
		pix_vm_container* cont = vm->c[ font->font ];
		if( cont )
		{
		    char_xsize = cont->xsize / font->xchars;
		    char_ysize = cont->ysize / font->ychars;
		}
	    }
	    if( max_xsize > 0 )
	    {
		if( line_xsize + char_xsize > max_xsize )
		{
		    //We need to break the current word:
		    int action;
		    if( (unsigned)c32 < 96 ) action = g_break_char_action[ (unsigned)c32 ]; else action = 2;
		    if( char_xsize > max_xsize ) action = 0;
		    switch( action )
		    {
			case 0:
			    //Ignore this char:
			    char_xsize = 0;
			    char_ysize = 0;
			    break;
			case 1:			    
			    //Make new line:
			    c32 = 0xA;
			    i--;
			    goto check_again;
			    break;
			case 2:
			    //Make new line and go to the end of the previous word:
			    if( word_end_i )
			    {
				line_xsize = word_end_xsize;
				line_ysize = word_end_ysize;
				c32 = 0xA;
				i = word_end_i;
				goto check_again;
			    }
			    break;
		    }
		}
	    }
	    line_xsize += char_xsize;
	    if( char_ysize > line_ysize ) line_ysize = char_ysize;
	}
    }
    
    //Save the bounds:
    if( out_xsize ) *out_xsize = lines_xsize;
    if( out_ysize ) *out_ysize = lines_ysize;
    
    //Draw:
    if( transp && dont_draw == 0 )
    {
	int screen_hxsize = vm->screen_xsize / 2;
	int screen_hysize = vm->screen_ysize / 2;
	if( lines > 0 )
	{
	    PIX_FLOAT line_x;
	    PIX_FLOAT line_y;
	    line_y = y - (PIX_FLOAT)lines_ysize / 2; //CENTER
	    if( align & 1 ) line_y = y; //TOP
	    if( align & 2 ) line_y = y - lines_ysize; //BOTTOM
	    for( int l = 0; l < lines; l++ )
	    {
		line_x = x - (PIX_FLOAT)vm->text_lines[ l ].xsize / 2; //CENTER
		if( align & 4 ) line_x = x; //LEFT
		if( align & 8 ) line_x = x - vm->text_lines[ l ].xsize; //RIGHT
		for( size_t i = vm->text_lines[ l ].offset; i < vm->text_lines[ l ].end; i++ )
		{
		    c32 = vm->text[ i ];
		    pix_vm_font* font = pix_vm_get_font_for_char( c32, vm );
		    if( font )
		    {
			pix_vm_container* cont = vm->c[ font->font ];
			if( cont )
			{
			    int char_xsize = cont->xsize / font->xchars;
			    int char_ysize = cont->ysize / font->ychars;
			    PIX_FLOAT yy = line_y + vm->text_lines[ l ].ysize - char_ysize;
			    
			    //Draw a char:
			    c32 -= font->first;
			    int char_x = ( c32 % font->xchars ) * char_xsize;
			    int char_y = ( c32 / font->xchars ) * char_ysize;
			    pix_vm_gfx_draw_container( font->font, line_x, yy, z, char_xsize, char_ysize, char_x, char_y, char_xsize, char_ysize, color, vm );
			    
			    line_x += char_xsize;
			}
		    }
		}
		line_y += vm->text_lines[ l ].ysize;
	    }
	}
    }
}
