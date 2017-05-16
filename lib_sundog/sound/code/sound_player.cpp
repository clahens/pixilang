/*
    sound_player.cpp. Main sound playing function
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

#ifndef NOSOUND

#include "core/core.h"
#include "../sound.h"

int main_sound_output_callback( sound_struct* ss, uint flags )
{
    bool not_filled = 1;
    bool silence = 1;

    int frame_size = g_sample_size[ ss->out_type ] * ss->out_channels;
    
    if( bmutex_lock( &ss->mutex ) ) goto mutex_end;

    for( int slot_num = 0; slot_num < SOUND_SLOTS; slot_num++ )
    {
        sound_slot* slot = &ss->slots[ slot_num ];
        if( slot->event_answer != slot->event )
        {
    	    slot->event_answer = slot->event;
	    switch( slot->event_answer >> 16 )
	    {
	        case 1: //Play:
	    	    slot->status = 1;
	    	    break;
		case 2: //Stop:
		    slot->status = 0;
		    break;
	    }
	}
	if( slot->callback && slot->status )
	{
	    int (*callback)( sound_struct*, int ) = ( int (*)( sound_struct*, int ) )slot->callback;
	    if( not_filled )
	    {
		if( ss->in_enabled )
		    slot->in_buffer = ss->in_buffer;
		else
		    slot->in_buffer = 0;
		slot->buffer = ss->out_buffer;
		slot->frames = ss->out_frames;
		slot->time = ss->out_time;
		int r = callback( ss, slot_num );
		// r == 0 : silence, buffer is not filled;
		// r == 1 : buffer is filled;
		// r == 2 : silence, buffer is filled;
		if( r ) not_filled = 0;
		if( r == 1 ) silence = 0;
	    }
	    else
	    {
		int ptr = 0;
		while( 1 )
		{
		    int size = ss->out_frames - ptr;
		    if( size > ss->slot_buffer_size )
			size = ss->slot_buffer_size;
		    if( ss->in_enabled )
		    {
			if( ss->in_type == sound_buffer_int16 )
			    slot->in_buffer = (char*)ss->in_buffer + ptr * ss->in_channels * 2;
			if( ss->in_type == sound_buffer_float32 )
			    slot->in_buffer = (char*)ss->in_buffer + ptr * ss->in_channels * 4;
		    }
		    else 
		    {
			slot->in_buffer = 0;
		    }
		    slot->buffer = ss->slot_buffer;
		    slot->frames = size;
		    slot->time = ss->out_time;
		    int r = callback( ss, slot_num );
		    if( r == 1 ) silence = 0;
		    if( r )
		    {
			//Add result to the main buffer:
			if( ss->out_type == sound_buffer_int16 )
			{
			    signed short* dest = (signed short*)ss->out_buffer;
			    signed short* src = (signed short*)ss->slot_buffer;
			    int add = ptr * ss->out_channels;
			    for( int i = 0; i < size * ss->out_channels; i++ )
			    {
				int v = (int)dest[ i + add ] + src[ i ];
				if( v > 32767 ) v = 32767;
				if( v < -32767 ) v = -32767;
				dest[ i + add ] = (signed short)v;
			    }
			}
			else
			if( ss->out_type == sound_buffer_float32 )
			{
			    float* dest = (float*)ss->out_buffer;
			    float* src = (float*)ss->slot_buffer;
			    int add = ptr * ss->out_channels;
			    for( int i = 0; i < size * ss->out_channels; i++ )
			    {
				dest[ i + add ] += src[ i ];
			    }
			}
		    }
		    ptr += size;
		    if( ptr >= ss->out_frames )
			break;
		}
	    }	    
	}
    }

    bmutex_unlock( &ss->mutex );
    
mutex_end:
    
    if( not_filled )
    {
	bmem_set( ss->out_buffer, ss->out_frames * frame_size, 0 );
    }
    
    if( ss->out_file )
    {
	size_t size = ss->out_frames * frame_size;
	size_t buf_size = bmem_get_size( ss->out_file_buf );
	size_t src_ptr = 0;
	while( size )
	{
	    size_t avail = buf_size - ss->out_file_buf_wp;
	    if( avail > size )
		avail = size;
    	    bmem_copy( ss->out_file_buf + ss->out_file_buf_wp, (uchar*)ss->out_buffer + src_ptr, avail );
	    size -= avail;
	    src_ptr += avail;
	    volatile size_t new_wp = ( ss->out_file_buf_wp + avail ) & ( buf_size - 1 );
	    COMPILER_MEMORY_BARRIER();
	    ss->out_file_buf_wp = new_wp;
	}	
    }
    
    if( silence )
	return 0;
    else
	return 1;
}

#endif
