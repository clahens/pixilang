//MMSYSTEM MIDI for Windows

#ifndef NOMIDI

#define MIDI_EVENTS ( 2048 )
#define MIDI_BYTES ( MIDI_EVENTS * 16 )

struct device_midi_event
{
    HMIDIOUT dev;
    sundog_midi_event evt;
};

struct device_midi_client
{
    device_midi_event* out_events;
    bthread out_thread;
    bmutex out_thread_mutex;
    HANDLE out_thread_event;
    volatile bool out_thread_exit_request;
    bmutex in_callback_mutex;
};

struct device_midi_port
{
    bool already_opened;
    int link;
    int dev_id;
    HMIDIIN in_dev;
    HMIDIOUT out_dev;
    ticks_hr_t start_time;
    
    uchar data[ MIDI_BYTES ];
    sundog_midi_event events[ MIDI_EVENTS ];
    volatile uint data_rp;
    volatile uint data_wp;
    volatile uint evt_rp;
    volatile uint evt_wp;
};

void* midi_out_thread( void* user_data )
{
    sundog_midi_client* c = (sundog_midi_client*)user_data;
    if( c == 0 )
    {
	blog( "midi_out_thread(): null client\n" );
	return 0;
    }
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	blog( "midi_out_thread(): null client (device_specific)\n" );
	return 0;
    }

#ifdef SHOW_DEBUG_MESSAGES
    blog( "midi_out_thread() begin\n" );
#endif
    
    while( d->out_thread_exit_request == 0 )
    {
	bool no_events = 1;
	int first_event = -1;
	
	if( d->out_events )
	{
	    bmutex_lock( &d->out_thread_mutex );
	    
	    ticks_hr_t t = time_ticks_hires();

	    ticks_hr_t first_event_t = 0;
	    size_t events = bmem_get_size( d->out_events ) / sizeof( device_midi_event );
	    for( int e = 0; e < events; e++ )
	    {
		device_midi_event* evt = &d->out_events[ e ];
		if( evt->evt.data )
		{
		    no_events = 0;
		    if( ( (uint)( t - evt->evt.t ) & 0x80000000 ) == 0 )
		    {
			if( first_event == -1 )
			{
			    first_event = e;
			    first_event_t = evt->evt.t;
			}
			else
			{
			    if( ( (uint)( first_event_t - evt->evt.t ) & 0x80000000 ) == 0 )
			    {
				first_event = e;
				first_event_t = evt->evt.t;
			    }
			}
		    }
		}
	    }
	    
	    if( first_event >= 0 )
	    {
		//Handle event:
		device_midi_event* evt = &d->out_events[ first_event ];
		int p = 0;
		DWORD msg = 0;
		for( int b = 0; b < evt->evt.size; b++ )
		{
		    if( ( evt->evt.data[ b ] & 0x80 ) || ( p > 2 ) )
		    {
			if( p > 0 )
			{
			    MMRESULT res = midiOutShortMsg( evt->dev, msg );
			    if( res != MMSYSERR_NOERROR )
			    {
				blog( "midiOutShortMsg error %d\n", (int)res );
			    }
			}
			p = 0;
			msg = 0;
		    }
		    uchar* bytes = (uchar*)&msg;
		    bytes[ p ] = evt->evt.data[ b ];
		    p++;
		}
		if( p > 0 )
		{
		    MMRESULT res = midiOutShortMsg( evt->dev, msg );
		    if( res != MMSYSERR_NOERROR )
		    {
			blog( "midiOutShortMsg error %d\n", (int)res );
		    }
		}
		bmem_free( evt->evt.data );
		evt->evt.data = 0;
	    }
	
	    if( no_events )
	    {
		bmem_free( d->out_events );
		d->out_events = 0;
	    }
			
	    bmutex_unlock( &d->out_thread_mutex );
	}
	
	if( first_event == -1 )
	{
	    if( no_events )
		WaitForSingleObject( d->out_thread_event, 200 );
	    else 
		time_sleep( 10 );
	}
    }

