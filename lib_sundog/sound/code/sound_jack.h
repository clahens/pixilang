//JACK Audio Connection Kit

#ifdef JACK_AUDIO

#ifdef LINUX
    #define JACK_DYNAMIC_LINK
    #include <dlfcn.h>
#endif

#include <jack/jack.h>
#include <jack/midiport.h>

void* g_jack_lib = 0;
jack_client_t* g_jack_client = 0;
#ifdef JACK_INPUT
    jack_port_t* g_jack_in_ports[ 2 ] = { 0, 0 };
    void* g_jack_temp_input = 0;
#endif
jack_port_t* g_jack_out_ports[ 2 ] = { 0, 0 };
void* g_jack_temp_output = 0;
jack_nframes_t g_jack_callback_nframes = 0;
ticks_hr_t g_jack_callback_output_time = 0;

/*struct closed_port
{
    const utf8_char* name;
    const utf8_char** connections;
};
#defined MAX_CLOSED_PORTS	256
closed_port closed_ports[ MAX_CLOSED_PORTS ];*/

struct device_midi_port_jack
{
    jack_port_t* port;
    ticks_hr_t output_time;
    void* buffer;
    //for get_event():
    uchar* r_data;
    sundog_midi_event* r_events;
    volatile uint r_data_wp;
    volatile uint r_events_wp;
    volatile uint r_events_rp;
};

struct device_midi_event_jack
{
    device_midi_port_jack* port;
    sundog_midi_event evt;
};

uchar g_jack_midi_out_data[ MIDI_BYTES ];
device_midi_event_jack g_jack_midi_out_events[ MIDI_EVENTS ];
uint g_jack_midi_out_data_wp = 0;
volatile uint g_jack_midi_out_evt_rp = 0;
volatile uint g_jack_midi_out_evt_wp = 0;
device_midi_port_jack* g_jack_midi_ports_to_clear[ 128 ];
int g_jack_midi_clear_count = 0;

extern void jack_publish_client_icon( jack_client_t* client );

#ifdef JACK_DYNAMIC_LINK

#define JACK_GET_FN( NAME ) \
    const utf8_char* fn_name = NAME; \
    static void* f = 0; \
    if( f == 0 ) f = dlsym( g_jack_lib, fn_name ); \
    if( f == 0 ) \
	blog( "JACK: Function %s() not found.\n", fn_name ); \

const char* jack_get_version_string( void )
{
    JACK_GET_FN( "jack_get_version_string" );
    if( f ) return ( (const char*(*)(void))f ) ();
    return 0;
}

jack_client_t* jack_client_open( const char* client_name, jack_options_t options, jack_status_t* status, ... )
{
    JACK_GET_FN( "jack_client_open" );
    if( f ) return ( (jack_client_t*(*)(const char*,jack_options_t,jack_status_t*,...))f ) ( client_name, options, status );
    return 0;
}

int jack_client_close( jack_client_t* client )
{
    JACK_GET_FN( "jack_client_close" );
    if( f ) return ( (int(*)(jack_client_t*))f ) ( client );
    return 0;
}

int jack_activate( jack_client_t* client )
{
    JACK_GET_FN( "jack_activate" );
    if( f ) return ( (int(*)(jack_client_t*))f ) ( client );
    return 0;
}

int jack_set_process_callback( jack_client_t* client, JackProcessCallback process_callback, void* arg )
{
    JACK_GET_FN( "jack_set_process_callback" );
    if( f ) return ( (int(*)(jack_client_t*,JackProcessCallback,void*))f ) ( client, process_callback, arg );
    return 0;
}

void jack_on_shutdown( jack_client_t* client, JackShutdownCallback shutdown_callback, void* arg )
{
    JACK_GET_FN( "jack_on_shutdown" );
    if( f ) ( (void(*)(jack_client_t*,JackShutdownCallback,void*))f ) ( client, shutdown_callback, arg );
}

