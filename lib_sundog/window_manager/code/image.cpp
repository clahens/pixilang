/*
    image.cpp. Functions for working with images
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

#include "core/core.h"

sundog_image* new_image( 
    int xsize, 
    int ysize, 
    void* src,
    int src_x,
    int src_y,
    int src_xsize, //whole source image size
    int src_ysize,
    uint flags,
    window_manager* wm )
{
    sundog_image* img = 0;
    
    img = (sundog_image*)bmem_new( sizeof( sundog_image ) );
    if( img )
    {
	img->wm = wm;
	img->xsize = xsize;
	img->ysize = ysize;
	img->flags = flags;
	img->backcolor = COLORMASK;
	img->color = COLORMASK;
	img->opacity = 255;
#if defined(OPENGL) && !defined(FRAMEBUFFER)
	img->int_xsize = round_to_power_of_two( xsize );
	img->int_ysize = round_to_power_of_two( ysize );
#else
	img->int_xsize = xsize;
	img->int_ysize = ysize;
#endif
	if( flags & IMAGE_STATIC_SOURCE )
	{
	    img->data = src;
	}
	else
	{
	    if( src_xsize - src_x < xsize ) xsize -= ( xsize - ( src_xsize - src_x ) );
	    if( src_ysize - src_y < ysize ) ysize -= ( ysize - ( src_ysize - src_y ) );
	    if( flags & IMAGE_NATIVE_RGB )
	    {
		img->data = bmem_new( img->int_xsize * img->int_ysize * COLORLEN );
		if( flags & IMAGE_CLEAN )
		    bmem_zero( img->data );
		if( src )
		{
		    COLORPTR data = (COLORPTR)img->data;
	    	    COLORPTR psrc = (COLORPTR)src;
	    	    for( int y = 0; y < ysize; y++ )
	    	    {
			int ptr = y * img->int_xsize;
			int src_ptr = ( y + src_y ) * src_xsize + src_x;
			for( int x = 0; x < xsize; x++ )
			{
		    	    data[ ptr ] = psrc[ src_ptr ];
			    ptr++;
			    src_ptr++;
			}
		    }
		}
	    }
	    if( flags & IMAGE_ALPHA8 )
	    {
		img->data = bmem_new( img->int_xsize * img->int_ysize );
		if( flags & IMAGE_CLEAN )
		    bmem_zero( img->data );
		if( src )
		{
	    	    uchar* data = (uchar*)img->data;
	    	    uchar* csrc = (uchar*)src;
	    	    for( int y = 0; y < ysize; y++ )
	    	    {
			int ptr = y * img->int_xsize;
			int src_ptr = ( y + src_y ) * src_xsize + src_x;
	    		for( int x = 0; x < xsize; x++ )
			{
		    	    data[ ptr ] = csrc[ src_ptr ];
		    	    ptr++;
		    	    src_ptr++;
			}
		    }
		}
	    }
	}
#if defined(OPENGL) && !defined(FRAMEBUFFER)
	if( img && img->data )
	{
	    gl_lock( wm );
	    
	    unsigned int texture_id;
	    glGenTextures( 1, &texture_id );
	    img->gl_texture_id = texture_id;
	    flush_image( img );
	    
	    gl_unlock( wm );
	}
#endif
    }
    
    return img;
}

void flush_image( sundog_image* img )
{
#if defined(OPENGL) && !defined(FRAMEBUFFER)
    if( img && img->data )
    {
	gl_lock( img->wm );
	
	void* data = img->data;
#if defined(OPENGL) && !defined(FRAMEBUFFER)
	void* temp_data = 0;
#endif
	
	if( img->flags & IMAGE_STATIC_SOURCE )
	{
#if defined(OPENGL) && !defined(FRAMEBUFFER)
	    if( img->int_xsize != img->xsize || img->int_ysize != img->ysize )
	    {
		int color_len = 1;
		if( img->flags & IMAGE_NATIVE_RGB ) color_len = COLORLEN;
		temp_data = bmem_new( img->int_xsize * img->int_ysize * color_len );
		data = temp_data;
		uchar* src_ptr = (uchar*)img->data;
		for( int y = 0; y < img->ysize; y++ )
		{
		    uchar* dest_ptr = (uchar*)temp_data + y * img->int_xsize * color_len;
		    bmem_copy( dest_ptr, src_ptr, img->xsize * color_len );
		    dest_ptr += img->xsize * color_len;
		    src_ptr += img->xsize * color_len;
		}
	    }
#endif
	}
	
        glBindTexture( GL_TEXTURE_2D, img->gl_texture_id );
	if( img->flags & IMAGE_INTERPOLATION )
	{
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}
	else
	{
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}
	if( img->flags & IMAGE_NO_REPEAT )
	{
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}
    	int internal_format;
        int format;
        int type;
        if( img->flags & IMAGE_NATIVE_RGB )
        {
#ifdef COLOR32BITS
            internal_format = GL_RGB;
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
#endif
#ifdef COLOR16BITS
            internal_format = GL_RGB;
            format = GL_RGB;
            type = GL_UNSIGNED_SHORT_5_6_5;
#endif
	}
	else if( img->flags & IMAGE_ALPHA8 )
	{
            internal_format = GL_ALPHA;
            format = GL_ALPHA;
            type = GL_UNSIGNED_BYTE;
	}
#ifdef OPENGLES
        if( internal_format == GL_RGB && format == GL_RGBA )
	{
    	    internal_format = GL_RGBA;
    	    uint* p = (uint*)data;
    	    for( size_t i = 0; i < img->int_xsize * img->int_ysize; i++ ) { (*p) |= 0xFF000000; p++; }
        }
#endif
        glTexImage2D(
	    GL_TEXTURE_2D,
    	    0,
    	    internal_format,
    	    img->int_xsize, img->int_ysize,
    	    0,
    	    format,
    	    type,
    	    data );

#if defined(OPENGL) && !defined(FRAMEBUFFER)
	bmem_free( temp_data );
#endif
	
	gl_unlock( img->wm );
    }
#endif
}

sundog_image* resize_image( int resize_flags, int new_xsize, int new_ysize, sundog_image* img )
{
    if( img && !( img->flags & IMAGE_STATIC_SOURCE ) && img->data )
    {
	sundog_image* new_img = new_image( new_xsize, new_ysize, img->data, 0, 0, img->int_xsize, img->int_ysize, img->flags, img->wm );
	remove_image( img );
	return new_img;
    }
    return 0;
}

void remove_image( sundog_image* img )
{
    if( img )
    {
#if defined(OPENGL) && !defined(FRAMEBUFFER)
	gl_lock( img->wm );
	glDeleteTextures( 1, &img->gl_texture_id );
	gl_unlock( img->wm );
#endif
	if( img->data && !( img->flags & IMAGE_STATIC_SOURCE ) )
	{
	    bmem_free( img->data );
	}
	bmem_free( img );
    }
}