#ifdef SHOW_DEBUG_MESSAGES
    blog( "midi_out_thread() end\n" );
#endif
    
    return 0;
}

void CALLBACK midi_in_callback( HMIDIIN handle, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    sundog_midi_client* c = (sundog_midi_client*)dwInstance;
    if( c == 0 ) return;
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 ) return;

#ifdef SHOW_DEBUG_MESSAGES
    blog( "midi_in_callback() begin\n" );
#endif
    
    g_last_midi_in_activity = time_ticks_hires();
    
    if( uMsg == MIM_DATA )
    {
	uchar* msg = (uchar*)&dwParam1;
	DWORD timestamp = dwParam2;
	
	unsigned char status = msg[ 0 ];
	if ( !( status & 0x80 ) ) return;
	
	int msg_bytes = 1;
	while( 1 )
	{
	    if( status < 0xC0 ) { msg_bytes = 3; break; }
	    if( status < 0xE0 ) { msg_bytes = 2; break; }
	    if( status < 0xF0 ) { msg_bytes = 3; break; }
	    if( status == 0xF1 ) { msg_bytes = 2; break; }
	    if( status == 0xF2 ) { msg_bytes = 3; break; }
	    if( status == 0xF3 ) { msg_bytes = 2; break; }
	    break;
	}
	
	size_t ports_num = 0;
	if( c->ports ) ports_num = bmem_get_size( c->ports ) / sizeof( sundog_midi_port );
	
	bmutex_lock( &d->in_callback_mutex );
	
	for( int port_num = 0; port_num < ports_num; port_num++ )
	{
	    if( c->ports[ port_num ].active )
	    {
		device_midi_port* port = (device_midi_port*)c->ports[ port_num ].device_specific;
		if( port == 0 ) continue;
		if( port->already_opened )
		{
		    port = (device_midi_port*)c->ports[ port->link ].device_specific;
		}
		if( port == 0 ) continue;
		if( port->in_dev == handle )
		{
		    //Our device. Send data to it.
		    unsigned int empty_data = 0;
		    if( port->data_wp + msg_bytes > MIDI_BYTES )
			empty_data = MIDI_BYTES - port->data_wp;
		    unsigned int can_write = ( port->data_rp - port->data_wp ) & ( MIDI_BYTES - 1 );
		    if( can_write == 0 ) can_write = MIDI_BYTES;
		    if( ( ( port->data_wp + can_write ) & ( MIDI_BYTES - 1 ) ) == port->data_rp ) can_write--;
		    if( empty_data + msg_bytes <= can_write )
		    {
			if( ( ( port->evt_rp - port->evt_wp ) & ( MIDI_EVENTS - 1 ) ) != 1 )
			{
			    for( int b = 0; b < msg_bytes; b++ )
			    {
				port->data[ ( port->data_wp + b + empty_data ) & ( MIDI_BYTES - 1 ) ] = msg[ b ];
			    }
			    sundog_midi_event* evt = &port->events[ port->evt_wp ];
			    unsigned long long t = port->start_time + ( timestamp * time_ticks_per_second_hires() ) / 1000;
			    evt->t = (ticks_hr_t)t;
			    evt->size = msg_bytes;
			    evt->data = port->data + ( ( port->data_wp + empty_data ) & ( MIDI_BYTES - 1 ) );
			    uint new_data_wp = ( port->data_wp + msg_bytes + empty_data ) & ( MIDI_BYTES - 1 );
			    port->data_wp = new_data_wp;
			    uint new_evt_wp = ( port->evt_wp + 1 ) & ( MIDI_EVENTS - 1 );
			    COMPILER_MEMORY_BARRIER();
			    port->evt_wp = new_evt_wp;
			}
		    }
		}
	    }
	}
	
	bmutex_unlock( &d->in_callback_mutex );
    }

#ifdef SHOW_DEBUG_MESSAGES
    blog( "midi_in_callback() end\n" );
