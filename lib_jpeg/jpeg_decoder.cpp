/*
    jpeg_decoder.cpp
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

#include "core/core.h"
#include "jpeg_decoder.h"

#ifdef SIMPLE_JPEG_DECODER

// Simple JPEG Decoder:

void* load_jpeg( const utf8_char* filename, bfs_file f, int* width, int* height, int* num_components, jd_pixel_format pixel_format )
{
    void* dest = 0;
    
    jpeg_file_desc* jp = (jpeg_file_desc*)bmem_new( sizeof( jpeg_file_desc ) );
    if( jp == 0 ) return 0;
    bmem_zero( jp );
    if( filename && f == 0 )
    {
        f = bfs_open( filename, "rb" );
    }
    jp->file_name = filename;
    jp->file = f;
    if( jp->file == 0 )
    {
        blog( "JPEG decoder error: no file\n" );
        goto load_jpeg_end;
    }
    jpeg_readmarkers( jp );
    if( jp->width == 0 || jp->height == 0 )
    {
        blog( "JPEG decoder error: wrong image size %d %d %d\n", jp->width, jp->height, jp->num_components );
        goto load_jpeg_end;
    }
    jp->data = (unsigned char*)bmem_new( jp->width * jp->height * 3 );
    jpeg_decompress( jp );

    if( width ) *width = jp->width;
    if( height ) *height = jp->height;
    if( num_components ) *num_components = jp->num_components;

    switch( pixel_format )
    {
        case JD_GRAYSCALE:
    	    dest = bmem_new( jp->width * jp->height );
    	    if( dest )
    	    {
    		if( jp->num_components == 1 )
    		    bmem_copy( dest, jp->data, jp->width * jp->height );
		if( jp->num_components == 3 )
    		    jpeg_ycbcr2grayscale( (uchar*)dest, jp->data, jp->width, jp->height );
    	    }
    	    break;
    	case JD_RGB:
    	    dest = bmem_new( jp->width * jp->height * 3 );
    	    if( dest )
    	    {
    		uchar* p = (uchar*)dest;
    		if( jp->num_components == 1 )
		{
    		    for( size_t i = 0; i < jp->width * jp->height; i++ )
    		    {
        		uchar c = jp->data[ i ];
        		*p = c; p++;
        		*p = c; p++;
        		*p = c; p++;
    		    }
		}
		if( jp->num_components == 3 )
    		    jpeg_ycbcr2rgb( (uchar*)dest, jp->data, jp->width, jp->height );
    	    }
    	    break;
    	case JD_SUNDOG_COLOR:
    	    dest = bmem_new( jp->width * jp->height * COLORLEN );
    	    if( dest )
    	    {
    		COLORPTR p = (COLORPTR)dest;
    		if( jp->num_components == 1 )
		{
    		    for( size_t i = 0; i < jp->width * jp->height; i++ )
    		    {
        		uchar c = jp->data[ i ];
        		*p = get_color( c, c, c );
        		p++;
    		    }
		}
	        if( jp->num_components == 3 )
    		    jpeg_ycbcr2color( (COLORPTR)dest, jp->data, jp->width, jp->height );
    	    }
    	    break;
    }

load_jpeg_end:

    if( jp->file_name && jp->file )
    {
        bfs_close( jp->file );
    }
    bmem_free( jp->data );
    bmem_free( jp );
    
    return dest;
}

#else

// Normal JPEG Decoder:

void* load_jpeg( const utf8_char* filename, bfs_file f, int* width, int* height, int* num_components, jd_pixel_format pixel_format )
{
    void* rv = 0;
    
    if( filename && f == 0 )
	f = bfs_open( filename, "rb" );
    if( f == 0 ) return 0;
	
    while( 1 )
    {
	jpeg_decoder jd;
	jd_init( f, &jd );
	if( jd.m_error_code != JPGD_SUCCESS )
	{
	    blog( "JPEG loading: jd_init() error %d\n", jd.m_error_code );
	    break;
	}
	
	int image_width = jd.m_image_x_size;
	int image_height = jd.m_image_y_size;
	int image_num_components = jd.m_comps_in_frame;
	if( width ) *width = image_width;
	if( height ) *height = image_height;
	if( num_components ) *num_components = image_num_components;
    
	if( jd_begin_decoding( &jd ) != JPGD_SUCCESS )
	{
	    blog( "JPEG loading: jd_begin_decoding() error %d\n", jd.m_error_code );
	    break;
	}
	
	int dest_bpp;
	switch( pixel_format )
	{
	    case JD_GRAYSCALE: dest_bpp = 1; break;
	    case JD_RGB: dest_bpp = 3; break;
	    case JD_SUNDOG_COLOR: dest_bpp = COLORLEN; break;
	}
	const int dest_bpl = image_width * dest_bpp;

        uchar* dest = (uchar*)bmem_new( dest_bpl * image_height );
	if( !dest )
	    break;
	
	for( int y = 0; y < image_height; y++ )
	{
	    const uchar* scan_line;
	    uint scan_line_len;
	    if( jd_decode( (const void**)&scan_line, &scan_line_len, &jd ) != JPGD_SUCCESS )
	    {
		blog( "JPEG loading: jd_decode() error %d\n", jd.m_error_code );
    		bmem_free( dest );
    		dest = 0;
    		break;
	    }

	    uchar* dest_line = dest + y * dest_bpl;
	
	    if( pixel_format == JD_GRAYSCALE && image_num_components == 1 )
	    {
		memcpy( dest_line, scan_line, dest_bpl );
	    }
	    else
	    {
		switch( pixel_format )
		{
    		    case JD_GRAYSCALE:
    			if( image_num_components == 3 )
    			{
    			    int YR = 19595, YG = 38470, YB = 7471;
    			    for( int x = 0; x < image_width * 4; x += 4 )
    			    {
        			int r = scan_line[ x + 0 ];
        			int g = scan_line[ x + 1 ];
        			int b = scan_line[ x + 2 ];
        			*dest_line++ = static_cast<uchar>( ( r * YR + g * YG + b * YB + 32768 ) >> 16 );
    			    }
    			}
    			break;
    		    case JD_RGB:
    			if( image_num_components == 3 )
    			{
    			    for( int x = 0; x < image_width * 4; x += 4 )
    			    {
        			*dest_line++ = scan_line[ x + 0 ];
        			*dest_line++ = scan_line[ x + 1 ];
        			*dest_line++ = scan_line[ x + 2 ];
    			    }
    			}
    			if( image_num_components == 1 )
    			{
    			    for( int x = 0; x < image_width; x++ )
    			    {
        			uchar luma = scan_line[ x ];
        			*dest_line++ = luma;
        			*dest_line++ = luma;
        			*dest_line++ = luma;
    			    }
    			}
    			break;
    		    case JD_SUNDOG_COLOR:
    			if( image_num_components == 3 )
    			{
    			    COLORPTR p = (COLORPTR)dest_line;
    			    for( int x = 0; x < image_width * 4; x += 4 )
    			    {
        			int r = scan_line[ x + 0 ];
        			int g = scan_line[ x + 1 ];
        			int b = scan_line[ x + 2 ];
        			*p++ = get_color( r, g, b );
    			    }
    			}
    			if( image_num_components == 1 )
    			{
    			    COLORPTR p = (COLORPTR)dest_line;
    			    for( int x = 0; x < image_width; x++ )
    			    {
        			uchar luma = scan_line[ x ];
        			*p++ = get_color( luma, luma, luma );
    			    }
    			}
    			break;
    		}
	    }
	}

	jd_deinit( &jd );
	
	rv = (void*)dest;
	break;
    }

    if( f && filename )
    {
        bfs_close( f );
    }
    
    return rv;
}

#endif // normal JPEG decoder
