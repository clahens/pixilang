/*
    pixilang_vm_load_save.cpp
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

#ifndef NOJPEG
    #include "jpeg_decoder.h"
    #include "jpeg_encoder.h"
#endif

#ifndef NOPNG
#include "png.h"

static void user_png_read( png_structp png_ptr, png_bytep data, png_size_t length )
{
    bfs_file f = *(bfs_file*)png_get_io_ptr( png_ptr );
    bfs_read( data, 1, length, f );
}

static void user_png_write( png_structp png_ptr, png_bytep data, png_size_t length )
{
    bfs_file f = *(bfs_file*)png_get_io_ptr( png_ptr );
    bfs_write( data, 1, length, f );
}

static void user_png_flush( png_structp png_ptr )
{
    bfs_file f = *(bfs_file*)png_get_io_ptr( png_ptr );
    bfs_flush( f );
}

#endif

#ifndef NOGIF
#include "gif_lib.h"

int g_gif_interlaced_offset[] = { 0, 4, 2, 1 }; // The way Interlaced image should
int g_gif_interlaced_jumps[] = { 8, 8, 4, 2 };  // be read - offsets and jumps... 
GifColorType g_gif_bwcolors[ 2 ] = { {0,0,0}, {255,255,255} };
const uchar g_gif_netscape20ext[] = "NETSCAPE2.0";

static int user_gif_read( GifFileType* gf, GifByteType* buf, int count )
{
    bfs_file f = *(bfs_file*)gf->UserData;
    return bfs_read( buf, 1, count, f );
}

static int user_gif_write( GifFileType* gf, const GifByteType* buf, int count )
{
    bfs_file f = *(bfs_file*)gf->UserData;
    return bfs_write( (void*)buf, 1, count, f );
}

static int gif_load_create_frame( int frame_num, PIX_CID cnum, GifFileType* gf, pix_vm* vm )
{
    pix_vm_container* cont = pix_vm_get_container( cnum, vm );
    if( cont == 0 ) return -1;
    
    if( frame_num > 0 )
    {
	//Create new frame:
	if( pix_vm_get_container_hdata( cnum, vm ) == 0 )
	    pix_vm_create_container_hdata( cnum, pix_vm_container_hdata_type_anim, sizeof( pix_vm_container_hdata_anim ), vm );
	pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)pix_vm_get_container_hdata( cnum, vm );
	if( hdata == 0 ) return -1;
	hdata->frame_count = frame_num + 1;
	if( hdata->frames == 0 )
	    hdata->frames = (pix_vm_anim_frame*)bmem_new( hdata->frame_count * sizeof( pix_vm_anim_frame ) );
	if( hdata->frames == 0 ) return -1;
	if( hdata->frame_count > bmem_get_size( hdata->frames ) / sizeof( pix_vm_anim_frame ) )
	{
	    hdata->frames = (pix_vm_anim_frame*)bmem_resize( hdata->frames, hdata->frame_count * sizeof( pix_vm_anim_frame ) );
	    if( hdata->frames == 0 ) return -1;
	}
	pix_vm_anim_frame* f = &hdata->frames[ frame_num ];
	bmem_set( f, sizeof( pix_vm_anim_frame ), 0 );
	
	PIX_VAL prop_val;
	prop_val.i = hdata->frame_count; pix_vm_set_container_property( cnum, "frames", -1, 0, prop_val, vm );
	
	if( frame_num == 1 )
	{
	    //Create first frame:
	    bmem_set( &hdata->frames[ 0 ], sizeof( pix_vm_anim_frame ), 0 );
	    pix_vm_container* c = pix_vm_get_container( cnum, vm );
	    if( c && c->data )
	    {
		pix_vm_container_hdata_pack_frame_from_buf( cnum, 0, (COLORPTR)c->data, c->type, c->xsize, c->ysize, vm );
	    }
	}
	
	//Copy previous frame to current:
	pix_vm_anim_frame* prev_f = &hdata->frames[ frame_num - 1 ];
	bmem_copy( f, prev_f, sizeof( pix_vm_anim_frame ) );
	f->pixels = (COLORPTR)bmem_new( bmem_get_size( prev_f->pixels ) );
	bmem_copy( f->pixels, prev_f->pixels, bmem_get_size( prev_f->pixels ) );
    }
    
    return 0;
}

static COLORPTR gif_load_get_frame_ptr( int frame_num, PIX_CID cnum, GifFileType* gf, pix_vm* vm )
{
    COLORPTR dest = 0;
    if( frame_num == 0 )
	dest = (COLORPTR)pix_vm_get_container_data( cnum, vm );
    else
    {
	pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)pix_vm_get_container_hdata( cnum, vm );
	dest = (COLORPTR)bmem_new( gf->SWidth * gf->SHeight * COLORLEN );
	if( dest )
	{
	    if( pix_vm_container_hdata_unpack_frame_to_buf( cnum, frame_num, dest, vm ) != 0 )
	    {
		bmem_free( dest );
		dest = 0;
	    }
	}
    }
    return dest;
}

static void gif_load_release_frame_ptr( COLORPTR p, int frame_num, PIX_CID cnum, GifFileType* gf, pix_vm* vm )
{
    if( p == 0 ) return;
    if( frame_num > 0 )
    {
	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont == 0 ) return;
	pix_vm_container_hdata_pack_frame_from_buf( cnum, frame_num, p, cont->type, gf->SWidth, gf->SHeight, vm );
	bmem_free( p );
    }
}

static GifColorType* gif_load_get_colormap( int frame_num, PIX_CID cnum, GifFileType* gf, pix_vm* vm )
{
    GifColorType* cmap = 0;
    if( gf->SColorMap )
    {
	cmap = gf->SColorMap->Colors;
    }
    if( gf->Image.ColorMap )
    {
	cmap = gf->Image.ColorMap->Colors;
    }
    if( cmap == 0 )
    {
	cmap = g_gif_bwcolors;
    }
    return cmap;
}

#endif

static void swap_bytes( void* vdata, int size )
{
    char* data = (char*)vdata;
    for( int i = 0, i2 = size - 1; i < size / 2; i++, i2-- )
    {
	char b1 = data[ i ];
	char b2 = data[ i2 ];
	data[ i ] = b2;
	data[ i2 ] = b1;
    }
}

PIX_CID pix_vm_load( const utf8_char* filename, bfs_file f, int par1, pix_vm* vm )
{
    PIX_CID rv = -1;
    
    int format = PIX_FORMAT_RAW;
    
    if( filename && f == 0 ) f = bfs_open( filename, "rb" );
    if( f == 0 ) return -1;

    bfs_file_type ftype = bfs_get_file_type( 0, f );
    switch( ftype )
    {
	case BFS_FILE_TYPE_WAVE: format = PIX_FORMAT_WAVE; break;
	case BFS_FILE_TYPE_AIFF: format = PIX_FORMAT_AIFF; break;
        case BFS_FILE_TYPE_JPEG: format = PIX_FORMAT_JPEG; break;
	case BFS_FILE_TYPE_PNG: format = PIX_FORMAT_PNG; break;
	case BFS_FILE_TYPE_GIF: format = PIX_FORMAT_GIF; break;
	case BFS_FILE_TYPE_PIXICONTAINER: format = PIX_FORMAT_PIXICONTAINER; break;
	default: break;
    }
        
    PIX_VAL prop_val;
    
    switch( format )
    {
#ifndef NOJPEG
	case PIX_FORMAT_JPEG:
	    {
		int width;
		int height;
		void* img = load_jpeg( 0, f, &width, &height, 0, JD_SUNDOG_COLOR );
		if( img )
		{
		    rv = pix_vm_new_container( -1, width, height, 32, img, vm );
		}
	    }
	    break;
#endif
#ifndef NOPNG
	case PIX_FORMAT_PNG:
	    {
		png_structp png_ptr = 0;
		png_infop info_ptr = 0;
		png_bytep* row_pointers = 0;
		int width = 0;
		int height = 0;
		int color_type = 0;
		int bit_depth = 0;
		int channels = 0;
		int number_of_passes = 0;
		bool color_transformation = 0;
		png_bytep trans = 0;
		int num_trans = 0;
		png_color_16p trans_values = 0;
		COLORPTR palette = 0;
		int palette_colors = 0;
		
		while( 1 )
		{
		    png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
		    if( !png_ptr )
		    {
			PIX_VM_LOG( "PNG loading: png_create_read_struct failed\n" );
			break;
		    }
		    info_ptr = png_create_info_struct( png_ptr );
		    if( !info_ptr )
		    {
			PIX_VM_LOG( "PNG loading: png_create_info_struct failed\n" );
			break;
		    }
		    if( setjmp( png_jmpbuf( png_ptr ) ) )
		    {
			PIX_VM_LOG( "PNG loading: error during read_info\n" );
			break;
		    }
		    
		    png_set_read_fn( png_ptr, &f, user_png_read );
		    png_read_info( png_ptr, info_ptr );

		    width = png_get_image_width( png_ptr, info_ptr );
		    height = png_get_image_height( png_ptr, info_ptr );
		    color_type = png_get_color_type( png_ptr, info_ptr );
		    bit_depth = png_get_bit_depth( png_ptr, info_ptr );
		    channels = png_get_channels( png_ptr, info_ptr );

		    if( 0 ) //color_type == PNG_COLOR_TYPE_PALETTE )
		    {
			//Expand paletted colors into true RGB triplets:
			// (optional)
			png_set_expand( png_ptr );
			color_transformation = 1;
		    }
		    
		    if( png_get_tRNS( png_ptr, info_ptr, &trans, &num_trans, &trans_values ) )
		    {
			if( num_trans > 1 )
			{
			    //More than one transparent colors:
			    //Expand into RGBA:
			    png_set_expand( png_ptr );
			    color_transformation = 1;
			}
		    }

		    number_of_passes = png_set_interlace_handling( png_ptr );
		    png_read_update_info( png_ptr, info_ptr );
		    
		    //Read:
		    if( setjmp( png_jmpbuf( png_ptr ) ) )
		    {
			PIX_VM_LOG( "PNG loading: error during read_image\n" );
			break;
		    }
		    row_pointers = (png_bytep*)bmem_new( sizeof( png_bytep ) * height );
		    for( int y = 0; y < height; y++ )
			row_pointers[ y ] = (png_byte*)bmem_new( png_get_rowbytes( png_ptr, info_ptr ) );
		    png_read_image( png_ptr, row_pointers );
		    
		    if( color_transformation )
		    {
			color_type = png_get_color_type( png_ptr, info_ptr );
			bit_depth = png_get_bit_depth( png_ptr, info_ptr );
			channels = png_get_channels( png_ptr, info_ptr );
		    }
		    
		    if( png_get_tRNS( png_ptr, info_ptr, &trans, &num_trans, &trans_values ) == 0 )
		    {
			trans_values = 0;
		    }
		    
		    if( color_type == PNG_COLOR_TYPE_PALETTE )
		    {
			png_colorp pp = 0;
			int items = 0;
			if( png_get_PLTE( png_ptr, info_ptr, &pp, &items ) )
			{
			    palette_colors = items;
			    palette = (COLORPTR)bmem_new( items * sizeof( COLOR ) );
			    for( int i = 0; i < items; i++ )
			    {
				int r = pp[ i ].red;
				int g = pp[ i ].green;
				int b = pp[ i ].blue;
				palette[ i ] = get_color( r, g, b );
			    }
			}
		    }
		    
		    //Create a container:
		    PIX_CID alpha = -1;
		    uchar* alpha_data = 0;
		    COLORPTR dest = 0;
		    rv = pix_vm_new_container( -1, width, height, 32, 0, vm );
		    if( rv >= 0 )
		    {
			dest = (COLORPTR)pix_vm_get_container_data( rv, vm );
			if( color_type & PNG_COLOR_MASK_ALPHA )
			{
			    alpha = pix_vm_new_container( -1, width, height, PIX_CONTAINER_TYPE_INT8, 0, vm );
			    pix_vm_set_container_alpha( rv, alpha, vm );
			    alpha_data = (uchar*)pix_vm_get_container_data( alpha, vm );
			}
			COLOR transp_color = 0;
			int transp_r = 0;
			int transp_g = 0;
			int transp_b = 0;
			if( trans_values )
			{
			    while( 1 )
			    {
				if( palette )
				{
				    transp_color = palette[ trans_values[ 0 ].index ];
				    bool fix_colors = 0;
				    for( int i = 0; i < palette_colors; i++ )
					if( i != trans_values[ 0 ].index )
					    if( transp_color == palette[ i ] )
					    {
						fix_colors = 1;
						break;
					    }
				    if( fix_colors )
				    {
					for( int i = 0; i < palette_colors; i++ )
					    if( palette[ i ] == 1 )
						palette[ i ] = 2;
					palette[ trans_values[ 0 ].index ] = 1;
					transp_color = 1;
				    }
				    pix_vm_set_container_key_color( rv, transp_color, vm );
				    break;
				}
				if( bit_depth == 8 && channels == 1 )
				{
				    int t = trans_values[ 0 ].gray;
				    transp_r = t;
				    transp_color = get_color( t, t, t );
				    if( sizeof( COLOR ) <= 2 )
				    {
					transp_color = 1;
				    }
				    pix_vm_set_container_key_color( rv, transp_color, vm );
				    break;
				}
				if( bit_depth == 8 && channels == 3 )
				{
				    int r = trans_values[ 0 ].red;
				    int g = trans_values[ 0 ].green;
				    int b = trans_values[ 0 ].blue;
				    transp_r = r;
				    transp_g = g;
				    transp_b = b;
				    transp_color = get_color( r, g, b );
				    if( sizeof( COLOR ) <= 2 )
				    {
					transp_color = 1;
				    }
				    pix_vm_set_container_key_color( rv, transp_color, vm );
				    break;
				}
				break;
			    }
			}
			for( int y = 0; y < height; y++ )
			{
			    if( bit_depth == 8 )
			    {
				//8bit per channel:
				uchar* src = row_pointers[ y ];
				switch( channels )
				{
				    case 1:
					if( palette )
					{
					    //Palette:
					    for( int x = 0; x < width; x++ )
					    {
						int i = *src; src++;
						*dest = palette[ i ];
						dest++;
					    }
					}
					else
					{
					    //Grayscale:
					    for( int x = 0; x < width; x++ )
					    {
						int grey = *src; src++;
						*dest = get_color( grey, grey, grey );
						if( sizeof( COLOR ) <= 2 )
						{
						    if( transp_color == 1 )
						    {
							if( transp_r == grey )
							    *dest = 1;
							else
							    if( *dest == 1 )
								*dest = 2;
						    }
						}
						dest++;
					    }
					}
					break;
				    case 2:
					if( palette )
					{
					    //Palette:
					    for( int x = 0; x < width; x++ )
					    {
						int i = *src; src++;
						int a = *src; src++;
						*dest = palette[ i ];
						dest++;
						*alpha_data = a;
						alpha_data++;
					    }
					}
					else
					{
					    //Grayscale:
					    for( int x = 0; x < width; x++ )
					    {
						int grey = *src; src++;
						int a = *src; src++;
						*dest = get_color( grey, grey, grey );
						if( sizeof( COLOR ) <= 2 )
						{
						    if( transp_color == 1 )
						    {
							if( transp_r == grey )
							    *dest = 1;
							else
							    if( *dest == 1 )
								*dest = 2;
						    }
						}
						dest++;
						*alpha_data = a;
						alpha_data++;
					    }
					}
					break;
				    case 3:
					for( int x = 0; x < width; x++ )
					{
					    int r = *src; src++;
					    int g = *src; src++;
					    int b = *src; src++;
					    *dest = get_color( r, g, b );
					    if( sizeof( COLOR ) <= 2 )
					    {
						if( sizeof( COLOR ) <= 2 )
						{
						    if( transp_color == 1 )
						    {
							if( transp_r == r && transp_g == g && transp_b == b )
							    *dest = 1;
							else
							    if( *dest == 1 )
								*dest = 2;
						    }
						}
					    }
					    dest++;
					}
					break;
				    case 4:
					for( int x = 0; x < width; x++ )
					{
					    int r = *src; src++;
					    int g = *src; src++;
					    int b = *src; src++;
					    int a = *src; src++;
					    *dest = get_color( r, g, b );
					    dest++;
					    *alpha_data = a;
					    alpha_data++;
					}
					break;
				}
			    }
			}
		    }
		    
		    break;
		}

		//Cleanup:
		png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
		if( row_pointers )
		{
		    for( int y = 0; y < height; y++ )
			bmem_free( row_pointers[ y ] );
		    bmem_free( row_pointers );
		}
		
		bmem_free( palette );
	    }
	    break;
#endif
#ifndef NOGIF
	case PIX_FORMAT_GIF:
	    {
		uchar* screen_line = 0;
		COLORPTR next_screen = 0; //Next screen (layer 1). Next screen images will be on layer 2.
		GifFileType* gf = DGifOpen( &f, user_gif_read );
		while( gf )
		{
		    rv = pix_vm_new_container( -1, gf->SWidth, gf->SHeight, 32, 0, vm );
		    if( rv < 0 )
		    {
			PIX_VM_LOG( "GIF loading error: can't create a container\n" );
			break;
		    }
		    
		    GifRecordType rec_type = UNDEFINED_RECORD_TYPE;
		    int images_num = 0;
		    int ext_code;
		    GifByteType* extension;
		    bool transp_used = 0;
		    bool gce_transp = 0;
		    int gce_transp_index = 0;
		    int gce_delay = 0;
		    int gce_disposal = 0;
		    int anim_fps = 0;
		    int frame_num = 0;
		    size_t screen_size = gf->SWidth * gf->SHeight;
		    screen_line = (uchar*)bmem_new( gf->SWidth );
		    next_screen = (COLORPTR)bmem_new( screen_size * COLORLEN );
		    if( screen_line == 0 ) break;
		    if( next_screen == 0 ) break;
		    for( size_t i = 0; i < screen_size; i++ ) next_screen[ i ] = 1;
		    while( rec_type != TERMINATE_RECORD_TYPE )
		    {
			if( DGifGetRecordType( gf, &rec_type ) == GIF_ERROR ) break;
			switch( rec_type ) 
			{
			    case IMAGE_DESC_RECORD_TYPE:
				{
				    //printf( "IMG\n" );
				    if( DGifGetImageDesc( gf ) == GIF_ERROR ) break;
				    images_num++;
				    gif_load_create_frame( frame_num, rv, gf, vm );
				    int top = gf->Image.Top; // Image Position relative to Screen.
				    int left = gf->Image.Left;
				    int width = gf->Image.Width;
				    int height = gf->Image.Height;
				    if( gf->Image.Left + gf->Image.Width > gf->SWidth ||
					gf->Image.Top + gf->Image.Height > gf->SHeight ) 
				    {
					PIX_VM_LOG( "GIF loading error: image is not confined to screen dimension.\n" );
					break;
				    }
				    GifColorType* cmap = gif_load_get_colormap( frame_num, rv, gf, vm );
				    COLORPTR frame_pixels = gif_load_get_frame_ptr( frame_num, rv, gf, vm );
				    bmem_copy( frame_pixels, next_screen, screen_size * COLORLEN );
				    if( gf->Image.Interlace ) 
				    {
					// Need to perform 4 passes on the images:
					for( int i = 0; i < 4; i++ )
					{
					    for( int j = top + g_gif_interlaced_offset[ i ]; j < top + height; j += g_gif_interlaced_jumps[ i ] )
					    {
						if( DGifGetLine( gf, screen_line, width ) == GIF_ERROR ) break;
						COLORPTR dest = frame_pixels + j * gf->SWidth + left;
						for( int p = 0; p < width; p++ )
						{
						    uchar pixel = screen_line[ p ];
						    if( !( gce_transp && pixel == gce_transp_index ) )
						    {
							GifColorType* c = &cmap[ pixel ];
							COLOR col = get_color( c->Red, c->Green, c->Blue );
							if( col == 1 ) col = 2;
							*dest = col;
						    }
						    dest++;
						}
					    }
					}
				    }
				    else
				    {
					COLORPTR dest = frame_pixels + top * gf->SWidth + left;
					for( int y = 0; y < height; y++ ) 
					{
					    if( DGifGetLine( gf, screen_line, width ) == GIF_ERROR ) break;
					    for( int x = 0; x < width; x++ )
					    {
						uchar pixel = screen_line[ x ];
						if( !( gce_transp && pixel == gce_transp_index ) )
						{
						    GifColorType* c = &cmap[ pixel ];
						    COLOR col = get_color( c->Red, c->Green, c->Blue );
						    if( col == 1 ) col = 2;
						    *dest = col;
						}
						dest++;
					    }
					    dest += gf->SWidth - width;
					}
				    }
				    switch( gce_disposal )
				    {
					case 0:
					    //No disposal specified:
					case 1:
					    //Do not dispose:
					    bmem_copy( next_screen, frame_pixels, screen_size * COLORLEN );
					    break;
					case 2:
					    //Restore to background color:
					    {
						COLORPTR dest = next_screen + top * gf->SWidth + left;
						for( int y = 0; y < height; y++ )
						{
						    for( int x = 0; x < width; x++ )
						    {
							*dest = 1;
							dest++;
						    }
						    dest += gf->SWidth - width;
						}
					    }
					    break;
				    }
				    gif_load_release_frame_ptr( frame_pixels, frame_num, rv, gf, vm );
				    gce_disposal = 0;
				    if( par1 & PIX_LOAD_FIRST_FRAME )
					rec_type = TERMINATE_RECORD_TYPE;
				}
				break;
			    case EXTENSION_RECORD_TYPE:
				if( DGifGetExtension( gf, &ext_code, &extension ) == GIF_ERROR ) break;
				if( ext_code == GRAPHICS_EXT_FUNC_CODE ) 
				{
				    // Graphic Control Extension
				    //printf( "EXT\n" );
				    GifByteType* ptr = extension + 1;
				    if( *ptr & 0x1 ) 
				    {
					gce_transp_index = *( ptr + 3 );
					gce_transp = 1;
					transp_used = 1;
				    }
				    else
				    {
					gce_transp = 0;
				    }
				    
				    gce_delay = *( ptr + 1 ) | ( *( ptr + 2 ) << 8 );
				    if( anim_fps == 0 )
				    {
					if( gce_delay == 0 ) gce_delay = 1;
					anim_fps = 100 / gce_delay;
					if( anim_fps == 0 ) anim_fps = 1;
				    }
				    gce_disposal = ( (*ptr) >> 2 ) & 0x7;
				    if( images_num > 0 )
				    {
					frame_num++;
				    }
				    //printf( "FRAME %d. disposal: %d. transp: %d %d\n", frame_num, gce_disposal, gce_transp, gce_transp_index );
				}
				while( extension != NULL ) 
				{
				    if( DGifGetExtensionNext( gf, &extension ) == GIF_ERROR ) break;
				}
				break;
			    case TERMINATE_RECORD_TYPE:
				break;
			    default:
				break;
			}
		    }
		    
		    if( frame_num > 0 )
		    {
			prop_val.i = anim_fps; pix_vm_set_container_property( rv, "fps", -1, 0, prop_val, vm );
			prop_val.i = 0; pix_vm_set_container_property( rv, "frame", -1, 0, prop_val, vm );
			prop_val.i = -1; pix_vm_set_container_property( rv, "repeat", -1, 0, prop_val, vm );
		    }

		    if( transp_used )
			pix_vm_set_container_key_color( rv, 1, vm );
		    
		    break;
		}
		if( gf ) DGifCloseFile( gf );
		bmem_free( screen_line );
		bmem_free( next_screen );
	    }
	    break;
#endif
	case PIX_FORMAT_WAVE:
	    {
		bfs_seek( f, 0, 2 );
		size_t file_size = bfs_tell( f );
		bfs_seek( f, 12, 0 );
		    
		uint chunk[ 2 ]; //Chunk type and size
		int other_info;
		uint16 compression = 1;
		uint16 channels = 1;
		uint bytes_per_second = 0;
		uint16 block_align = 0;
		uint16 bits = 16;
		int freq = 44100;
		uint loop_start = 0;
		uint loop_len = 0;
		uint loop_type = 0;
		
		while( 1 )
		{
		    bfs_read( &chunk, 8, 1, f );
		    if( bfs_eof( f ) != 0 ) break;
		    bool chunk_handled = false;
		    if( chunk[ 0 ] == 0x20746D66 ) //'fmt ':
		    {
			bfs_read( &compression, 2, 1, f );
			bfs_read( &channels, 2, 1, f );
			bfs_read( &freq, 4, 1, f );
			bfs_read( &bytes_per_second, 4, 1, f );
			bfs_read( &block_align, 2, 1, f );
			bfs_read( &bits, 2, 1, f );
			if( bits != 8 && bits != 16 && bits != 24 && bits != 32 )
			{
			    PIX_VM_LOG( "WAVE loading: %d bits not supported\n", bits );
			    break;
			}
			if( compression != 1 && compression != 3 )
			{
			    PIX_VM_LOG( "WAVE loading: only uncompressed PCM files supported\n" );
			    break;
			}
			other_info = chunk[ 1 ] - 16;
			if( other_info ) 
			{
			    if( other_info & 1 ) other_info++;
			    bfs_seek( f, other_info, 1 );
			}
			chunk_handled = true;
		    } 
	            if( chunk[ 0 ] == 0x6C706D73 ) //'smpl':
	            {
        		uint dwManufacturer;
	                uint dwProduct;
	                uint dwSamplePeriod;
	                uint dwMIDIUnityNote;
	                uint dwMIDIPitchFraction;
	                uint dwSMPTEFormat;
	                uint dwSMPTEOffset;
	                uint cSampleLoops;
	                uint cbSamplerData;
	                bfs_read( &dwManufacturer, 4, 1, f );
	                bfs_read( &dwProduct, 4, 1, f );
	                bfs_read( &dwSamplePeriod, 4, 1, f );
	                bfs_read( &dwMIDIUnityNote, 4, 1, f );
	                bfs_read( &dwMIDIPitchFraction, 4, 1, f );
	                bfs_read( &dwSMPTEFormat, 4, 1, f );
	                bfs_read( &dwSMPTEOffset, 4, 1, f );
	                bfs_read( &cSampleLoops, 4, 1, f );
	                bfs_read( &cbSamplerData, 4, 1, f );
	                for( uint i = 0; i < cSampleLoops; i++ )
	                {
	                    uint dwIdentifier;
	                    uint dwType;
	                    uint dwStart;
	                    uint dwEnd;
	                    uint dwFraction;
	                    uint dwPlayCount;
	                    bfs_read( &dwIdentifier, 4, 1, f );
	                    bfs_read( &dwType, 4, 1, f );
	                    bfs_read( &dwStart, 4, 1, f );
	                    bfs_read( &dwEnd, 4, 1, f );
	                    bfs_read( &dwFraction, 4, 1, f );
	                    bfs_read( &dwPlayCount, 4, 1, f );
	                    if( i == 0 )
	                    {
		                switch( dwType )
    	    		        {
	                            case 0: loop_type = 1; break;
	                            case 1: loop_type = 2; break;
	                            default: loop_type = 1; break;
	                        }
	                        loop_start = dwStart;
	                        loop_len = dwEnd - dwStart + 1;
	                    }
	                }
	                bfs_seek( f, cbSamplerData, 1 );
        		chunk_handled = true;
		    }
		    if( chunk[ 0 ] == 0x61746164 ) //'data':
		    {
		        if( rv != 0 )
		        {
		    	    if( chunk[ 1 ] >= file_size )
			    {
			        PIX_VM_LOG( "WAVE loading: incorrect 'data' chunk (%d bytes)\n", (int)chunk[ 1 ] );
			        break;
			    }
			    int samples_num = chunk[ 1 ] / ( bits / 8 );
			    uint16 prev_bits = bits;
			    if( bits == 24 )
			        bits = 32;
			    pix_container_type ctype = PIX_CONTAINER_TYPE_INT8;
			    if( bits == 16 ) ctype = PIX_CONTAINER_TYPE_INT16;
			    if( bits == 32 ) ctype = PIX_CONTAINER_TYPE_FLOAT32;
			    rv = pix_vm_new_container( -1, samples_num, 1, ctype, 0, vm );
			    if( rv >= 0 )
			    {
			        prop_val.i = freq;
			        pix_vm_set_container_property( rv, "sample_rate", -1, 0, prop_val, vm );
			        prop_val.i = channels;
			        pix_vm_set_container_property( rv, "channels", -1, 0, prop_val, vm );
			        void* wave_data = pix_vm_get_container_data( rv, vm );
			        bfs_read( wave_data, 1, chunk[ 1 ], f ); //read sample data
			        if( chunk[ 1 ] & 1 ) bfs_seek( f, 1, 1 );
			        if( prev_bits == 24 )
			        {
			    	    //Convert from 24 to 32:
				    uchar* wave_data_char = (uchar*)wave_data;
				    float* wave_data_float = (float*)wave_data;
				    for( int i = samples_num - 1; i >= 0; i-- )
				    {
				        int p = i * 3;
				        int v;
				        v = wave_data_char[ p ];
				        v += wave_data_char[ p + 1 ] << 8; 
				        v += wave_data_char[ p + 2 ] << 16; 
				        if( v & 0x800000 )
					    v |= 0xFF000000;
					float fv = (float)v / ( 1 << 23 );
					wave_data_float[ i ] = fv;
				    }
				}
				if( bits == 8 )
				{
				    //Convert 8bit data:
				    uchar* wave_data_u = (uchar*)wave_data;
				    signed char* wave_data_s = (signed char*)wave_data;
				    for( uint s = 0; s < chunk[ 1 ]; s++ )
				    {
				        int v = wave_data_u[ s ];
				        v += 128;
				        wave_data_s[ s ] = (signed char)v;
				    }
				}
			    }
			    else 
			    {
			        PIX_VM_LOG( "WAVE loading: can't create the sample\n" );
			        break;
			    }
			}
			else 
			{
			    //Already got sound data:
			    bfs_seek( f, chunk[ 1 ], 1 );
			}
			if( chunk[ 1 ] & 1 ) bfs_seek( f, 1, 1 );
			chunk_handled = true;
		    }
		    if( chunk_handled == false )
		    {
		        bfs_seek( f, chunk[ 1 ], 1 );
		        if( chunk[ 1 ] & 1 ) bfs_seek( f, 1, 1 );
		    }
		}
		
		if( loop_type )
		{
		    prop_val.i = loop_start;
		    pix_vm_set_container_property( rv, "loop_start", -1, 0, prop_val, vm );
		    prop_val.i = loop_len;
		    pix_vm_set_container_property( rv, "loop_len", -1, 0, prop_val, vm );
		    prop_val.i = loop_type;
		    pix_vm_set_container_property( rv, "loop_type", -1, 0, prop_val, vm );
		}
	    }
	    break;
	case PIX_FORMAT_AIFF:
	    {
		bfs_seek( f, 4, 0 );
		
		uint size;
		bfs_read( &size, 1, 4, f );
		
		char form_type[ 4 ];
		bfs_read( &form_type, 1, 4, f );
    
		int type = -1;
		if( bmem_cmp( form_type, "AIFF", 4 ) == 0 ) type = 0;
		if( bmem_cmp( form_type, "AIFC", 4 ) == 0 ) type = 1;
		
		if( type == -1 )
		{
		    PIX_VM_LOG( "AIFF loading: unknown type: %s\n", form_type );
		    break;
		}
		
		if( type == 0 ) swap_bytes( &size, 4 );
		size -= 4;
		
		uint16 channels = 1;
		uint frames = 0;
		uint16 bits = 8;
		int freq = 44100;
		char compression_type[ 4 ];
		
		uint chunk_ids[ 2 ];
		bmem_copy( &chunk_ids[ 0 ], "COMM", 4 );
		bmem_copy( &chunk_ids[ 1 ], "SSND", 4 );
		uint ptr = 0;
		uint chunk[ 2 ]; //Chunk type and size
		while( ptr < size )
		{
		    bfs_read( &chunk, 8, 1, f );
		    if( bfs_eof( f ) != 0 ) break;
		    if( type == 0 )
		    {
			swap_bytes( &chunk[ 1 ], 4 );
		    }
		    uint chunk_type = chunk[ 0 ];
		    if( chunk_type == chunk_ids[ 0 ] )
		    {
			//Common:
			bfs_read( &channels, 1, 2, f );
			bfs_read( &frames, 1, 4, f );
			bfs_read( &bits, 1, 2, f );
			uchar freq80[ 10 ];
			bfs_read( freq80, 1, 10, f );
			if( type == 1 )
			{
			    bfs_read( compression_type, 1, 4, f );
			    uchar tt;
			    bfs_read( &tt, 1, 1, f );
			    bfs_seek( f, tt + 1, 1 );
			    bool comp_error = 1;
			    if( bmem_cmp( compression_type, "NONE", 4 ) == 0 ) comp_error = 0;
			    if( bmem_cmp( compression_type, "fl32", 4 ) == 0 ) comp_error = 0;
			    if( bmem_cmp( compression_type, "FL32", 4 ) == 0 ) comp_error = 0;
			    if( comp_error )
			    {
				PIX_VM_LOG( "AIFF Compression %s not supported\n", compression_type );
				break;
			    }
			}
			if( type == 0 )
			{
			    swap_bytes( &channels, 2 );
			    swap_bytes( &frames, 4 );
			    swap_bytes( &bits, 2 );
			    swap_bytes( freq80, 10 );
			}
			if( bits != 8 && bits != 16 && bits != 24 && bits != 32 )
			{
			    PIX_VM_LOG( "AIFF %d bits not supported\n", bits );
			    break;
			}	    
			bool sign;
			if( freq80[ 9 ] & 0x80 )
			    sign = 1;
			else 
			    sign = 0;
			uint16 exponent = freq80[ 8 ] + ( ( freq80[ 9 ] & 0x7F ) << 8 );
			exponent = exponent - 16383 + 127;
			exponent &= 255;
			uint fraction;
			bmem_copy( &fraction, &freq80[ 4 ], 4 );
			volatile float freq32;
			volatile uint* p = (uint*)&freq32;
			*p = ( ( fraction >> 8 ) & 0x7FFFFF ) | ( (uint)exponent << 23 ) | ( (uint)sign << 31 );
			freq = freq32;
		    }
		    else if( chunk_type == chunk_ids[ 1 ] )
		    {
			//Sound data:
			uint offset;
			uint block_size;
			bfs_read( &offset, 1, 4, f );
			bfs_read( &block_size, 1, 4, f );
			if( type == 0 )
			{
			    swap_bytes( &offset, 4 );
			    swap_bytes( &block_size, 4 );
			}
			if( rv < 0 )
			{
			    int samples_num = ( chunk[ 1 ] - 8 ) / ( bits / 8 );
			    uint16 prev_bits = bits;
			    if( bits == 24 )
				bits = 32;
			    pix_container_type ctype = PIX_CONTAINER_TYPE_INT8;
			    if( bits == 16 ) ctype = PIX_CONTAINER_TYPE_INT16;
			    if( bits == 32 ) ctype = PIX_CONTAINER_TYPE_FLOAT32;
			    rv = pix_vm_new_container( -1, samples_num, 1, ctype, 0, vm );
			    if( rv >= 0 )
			    {
				prop_val.i = freq;
				pix_vm_set_container_property( rv, "sample_rate", -1, 0, prop_val, vm );
				prop_val.i = channels;
				pix_vm_set_container_property( rv, "channels", -1, 0, prop_val, vm );
				void* smp_data = pix_vm_get_container_data( rv, vm );
				bfs_read( smp_data, 1, chunk[ 1 ] - 8, f ); //read sample data
				if( prev_bits != 24 && type == 0 )
				{
				    int sample_size = bits / 8;
				    char* smp_data_char = (char*)smp_data;
				    for( int i = 0; i < samples_num * sample_size; i += sample_size )
				    {
					swap_bytes( &smp_data_char[ i ], sample_size );
				    }
				}
				if( prev_bits == 24 )
				{
				    //Convert from 24 to 32:
				    uchar* smp_data_char = (uchar*)smp_data;
				    float* smp_data_float = (float*)smp_data;
				    for( int i = samples_num - 1; i >= 0; i-- )
				    {
					int p = i * 3;
					int v;
					if( type == 0 )
					{
					    v = smp_data_char[ p ] << 16;
					    v += smp_data_char[ p + 1 ] << 8; 
					    v += smp_data_char[ p + 2 ]; 
					}
					else if( type == 1 )
					{
					    v = smp_data_char[ p ];
					    v += smp_data_char[ p + 1 ] << 8; 
					    v += smp_data_char[ p + 2 ] << 16; 
					}
					if( v & 0x800000 )
					    v |= 0xFF000000;
					float fv = (float)v / ( 1 << 23 );
					smp_data_float[ i ] = fv;
				    }
				}
			    }
			    else 
			    {
				PIX_VM_LOG( "AIFF loading: can't create the new container\n" );
				break;
			    }
			}
			else 
			{
			    //Already got sound data.
			    bfs_seek( f, chunk[ 1 ], 1 );
			}
		    }
		    else 
		    {
			//Unknown chunk:
			bfs_seek( f, chunk[ 1 ], 1 );
		    }
		    ptr += chunk[ 1 ];
		}
	    }
	    break;
	case PIX_FORMAT_PIXICONTAINER:
	    {
		utf8_char chunk_name[ 64 ];
		uint64 chunk_size;
		
		int colorlen = COLORLEN;
		int intlen = sizeof( PIX_INT );
		int floatlen = sizeof( PIX_FLOAT );

		pix_container_type cont_type = PIX_CONTAINER_TYPE_INT8;
		uint cont_flags = 0;
		PIX_INT cont_xsize = 0;
		PIX_INT cont_ysize = 0;
		void* cont_data = 0;
		COLOR cont_key = 0;
		
		bool eof = 0;		    
		while( eof == 0 )
		{
		    //Read chunk name:
		    for( int i = 0; i < sizeof( chunk_name ); i++ )
		    {
			int c = bfs_getc( f );
			if( c < 0 ) { chunk_name[ i ] = 0; break; }
			chunk_name[ i ] = c;
			if( c == 0 ) break;
		    }
		    if( eof ) break;
		    chunk_name[ sizeof( chunk_name ) - 1 ] = 0;
		    
		    //Read chunk size:
		    if( bfs_read( &chunk_size, 1, sizeof( chunk_size ), f ) != sizeof( chunk_size ) )
			break;
			
		    if( chunk_name[ 0 ] == 'P' )
		    {
			//Property:
			if( rv )
			{
			    PIX_VAL val;
			    char type = 0;
			    bmem_set( &val, sizeof( val ), 0 );
			    type = bfs_getc( f );
			    if( type < 0 ) break;
			    chunk_size -= 1;
			    if( type == 0 || ( type == 1 && floatlen == sizeof( PIX_FLOAT ) ) )
			    {
				size_t size = chunk_size;
				if( chunk_size > sizeof( val ) ) size = sizeof( val );
				if( bfs_read( &val, 1, size, f ) != size ) break;
				chunk_size -= size;
			    }
			    if( type == 1 && floatlen != sizeof( PIX_FLOAT ) )
			    {
				uchar ff[ 16 ];
				if( bfs_read( ff, 1, chunk_size, f ) != chunk_size ) break;
				if( floatlen == 4 ) val.f = ((float*)&ff)[ 0 ];
				if( floatlen == 8 ) val.f = ((double*)&ff)[ 0 ];
				chunk_size = 0;
			    }
			    pix_vm_set_container_property( rv, chunk_name + 1, -1, type, val, vm );
			}
			goto next_pc_chunk;
		    }

		    if( bmem_strcmp( (const utf8_char*)chunk_name, "colorlen" ) == 0 )
		    {
			colorlen = bfs_getc( f );
			if( colorlen < 0 ) break;
			chunk_size -= 1;
			goto next_pc_chunk;
		    }

		    if( bmem_strcmp( (const utf8_char*)chunk_name, "intlen" ) == 0 )
		    {
			intlen = bfs_getc( f );
			if( intlen < 0 ) break;
			chunk_size -= 1;
			goto next_pc_chunk;
		    }

		    if( bmem_strcmp( (const utf8_char*)chunk_name, "floatlen" ) == 0 )
		    {
			floatlen = bfs_getc( f );
			if( floatlen < 0 ) break;
			chunk_size -= 1;
			goto next_pc_chunk;
		    }
			
		    if( bmem_strcmp( (const utf8_char*)chunk_name, "type" ) == 0 )
		    {
			size_t size = chunk_size;
			if( chunk_size > sizeof( cont_type ) ) size = sizeof( cont_type );
			if( bfs_read( &cont_type, 1, size, f ) != size ) break;
			chunk_size -= size;
			goto next_pc_chunk;
		    }

		    if( bmem_strcmp( (const utf8_char*)chunk_name, "flags" ) == 0 )
		    {
			size_t size = chunk_size;
			if( chunk_size > sizeof( cont_flags ) ) size = sizeof( cont_flags );
			if( bfs_read( &cont_flags, 1, size, f ) != size ) break;
			chunk_size -= size;
			goto next_pc_chunk;
		    }

		    if( bmem_strcmp( (const utf8_char*)chunk_name, "xsize" ) == 0 )
		    {
			size_t size = chunk_size;
			if( chunk_size > sizeof( cont_xsize ) ) size = sizeof( cont_xsize );
			if( bfs_read( &cont_xsize, 1, size, f ) != size ) break;
			chunk_size -= size;
			goto next_pc_chunk;
		    }

		    if( bmem_strcmp( (const utf8_char*)chunk_name, "ysize" ) == 0 )
		    {
			size_t size = chunk_size;
			if( chunk_size > sizeof( cont_ysize ) ) size = sizeof( cont_ysize );
			if( bfs_read( &cont_ysize, 1, size, f ) != size ) break;
			chunk_size -= size;
			goto next_pc_chunk;
		    }

		    if( bmem_strcmp( (const utf8_char*)chunk_name, "data" ) == 0 )
		    {
			cont_data = bmem_new( chunk_size );
			if( cont_data == 0 ) break;
			if( bfs_read( cont_data, 1, chunk_size, f ) != chunk_size ) break;
			chunk_size = 0;
			goto next_pc_chunk;
		    }

		    if( bmem_strcmp( (const utf8_char*)chunk_name, "key color" ) == 0 )
		    {
			uchar rgb[ 3 ];
			if( bfs_read( rgb, 1, 3, f ) != 3 ) break;
			cont_key = get_color( rgb[ 0 ], rgb[ 1 ], rgb[ 2 ] );
			chunk_size -= 3;
			goto next_pc_chunk;
		    }

		    if( bmem_strcmp( (const utf8_char*)chunk_name, "create" ) == 0 )
		    {
			rv = pix_vm_new_container( -1, cont_xsize, cont_ysize, cont_type, cont_data, vm );
			goto next_pc_chunk;
		    }

		    if( bmem_strcmp( (const utf8_char*)chunk_name, "hdata" ) == 0 )
		    {
			if( pix_vm_load_container_hdata( rv, f, vm ) != chunk_size ) break;
			chunk_size = 0;
			goto next_pc_chunk;
		    }
		    
next_pc_chunk:
		    
		    if( chunk_size > 0 )
		    {
			//Skip the rest of chunk:
			if( bfs_seek( f, chunk_size, 1 ) ) break;
		    }
		}
	    }
	    break;
    }
    
    if( rv < 0 )
    {
	//Some error or format not recognized:
	format = PIX_FORMAT_RAW;
	bfs_seek( f, 0, 2 );
	size_t size = bfs_tell( f );
	bfs_rewind( f );
	void* data = bmem_new( size );
	bfs_read( data, 1, size, f );
	rv = pix_vm_new_container( -1, size, 1, PIX_CONTAINER_TYPE_INT8, data, vm );
    }
    
    if( rv >= 0 )
    {
	prop_val.i = format;
	pix_vm_set_container_property( rv, "file_format", -1, 0, prop_val, vm );
    }
    
    if( filename && f ) bfs_close( f );
    
    return rv;
}

#define GIF_DITHERING \
    if( ( x + y ) & 1 ) \
    { \
	if( y & 1 ) \
	{ \
	    if( ( r & 31 ) >= 10 && r < 256 - 32 ) r += 32; \
    	    if( ( g & 31 ) >= 10 && g < 256 - 32 ) g += 32; \
	    if( ( b & 63 ) >= 20 && b < 256 - 64 ) b += 64; \
	} \
	else \
	{ \
	    if( ( r & 31 ) >= 21 && r < 256 - 32 ) r += 32; \
	    if( ( g & 31 ) >= 21 && g < 256 - 32 ) g += 32; \
	    if( ( b & 63 ) >= 42 && b < 256 - 64 ) b += 64; \
	} \
    }
    
int pix_vm_save_chunk( uint64 size, const utf8_char* name, bfs_file f )
{
    size_t len = bmem_strlen( name ) + 1; 
    if( bfs_write( name, 1, len, f ) != len ) return -1;
    if( bfs_write( &size, 1, sizeof( size ), f ) != sizeof( size ) ) return -1;
    return 0;
}

int pix_vm_save( PIX_CID cnum, const utf8_char* filename, bfs_file f, int format, int par1, pix_vm* vm )
{
    int rv = -1;
    
    if( filename && f == 0 ) f = bfs_open( filename, "wb" );
    if( f == 0 ) return -1;
    
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( c && c->data && c->size )
	{
	    switch( format )
	    {
		case PIX_FORMAT_RAW:
		    {
			bfs_write( c->data, 1, c->size * g_pix_container_type_sizes[ c->type ], f );
			bfs_close( f );
			rv = 0;
		    }
		    break;
#ifndef NOJPEG
		case PIX_FORMAT_JPEG:
		    {
			if( sizeof( COLOR ) != g_pix_container_type_sizes[ c->type ] )
			{
			    PIX_VM_LOG( "JPEG saving: type of the container != PIXEL; can't write.\n" );
			    break;
			}
			
			je_params jpg_params;
			init_je_params( &jpg_params );
			if( par1 ) 
			{
			    int q = PIX_JPEG_QUALITY( par1 );
			    if( q == 0 ) q = 1;
			    if( q > 100 ) q = 100;
			    jpg_params.quality = q;
			    if( ( par1 & PIX_JPEG_SUBSAMPLING_MASK ) == PIX_JPEG_H1V1 ) jpg_params.subsampling = JE_H1V1;
			    if( ( par1 & PIX_JPEG_SUBSAMPLING_MASK ) == PIX_JPEG_H2V1 ) jpg_params.subsampling = JE_H2V1;
			    if( ( par1 & PIX_JPEG_SUBSAMPLING_MASK ) == PIX_JPEG_H2V2 ) jpg_params.subsampling = JE_H2V2;
			    if( par1 & PIX_JPEG_TWOPASS )
				jpg_params.two_pass_flag = 1;
			}
			jpg_params.pixel_format = JE_SUNDOG_COLOR;
			save_jpeg( 0, f, c->xsize, c->ysize, (const uchar*)pix_vm_get_container_data( cnum, vm ), &jpg_params );

			rv = 0;
		    }
		    break;
#endif
#ifndef NOPNG
		case PIX_FORMAT_PNG:
		    {
			if( sizeof( COLOR ) != g_pix_container_type_sizes[ c->type ] )
			{
			    PIX_VM_LOG( "PNG saving: type of the container != PIXEL; can't write.\n" );
			    break;
			}
			
			uchar* alpha = (uchar*)pix_vm_get_container_alpha_data( cnum, vm );
			COLORPTR img = (COLORPTR)pix_vm_get_container_data( cnum, vm );
			
			png_structp png_ptr;
			png_infop info_ptr;
			png_bytep* row_pointers = 0;

			//Create rows:
			row_pointers = (png_bytep*)bmem_new( sizeof( png_bytep* ) * c->ysize );
			for( int y = 0; y < c->ysize; y++ )
			{
			    if( alpha )
				row_pointers[ y ] = (png_bytep)bmem_new( 4 * c->xsize );
			    else
				row_pointers[ y ] = (png_bytep)bmem_new( 3 * c->xsize );
			    COLORPTR src = img + y * c->xsize;
			    uchar* src_alpha = 0;
			    if( alpha ) src_alpha = alpha + y * c->xsize;
			    uchar* dest = row_pointers[ y ];
			    for( int x = 0; x < c->xsize; x++ )
			    {
				COLOR pixel = *src; src++;
				*dest = red( pixel ); dest++;
				*dest = green( pixel ); dest++;
				*dest = blue( pixel ); dest++;
				if( alpha ) 
				{
				    *dest = *src_alpha; 
				    dest++;
				    src_alpha++;
				}
			    }
			}
			
			while( 1 )
			{
			    //Initialize stuff:
			    png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
			    if( !png_ptr )
			    {
				PIX_VM_LOG( "PNG saving: png_create_write_struct failed\n" );
				break;
			    }
			    info_ptr = png_create_info_struct( png_ptr );
			    if( !info_ptr )
			    {
				PIX_VM_LOG( "PNG saving: png_create_info_struct failed\n" );
				break;
			    }
			    
			    //Write header:
			    if( setjmp( png_jmpbuf( png_ptr ) ) )
			    {
				PIX_VM_LOG( "PNG saving: error during writing header\n" );
				break;
			    }
			    if( alpha )
			    {
				png_set_IHDR( png_ptr, info_ptr, c->xsize, c->ysize, 8, 
						PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
						PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );
			    }
			    else
			    {
				png_set_IHDR( png_ptr, info_ptr, c->xsize, c->ysize, 8, 
						PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
						PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );
			    }
			    if( pix_vm_get_container_flags( cnum, vm ) & PIX_CONTAINER_FLAG_USES_KEY )
			    {
				COLOR transp_color = pix_vm_get_container_key_color( cnum, vm );
				png_color_16 trans_values;
				trans_values.red = red( transp_color );
				trans_values.green = green( transp_color );
				trans_values.blue = blue( transp_color );
				png_set_tRNS( png_ptr, info_ptr, 0, 0, &trans_values );
			    }
			    png_set_write_fn( png_ptr, &f, user_png_write, user_png_flush );
			    png_write_info( png_ptr, info_ptr );
			    
			    //Write bytes:
			    if( setjmp( png_jmpbuf( png_ptr ) ) )
			    {
				PIX_VM_LOG( "PNG saving: error during writing bytes\n" );
				break;
			    }
			    png_write_image( png_ptr, row_pointers );
			    
			    //End write:
			    if( setjmp( png_jmpbuf( png_ptr ) ) )
			    {
				PIX_VM_LOG( "PNG saving: error during end of write\n" );
				break;
			    }
			    png_write_end( png_ptr, NULL );
			    
			    rv = 0;
			    
			    break;
			}

			png_destroy_write_struct( &png_ptr, &info_ptr );
			if( row_pointers )
			{
			    for( int y = 0; y < c->ysize; y++ )
				bmem_free( row_pointers[ y ] );
			    bmem_free( row_pointers );
			}
		    }
		    break;
#endif
#ifndef NOGIF
		case PIX_FORMAT_GIF:
		    {
			if( sizeof( COLOR ) != g_pix_container_type_sizes[ c->type ] )
			{
			    PIX_VM_LOG( "GIF saving: type of the container != PIXEL; can't write.\n" );
			    break;
			}
			
			GifColorType* pal = 0;
			uchar* screen_line = 0;
			GifFileType* gf = EGifOpen( &f, user_gif_write );
			while( gf )
			{
			    screen_line = (uchar*)bmem_new( c->xsize );
			    if( screen_line == 0 ) break;
			    
			    //Create palette:
			    pal = (GifColorType*)bmem_new( sizeof( GifColorType ) * 256 );
			    int pal_transp_index = 1;
			    if( pal == 0 ) break;
			    if( ( par1 & PIX_GIF_PALETTE_MASK ) == 0 )
			    {
				//256 color palette:
				for( int i = 0; i < 256; i++ )
				{
				    GifColorType* c = &pal[ i ];
				    c->Red = i & 7;
				    if( c->Red == 7 ) c->Red = 255; else c->Red <<= 5;
				    c->Green = ( i >> 3 ) & 7;
				    if( c->Green == 7 ) c->Green = 255; else c->Green <<= 5;
				    c->Blue = ( i >> 6 ) & 3;
				    if( c->Blue == 3 ) c->Blue = 255; else c->Blue <<= 6;
				}
			    }
			    if( ( par1 & PIX_GIF_PALETTE_MASK ) == PIX_GIF_GRAYSCALE )
			    {
				//256 grayscale palette:
				for( int i = 0; i < 256; i++ )
				{
				    GifColorType* c = &pal[ i ];
				    c->Red = i;
				    c->Green = i;
				    c->Blue = i;
				}
			    }
			    ColorMapObject* cmap = MakeMapObject( 256, pal );

			    if( EGifPutScreenDesc( gf, c->xsize, c->ysize, 8, 0, cmap ) == GIF_ERROR ) break;

			    int frame_count = pix_vm_container_hdata_get_frame_count( cnum, vm );
			    if( c->flags & PIX_CONTAINER_FLAG_USES_KEY || frame_count > 0 )
			    {
				//Animation or/and transparency.
				{
				    EGifPutExtensionFirst( gf, APPLICATION_EXT_FUNC_CODE, 11, (const VoidPtr)g_gif_netscape20ext );
				    int rep = pix_vm_get_container_property_i( cnum, "repeat", -1, vm );
				    if( rep < 0 ) rep = 0;
				    uchar netscape_ext_subblock[ 3 ];
				    netscape_ext_subblock[ 0 ] = 1;
				    netscape_ext_subblock[ 1 ] = rep & 255;
				    netscape_ext_subblock[ 2 ] = ( rep >> 8 ) & 255;
				    EGifPutExtensionLast( gf, 0, 3, (const VoidPtr)netscape_ext_subblock );
				}
				for( int fnum = 0; fnum < frame_count; fnum++ )
				{
				    pix_container_type type;
				    int xsize, ysize;
				    if( pix_vm_container_hdata_get_frame_size( cnum, fnum, &type, &xsize, &ysize, vm ) ) break;
				    void* frame_buf = bmem_new( xsize * ysize * g_pix_container_type_sizes[ type ] );
				    if( frame_buf == 0 ) break;
				    if( pix_vm_container_hdata_unpack_frame_to_buf( cnum, fnum, (COLORPTR)frame_buf, vm ) ) break;
				    {
					//Graphic Control Extension:
					uchar gce[ 4 ];
					int fps = pix_vm_get_container_property_i( cnum, "fps", -1, vm );
					if( fps <= 0 ) fps = 20;
					fps = 100 / fps;
					gce[ 0 ] = 0;
					gce[ 1 ] = fps & 255;
					gce[ 2 ] = ( fps >> 8 ) & 255;
					gce[ 3 ] = 0;
					if( c->flags & PIX_CONTAINER_FLAG_USES_KEY )
					{
					    gce[ 0 ] |= 2 << 2; //Disposal method: Restore to background color
					    gce[ 0 ] |= 1; //Transparent Color Flag
					    gce[ 3 ] = pal_transp_index; //Transparent color index
					}
					EGifPutExtension( gf, GRAPHICS_EXT_FUNC_CODE, sizeof( gce ), (const VoidPtr)gce );
				    }
				    if( EGifPutImageDesc( gf, 0, 0, xsize, ysize, 0, 0 ) == GIF_ERROR ) break;
				    COLORPTR src = (COLORPTR)frame_buf;
				    for( int y = 0; y < ysize; y++ )
				    {
					if( c->flags & PIX_CONTAINER_FLAG_USES_KEY )
					{
					    if( ( par1 & PIX_GIF_PALETTE_MASK ) == 0 )
					    {
						//Full color:
						if( ( par1 & PIX_GIF_DITHER ) == 0 )
						    //No dithering:
						    for( int x = 0; x < xsize; x++ )
						    {
							COLOR pixel = *src;
							uchar val;
							if( pixel == c->key )
							    val = pal_transp_index;
							else
							{
							    int r = red( pixel );
							    int g = green( pixel );
							    int b = blue( pixel );
							    val = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
							    if( val == pal_transp_index ) val = 0;
							}
							screen_line[ x ] = val;
							src++;
						    }
						else
						    //Dithering:
						    for( int x = 0; x < xsize; x++ )
						    {
							COLOR pixel = *src;
							uchar val;
							if( pixel == c->key )
							    val = pal_transp_index;
							else
							{
							    int r = red( pixel );
							    int g = green( pixel );
							    int b = blue( pixel );
							    GIF_DITHERING;
							    val = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
							    if( val == pal_transp_index ) val = 0;
							}
							screen_line[ x ] = val;
							src++;
						    }
					    }
					    if( ( par1 & PIX_GIF_PALETTE_MASK ) == PIX_GIF_GRAYSCALE )
						//Grayscale:
						for( int x = 0; x < xsize; x++ )
						{
						    COLOR pixel = *src;
						    uchar val;
						    if( pixel == c->key )
							val = pal_transp_index;
						    else
						    {
							int g = ( red( pixel ) + green( pixel ) + blue( pixel ) ) / 3;
							val = g;
							if( val == pal_transp_index ) val = 0;
						    }
						    screen_line[ x ] = val;
						    src++;
						}
					}
					else
					{
					    if( ( par1 & PIX_GIF_PALETTE_MASK ) == 0 )
					    {
						//Full color:
						if( ( par1 & PIX_GIF_DITHER ) == 0 )
						    //No dithering:
						    for( int x = 0; x < xsize; x++ )
						    {
							COLOR pixel = *src;
							int r = red( pixel );
							int g = green( pixel );
							int b = blue( pixel );
							screen_line[ x ] = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
							src++;
						    }
						else
						    //Dithering:
						    for( int x = 0; x < xsize; x++ )
						    {
							COLOR pixel = *src;
							int r = red( pixel );
							int g = green( pixel );
							int b = blue( pixel );
							GIF_DITHERING;
							screen_line[ x ] = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
							src++;
						    }
					    }
					    if( ( par1 & PIX_GIF_PALETTE_MASK ) == PIX_GIF_GRAYSCALE )
						//Grayscale:
						for( int x = 0; x < xsize; x++ )
						{
						    COLOR pixel = *src;
						    int g = ( red( pixel ) + green( pixel ) + blue( pixel ) ) / 3;
						    screen_line[ x ] = (uchar)g;
						    src++;
						}
					}
					EGifPutLine( gf, screen_line, xsize );    
				    }
				    bmem_free( frame_buf );
				}
			    }
			    else
			    {
				//Just an image. No animation.
				if( EGifPutImageDesc( gf, 0, 0, c->xsize, c->ysize, 0, 0 ) == GIF_ERROR ) break;
				COLORPTR src = (COLORPTR)c->data;
				for( int y = 0; y < c->ysize; y++ )
				{
				    if( ( par1 & PIX_GIF_PALETTE_MASK ) == 0 )
				    {
					if( ( par1 & PIX_GIF_DITHER ) == 0 )
					    for( int x = 0; x < c->xsize; x++ )
					    {
						COLOR pixel = *src;
						int r = red( pixel );
						int g = green( pixel );
						int b = blue( pixel );
						screen_line[ x ] = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
						src++;
					    }
					else
					    for( int x = 0; x < c->xsize; x++ )
					    {
						COLOR pixel = *src;
						int r = red( pixel );
						int g = green( pixel );
						int b = blue( pixel );
						GIF_DITHERING;
						screen_line[ x ] = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
						src++;
					    }					
				    }
				    if( ( par1 & PIX_GIF_PALETTE_MASK ) == PIX_GIF_GRAYSCALE )
					for( int x = 0; x < c->xsize; x++ )
					{
					    COLOR pixel = *src;
					    int g = ( red( pixel ) + green( pixel ) + blue( pixel ) ) / 3;
					    screen_line[ x ] = (uchar)g;
					    src++;
					}
				    EGifPutLine( gf, screen_line, c->xsize );    
				}
			    }
			    
			    FreeMapObject( cmap );
			    
			    rv = 0;

			    break;
			}
			if( gf ) EGifCloseFile( gf );
			bmem_free( pal );
			bmem_free( screen_line );
		    }
		    break;
#endif
		case PIX_FORMAT_WAVE:
		    {
			int freq = 0;
			int bits = 8;
			int channels = 0;
			uint loop_start = 0;
			uint loop_len = 0;
			uint loop_type = 0;
			
			switch( c->type )
			{
			    case PIX_CONTAINER_TYPE_INT8: bits = 8; break;
			    case PIX_CONTAINER_TYPE_INT16: bits = 16; break;
			    case PIX_CONTAINER_TYPE_FLOAT32: bits = 32; break;
			    default: break;
			}
			
			freq = (int)pix_vm_get_container_property_i( cnum, "sample_rate", -1, vm );
			channels = (int)pix_vm_get_container_property_i( cnum, "channels", -1, vm );
			loop_start = (uint)pix_vm_get_container_property_i( cnum, "loop_start", -1, vm );
			loop_len = (uint)pix_vm_get_container_property_i( cnum, "loop_len", -1, vm );
			loop_type = (uint)pix_vm_get_container_property_i( cnum, "loop_type", -1, vm );
			if( freq == 0 ) freq = 44100;
			if( channels == 0 ) channels = 1;

			//WAV header:
			uint sdata_size = (uint)( c->size * g_pix_container_type_sizes[ c->type ] );
			bfs_write( (void*)"RIFF", 1, 4, f );
			uint val;
			val = 4 + 24 + 8 + sdata_size; bfs_write( &val, 1, 4, f );
			bfs_write( (void*)"WAVE", 1, 4, f );
			
			//WAV FORMAT:
			bfs_write( (void*)"fmt ", 1, 4, f );
			val = 16; bfs_write( &val, 1, 4, f );
			val = 1; 
			if( bits == 32 ) val = 3; 
			bfs_write( &val, 1, 2, f ); //format
			val = channels; bfs_write( &val, 1, 2, f ); //channels
			val = freq; bfs_write( &val, 1, 4, f ); //frames per second
			val = freq * channels * (bits/8); bfs_write( &val, 1, 4, f ); //bytes per second
			val = channels * ( bits / 8 ); bfs_write( &val, 1, 2, f ); //block align
			val = bits; bfs_write( &val, 1, 2, f ); //bits
			
			//LOOP info:
			if( loop_type && loop_len )
			{
			    bfs_write( (void*)"smpl", 1, 4, f );
            		    val = ( 9 + 6 ) * 4; bfs_write( &val, 4, 1, f );
	                    uint smpl[ 9 + 6 ];
	                    smpl[ 0 ] = 0; //dwManufacturer
        		    smpl[ 1 ] = 0; //dwProduct
	                    smpl[ 2 ] = (uint)( (uint64)1000000000 / (uint64)freq ); //dwSamplePeriod
        		    smpl[ 3 ] = 60; //dwMIDIUnityNote
	                    smpl[ 4 ] = 0; //dwMIDIPitchFraction
	                    smpl[ 5 ] = 0; //dwSMPTEFormat
        		    smpl[ 6 ] = 0; //dwSMPTEOffset
	                    smpl[ 7 ] = 1; //cSampleLoops
	                    smpl[ 8 ] = 0; //cbSamplerData
	                    smpl[ 9 ] = 0; //SampleLoop.dwIdentifier
	                    uint type = 0;
	                    if( loop_type == 2 ) type = 1;
        		    smpl[ 10 ] = type; //SampleLoop.dwType
            		    smpl[ 11 ] = loop_start; //SampleLoop.dwStart
	            	    smpl[ 12 ] = loop_start + loop_len - 1; //SampleLoop.dwEnd
	                    smpl[ 13 ] = 0; //SampleLoop.dwFraction
	                    smpl[ 14 ] = 0; //SampleLoop.dwPlayCount
	                    bfs_write( smpl, 1, sizeof( smpl ), f );
			}

			//WAV DATA:
			bfs_write( (void*)"data", 1, 4, f );
			bfs_write( &sdata_size, 1, 4, f );
			if( bits == 8 )
			{
			    signed char* sdata = (signed char*)c->data;
			    for( uint s = 0; s < sdata_size; s++ )
			    {
				int v = sdata[ s ] + 128;
				bfs_putc( v, f );
			    }
			}
			else
			{
			    bfs_write( c->data, 1, sdata_size, f );
			}
			
			rv = 0;
		    }
		    break;
		case PIX_FORMAT_PIXICONTAINER:
		    {
                        while( 1 )
                        {
                    	    const utf8_char* str;
                    	    size_t len;
                    	    uint64 v;
                    	    
                    	    if( pix_vm_save_chunk( 0, "pixiCONT", f ) ) break;

                    	    if( pix_vm_save_chunk( 1, "colorlen", f ) ) break;
                	    if( bfs_putc( COLORLEN, f ) == EOF ) break;

                    	    if( pix_vm_save_chunk( 1, "intlen", f ) ) break;
                	    if( bfs_putc( sizeof( PIX_INT ), f ) == EOF ) break;

                    	    if( pix_vm_save_chunk( 1, "floatlen", f ) ) break;
                	    if( bfs_putc( sizeof( PIX_FLOAT ), f ) == EOF ) break;

                    	    if( pix_vm_save_chunk( 1, "type", f ) ) break;
                	    bfs_putc( c->type, f );

                    	    if( pix_vm_save_chunk( sizeof( c->flags ), "flags", f ) ) break;
                	    if( bfs_write( &c->flags, 1, sizeof( c->flags ), f ) != sizeof( c->flags ) ) break;

                    	    if( pix_vm_save_chunk( sizeof( c->xsize ), "xsize", f ) ) break;
                	    if( bfs_write( &c->xsize, 1, sizeof( c->xsize ), f ) != sizeof( c->xsize ) ) break;

                    	    if( pix_vm_save_chunk( sizeof( c->ysize ), "ysize", f ) ) break;
                	    if( bfs_write( &c->ysize, 1, sizeof( c->ysize ), f ) != sizeof( c->ysize ) ) break;

			    if( c->data )
			    {
				size_t size = c->size * g_pix_container_type_sizes[ c->type ];
                    		if( pix_vm_save_chunk( size, "data", f ) ) break;
                		if( bfs_write( c->data, 1, size, f ) != size ) break;
                	    }

                    	    if( pix_vm_save_chunk( 3, "key color", f ) ) break;
                	    if( bfs_putc( red( c->key ), f ) == EOF ) break;
                	    if( bfs_putc( green( c->key ), f ) == EOF ) break;
                	    if( bfs_putc( blue( c->key ), f ) == EOF ) break;

                    	    if( pix_vm_save_chunk( 0, "create", f ) ) break;
                	    
                	    if( c->opt_data )
                	    {
                		pix_sym* props = pix_symtab_get_list( &c->opt_data->props );
                		if( props )
                		{
                		    //Save properties:
                		    bool err = 0;
                		    size_t size = bmem_get_size( props ) / sizeof( pix_sym );
                		    for( size_t p = 0; p < size; p++ )
                		    {
                			pix_sym* sym = &props[ p ];
                			if( bfs_putc( 'P', f ) == EOF ) { err = 1; break; }
                			if( pix_vm_save_chunk( 1 + sizeof( PIX_VAL ), sym->name, f ) ) { err = 1; break; }
                			int type = 0;
                			if( sym->type == SYMTYPE_NUM_F ) type = 1;
                			if( bfs_putc( type, f ) == EOF ) { err = 1; break; }
                			if( type == 0 )
                			    for( int i = sizeof( PIX_INT ); i < sizeof( PIX_VAL ); i++ ) ((uchar*)&sym->val)[ i ] = 0;
                			else
                			    for( int i = sizeof( PIX_FLOAT ); i < sizeof( PIX_VAL ); i++ ) ((uchar*)&sym->val)[ i ] = 0;
                			if( bfs_write( &sym->val, 1, sizeof( PIX_VAL ), f ) != sizeof( PIX_VAL ) ) { err = 1; break; }
                		    }
                		    bmem_free( props );
                		    if( err ) break;
                		}
                		
                		size_t hdata_size = pix_vm_get_container_hdata_size( cnum, vm );
                		if( hdata_size )
                		{
                		    //Save hidden data:
                		    if( pix_vm_save_chunk( hdata_size, "hdata", f ) ) break;
                		    if( pix_vm_save_container_hdata( cnum, f, vm ) != hdata_size ) break;
                		}
                	    }
                        
                    	    rv = 0;
                    	    break;
                        }
		    }
		default:
		    break;
	    }
	}
    }
    
    if( filename && f ) 
    {
	bfs_close( f );
	if( rv != 0 ) bfs_remove( filename );
    }
    
    return rv;
}