#endif
}

int device_midi_client_open( sundog_midi_client* c, const utf8_char* name )
{
    c->device_specific = bmem_new( sizeof( device_midi_client ) );
    device_midi_client* d = (device_midi_client*)c->device_specific;
    bmem_zero( d );
    
    //Open midi out thread:
    d->out_thread_event = CreateEvent( 0, 0, 0, 0 );
    bmutex_init( &d->out_thread_mutex, 0 ); 
    bthread_create( &d->out_thread, midi_out_thread, c, 0 );

    //Create midi in callback mutex:
    bmutex_init( &d->in_callback_mutex, 0 ); 
    
    return 0;
}

int device_midi_client_close( sundog_midi_client* c )
{
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    
    //Close midi out thread:
    d->out_thread_exit_request = 1;
    SetEvent( d->out_thread_event );
    bthread_destroy( &d->out_thread, 1000 );
    bmutex_destroy( &d->out_thread_mutex );
    CloseHandle( d->out_thread_event );
    
    //Destroy midi in callback mutex:
    bmutex_destroy( &d->in_callback_mutex ); 
    
    bmem_free( d );
    c->device_specific = 0;
    return 0;
}

int device_midi_client_get_devices( sundog_midi_client* c, utf8_char*** devices, uint flags )
{
    int devs = 0;
    if( flags & MIDI_PORT_READ )
	devs = midiInGetNumDevs();
    else 
	devs = midiOutGetNumDevs();
    
    utf8_char* name = (utf8_char*)bmem_new( 1024 );
    utf8_char* prev_name = 0;
    int name_cnt = 1;
    
    for( int i = 0; i < devs; i++ )
    {
	name[ 0 ] = 0;
	if( flags & MIDI_PORT_READ )
	{
	    MIDIINCAPSW caps;
	    if( midiInGetDevCapsW( i, &caps, sizeof( MIDIINCAPSW ) ) == MMSYSERR_NOERROR )
		utf16_to_utf8( name, 1024, (const utf16_char*)caps.szPname );
	}
	else 
	{
	    MIDIOUTCAPSW caps;
	    if( midiOutGetDevCapsW( i, &caps, sizeof( MIDIOUTCAPSW ) ) == MMSYSERR_NOERROR )
		utf16_to_utf8( name, 1024, (const utf16_char*)caps.szPname );
	}
    	if( *devices == 0 )
    	{
    	    *devices = (utf8_char**)bmem_new( sizeof( void* ) * 128 );
    	}
	else
	{
	    if( i + 1 > bmem_get_size( *devices ) / sizeof( void* ) )
	    {
		*devices = (utf8_char**)bmem_resize( *devices, sizeof( void* ) * ( i + 1 ) * 2 );
	    }
	}
	utf8_char* name2 = (utf8_char*)bmem_new( bmem_strlen( name ) + 8 );
	if( prev_name == 0 )
	{
	    prev_name = bmem_strdup( name );
	    sprintf( name2, "%s", name );
	}
	else 
	{
	    if( bmem_strcmp( prev_name, name ) == 0 )
	    {
	        name_cnt++;
	        sprintf( name2, "%s %d", name, name_cnt );
	    }
	    else
	    {
	        name_cnt = 1;
	        bmem_free( prev_name );
	        prev_name = bmem_strdup( name );
	        sprintf( name2, "%s", name );
	    }
	}
	(*devices)[ i ] = name2;
    }
    
    bmem_free( prev_name );
    bmem_free( name );
    
    return devs;
}