jack_port_t* jack_port_register( jack_client_t* client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size )
{
    JACK_GET_FN( "jack_port_register" );
    if( f ) return ( (jack_port_t*(*)(jack_client_t*,const char*,const char*,unsigned long,unsigned long))f ) ( client, port_name, port_type, flags, buffer_size );
    return 0;
}

int jack_port_unregister( jack_client_t* client, jack_port_t* port )
{
    JACK_GET_FN( "jack_port_unregister" );
    if( f ) return ( (int(*)(jack_client_t*,jack_port_t*))f ) ( client, port );
    return 0;
}

jack_nframes_t jack_port_get_total_latency( jack_client_t* client, jack_port_t* port )
{
    JACK_GET_FN( "jack_port_get_total_latency" );
    if( f ) return ( (jack_nframes_t(*)(jack_client_t*,jack_port_t*))f ) ( client, port );
    return 0;
}

const char* jack_port_name( const jack_port_t* port )
{
    JACK_GET_FN( "jack_port_name" );
    if( f ) return ( (const char*(*)(const jack_port_t*))f ) ( port );
    return 0;
}

jack_time_t jack_frames_to_time( const jack_client_t* client, jack_nframes_t frames )
{
    JACK_GET_FN( "jack_frames_to_time" );
    if( f ) return ( (jack_time_t(*)(const jack_client_t*,jack_nframes_t))f ) ( client, frames );
    return 0;
}

jack_nframes_t jack_last_frame_time( const jack_client_t* client )
{
    JACK_GET_FN( "jack_last_frame_time" );
    if( f ) return ( (jack_nframes_t(*)(const jack_client_t*))f ) ( client );
    return 0;
}

jack_time_t jack_get_time( void )
{
    JACK_GET_FN( "jack_get_time" );
    if( f ) return ( (jack_time_t(*)(void))f ) ();
    return 0;
}

void* jack_port_get_buffer( jack_port_t* port, jack_nframes_t frames )
{
    JACK_GET_FN( "jack_port_get_buffer" );
    if( f ) return ( (void*(*)(jack_port_t*,jack_nframes_t))f ) ( port, frames );
    return 0;
}

int jack_port_flags( const jack_port_t* port )
{
    JACK_GET_FN( "jack_port_flags" );
    if( f ) return ( (int(*)(const jack_port_t*))f ) ( port );
    return 0;
}

const char** jack_port_get_connections( const jack_port_t* port )
{
    JACK_GET_FN( "jack_port_get_connections" );
    if( f ) return ( (const char**(*)(const jack_port_t*))f ) ( port );
    return 0;
}

void jack_midi_clear_buffer( void* port_buffer )
{
    JACK_GET_FN( "jack_midi_clear_buffer" );
    if( f ) ( (void(*)(void*))f ) ( port_buffer );
}

jack_midi_data_t* jack_midi_event_reserve( void* port_buffer, jack_nframes_t time, size_t data_size )
{
    JACK_GET_FN( "jack_midi_event_reserve" );
    if( f ) return ( (jack_midi_data_t*(*)(void*,jack_nframes_t,size_t))f ) ( port_buffer, time, data_size );
    return 0;
}

jack_nframes_t jack_midi_get_event_count( void* port_buffer )
{
    JACK_GET_FN( "jack_midi_get_event_count" );
    if( f ) return ( (jack_nframes_t(*)(void*))f ) ( port_buffer );
    return 0;
}

int jack_midi_event_get( jack_midi_event_t* event, void* port_buffer, jack_nframes_t event_index )
{
    JACK_GET_FN( "jack_midi_event_get" );
    if( f ) return ( (int(*)(jack_midi_event_t*,void*,jack_nframes_t))f ) ( event, port_buffer, event_index );
    return 0;
}

const char** jack_get_ports( jack_client_t* client, const char* port_name_pattern, const char* type_name_pattern, unsigned long flags )
{
    JACK_GET_FN( "jack_get_ports" );
    if( f ) return ( (const char**(*)(jack_client_t*,const char*,const char*,unsigned long))f ) ( client, port_name_pattern, type_name_pattern, flags );
    return 0;
}

