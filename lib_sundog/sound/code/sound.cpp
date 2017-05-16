/*
    SunDog: sound.cpp. Sound and MIDI in/out
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

#include "core/core.h"
#include "../sound.h"

sound_struct g_snd; //Main sound structure;
volatile bool g_snd_initialized = 0;

int g_sample_size[ sound_buffer_max ] = 
{
    0,
    sizeof( int16 ),
    sizeof( float )
};

volatile int g_snd_play_request = 0;
volatile int g_snd_stop_request = 0;
volatile int g_snd_rewind_request = 0;
volatile bool g_snd_play_status = 0;

#ifndef NOMIDI
    ticks_hr_t g_last_midi_in_activity = 0;
    ticks_hr_t g_last_midi_out_activity = 0;
    bmutex g_sundog_midi_mutex;
    int g_sundog_midi_open_counter = 0;
#endif

//#define SHOW_DEBUG_MESSAGES

#ifndef NOSOUND

#include "sound_common.h"

#ifdef LINUX
#ifdef ANDROID
    #include "sound_android.h"
#else
    #include "sound_linux.h"
#endif
#endif

#ifdef OSX
    #include "sound_osx.h"
#endif

#ifdef IPHONE
    #include "sound_iphone.h"
#endif

#if defined(WIN) || defined(WINCE)
    #include "sound_win.h"
    #include "sound_win_mmsystem_midi.h"
#endif

#endif

//
// Sound Stream
//

void user_controlled_audio_callback( void* buffer, int frames, int latency, ticks_hr_t output_time )
{
    g_snd.out_buffer = buffer;
    g_snd.out_frames = frames;
    g_snd.out_time = output_time;
    g_snd.out_latency = latency;
    main_sound_output_callback( &g_snd, 0 );
}

int sound_stream_init( sound_buffer_type type, int freq, int channels, uint flags )
{
#ifndef NOSOUND

    if( g_snd_initialized ) return 0;

    if( type == sound_buffer_default )
    {
	//Default mode:
	type = sound_buffer_int16;
#ifdef OSX
	type = sound_buffer_float32;
#endif
    }
    if( freq < 0 )
    {
	//Default frequency:
	freq = profile_get_int_value( KEY_FREQ, 44100, 0 );
    }
    if( channels < 0 )
    {
	//Default number of channels:
	channels = 2;
    }
    if( freq < 44100 )
    {
	blog( "WARNING. Wrong sampling frequency %d (must be >= 44100). Using 44100.\n", freq );
	freq = 44100;
    }
#ifdef ONLY44100
    if( freq != 44100 ) 
    {
	blog( "WARNING. Sampling frequency must be 44100 for this device. Using 44100.\n" );
	freq = 44100;
    }
#endif
    
    bmem_set( &g_snd, sizeof( g_snd ), 0 );
    g_snd.out_type = type;
    g_snd.in_type = type;
    g_snd.flags = flags;
    g_snd.freq = freq;
    g_snd.out_channels = channels;
    g_snd.in_channels = channels;

    bmutex_init( &g_snd.mutex, 0 );
    bmutex_init( &g_snd.in_mutex, 0 );

    int res;

    if( flags & SOUND_STREAM_FLAG_USER_CONTROLLED )
    {
    }
    else
    {
	res = device_sound_stream_init();
	if( res )
	    return res;
    }

    int frame = 1;
    if( g_snd.out_type == sound_buffer_int16 ) frame = 2 * g_snd.out_channels;
    if( g_snd.out_type == sound_buffer_float32 ) frame = 4 * g_snd.out_channels;
    g_snd.slot_buffer_size = 1024 * 8;
    g_snd.slot_buffer = bmem_new( g_snd.slot_buffer_size * frame );

    COMPILER_MEMORY_BARRIER();    
    g_snd_initialized = 1;

#endif    
    
    return 0;
}

void sound_stream_set_slot_callback( int slot, void* callback, void* user_data )
{
#ifndef NOSOUND
    
#ifdef SHOW_DEBUG_MESSAGES
    blog( "SET SOUND SLOT CALLBACK %d\n", slot );
#endif
    if( g_snd_initialized == 0 ) return;
    g_snd.slots[ slot ].callback = callback;
    g_snd.slots[ slot ].user_data = user_data;
    
#endif
}

void sound_stream_remove_slot_callback( int slot )
{
#ifndef NOSOUND

#ifdef SHOW_DEBUG_MESSAGES
    blog( "REMOVE SOUND SLOT CALLBACK %d\n", slot );
#endif
    if( g_snd_initialized == 0 ) return;
    sound_stream_stop( slot );
    g_snd.slots[ slot ].callback = 0;
    
#endif
}

void sound_stream_lock( void )
{
#ifndef NOSOUND     

#ifdef SHOW_DEBUG_MESSAGES
    blog( "LOCK\n" );
#endif
    if( g_snd_initialized == 0 ) return;
    bmutex_lock( &g_snd.mutex );
#ifdef SHOW_DEBUG_MESSAGES
    blog( "LOCK OK\n" );
#endif

#endif
}

void sound_stream_unlock( void )
{
#ifndef NOSOUND 
    
#ifdef SHOW_DEBUG_MESSAGES
    blog( "UNLOCK\n" );
#endif
    if( g_snd_initialized == 0 ) return;
    bmutex_unlock( &g_snd.mutex );
#ifdef SHOW_DEBUG_MESSAGES
    blog( "UNLOCK OK\n" );
#endif

#endif
}

void sound_stream_play( int slot )
{
#ifndef NOSOUND 
    
#ifdef SHOW_DEBUG_MESSAGES
    blog( "PLAY %d\n", slot );
#endif
    if( g_snd_initialized == 0 ) return;
    if( g_snd.slots[ slot ].callback )
    {
	if( g_snd.slots[ slot ].status != 1 )
	{
	    if( g_snd.flags & SOUND_STREAM_FLAG_ONE_THREAD )
		g_snd.slots[ slot ].status = 1;
	    else
	    {
		volatile int event = g_snd.slots[ slot ].event;
		event++;
		event &= 0x0FFFF;
		event += 0x10000;
		g_snd.slots[ slot ].event = event;
		while( g_snd.slots[ slot ].event_answer != event ) { HANDLE_THREAD_EVENTS; time_sleep( 5 ); }
	    }
	}
    }
#ifdef SHOW_DEBUG_MESSAGES
    blog( "PLAY OK %d\n", slot );
#endif
    
#endif
}

void sound_stream_stop( int slot )
{
#ifndef NOSOUND 
    
#ifdef SHOW_DEBUG_MESSAGES
    blog( "STOP %d\n", slot );
#endif
    if( g_snd_initialized == 0 ) return;
    if( g_snd.slots[ slot ].callback )
    {
	if( g_snd.slots[ slot ].status != 0 )
	{
	    if( g_snd.flags & SOUND_STREAM_FLAG_ONE_THREAD )
		g_snd.slots[ slot ].status = 0;
	    else
	    {
		volatile int event = g_snd.slots[ slot ].event;
		event++;
		event &= 0x0FFFF;
		event += 0x20000;
		g_snd.slots[ slot ].event = event;
		while( g_snd.slots[ slot ].event_answer != event ) { HANDLE_THREAD_EVENTS; time_sleep( 5 ); }
	    }
	}
    }
#ifdef SHOW_DEBUG_MESSAGES
    blog( "STOP OK %d\n", slot );
#endif
    
#endif
}

int sound_stream_device_play( void )
{
#ifndef NOSOUND

    if( g_snd_initialized == 0 ) return 0;
    if( g_snd.flags & SOUND_STREAM_FLAG_USER_CONTROLLED )
    {
    }
    else
    {
	int res = device_sound_stream_init();
	if( res )
	    return res;
    }
    
#endif
    
    return 0;
}

void sound_stream_device_stop( void )
{
#ifndef NOSOUND
    
    if( g_snd_initialized == 0 ) return;    
    if( g_snd.flags & SOUND_STREAM_FLAG_USER_CONTROLLED )
    {
    }
    else
    {
	device_sound_stream_deinit();
    }
    
#endif
}

//Call this function in the main app thread, where the sound stream is not locked.
//Or use sound_stream_input_request()
void sound_stream_input( bool enable )
{
#ifndef NOSOUND
    
    if( g_snd_initialized == 0 ) return;
    if( enable ) 
	g_snd.in_enabled++;
    else
	g_snd.in_enabled--;
    if( g_snd.flags & SOUND_STREAM_FLAG_USER_CONTROLLED )
    {
    }
    else
    {
	if( g_snd.in_enabled == 0 )
	    device_sound_stream_input( 0 );
	if( enable && g_snd.in_enabled == 1 )
	    device_sound_stream_input( 1 );
    }
        
#endif    
}

void sound_stream_input_request( bool enable )
{
#ifndef NOSOUND
    
    if( g_snd_initialized == 0 ) return;
    if( enable ) 
	g_snd.in_request++;
    else
	g_snd.in_request--;
        
#endif    
}

void sound_stream_handle_input_requests( void )
{
#ifndef NOSOUND
    
    if( g_snd_initialized == 0 ) return;
    if( g_snd.in_request_answer < g_snd.in_request )
    {
	if( g_snd.in_request_answer == 0 )
	    sound_stream_input( 1 );
    }
    if( g_snd.in_request_answer > g_snd.in_request )
    {
	if( g_snd.in_request_answer > 0 && g_snd.in_request <= 0 )
	    sound_stream_input( 0 );
    }
    g_snd.in_request_answer = g_snd.in_request;
        
#endif    
}

const utf8_char* sound_stream_get_driver_name( void )
{
#ifndef NOSOUND
    
    if( g_snd_initialized == 0 ) return "";
    if( g_snd.flags & SOUND_STREAM_FLAG_USER_CONTROLLED )
	return "unknown";
    else
	return device_sound_stream_get_driver_name();
    
#else

    return "nosound";
    
#endif
}

const utf8_char* sound_stream_get_driver_info( void )
{
#ifndef NOSOUND
    
    if( g_snd_initialized == 0 ) return "";
    if( g_snd.flags & SOUND_STREAM_FLAG_USER_CONTROLLED )
	return "Unknown (User Controlled)";
    else
	return device_sound_stream_get_driver_info();
    
#else

    return "No sound";
    
#endif
}

int sound_stream_get_drivers( utf8_char*** names, utf8_char*** infos )
{
#ifndef NOSOUND

    return device_sound_stream_get_drivers( names, infos );

#else

    return 0;
    
#endif
}

int sound_stream_get_devices( const utf8_char* driver, utf8_char*** names, utf8_char*** infos, bool input )
{
#ifndef NOSOUND

    return device_sound_stream_get_devices( driver, names, infos, input );

#else

    return 0;
    
#endif
}

void* sound_stream_capture_thread( void* data )
{
    sound_struct* ss = (sound_struct*)data;
    size_t buf_size = bmem_get_size( ss->out_file_buf );
    while( ss->out_file_exit_request == 0 )
    {
	size_t rp = ss->out_file_buf_rp;
	size_t wp = ss->out_file_buf_wp;
	if( rp != wp )
	{
	    size_t size = ( wp - rp ) & ( buf_size - 1 );
	    while( size )
	    {
		size_t avail = buf_size - rp;
		if( avail > size )
            	    avail = size;
            	ss->out_file_size += bfs_write( ss->out_file_buf + rp, 1, avail, ss->out_file );
            	rp = ( rp + avail ) & ( buf_size - 1 );
            	size -= avail;
	    }
	    ss->out_file_buf_rp = rp;
	}
	else
	{
	    time_sleep( 50 );
	}
    }
    ss->out_file_exit_request = 0;
    return 0;
}

int sound_stream_capture_start( const utf8_char* filename )
{
#ifndef NOSOUND

    if( g_snd_initialized == 0 ) return -1;
    if( g_snd.out_file ) return -1;
    int rv = -1;
    
    bfs_file f = bfs_open( filename, "wb" );
    if( f )
    {
        uint bits = 16;
        if( g_snd.out_type == sound_buffer_float32 ) bits = 32;
	
	//WAV HEADER:
	uint n;
	bfs_write( "RIFF", 1, 4, f );
	n = 4 + 24 + 8; bfs_write( &n, 1, 4, f );
        bfs_write( "WAVE", 1, 4, f );
        
        //WAV FORMAT:
        bfs_write( "fmt ", 1, 4, f );
        n = 16; bfs_write( &n, 1, 4, f );
        n = 1; if( g_snd.out_type == sound_buffer_float32 ) n = 3; bfs_write( &n, 1, 2, f ); //format
        n = g_snd.out_channels; bfs_write( &n, 1, 2, f ); //channels
        n = g_snd.freq; bfs_write( &n, 1, 4, f ); //frames per second
        n = g_snd.freq * g_snd.out_channels * ( bits / 8 ); bfs_write( &n, 1, 4, f ); //bytes per second
        n = g_snd.out_channels * ( bits / 8 ); bfs_write( &n, 1, 2, f ); //block align
        bfs_write( &bits, 1, 2, f ); //bits
        
        //WAV DATA:
        bfs_write( "data", 1, 4, f );
        bfs_write( &n, 1, 4, f ); //size

	int frame_size = g_sample_size[ g_snd.out_type ] * g_snd.out_channels;        
        uchar* buf = (uchar*)bmem_new( round_to_power_of_two( 2 * g_snd.freq * frame_size ) );
        	
	sound_stream_lock();
	g_snd.out_file = f;
	g_snd.out_file_size = 0;
	g_snd.out_file_buf = buf;
	g_snd.out_file_buf_wp = 0;
	g_snd.out_file_buf_rp = 0;
        sound_stream_unlock();

        bthread_create( &g_snd.out_file_thread, sound_stream_capture_thread, &g_snd, 0 );
        
        blog( "Audio capturer started.\n" );
	rv = 0;
    }
    else
    {
	blog( "Can't open %s for writing\n", filename );
    }
    	
    return rv;

#else

    return -1;
    
#endif
}

void sound_stream_capture_stop( void )
{
#ifndef NOSOUND

    if( g_snd_initialized == 0 ) return;
    if( g_snd.out_file == 0 ) return;

    g_snd.out_file_exit_request = 1;
    bthread_destroy( &g_snd.out_file_thread, 5000 );

    bfs_file f = g_snd.out_file;

    sound_stream_lock();
    g_snd.out_file = 0;
    bmem_free( g_snd.out_file_buf );
    g_snd.out_file_buf = 0;
    sound_stream_unlock();

    //Fix WAV data:
    uint n;
    bfs_seek( f, 4, BFS_SEEK_SET );
    n = 4 + 24 + 8 + g_snd.out_file_size; bfs_write( &n, 1, 4, f );
    bfs_seek( f, 36 + 4, BFS_SEEK_SET );
    n = g_snd.out_file_size; bfs_write( &n, 1, 4, f );

    bfs_close( f );

    blog( "Audio capturer stopped. Received %d bytes\n", (int)g_snd.out_file_size );
    
#endif
}

void sound_stream_deinit( void )
{
#ifndef NOSOUND 
    
    if( g_snd_initialized == 0 ) return;
    blog( "SOUND: sound_stream_deinit() begin\n" );

    sound_stream_capture_stop();

    if( g_snd.flags & SOUND_STREAM_FLAG_USER_CONTROLLED )
    {
    }
    else
    {
	device_sound_stream_deinit();
    }

    if( g_snd.slot_buffer )
	bmem_free( g_snd.slot_buffer );
	
    bmutex_destroy( &g_snd.mutex );
    bmutex_destroy( &g_snd.in_mutex );
    
    blog( "SOUND: sound_stream_deinit() end\n" );
    g_snd_initialized = 0;

#endif
}

//
// MIDI
//

int sundog_midi_init( void )
{
    int rv = 0;

#ifndef NOMIDI

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_init() begin\n" );
#endif

    if( g_sundog_midi_open_counter == 0 )
    {
	bmutex_init( &g_sundog_midi_mutex, 0 );
    }
    g_sundog_midi_open_counter++;

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_init() end\n" );
#endif

#endif

    return rv;
}

int sundog_midi_deinit( void )
{
    int rv = 0;

#ifndef NOMIDI

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_deinit() begin\n" );
#endif

    g_sundog_midi_open_counter--;
    if( g_sundog_midi_open_counter == 0 )
    {
	bmutex_destroy( &g_sundog_midi_mutex );
    }

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_deinit() end\n" );
#endif

#endif

    return rv;    
}

int sundog_midi_client_open( sundog_midi_client* c, const utf8_char* name )
{
    int rv = 0;

#ifndef NOMIDI

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_open() begin\n" );
#endif
    
    bmutex_lock( &g_sundog_midi_mutex );
    
    bmem_set( c, sizeof( sundog_midi_client ), 0 );
    c->name = (utf8_char*)bmem_new( bmem_strlen( name ) + 1 );
    c->name[ 0 ] = 0;
    bmem_strcat_resize( c->name, name );
    
    rv = device_midi_client_open( c, name );

    bmutex_unlock( &g_sundog_midi_mutex );

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_open() end %d\n", rv );
#endif

#endif
    
    return rv;
}

int sundog_midi_client_close( sundog_midi_client* c )
{
    int rv = 0;

#ifndef NOMIDI

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_close() begin\n" );
#endif
    
    bmutex_lock( &g_sundog_midi_mutex );
    
    if( c->ports )
    {
	int ports_count = bmem_get_size( c->ports ) / sizeof( sundog_midi_port );
	for( int i = 0; i < ports_count; i++ )
	{
	    sundog_midi_client_close_port( c, i );
	}
	bmem_free( c->ports );
	c->ports = 0;
    }

    rv = device_midi_client_close( c );
    
    bmem_free( c->name );
    c->name = 0;
    
    bmutex_unlock( &g_sundog_midi_mutex );

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_close() end\n" );
#endif
    
#endif

    return rv;
}

int sundog_midi_client_get_devices( sundog_midi_client* c, utf8_char*** devices, uint flags )
{
    int rv = 0;
    
#ifndef NOMIDI

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_get_devices() begin\n" );
#endif
    
    bmutex_lock( &g_sundog_midi_mutex );
    
    rv = device_midi_client_get_devices( c, devices, flags );
    
    bmutex_unlock( &g_sundog_midi_mutex );

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_get_devices() end\n" );
#endif
    
#endif
    
    return rv;
}

int sundog_midi_client_open_port( sundog_midi_client* c, const utf8_char* port_name, const utf8_char* dev_name, uint flags )
{
    int rv = -1;
    
#ifndef NOMIDI

    if( port_name == 0 ) return -1;
    if( dev_name == 0 ) return -1;

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_open_port() begin\n" );
#endif
    
    bmutex_lock( &g_sundog_midi_mutex );
    
    while( 1 )
    {
	if( c->ports == 0 )
	{
	    c->ports = (sundog_midi_port*)bmem_new( sizeof( sundog_midi_port ) );
	    bmem_zero( c->ports );
	    rv = 0;
	}
	else
	{
	    for( int i = 0; i < (int)( bmem_get_size( c->ports ) / sizeof( sundog_midi_port ) ); i++ )
	    {
		if( c->ports[ i ].active == 0 )
		{
		    rv = i;
		    break;
		}
	    }
	    if( rv < 0 )
	    {
		rv = (int)( bmem_get_size( c->ports ) / sizeof( sundog_midi_port ) );
		c->ports = (sundog_midi_port*)bmem_resize( c->ports, bmem_get_size( c->ports ) + sizeof( sundog_midi_port ) );
		bmem_set( &c->ports[ rv ], sizeof( sundog_midi_port ), 0 );
	    }
	}
	
	sundog_midi_port* port = &c->ports[ rv ];
	
	port->flags = flags;
	
	if( device_midi_client_open_port( c, rv, port_name, dev_name, flags ) )
	{
	    port->flags |= MIDI_NO_DEVICE;
	    port->device_specific = 0;
	}
	else
	{
	    port->flags &= ~MIDI_NO_DEVICE;
	}
	
	bmem_free( port->port_name );
	port->port_name = (utf8_char*)bmem_new( bmem_strlen( port_name ) + 1 );
	port->port_name[ 0 ] = 0;
	bmem_strcat_resize( port->port_name, port_name );

	bmem_free( port->dev_name );
	port->dev_name = (utf8_char*)bmem_new( bmem_strlen( dev_name ) + 1 );
	port->dev_name[ 0 ] = 0;
	bmem_strcat_resize( port->dev_name, dev_name );
	
	port->active = 1;
	
	break;
    }
    
    bmutex_unlock( &g_sundog_midi_mutex );

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_open_port() end %d\n", rv );
#endif
    
#endif
    
    return rv;
}

int sundog_midi_client_reopen_port( sundog_midi_client* c, int pnum )
{
    int rv = 0;
    
#ifndef NOMIDI

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_reopen_port() begin\n" );
#endif
    
    bmutex_lock( &g_sundog_midi_mutex );
    
    while( 1 )
    {
	if( c->ports == 0 )
	{
	    rv = -1;
	    break;
	}
	
	if( (unsigned)pnum >= bmem_get_size( c->ports ) / sizeof( sundog_midi_port ) ) 
	{
	    rv = -1;
	    break;
	}
	
	sundog_midi_port* port = &c->ports[ pnum ];
	
	if( port->active == 0 )
	{
	    rv = -1;
	    break;
	}
	
	if( ( port->flags & MIDI_NO_DEVICE ) == 0 )
	    device_midi_client_close_port( c, pnum );
	if( device_midi_client_open_port( c, pnum, port->port_name, port->dev_name, port->flags ) )
	{
	    port->flags |= MIDI_NO_DEVICE;
	    port->device_specific = 0;
	}
	else
	{
	    port->flags &= ~MIDI_NO_DEVICE;
	}
	
	break;
    }
    
    bmutex_unlock( &g_sundog_midi_mutex );

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_reopen_port() end %d\n", rv );
#endif
    
#endif
    
    return rv;
}

int sundog_midi_client_close_port( sundog_midi_client* c, int pnum )
{
    int rv = 0;
    
#ifndef NOMIDI
    
#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_close_port() begin\n" );
#endif

    bmutex_lock( &g_sundog_midi_mutex );
    
    while( 1 )
    {
	if( c->ports == 0 )
	{
	    rv = -1;
	    break;
	}
	
	if( (unsigned)pnum >= bmem_get_size( c->ports ) / sizeof( sundog_midi_port ) ) 
	{
	    rv = -1;
	    break;
	}
	
	sundog_midi_port* port = &c->ports[ pnum ];
	
	if( port->active == 0 )
	{
	    rv = -1;
	    break;
	}

	if( ( port->flags & MIDI_NO_DEVICE ) == 0 )
	    rv = device_midi_client_close_port( c, pnum );
	
	bmem_free( port->port_name );
	bmem_free( port->dev_name );
	port->port_name = 0;
	port->dev_name = 0;
	
	port->active = 0;
	
	break;
    }
    
    bmutex_unlock( &g_sundog_midi_mutex );

#ifdef SHOW_DEBUG_MESSAGES
    blog( "sundog_midi_client_close_port() end %d\n", rv );
#endif

#endif
    
    return rv;
}

sundog_midi_event* sundog_midi_client_get_event( sundog_midi_client* c, int pnum )
{
    sundog_midi_event* evt = 0;

#ifndef NOMIDI

    bmutex_lock( &g_sundog_midi_mutex );
    
    if( ( c->ports != 0 ) && ( (unsigned)pnum < bmem_get_size( c->ports ) / sizeof( sundog_midi_port ) ) )
    {
	evt = device_midi_client_get_event( c, pnum );
    }
    
    bmutex_unlock( &g_sundog_midi_mutex );
    
#endif
    
    return evt;
}

int sundog_midi_client_next_event( sundog_midi_client* c, int pnum )
{
    int rv = 0;

#ifndef NOMIDI
    
    bmutex_lock( &g_sundog_midi_mutex );
    
    if( ( c->ports != 0 ) && ( (unsigned)pnum < bmem_get_size( c->ports ) / sizeof( sundog_midi_port ) ) )
    {
	rv = device_midi_client_next_event( c, pnum );
    }
    
    bmutex_unlock( &g_sundog_midi_mutex );
    
#endif
    
    return rv;
}

int sundog_midi_client_send_event( sundog_midi_client* c, int pnum, sundog_midi_event* evt )
{
    int rv = 0;

#ifndef NOMIDI

    bmutex_lock( &g_sundog_midi_mutex );
    
    while( 1 )
    {
	if( c->ports == 0 ) { rv = -1; break; }
	if( (unsigned)pnum >= bmem_get_size( c->ports ) / sizeof( sundog_midi_port ) ) { rv = -1; break; }
	if( evt == 0 || evt->size == 0 || evt->data == 0 ) { rv = -1; break; }
	
	rv = device_midi_client_send_event( c, pnum, evt );
	
	break;
    }
    
    bmutex_unlock( &g_sundog_midi_mutex );
    
#endif
    
    return rv;
}