int device_midi_client_open_port( sundog_midi_client* c, int pnum, const utf8_char* port_name, const utf8_char* dev_name, uint flags )
{
    int rv = 0;
    
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    sd_port->device_specific = bmem_new( sizeof( device_midi_port ) );
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    bmem_zero( port );
    
    size_t ports_num = bmem_get_size( c->ports ) / sizeof( sundog_midi_port );
    for( int i = 0; i < ports_num; i++ )
    {
	if( c->ports[ i ].active )
	{
	    if( bmem_strcmp( c->ports[ i ].dev_name, dev_name ) == 0 )
	    {
		if( ( flags & ( MIDI_PORT_READ | MIDI_PORT_WRITE ) ) == ( c->ports[ i ].flags & ( MIDI_PORT_READ | MIDI_PORT_WRITE ) ) )
		{
		    device_midi_port* port2 = (device_midi_port*)c->ports[ i ].device_specific;
		    if( port2 && port2->already_opened == 0 )
		    {
			//Already opened:
			port->already_opened = 1;
			port->link = i;
			break;
		    }
		}
	    }
	}
    }
    if( port->already_opened == 0 )
    {
	port->dev_id = -1;
	
	utf8_char** devices = 0;
	int devs = device_midi_client_get_devices( c, &devices, flags );
	
	for( int i = 0; i < devs; i++ )
	{
	    if( bmem_strcmp( devices[ i ], dev_name ) == 0 )
	    {
		//Device found:
		port->dev_id = i;
		break;
	    }
	}
	for( int i = 0; i < devs; i++ )
	{
	    bmem_free( devices[ i ] );
	}
	bmem_free( devices );
	
	if( port->dev_id >= 0 )
	{
	    if( flags & MIDI_PORT_READ )
	    {
		MMRESULT res = midiInOpen( &port->in_dev, port->dev_id, (DWORD_PTR)&midi_in_callback, (DWORD_PTR)c, CALLBACK_FUNCTION );
		if( res != MMSYSERR_NOERROR )
		{
		    blog( "midiInOpen error %d\n", (int)res );
		    rv = -1;
		    goto open_end;
		}
		midiInStart( port->in_dev );
		port->start_time = time_ticks_hires();
	    }
	    else 
	    {
		MMRESULT res = midiOutOpen( &port->out_dev, port->dev_id, 0, 0, CALLBACK_NULL );
		if( res != MMSYSERR_NOERROR )
		{
		    blog( "midiOutOpen error %d\n", (int)res );
		    rv = -1;
		    goto open_end;
		}
	    }
	}
	else 
	{
	    blog( "MIDI device not found.\n" );
	    rv = -1;
	    goto open_end;
	}
    }
    
open_end:

    if( rv != 0 )
    {
	bmem_free( sd_port->device_specific );
    }
    
    return rv;
}

int device_midi_client_close_port( sundog_midi_client* c, int pnum )
{
    if( pnum < 0 ) return 0;

    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    if( port == 0 ) return 0;

    if( port->already_opened == 0 )
    {
	bmutex_lock( &d->out_thread_mutex );

	//Are there linked ports?
	int new_parent = -1;
	size_t ports_num = bmem_get_size( c->ports ) / sizeof( sundog_midi_port );
	for( int i = 0; i < ports_num; i++ )
	{
	    if( c->ports[ i ].active )
	    {
		device_midi_port* port2 = (device_midi_port*)c->ports[ i ].device_specific;
		if( port2 && port2->already_opened && port2->link == pnum )
		{
		    if( new_parent == -1 )
		    {
			//Now port2 is parent
			bmem_copy( port2, port, sizeof( device_midi_port ) );
			new_parent = i;
		    }
		    else
		    {
			port2->link = new_parent;
		    }
		}
	    }
	}

	if( new_parent == -1 )
	{
	    //No linked ports. We can remove this parent:
    	    if( sd_port->flags & MIDI_PORT_READ )
    	    {
    		HMIDIIN in_dev = port->in_dev;
    		bmutex_lock( &d->in_callback_mutex );
    		port->in_dev = 0;
    		bmutex_unlock( &d->in_callback_mutex );
    		midiInStop( in_dev );
    		midiInReset( in_dev );
		midiInClose( in_dev );
	    }
    	    else
	    {
                if( d->out_events )
                {
                    bool no_events = 1;
                    size_t events = bmem_get_size( d->out_events ) / sizeof( device_midi_event );
                    for( int e = 0; e < events; e++ )
                    {
                        device_midi_event* evt = &d->out_events[ e ];
                        if( evt->evt.data && evt->dev == port->out_dev )
                        {
                            bmem_free( evt->evt.data );
                            evt->evt.data = 0;
                        }
                        if( evt->evt.data ) no_events = 0;
                    }
                    if( no_events )
                    {
                        bmem_free( d->out_events );
                        d->out_events = 0;
                    }
                }
		midiOutReset( port->out_dev ); //Turn any MIDI notes currently playing
		midiOutClose( port->out_dev );
	    }
	}

	bmutex_unlock( &d->out_thread_mutex );
    }
    
    bmem_free( sd_port->device_specific );
    sd_port->device_specific = 0;
    return 0;
}

