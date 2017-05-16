/*
    pixilang_vm_audio.cpp
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

#include <errno.h>

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
    #define DPRINT( fmt, ARGS... ) blog( fmt, ## ARGS )
#else
    #define DPRINT( fmt, ARGS... ) {}
#endif

static void pix_vm_fill_input_buffer( sound_struct* ss, sound_slot* slot, pix_vm* vm, int chnum, int frames )
{
    int src_offset = 0;
    int src_step = ( slot->frames << 15 ) / frames;
    switch( vm->audio_format )
    {
        default: break;
	case PIX_CONTAINER_TYPE_INT8:
	    {
		signed char* dest_ptr = (signed char*)vm->audio_input_buffers[ chnum ];
		switch( ss->in_type )
		{
		    case sound_buffer_int16:
			{
			    signed short* src_ptr = (signed short*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] >> 8;
				src_offset += src_step;
			    }
			}
			break;
		    case sound_buffer_float32:
			{
			    float* src_ptr = (float*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				float v = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] * 128;
				if( v > 127 ) v = 127;
				if( v < -127 ) v = -127;
				dest_ptr[ i ] = v;
				src_offset += src_step;
			    }
			}
			break;
		}
	    }
	    break;
	case PIX_CONTAINER_TYPE_INT16:
	    {
		signed short* dest_ptr = (signed short*)vm->audio_input_buffers[ chnum ];
		switch( ss->in_type )
		{
		    case sound_buffer_int16:
			{
			    signed short* src_ptr = (signed short*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ];
				src_offset += src_step;
			    }
			}
			break;
		    case sound_buffer_float32:
			{
			    float* src_ptr = (float*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				float v = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] * 32768;
				if( v > 32767 ) v = 32767;
				if( v < -32767 ) v = -32767;
				dest_ptr[ i ] = v;
				src_offset += src_step;
			    }
			}
			break;
		}
	    }
	    break;
	case PIX_CONTAINER_TYPE_INT32:
	    {
		int* dest_ptr = (int*)vm->audio_input_buffers[ chnum ];
		switch( ss->in_type )
		{
		    case sound_buffer_int16:
			{
			    signed short* src_ptr = (signed short*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ];
				src_offset += src_step;
			    }
			}
			break;
		    case sound_buffer_float32:
			{
			    float* src_ptr = (float*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				float v = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] * 32768;
				if( v > 32767 ) v = 32767;
				if( v < -32767 ) v = -32767;
				dest_ptr[ i ] = v;
				src_offset += src_step;
			    }
			}
			break;
		}
	    }
	    break;
	case PIX_CONTAINER_TYPE_FLOAT32:
	    {
		float* dest_ptr = (float*)vm->audio_input_buffers[ chnum ];
		switch( ss->in_type )
		{
		    case sound_buffer_int16:
			{
			    signed short* src_ptr = (signed short*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = (float)src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] / 32768;
				src_offset += src_step;
			    }
			}
			break;
		    case sound_buffer_float32:
			{
			    float* src_ptr = (float*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ];
				src_offset += src_step;
			    }
			}
			break;
		}
	    }
	    break;
#ifdef PIX_FLOAT64_ENABLED
	case PIX_CONTAINER_TYPE_FLOAT64:
	    {
		double* dest_ptr = (double*)vm->audio_input_buffers[ chnum ];
		switch( ss->in_type )
		{
		    case sound_buffer_int16:
			{
			    signed short* src_ptr = (signed short*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = (double)src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] / 32768;
				src_offset += src_step;
			    }
			}
			break;
		    case sound_buffer_float32:
			{
			    float* src_ptr = (float*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ];
				src_offset += src_step;
			    }
			}
			break;
		}
	    }
	    break;
#endif
    }
}

static int pix_vm_render_piece_of_sound( sound_struct* ss, int slot_num )
{
    int handled = 0;
    
    if( ss )
    {
	sound_slot* slot = &ss->slots[ slot_num ];
	int frames = slot->frames;
	
	pix_vm* vm = (pix_vm*)slot->user_data;
	if( vm->audio_callback < 0 ) return 0;

	uint freq_ratio = ( vm->audio_freq << PIX_AUDIO_PTR_PREC ) / ss->freq;
	
	vm->audio_src_ptr += frames * freq_ratio;
	uint interpolation_frames = 1;
	uint src_frames = ( ( ( vm->audio_src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 + interpolation_frames ) - vm->audio_src_rendered ) & ((1<<(32-PIX_AUDIO_PTR_PREC))-1);
	vm->audio_src_rendered += src_frames;
	
	if( src_frames ) 
	{
	    //Need a new piece of sound:
	    uint src_buffer_offset;
	    if( vm->audio_src_buffers[ 0 ] == 0 )
	    {
		//Buffers are empty. Create it:
		for( int i = 0; i < vm->audio_channels; i++ )
		{
		    bmem_free( vm->audio_src_buffers[ i ] );
		    vm->audio_src_buffers[ i ] = bmem_new( g_pix_container_type_sizes[ vm->audio_format ] * src_frames );
		}
		vm->audio_src_buffer_size = src_frames;
		vm->audio_src_buffer_ptr = 0;
		src_buffer_offset = 0;
	    }
	    else
	    {
		//Buffer already has some data. We need to save last 16 frames from it - for interpolation:
		if( vm->audio_src_buffer_size > 16 )
		{
		    for( int i = 0; i < vm->audio_channels; i++ )
			bmem_copy( vm->audio_src_buffers[ i ], (char*)vm->audio_src_buffers[ i ] + ( vm->audio_src_buffer_size - 16 ) * g_pix_container_type_sizes[ vm->audio_format ], 16 * g_pix_container_type_sizes[ vm->audio_format ] );
		    vm->audio_src_buffer_ptr -= ( vm->audio_src_buffer_size - 16 ) << PIX_AUDIO_PTR_PREC;
		    vm->audio_src_buffer_size = 16;
		}
		src_buffer_offset = vm->audio_src_buffer_size;
		vm->audio_src_buffer_size += src_frames;
		size_t new_buffer_bytes = vm->audio_src_buffer_size * g_pix_container_type_sizes[ vm->audio_format ];
		if( new_buffer_bytes > bmem_get_size( vm->audio_src_buffers[ 0 ] ) )
		    for( int i = 0; i < vm->audio_channels; i++ )
			vm->audio_src_buffers[ i ] = bmem_resize( vm->audio_src_buffers[ i ], new_buffer_bytes );
	    }
	    //Render a new piece of sound:
	    {
		//Output:
		pix_vm_container* c = vm->c[ vm->audio_channels_cont ];
		int* cdata = (int*)c->data;
		for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
		{
		    if( i < vm->audio_channels )
			cdata[ i ] = vm->audio_buffers_conts[ i ];
		    else
			cdata[ i ] = -1;
		}
		for( int i = 0; i < vm->audio_channels; i++ )
		{
		    c = vm->c[ vm->audio_buffers_conts[ i ] ];
		    c->type = vm->audio_format;
		    c->data = (void*)( (char*)vm->audio_src_buffers[ i ] + src_buffer_offset * g_pix_container_type_sizes[ vm->audio_format ] );
		    c->xsize = src_frames;
		    c->size = src_frames;
		}
		
		//Input:
		if( vm->audio_input_enabled && slot->in_buffer )
		{
		    c = vm->c[ vm->audio_input_channels_cont ];
		    cdata = (int*)c->data;
		    for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
		    {
			if( i < vm->audio_channels )
			    cdata[ i ] = vm->audio_input_buffers_conts[ i ];
			else
			    cdata[ i ] = -1;
		    }
		    for( int i = 0; i < vm->audio_channels; i++ )
		    {
			if( bmem_get_size( vm->audio_input_buffers[ i ] ) < src_frames * g_pix_container_type_sizes[ vm->audio_format ] )
			    vm->audio_input_buffers[ i ] = bmem_resize( vm->audio_input_buffers[ i ], src_frames * g_pix_container_type_sizes[ vm->audio_format ] );
			c = vm->c[ vm->audio_input_buffers_conts[ i ] ];
			c->type = vm->audio_format;
			if( i < ss->in_channels )
			{
			    if( bmem_get_size( vm->audio_input_buffers[ i ] ) < src_frames * g_pix_container_type_sizes[ vm->audio_format ] )
				vm->audio_input_buffers[ i ] = bmem_resize( vm->audio_input_buffers[ i ], src_frames * g_pix_container_type_sizes[ vm->audio_format ] );
			    c->data = vm->audio_input_buffers[ i ];
			    pix_vm_fill_input_buffer( ss, slot, vm, i, src_frames );
			}
			else
			{
			    c->data = vm->audio_input_buffers[ 0 ];
			}
			c->xsize = src_frames;
			c->size = src_frames;
		    }
		}
	    }
	    pix_vm_function fun;
	    PIX_VAL pp[ 7 ];
	    char pp_types[ 7 ];
	    fun.p = pp;
	    fun.p_types = pp_types;
	    fun.addr = vm->audio_callback;
	    fun.p[ 0 ].i = 0;
	    fun.p_types[ 0 ] = 0;
	    fun.p[ 1 ] = vm->audio_userdata;
	    fun.p_types[ 1 ] = vm->audio_userdata_type;
	    fun.p[ 2 ].i = vm->audio_channels_cont;
	    fun.p_types[ 2 ] = 0;
	    fun.p[ 3 ].i = src_frames;
	    fun.p_types[ 3 ] = 0;
	    fun.p[ 4 ].i = slot->time;
	    fun.p_types[ 4 ] = 0;
	    if( vm->audio_input_enabled && slot->in_buffer )
		fun.p[ 5 ].i = vm->audio_input_channels_cont;
	    else
		fun.p[ 5 ].i = -1;
	    fun.p_types[ 5 ] = 0;
	    fun.p[ 6 ].i = ( ss->out_latency * freq_ratio ) >> PIX_AUDIO_PTR_PREC;
	    fun.p_types[ 6 ] = 0;
	    fun.p_num = 7;
	    pix_vm_run( PIX_VM_THREADS - 1, 0, &fun, PIX_VM_CALL_FUNCTION, vm );
	    PIX_VAL retval;
	    char retval_type;
	    pix_vm_get_thread_retval( PIX_VM_THREADS - 1, vm, &retval, &retval_type );
	    if( retval_type == 0 )
		handled = retval.i;
	    else 
		handled = retval.f;
	    {
		for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
		{
		    pix_vm_container* c = vm->c[ vm->audio_buffers_conts[ i ] ];
		    c->data = 0;
		    c->xsize = 0;
		    c->size = 0;
		}
	    }
	}
	
	//Final interpolation:
	if( handled )
	{
	    uint src_ptr = vm->audio_src_buffer_ptr;
	    if( ss->out_type == sound_buffer_int16 )
	    {
		signed short* dest = (signed short*)slot->buffer;
		switch( vm->audio_format )
		{
		    case PIX_CONTAINER_TYPE_INT8:
			{
			    signed char* src1 = (signed char*)vm->audio_src_buffers[ 0 ];
			    signed char* src2 = (signed char*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int c1 = src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 );
				    int c2 = ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1;
				    int v1 = ( (int)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    int v2 = ( (int)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    dest[ i * 2 + 0 ] = (signed short)v1 << 8;
				    dest[ i * 2 + 1 ] = (signed short)v2 << 8;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = (signed short)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] << 8;
				    dest[ i * 2 + 1 ] = (signed short)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] << 8;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT16:
			{
			    signed short* src1 = (signed short*)vm->audio_src_buffers[ 0 ];
			    signed short* src2 = (signed short*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int c1 = src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 );
				    int c2 = ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1;
				    int v1 = ( (int)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    int v2 = ( (int)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    dest[ i * 2 + 0 ] = (signed short)v1;
				    dest[ i * 2 + 1 ] = (signed short)v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    dest[ i * 2 + 1 ] = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT32:
			{
			    signed int* src1 = (signed int*)vm->audio_src_buffers[ 0 ];
			    signed int* src2 = (signed int*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int c1 = src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 );
				    int c2 = ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1;
				    int v_1 = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    if( v_1 > 32767 ) v_1 = 32767;
				    if( v_1 < -32767 ) v_1 = -32767;
				    int v_2 = src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ];
				    if( v_2 > 32767 ) v_2 = 32767;
				    if( v_2 < -32767 ) v_2 = -32767;
				    int v1 = ( v_1 * c2 + v_2 * c1 ) >> PIX_AUDIO_PTR_PREC;
				    v_1 = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    if( v_1 > 32767 ) v_1 = 32767;
				    if( v_1 < -32767 ) v_1 = -32767;
				    v_2 = src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ];
				    if( v_2 > 32767 ) v_2 = 32767;
				    if( v_2 < -32767 ) v_2 = -32767;
				    int v2 = ( v_1 * c2 + v_2 * c1 ) >> PIX_AUDIO_PTR_PREC;
				    dest[ i * 2 + 0 ] = (signed short)v1;
				    dest[ i * 2 + 1 ] = (signed short)v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int v = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 0 ] = (signed short)v;
				    v = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 1 ] = (signed short)v;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_FLOAT32:
			{
			    float* src1 = (float*)vm->audio_src_buffers[ 0 ];
			    float* src2 = (float*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    float c1 = (float)( src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) ) / (float)( 1 << PIX_AUDIO_PTR_PREC );
				    float c2 = 1 - c1;
				    float v1 = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    float v2 = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    v1 *= 32768.0;
				    v2 *= 32768.0;
				    if( v1 > 32767 ) v1 = 32767;
				    if( v1 < -32767 ) v1 = -32767;
				    if( v2 > 32767 ) v2 = 32767;
				    if( v2 < -32767 ) v2 = -32767;
				    dest[ i * 2 + 0 ] = (signed short)v1;
				    dest[ i * 2 + 1 ] = (signed short)v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int v = (int)( src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * 32767.0F );
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 0 ] = (signed short)v;
				    v = (int)( src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * 32767.0F );
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 1 ] = (signed short)v;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64:
			{
			    double* src1 = (double*)vm->audio_src_buffers[ 0 ];
			    double* src2 = (double*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    double c1 = (double)( src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) ) / (double)( 1 << PIX_AUDIO_PTR_PREC );
				    double c2 = 1 - c1;
				    double v1 = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    double v2 = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    v1 *= 32768.0;
				    v2 *= 32768.0;
				    if( v1 > 32767 ) v1 = 32767;
				    if( v1 < -32767 ) v1 = -32767;
				    if( v2 > 32767 ) v2 = 32767;
				    if( v2 < -32767 ) v2 = -32767;
				    dest[ i * 2 + 0 ] = (signed short)v1;
				    dest[ i * 2 + 1 ] = (signed short)v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int v = (int)( src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * 32767.0 );
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 0 ] = (signed short)v;
				    v = (int)( src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * 32767.0 );
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 1 ] = (signed short)v;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
#endif
		    default:
			break;
		}
	    }
	    if( ss->out_type == sound_buffer_float32 )
	    {
		float* dest = (float*)slot->buffer;
		switch( vm->audio_format )
		{
		    case PIX_CONTAINER_TYPE_INT8:
			{
			    signed char* src1 = (signed char*)vm->audio_src_buffers[ 0 ];
			    signed char* src2 = (signed char*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int c1 = src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 );
				    int c2 = ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1;
				    int v1 = ( (int)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    int v2 = ( (int)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    dest[ i * 2 + 0 ] = (float)v1 / 128.0F;
				    dest[ i * 2 + 1 ] = (float)v2 / 128.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = (float)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 128.0F;
				    dest[ i * 2 + 1 ] = (float)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 128.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT16:
			{
			    signed short* src1 = (signed short*)vm->audio_src_buffers[ 0 ];
			    signed short* src2 = (signed short*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int c1 = src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 );
				    int c2 = ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1;
				    int v1 = ( (int)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    int v2 = ( (int)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    dest[ i * 2 + 0 ] = (float)v1 / 32768.0F;
				    dest[ i * 2 + 1 ] = (float)v2 / 32768.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = (float)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 32768.0F;
				    dest[ i * 2 + 1 ] = (float)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 32768.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT32:
			{
			    signed int* src1 = (signed int*)vm->audio_src_buffers[ 0 ];
			    signed int* src2 = (signed int*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    float c1 = (float)( src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) ) / (float)( 1 << PIX_AUDIO_PTR_PREC );
				    float c2 = (float)( ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1 ) / (float)( 1 << PIX_AUDIO_PTR_PREC );
				    float v1 = (float)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (float)src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    float v2 = (float)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (float)src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    dest[ i * 2 + 0 ] = v1 / 32768.0F;
				    dest[ i * 2 + 1 ] = v2 / 32768.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = (float)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 32768.0F;
				    dest[ i * 2 + 1 ] = (float)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 32768.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_FLOAT32:
			{
			    float* src1 = (float*)vm->audio_src_buffers[ 0 ];
			    float* src2 = (float*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    float c1 = (float)( src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) ) / (float)( 1 << PIX_AUDIO_PTR_PREC );
				    float c2 = 1 - c1;
				    float v1 = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    float v2 = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    dest[ i * 2 + 0 ] = v1;
				    dest[ i * 2 + 1 ] = v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    dest[ i * 2 + 1 ] = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64:
			{
			    double* src1 = (double*)vm->audio_src_buffers[ 0 ];
			    double* src2 = (double*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    double c1 = (double)( src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) ) / (double)( 1 << PIX_AUDIO_PTR_PREC );
				    double c2 = 1 - c1;
				    double v1 = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    double v2 = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    dest[ i * 2 + 0 ] = v1;
				    dest[ i * 2 + 1 ] = v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    dest[ i * 2 + 1 ] = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
#endif
		    default:
			break;
		}
	    }
	}
	else
	{
	    uint src_ptr = vm->audio_src_buffer_ptr;
	    src_ptr += freq_ratio * frames;
	    vm->audio_src_buffer_ptr = src_ptr;
	}
    }
    
    return handled;
}

int pix_vm_set_audio_callback( PIX_ADDR callback, PIX_VAL userdata, char userdata_type, uint freq, pix_container_type format, int channels, uint flags, pix_vm* vm )
{
    bool play = 0;
    
    if( callback == -1 && vm->audio_callback != -1 )
    {
	//Close SunDog sound stream:
	sound_stream_deinit();
    }
    
    if( callback != -1 && vm->audio_callback == -1 )
    {
	//Open SunDog sound stream:
	if( sound_stream_init( sound_buffer_default, -1, -1, 0 ) )
	{
	    profile_remove_key( KEY_AUDIODRIVER, 0 );
    	    profile_remove_key( KEY_AUDIODEVICE, 0 );
    	    profile_remove_key( KEY_AUDIODEVICE_IN, 0 );
    	    profile_remove_key( KEY_SOUNDBUFFER, 0 );
    	    profile_remove_key( KEY_FREQ, 0 );
    	    profile_save( 0 );
#ifdef IPHONE
	    blog_show_error_report();
#endif
	    return -1;
	}
	play = 1;
    }
    
    //Set Pixilang audio parameters:
    sound_stream_lock();
    vm->audio_callback = callback;
    vm->audio_userdata = userdata;
    vm->audio_userdata_type = userdata_type;
    vm->audio_freq = freq;
    vm->audio_format = format;
    vm->audio_channels = channels;
    vm->audio_flags = flags;
    vm->audio_src_ptr = 0;
    vm->audio_src_rendered = 0;
    vm->audio_src_buffer_size = 0;
    vm->audio_src_buffer_ptr = 0;
    for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
    {
	if( vm->audio_src_buffers[ i ] )
	{
	    bmem_free( vm->audio_src_buffers[ i ] );
	    vm->audio_src_buffers[ i ] = 0;
	}
	if( vm->audio_input_buffers[ i ] )
	{
	    bmem_free( vm->audio_input_buffers[ i ] );
	    vm->audio_input_buffers[ i ] = 0;
	}
    }
    if( vm->audio_channels_cont == -1 )
    {
        //Create audio channels (container):
        vm->audio_channels_cont = pix_vm_new_container( -1, PIX_VM_AUDIO_CHANNELS, 1, PIX_CONTAINER_TYPE_INT32, 0, vm );
        pix_vm_set_container_flags( vm->audio_channels_cont, pix_vm_get_container_flags( vm->audio_channels_cont, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        vm->audio_input_channels_cont = pix_vm_new_container( -1, PIX_VM_AUDIO_CHANNELS, 1, PIX_CONTAINER_TYPE_INT32, 0, vm );
        pix_vm_set_container_flags( vm->audio_input_channels_cont, pix_vm_get_container_flags( vm->audio_input_channels_cont, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        //Create audio buffers (containers):
        for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
        {
    	    vm->audio_buffers_conts[ i ] = pix_vm_new_container( -1, 1, 1, PIX_CONTAINER_TYPE_INT32, &vm->audio_channels_cont, vm );
    	    vm->audio_input_buffers_conts[ i ] = pix_vm_new_container( -1, 1, 1, PIX_CONTAINER_TYPE_INT32, &vm->audio_input_channels_cont, vm );
    	    pix_vm_set_container_flags( vm->audio_buffers_conts[ i ], pix_vm_get_container_flags( vm->audio_buffers_conts[ i ], vm ) | PIX_CONTAINER_FLAG_STATIC_DATA | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
    	    pix_vm_set_container_flags( vm->audio_input_buffers_conts[ i ], pix_vm_get_container_flags( vm->audio_input_buffers_conts[ i ], vm ) | PIX_CONTAINER_FLAG_STATIC_DATA | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
	}
    }
    sound_stream_unlock();

    if( play )
    {
	//Play:
	sound_stream_stop( 0 );
    	sound_stream_set_slot_callback( 0, (void*)&pix_vm_render_piece_of_sound, vm );
	sound_stream_play( 0 );
    }
    
    return 0;
}