int jack_connect( jack_client_t* client, const char* source_port, const char* destination_port )
{
    JACK_GET_FN( "jack_connect" );
    if( f ) return ( (int(*)(jack_client_t*,const char*,const char*))f ) ( client, source_port, destination_port );
    return 0;
}

void jack_free( void* ptr )
{
    JACK_GET_FN( "jack_free" );
    if( f ) ( (void(*)(void*))f ) ( ptr );
}

jack_nframes_t jack_get_sample_rate( jack_client_t* client )
{
    JACK_GET_FN( "jack_get_sample_rate" );
    if( f ) return ( (jack_nframes_t(*)(jack_client_t*))f ) ( client );
    return 0;
}

#endif

static int jack_process_callback( jack_nframes_t nframes, void* arg )
{
    g_jack_callback_nframes = nframes;
    
    size_t out_sample_size = 0;
    if( g_snd.out_type == sound_buffer_float32 ) out_sample_size = 4;
    if( g_snd.out_type == sound_buffer_int16 ) out_sample_size = 2;
    size_t in_sample_size = 0;
    if( g_snd.in_type == sound_buffer_float32 ) in_sample_size = 4;
    if( g_snd.in_type == sound_buffer_int16 ) in_sample_size = 2;

    //Create temp buffers:
    size_t size, old_size;
#ifdef JACK_INPUT
    size = nframes * out_sample_size * g_snd.out_channels;
    if( g_jack_temp_input == 0 )
    {
        g_jack_temp_input = bmem_new( size );
        bmem_zero( g_jack_temp_input );
    }
    old_size = bmem_get_size( g_jack_temp_input );
    if( old_size < size )
    {
        g_jack_temp_input = bmem_resize( g_jack_temp_input, size );
        bmem_set( (char*)g_jack_temp_input + old_size, size - old_size, 0 );
    }
#endif
    size = nframes * in_sample_size * g_snd.in_channels;
    if( g_jack_temp_output == 0 )
    {
        g_jack_temp_output = bmem_new( size );
        bmem_zero( g_jack_temp_output );
    }
    old_size = bmem_get_size( g_jack_temp_output );
    if( old_size < size )
    {
        g_jack_temp_output = bmem_resize( g_jack_temp_output, size );
        bmem_set( (char*)g_jack_temp_output + old_size, size - old_size, 0 );
    }
    
    //Get latency:
    jack_nframes_t latency;
#ifdef LINUX
    latency = jack_port_get_total_latency( g_jack_client, g_jack_out_ports[ 0 ] );
#else
    jack_latency_range_t latency_range;
    jack_port_get_latency_range( g_jack_out_ports[ 0 ], JackPlaybackLatency, &latency_range );
    latency = latency_range.max;
#endif
    g_snd.out_latency = latency;
    
    //Get time:
    jack_time_t current_usecs = jack_frames_to_time( g_jack_client, jack_last_frame_time( g_jack_client ) );
    jack_time_t cur_jack_t = jack_get_time();
    ticks_hr_t cur_t = time_ticks_hires();
    jack_time_t output_time = current_usecs + ( latency * 1000000 ) / g_snd.freq;
    jack_time_t delta = ( ( output_time - cur_jack_t ) * time_ticks_per_second_hires() ) / 1000000;
    if( ( delta & 0x80000000 ) == 0 ) //delta is positive
        g_snd.out_time = cur_t + (ticks_hr_t)delta;
    else
        g_snd.out_time = cur_t;
    g_jack_callback_output_time = g_snd.out_time;
    
#ifdef JACK_INPUT
    //Fill input buffer:
    for( int c = 0; c < g_snd.in_channels; c++ )
    {
        float* buf = (float*)jack_port_get_buffer( g_jack_in_ports[ c ], nframes );
        if( g_snd.in_type == sound_buffer_int16 )
        {
    	    signed short* input = (signed short*)g_jack_temp_input;
            for( size_t i = c, i2 = 0; i < nframes * g_snd.in_channels; i += g_snd.in_channels, i2++ )
            {
                FLOAT32_TO_INT16( input[ i ], buf[ i2 ] );
            }
        }
        if( g_snd.in_type == sound_buffer_float32 )
        {
    	    float* input = (float*)g_jack_temp_input;
            for( size_t i = c, i2 = 0; i < nframes * g_snd.in_channels; i += g_snd.in_channels, i2++ )
            {
                input[ i ] = buf[ i2 ];
            }
        }
    }
#endif

    //Render:
    g_snd.out_buffer = g_jack_temp_output;
    g_snd.out_frames = nframes;
#ifdef JACK_INPUT
    g_snd.in_buffer = g_jack_temp_input;
#else
    g_snd.in_buffer = 0;
#endif
    main_sound_output_callback( &g_snd, 0 );
    
    //Fill output buffers:
    for( int c = 0; c < g_snd.out_channels; c++ )
    {
        float* buf = (float*)jack_port_get_buffer( g_jack_out_ports[ c ], nframes );
        if( g_snd.out_type == sound_buffer_int16 )
        {
    	    signed short* output = (signed short*)g_jack_temp_output;
            for( size_t i = c, i2 = 0; i < nframes * g_snd.out_channels; i += g_snd.out_channels, i2++ )
            {
                INT16_TO_FLOAT32( buf[ i2 ], output[ i ] );
            }
        }
        if( g_snd.out_type == sound_buffer_float32 )
        {
    	    float* output = (float*)g_jack_temp_output;
            for( size_t i = c, i2 = 0; i < nframes * g_snd.out_channels; i += g_snd.out_channels, i2++ )
            {
                buf[ i2 ] = output[ i ];
            }
        }
    }
    
    //Handle MIDI output:
    if( g_jack_midi_clear_count )
    {
        //Clear ports which were used in previous callback:
        for( int p = 0; p < g_jack_midi_clear_count; p++ )
        {
            device_midi_port_jack* port = g_jack_midi_ports_to_clear[ p ];
            if( port && port->port )
            {
                if( port->output_time != g_jack_callback_output_time )
                {
                    port->output_time = g_jack_callback_output_time;
                    jack_midi_clear_buffer( jack_port_get_buffer( port->port, g_jack_callback_nframes ) );
                    port->buffer = jack_port_get_buffer( port->port, g_jack_callback_nframes );
                }
            }
        }
        g_jack_midi_clear_count = 0;
    }
    if( g_jack_midi_out_evt_wp != g_jack_midi_out_evt_rp )
    {
        //Sort:
        size_t wp = g_jack_midi_out_evt_wp;
        size_t rp = g_jack_midi_out_evt_rp;
        bool sort_request = 1;
        while( sort_request )
        {
            sort_request = 0;
            for( size_t i = rp; i != ( ( wp - 1 ) & ( MIDI_EVENTS - 1 ) ); i = ( i + 1 ) & ( MIDI_EVENTS - 1 ) )
            {
                device_midi_event_jack* e1 = &g_jack_midi_out_events[ i ];
                device_midi_event_jack* e2 = &g_jack_midi_out_events[ ( i + 1 ) & ( MIDI_EVENTS - 1 ) ];
                if( ( e2->evt.t - e1->evt.t ) & 0x80000000 ) //if e1->evt.t > e2->evt.t
                {
                    device_midi_event_jack e = g_jack_midi_out_events[ i ];
                    g_jack_midi_out_events[ i ] = *e2;
                    g_jack_midi_out_events[ ( i + 1 ) & ( MIDI_EVENTS - 1 ) ] = e;
                    sort_request = 1;
                }
            }
        }
        //Send:
        int latency_ticks = (int)( ( (uint64)g_snd.out_latency * (uint64)time_ticks_per_second_hires() ) / (uint64)g_snd.freq );
        int buf_ticks = (int)( ( (uint64)nframes * (uint64)time_ticks_per_second_hires() ) / (uint64)g_snd.freq );
        for( ; rp != wp; rp = ( rp + 1 ) & ( MIDI_EVENTS - 1 ) )
        {
            device_midi_event_jack* evt = &g_jack_midi_out_events[ rp ];
            device_midi_port_jack* port = evt->port;
            if( port && port->port )
            {
                if( port->output_time != g_jack_callback_output_time )
                {
                    port->output_time = g_jack_callback_output_time;
                    jack_midi_clear_buffer( jack_port_get_buffer( port->port, g_jack_callback_nframes ) );
                    port->buffer = jack_port_get_buffer( port->port, g_jack_callback_nframes );
                }
                ticks_hr_t evt_t = evt->evt.t + latency_ticks * 2; //output time of this event
                if( ( evt_t - ( g_jack_callback_output_time + buf_ticks ) ) & 0x80000000 ) //if evt_t < ...
                {
                    bool clear_port_later = 1;
                    for( int p = 0; p < g_jack_midi_clear_count; p++ )
                    {
                        if( g_jack_midi_ports_to_clear[ p ] == port )
                        {
                            clear_port_later = 0;
                            break;
                        }
                    }
                    if( clear_port_later )
                    {
                        g_jack_midi_ports_to_clear[ g_jack_midi_clear_count ] = port;
                        g_jack_midi_clear_count++;
                    }
                    jack_nframes_t t;
                    if( ( evt_t - g_jack_callback_output_time ) & 0x80000000 ) //if evt_t < ...
                        t = 0;
                    else
                        t = ( ( evt_t - g_jack_callback_output_time ) * g_snd.freq ) / time_ticks_per_second_hires();
                    jack_midi_data_t* midi_data_buffer = jack_midi_event_reserve( port->buffer, t, evt->evt.size );
                    bmem_copy( midi_data_buffer, evt->evt.data, evt->evt.size );
                }
                else
                {
                    break;
                }
            }
        }
        //Save pointers:
        g_jack_midi_out_evt_rp = rp;
    }
    
    g_jack_callback_nframes = 0;
    
    return 0;
}