sundog_midi_event* device_midi_client_get_event( sundog_midi_client* c, int pnum )
{
    if( pnum < 0 ) return 0;
    
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return 0;
    }
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    if( port == 0 ) return 0;
    
    if( port->evt_rp == port->evt_wp ) return 0; //No events
    sundog_midi_event* evt = &port->events[ port->evt_rp ];
    return evt;
    
    return 0;
}

int device_midi_client_next_event( sundog_midi_client* c, int pnum )
{
    if( pnum < 0 ) return 0;
    
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    if( port == 0 ) return 0;
    
    if( port->evt_rp == port->evt_wp ) return 0; //No events
    sundog_midi_event* evt = &port->events[ port->evt_rp ];
    uint new_evt_rp = ( port->evt_rp + 1 ) & ( MIDI_EVENTS - 1 );
    size_t p = evt->data - port->data;
    p += evt->size;
    uint new_data_rp = (uint)p & ( MIDI_BYTES - 1 );
    port->evt_rp = new_evt_rp;
    port->data_rp = new_data_rp;
    
    return 0;
}

int device_midi_client_send_event( sundog_midi_client* c, int pnum, sundog_midi_event* evt )
{
    if( pnum < 0 ) return 0;
    
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    if( port == 0 ) return 0;
    
    g_last_midi_out_activity = time_ticks_hires();
    
    bmutex_lock( &d->out_thread_mutex );
    int free_event = -1;
    if( d->out_events == 0 )
    {
	d->out_events = (device_midi_event*)bmem_new( sizeof( device_midi_event ) * 4 );
	bmem_zero( d->out_events );
	free_event = 0;
    }
    else
    {
	//Get free event:
	size_t events = bmem_get_size( d->out_events ) / sizeof( device_midi_event );
	for( int e = 0; e < events; e++ )
	{
	    if( d->out_events[ e ].evt.data == 0 )
	    {
		free_event = e;
		break;
	    }
	}
	if( free_event == -1 )
	{
	    //No free events. Resize the storage:
	    d->out_events = (device_midi_event*)bmem_resize( d->out_events, ( events + 4 ) * sizeof( device_midi_event ) );
	    bmem_set( &d->out_events[ events ], 4 * sizeof( device_midi_event ), 0 );
	    free_event = events;
	    events += 4;
	}
    }
    //Save the event:
    d->out_events[ free_event ].evt.t = evt->t;
    d->out_events[ free_event ].evt.size = evt->size;
    d->out_events[ free_event ].evt.data = (uchar*)bmem_new( evt->size );
    if( port->already_opened == 0 )
    {
	d->out_events[ free_event ].dev = port->out_dev;
    }
    else
    {
	device_midi_port* port2 = (device_midi_port*)c->ports[ port->link ].device_specific;
	if( port2 )
	    d->out_events[ free_event ].dev = port2->out_dev;
    }
    bmem_copy( d->out_events[ free_event ].evt.data, evt->data, evt->size );
    SetEvent( d->out_thread_event );
    bmutex_unlock( &d->out_thread_mutex );
    
    return 0;
}

#endif