static void jack_shut_down( void* arg )
{
}

static void jack_set_default_connections( void )
{
#ifdef JACK_INPUT
    char** physicalOutPorts = (char**)jack_get_ports( g_jack_client, NULL, NULL, JackPortIsPhysical | JackPortIsOutput );
    if( physicalOutPorts != NULL )
    {
        for( int i = 0; i < g_snd.in_channels && physicalOutPorts[ i ]; i++ )
        {
            jack_connect( g_jack_client, physicalOutPorts[ i ], jack_port_name( g_jack_in_ports[ i ] ) );
        }
        jack_free( physicalOutPorts );
    }
#endif
    
    char** physicalInPorts = (char**)jack_get_ports( g_jack_client, NULL, NULL, JackPortIsPhysical | JackPortIsInput );
    if( physicalInPorts != NULL )
    {
        for( int i = 0; i < g_snd.out_channels && physicalInPorts[ i ]; i++ )
        {
            jack_connect( g_jack_client, jack_port_name( g_jack_out_ports[ i ] ), physicalInPorts[ i ] );
        }
        jack_free( physicalInPorts );
    }
}

int device_sound_stream_init_jack( void )
{
#ifdef JACK_DYNAMIC_LINK
    //Open library:
    if( g_jack_lib == 0 )
    {
#ifdef LINUX
	g_jack_lib = dlopen( "libjack.so", RTLD_NOW );
	if( g_jack_lib == 0 ) g_jack_lib = dlopen( "libjack.so.0", RTLD_NOW );
#endif
	if( g_jack_lib == 0 )
	{
	    blog( "JACK: Can't open libjack\n" );
	    return -1;
	}
    }
#endif
    
#ifdef IPHONE
    jack_app_register( JackRegisterDefaults );
#endif

    //Open client:
    jack_status_t status;
    g_jack_client = jack_client_open( user_window_name_short, JackNullOption, &status );
    if( !g_jack_client )
    {
	blog( "JACK: jack_client_open error %x\n", (int)status );
        if( status & JackVersionError )
            blog( "JACK: App not compatible with running JACK version!\n" );
        else
            blog( "JACK: Server app seems not to be running!\n" );
        return -1;
    }

    //Publish client icon:
#ifdef IPHONE
    jack_publish_client_icon( g_jack_client );
#endif
    
    //Register callback functions:
    jack_set_process_callback( g_jack_client, jack_process_callback, 0 );
    jack_on_shutdown( g_jack_client, jack_shut_down, 0 );
    
    //Create audio ports:
#ifdef JACK_INPUT
    g_jack_in_ports[ 0 ] = jack_port_register( g_jack_client, "Left In", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 );
    g_jack_in_ports[ 1 ] = jack_port_register( g_jack_client, "Right In", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 );
#endif
    g_jack_out_ports[ 0 ] = jack_port_register( g_jack_client, "Left Out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
    g_jack_out_ports[ 1 ] = jack_port_register( g_jack_client, "Right Out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
    
    //Get JACK parameters:
    g_snd.freq = jack_get_sample_rate( g_jack_client );
#ifdef ONLY44100
    if( g_snd.freq != 44100 )
    {
        blog( "JACK: Can't set sample rate %d. Only 44100 supported in this version.\n", g_snd.freq );
        jack_client_close( g_jack_client );
        g_jack_client = 0;
        return -1;
    }
#endif
    if( g_snd.freq < 44100 )
    {
        blog( "JACK: Can't set sample rate %d. Minimum 44100 supported.\n", g_snd.freq );
        jack_client_close( g_jack_client );
        g_jack_client = 0;
        return -1;
    }
    
    //Reset MIDI:
    g_jack_midi_clear_count = 0;

#ifndef IPHONE
#if CPUMARK >= 10
    //Set 32bit mode:
    g_snd.out_type = sound_buffer_float32;
    g_snd.in_type = sound_buffer_float32;
#endif
#endif
    
    //Activate client:
    if( jack_activate( g_jack_client ) )
    {
        blog( "JACK: Cannot activate client.\n" );
        jack_client_close( g_jack_client );
        g_jack_client = 0;
        return -1;
    }
    
    //Set default connections:
    jack_set_default_connections();
    
    return 0;
}

void device_sound_stream_deinit_jack( void )
{
    jack_client_close( g_jack_client );
#ifdef JACK_INPUT
    bmem_free( g_jack_temp_input );
    g_jack_temp_input = 0;
#endif
    bmem_free( g_jack_temp_output );
    g_jack_client = 0;
    g_jack_temp_output = 0;
}

//
// MIDI
//

int device_midi_client_open_jack( sundog_midi_client* c, const utf8_char* name )
{
    if( g_snd_initialized == 0 )
    {
	blog( "JACK: MIDI client can not be opened if the audio stream is not initialized.\n" );
	return -1;
    }
    return 0;
}

int device_midi_client_close_jack( sundog_midi_client* c )
{
    return 0;
}

int device_midi_client_get_devices_jack( sundog_midi_client* c, utf8_char*** devices, uint flags )
{
    *devices = (utf8_char**)bmem_new( sizeof( void* ) * 1 );
    (*devices)[ 0 ] = (utf8_char*)bmem_new( 32 );
    (*devices)[ 0 ][ 0 ] = 0;
    bmem_strcat_resize( (*devices)[ 0 ], "JACK" );
    return 1;
}

int device_midi_client_open_port_jack( sundog_midi_client* c, int pnum, const utf8_char* port_name, const utf8_char* dev_name, uint flags )
{
    if( g_jack_client == 0 ) return -1;
    if( pnum < 0 ) return 0;
    
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    sd_port->device_specific = bmem_new( sizeof( device_midi_port_jack ) );
    device_midi_port_jack* port = (device_midi_port_jack*)sd_port->device_specific;
    if( port == 0 ) return -1;
    bmem_zero( port );
    if( flags & MIDI_PORT_READ )
    {
        port->port = jack_port_register( g_jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0 );
	if( c->name && port_name ) //RESTORE CONNECTIONS
	{
	    utf8_char* fname = (utf8_char*)bmem_new( 4096 );
	    utf8_char* con_name = (utf8_char*)bmem_new( 4096 );
	    sprintf( fname, "2:/.sundog_jackmidi_%s_%s", c->name, port_name );
	    bfs_file f = bfs_open( fname, "rb" );
	    if( f )
	    {
		int i = 0;
		while( 1 )
		{		    
		    int cc = bfs_getc( f );
		    if( cc < 0 ) break;
		    con_name[ i ] = cc;
		    i++;
		    if( cc == 0 )
		    {
			blog( "JACK: restoring previous connection %s -> %s ...\n", con_name, jack_port_name( port->port ) );
			jack_connect( g_jack_client, con_name, jack_port_name( port->port ) );
			i = 0;
		    }
		}
		bfs_close( f );
	    }
    	    bmem_free( fname );
    	    bmem_free( con_name );
	}
    }
    else
        port->port = jack_port_register( g_jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0 );
    if( port->port == 0 ) return 0;
    
    return 0;
}

int device_midi_client_close_port_jack( sundog_midi_client* c, int pnum )
{
    if( g_jack_client == 0 ) return -1;
    if( pnum < 0 ) return 0;
    
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port_jack* port = (device_midi_port_jack*)sd_port->device_specific;
    if( port == 0 ) return 0;
    
    uint pflags = jack_port_flags( port->port );
    if( pflags & JackPortIsInput )
    {
	if( c->name && sd_port->port_name ) //SAVE CONNECTIONS
	{
	    utf8_char* fname = (utf8_char*)bmem_new( 4096 );
	    sprintf( fname, "2:/.sundog_jackmidi_%s_%s", c->name, sd_port->port_name );
	    const char** con = jack_port_get_connections( port->port );
	    if( con )
	    {
		bfs_file f = bfs_open( fname, "wb" );
	        if( f )
		{
		    for( int i = 0; ; i++ )
		    {
			const char* con_name = con[ i ];
			if( con_name == 0 ) break;
			bfs_write( con_name, 1, bmem_strlen( con_name ) + 1, f );
		    }
		    bfs_close( f );
		}
    		jack_free( con );
    	    }
    	    else
    	    {
    		bfs_remove( fname );
    	    }
    	    bmem_free( fname );
	}
    }
    
    //Remove this port from the output queue:
    sound_stream_lock();
    for( int p = 0; p < g_jack_midi_clear_count; p++ )
    {
        if( g_jack_midi_ports_to_clear[ p ] == port )
            g_jack_midi_ports_to_clear[ p ] = 0;
    }
    size_t wp = g_jack_midi_out_evt_wp;
    size_t rp = g_jack_midi_out_evt_rp;
    for( ; rp != wp; rp = ( rp + 1 ) & ( MIDI_EVENTS - 1 ) )
    {
        device_midi_event_jack* evt = &g_jack_midi_out_events[ rp ];
        if( evt->port == port ) evt->port = 0;
    }
    sound_stream_unlock();
    
    jack_port_unregister( g_jack_client, port->port );

    bmem_free( port->r_data );
    bmem_free( port->r_events );    
    bmem_free( sd_port->device_specific );
    sd_port->device_specific = 0;
    return 0;
}

sundog_midi_event* device_midi_client_get_event_jack( sundog_midi_client* c, int pnum )
{
    if( g_jack_client == 0 ) return 0;
    if( pnum < 0 ) return 0;
    
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port_jack* port = (device_midi_port_jack*)sd_port->device_specific;
    if( port == 0 ) return 0;
    
    if( port->output_time != g_jack_callback_output_time )
    {
        port->output_time = g_jack_callback_output_time;
        port->buffer = jack_port_get_buffer( port->port, g_jack_callback_nframes );
    	//Copy all incoming events to the r_data and r_events buffers:
	int event_count = jack_midi_get_event_count( port->buffer );
	if( event_count )
	{
            g_last_midi_in_activity = time_ticks_hires();
    	    if( port->r_data == 0 )
	        port->r_data = (uchar*)bmem_new( MIDI_BYTES );
    	    if( port->r_events == 0 )
    		port->r_events = (sundog_midi_event*)bmem_new( MIDI_EVENTS * sizeof( sundog_midi_event ) );
	    for( int i = 0; i < event_count; i++ )
	    {
		//Save data:
    		jack_midi_event_t ev;
    		jack_midi_event_get( &ev, port->buffer, i );
    		if( ev.buffer == 0 ) continue;
		if( port->r_data_wp + ev.size > MIDI_BYTES )
    		    port->r_data_wp = 0;
		if( port->r_data_wp + ev.size > MIDI_BYTES )
		{
    		    //No free space:
    		    continue;
		}
		size_t data_p = port->r_data_wp;
		bmem_copy( port->r_data + data_p, ev.buffer, ev.size );
		port->r_data_wp += ev.size;
		//Save event:
		size_t wp = port->r_events_wp;
		if( wp == ( ( port->r_events_rp - 1 ) & ( MIDI_EVENTS - 1 ) ) )
		{
		    //No free space:
    		    port->r_data_wp -= ev.size;
    		    continue;
		}
		sundog_midi_event* e = &port->r_events[ wp ];
        	jack_nframes_t t = ( ev.time * time_ticks_per_second_hires() ) / g_snd.freq; //res = ticks
        	e->t = port->output_time + t;
        	e->t -= (int)( ( (uint64)( g_snd.out_latency * 2 ) * (uint64)time_ticks_per_second_hires() ) / (uint64)g_snd.freq );
		e->size = ev.size;
		e->data = port->r_data + data_p;
		wp = ( wp + 1 ) & ( MIDI_EVENTS - 1 );
		COMPILER_MEMORY_BARRIER();
		port->r_events_wp = wp;
	    }
	}
    }
    
    if( port->r_events_rp != port->r_events_wp )
    {
	return &port->r_events[ port->r_events_rp ];
    }
    
    return 0;
}

int device_midi_client_next_event_jack( sundog_midi_client* c, int pnum )
{
    if( g_jack_client == 0 ) return 0;
    if( pnum < 0 ) return 0;
    
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port_jack* port = (device_midi_port_jack*)sd_port->device_specific;
    if( port == 0 ) return 0;
    
    if( port->r_events_rp != port->r_events_wp )
    {
	port->r_events_rp = ( port->r_events_rp + 1 ) & ( MIDI_EVENTS - 1 );
    }
    
    return 0;
}

int device_midi_client_send_event_jack( sundog_midi_client* c, int pnum, sundog_midi_event* evt )
{
    if( g_jack_client == 0 ) return 0;
    if( pnum < 0 ) return 0;
    
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port_jack* port = (device_midi_port_jack*)sd_port->device_specific;
    if( port == 0 ) return 0;
    
    g_last_midi_out_activity = time_ticks_hires();
    
    //Save data:
    if( g_jack_midi_out_data_wp + evt->size > MIDI_BYTES )
        g_jack_midi_out_data_wp = 0;
    if( g_jack_midi_out_data_wp + evt->size > MIDI_BYTES )
    {
        //No free space:
        return -1;
    }
    size_t data_p = g_jack_midi_out_data_wp;
    bmem_copy( g_jack_midi_out_data + g_jack_midi_out_data_wp, evt->data, evt->size );
    g_jack_midi_out_data_wp += evt->size;
    
    //Save event:
    size_t wp = g_jack_midi_out_evt_wp;
    if( wp == ( ( g_jack_midi_out_evt_rp - 1 ) & ( MIDI_EVENTS - 1 ) ) )
    {
        //No free space:
        g_jack_midi_out_data_wp -= evt->size;
        return -1;
    }
    device_midi_event_jack* e = &g_jack_midi_out_events[ wp ];
    e->port = port;
    e->evt.t = evt->t;
    e->evt.size = evt->size;
    e->evt.data = g_jack_midi_out_data + data_p;
    wp = ( wp + 1 ) & ( MIDI_EVENTS - 1 );
    COMPILER_MEMORY_BARRIER();
    g_jack_midi_out_evt_wp = wp;
    
    return 0;
}

#endif
