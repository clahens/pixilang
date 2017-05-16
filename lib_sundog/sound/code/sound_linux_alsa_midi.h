//ALSA MIDI for Linux

#ifndef NOMIDI

bool g_midi_info = 0;

struct device_midi_client
{
    snd_seq_t* 				handle;
    int 				queue;
    ticks_hr_t 				time_offset;
    bthread 				thread;
    int 				thread_control_pipe[ 2 ];
    volatile bool 			thread_exit_request;
};

struct device_midi_port
{
    int 				id;
    snd_seq_port_subscribe_t* 		subs;
    uchar 				data[ MIDI_BYTES ];
    sundog_midi_event 			events[ MIDI_EVENTS ];
    snd_midi_event_t* 			event_decoder;
    volatile 				uint data_rp;
    volatile 				uint data_wp;
    volatile 				uint evt_rp;
    volatile 				uint evt_wp;
};

void* midi_client_thread( void* user_data )
{
    sundog_midi_client* c = (sundog_midi_client*)user_data;
    if( c == 0 )
    {
        blog( "midi_client_thread(): null client\n" );
        return 0;
    }
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
        blog( "midi_client_thread(): null client (device_specific)\n" );
        return 0;
    }
    int npfd;
    struct pollfd* pfd;
    volatile bool reset_request;
    
    while( 1 )
    {
	reset_request = 0;
	npfd = snd_seq_poll_descriptors_count( d->handle, POLLIN );
	pfd = (struct pollfd*)bmem_new( ( npfd + 1 ) * sizeof( struct pollfd ) );
	snd_seq_poll_descriptors( d->handle, pfd, npfd, POLLIN );
	pfd[ npfd ].fd = d->thread_control_pipe[ 0 ];
	pfd[ npfd ].events = POLLIN;
	int rv = 0;
	while( d->thread_exit_request == 0 && reset_request == 0 )
	{
	    if( rv > 0 || poll( pfd, npfd + 1, 1000 ) > 0 ) 
	    {
		snd_seq_event_t* ev = 0;
		rv = snd_seq_event_input( d->handle, &ev );
		if( rv >= 0 && ev && ev->dest.client == snd_seq_client_id( d->handle ) )
		{
		    g_last_midi_in_activity = time_ticks_hires();
		    device_midi_port* port = 0;
		    for( int i = 0; i < (int)( bmem_get_size( c->ports ) / sizeof( sundog_midi_port ) ); i++ )
		    {
		        sundog_midi_port* sd_port = &c->ports[ i ];
		        if( sd_port )
		        {
		    	    port = (device_midi_port*)sd_port->device_specific;
			    if( port && port->id == ev->dest.port )
			    {
			        //Port found.
			        break;
			    }
			}
		    }
		    if( port )
		    {
			uchar decoder_buf[ 256 ];
			int data_size = snd_midi_event_decode( port->event_decoder, decoder_buf, 256, ev );
			if( data_size > 0 )
			{
			    /*printf( "%d >> ", rv );
			    for( int b = 0; b < data_size; b++ )
			    {
				printf( "%02x ", decoder_buf[ b ] );
			    }
			    printf( "\n" );*/
			    uint empty_data = 0;
			    if( port->data_wp + data_size > MIDI_BYTES )
				empty_data = MIDI_BYTES - port->data_wp;
			    uint can_write = ( port->data_rp - port->data_wp ) & ( MIDI_BYTES - 1 );
			    if( can_write == 0 ) can_write = MIDI_BYTES;
			    if( ( ( port->data_wp + can_write ) & ( MIDI_BYTES - 1 ) ) == port->data_rp ) can_write--;
			    if( empty_data + data_size <= can_write )
			    {
				if( ( ( port->evt_rp - port->evt_wp ) & ( MIDI_EVENTS - 1 ) ) != 1 )
				{
				    for( int b = 0; b < data_size; b++ )
				    {
					port->data[ ( port->data_wp + b + empty_data ) & ( MIDI_BYTES - 1 ) ] = decoder_buf[ b ];
				    }
				    sundog_midi_event* evt = &port->events[ port->evt_wp ];
				    evt->t = (ticks_hr_t)( ev->time.time.tv_nsec / ( 1000000000 / time_ticks_per_second_hires() ) ) + ev->time.time.tv_sec * time_ticks_per_second_hires();
				    evt->t += d->time_offset;
				    evt->size = data_size;
				    evt->data = port->data + ( ( port->data_wp + empty_data ) & ( MIDI_BYTES - 1 ) );
				    uint new_data_wp = ( port->data_wp + data_size + empty_data ) & ( MIDI_BYTES - 1 );
				    port->data_wp = new_data_wp;
				    uint new_evt_wp = ( port->evt_wp + 1 ) & ( MIDI_EVENTS - 1 );
				    COMPILER_MEMORY_BARRIER();
				    port->evt_wp = new_evt_wp;
				}
			    }
			}
		    }
		    snd_seq_free_event( ev );
		}
		if( pfd[ npfd ].revents & POLLIN )
		{
		    char buf = 0;
		    read( d->thread_control_pipe[ 0 ], &buf, 1 );
		    if( buf == 1 ) reset_request = 1;
		}
	    }
	}
	bmem_free( pfd );
	if( reset_request == 0 ) break;
    }

    return 0;
}

void find_midi_client( sundog_midi_client* c, const utf8_char* dev_name, int* client_id, int* port_id )
{
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 ) return;
    
    *client_id = -1;
    *port_id = -1;
    
    snd_seq_client_info_t* cinfo = 0;
    snd_seq_client_info_alloca( &cinfo );
    snd_seq_client_info_set_client( cinfo, -1 );
    snd_seq_port_info_t* pinfo = 0;
    snd_seq_port_info_alloca( &pinfo );
    while( snd_seq_query_next_client( d->handle, cinfo ) >= 0 ) 
    {
	int cid = snd_seq_client_info_get_client( cinfo );
	snd_seq_port_info_set_client( pinfo, cid );
	snd_seq_port_info_set_port( pinfo, -1 );
	while( snd_seq_query_next_port( d->handle, pinfo ) >= 0 ) 
	{
	    int pid = snd_seq_port_info_get_port( pinfo );
	    const char* port_name = snd_seq_port_info_get_name( pinfo );
	    char ts[ 32 ];
	    sprintf( ts, "%d:%d", cid, pid );
	    if( bmem_strcmp( ts, dev_name ) == 0 )
	    {
		*client_id = cid;
		*port_id = pid;
		return;
	    }
	    char* port_name2 = (char*)bmem_new( bmem_strlen( port_name ) + 1 );
	    port_name2[ 0 ] = 0;
	    bmem_strcat_resize( port_name2, port_name );
	    for( int i = bmem_strlen( port_name2 ) - 1; i >= 0; i-- )
	    {
		if( port_name2[ i ] == ' ' ) 
		    port_name2[ i ] = 0;
		else 
		    break;
	    }
	    if( bmem_strcmp( port_name2, dev_name ) == 0 )
	    {
		*client_id = cid;
		*port_id = pid;
		bmem_free( port_name2 );
		return;
	    }
	    bmem_free( port_name2 );
	}
    }
}

int device_midi_client_open( sundog_midi_client* c, const utf8_char* name )
{
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_open_jack( c, name );
#endif

    c->device_specific = bmem_new( sizeof( device_midi_client ) );
    device_midi_client* d = (device_midi_client*)c->device_specific;
    bmem_zero( d );

    //Open client:
    int err = snd_seq_open( &d->handle, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK );
    if( err < 0 )
    {
	blog( "snd_seq_open() error %d\n", err );
	bmem_free( c->device_specific );
	c->device_specific = 0;	
	return -1;
    }
    snd_seq_set_client_name( d->handle, name );
    d->queue = snd_seq_alloc_named_queue( d->handle, "queue" );
    snd_seq_start_queue( d->handle, d->queue, 0 );
    snd_seq_drain_output( d->handle );
    snd_seq_sync_output_queue( d->handle );
    d->time_offset = time_ticks_hires();
    
    //Open client thread:
    if( pipe( d->thread_control_pipe ) )
    {
	blog( "pipe() error %d\n", errno );
	return -1;
    }
    bthread_create( &d->thread, midi_client_thread, c, 0 );
    
    if( g_midi_info )
    {
	snd_seq_client_info_t* cinfo = 0;
	snd_seq_client_info_alloca( &cinfo );
	snd_seq_client_info_set_client( cinfo, -1 );
	snd_seq_port_info_t* pinfo = 0;
	snd_seq_port_info_alloca( &pinfo );
	while( snd_seq_query_next_client( d->handle, cinfo ) >= 0 ) 
	{
	    int id = snd_seq_client_info_get_client( cinfo );
	    const char* name = snd_seq_client_info_get_name( cinfo );
	    blog( "CLIENT %d: %s\n", id, name );
	    snd_seq_port_info_set_client( pinfo, id );
	    snd_seq_port_info_set_port( pinfo, -1 );
	    while( snd_seq_query_next_port( d->handle, pinfo ) >= 0 ) 
	    {
		int port_id = snd_seq_port_info_get_port( pinfo );
		const char* port_name = snd_seq_port_info_get_name( pinfo );
		blog( "  PORT %d: %s\n", port_id, port_name );
	    }
	}
	g_midi_info = 0;
    }
    
    return 0;
}

int device_midi_client_close( sundog_midi_client* c )
{
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_close_jack( c );
#endif

    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    if( d->handle == 0 ) return -1;
    
    //Close thread:
    d->thread_exit_request = 1;
    char buf = 0;
    write( d->thread_control_pipe[ 1 ], &buf, 1 );
    bthread_destroy( &d->thread, 1000 );
    close( d->thread_control_pipe[ 0 ] );
    close( d->thread_control_pipe[ 1 ] );
    
    //Close client:
    snd_seq_free_queue( d->handle, d->queue );
    int err = snd_seq_close( d->handle );
    if( err < 0 )
    {
	blog( "snd_seq_close() error %d\n", err );
	return -1;
    }
        
    bmem_free( d );
    c->device_specific = 0;
    return 0;
}

int device_midi_client_get_devices( sundog_midi_client* c, utf8_char*** devices, uint flags )
{
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_get_devices_jack( c, devices, flags );
#endif

    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return 0;
    }
    if( d->handle == 0 ) return 0;
    
    int rv = 0;
    
    *devices = 0;
    
    snd_seq_client_info_t* cinfo = 0;
    snd_seq_client_info_alloca( &cinfo );
    snd_seq_client_info_set_client( cinfo, -1 );
    snd_seq_port_info_t* pinfo = 0;
    snd_seq_port_info_alloca( &pinfo );
    while( snd_seq_query_next_client( d->handle, cinfo ) >= 0 ) 
    {
	int cid = snd_seq_client_info_get_client( cinfo );
	snd_seq_port_info_set_client( pinfo, cid );
	snd_seq_port_info_set_port( pinfo, -1 );
	while( snd_seq_query_next_port( d->handle, pinfo ) >= 0 ) 
	{
	    if( *devices == 0 )
	    {
		*devices = (utf8_char**)bmem_new( sizeof( void* ) * 128 );
	    }
	    else
	    {
		if( rv > (int)( bmem_get_size( *devices ) / sizeof( void* ) ) )
		{
		    *devices = (utf8_char**)bmem_resize( *devices, sizeof( void* ) * rv * 2 );
		}
	    }
	    
	    const char* port_name = snd_seq_port_info_get_name( pinfo );
	    int caps = snd_seq_port_info_get_capability( pinfo );
	    bool ignore = 1;
	    if( ( flags & MIDI_PORT_READ ) && ( caps & SND_SEQ_PORT_CAP_READ ) ) ignore = 0;
	    if( ( flags & MIDI_PORT_WRITE ) && ( caps & SND_SEQ_PORT_CAP_WRITE ) ) ignore = 0;
	    if( ignore == 0 )
	    {
		rv++;
		char* port_name2 = (char*)bmem_new( bmem_strlen( port_name ) + 1 );
		port_name2[ 0 ] = 0;
		bmem_strcat_resize( port_name2, port_name );
		for( int i = bmem_strlen( port_name2 ) - 1; i >= 0; i-- )
		{
		    if( port_name2[ i ] == ' ' ) 
			port_name2[ i ] = 0;
		    else 
			break;
		}
		(*devices)[ rv - 1 ] = port_name2;
	    }
	}
    }
    
    return rv;
}

int device_midi_client_open_port( sundog_midi_client* c, int pnum, const utf8_char* port_name, const utf8_char* dev_name, uint flags )
{
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_open_port_jack( c, pnum, port_name, dev_name, flags );
#endif

    int rv = 0;
    
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    if( d->handle == 0 ) return -1;
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    sd_port->device_specific = bmem_new( sizeof( device_midi_port ) );
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    bmem_zero( port );
    utf8_char* port_name_long = (utf8_char*)bmem_new( bmem_get_size( c->name ) + 1 + bmem_strlen( port_name ) + 1 );
    port_name_long[ 0 ] = 0;
    bmem_strcat_resize( port_name_long, c->name );
    bmem_strcat_resize( port_name_long, " " );
    bmem_strcat_resize( port_name_long, port_name );
    
    while( 1 )
    {
	uint dflags = 0;
	if( flags & MIDI_PORT_READ )
	    dflags |= SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;
	else 
	    dflags |= SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;
	port->id = snd_seq_create_simple_port( d->handle, port_name_long, dflags, SND_SEQ_PORT_TYPE_MIDI_GENERIC );
	if( port->id < 0 ) 
	{
	    blog( "snd_seq_create_simple_port error %d\n", port->id );
	    rv = -1;
	    break;
	}
	int client_id = -1;
	int port_id = -1;
	find_midi_client( c, dev_name, &client_id, &port_id );
	if( client_id >= 0 && port_id >= 0 )
	{
	    snd_seq_addr_t sender;
	    snd_seq_addr_t dest;
	    if( flags & MIDI_PORT_READ )
	    {
		sender.client = client_id;
		sender.port = port_id;
		dest.client = snd_seq_client_id( d->handle );
		dest.port = port->id;
	    }
	    else 
	    {
		sender.client = snd_seq_client_id( d->handle );
		sender.port = port->id;
		dest.client = client_id;
		dest.port = port_id;
	    }
	    snd_seq_port_subscribe_alloca( &port->subs );
	    snd_seq_port_subscribe_set_sender( port->subs, &sender );
	    snd_seq_port_subscribe_set_dest( port->subs, &dest );
	    snd_seq_port_subscribe_set_queue( port->subs, d->queue );
	    snd_seq_port_subscribe_set_time_update( port->subs, 1 );
	    snd_seq_port_subscribe_set_time_real( port->subs, 1 );
	    int v = snd_seq_subscribe_port( d->handle, port->subs );
	    if( v < 0 )
	    {
		snd_seq_delete_simple_port( d->handle, port->id );
		blog( "snd_seq_subscribe_port error %d\n", v );
		rv = -1;
		break;
	    }
	}
	
	snd_midi_event_new( 256, &port->event_decoder );
	
	char buf = 1; //reinit the thread
	write( d->thread_control_pipe[ 1 ], &buf, 1 );
	
	break;
    }
    
    bmem_free( port_name_long );
    
    if( rv )
    {
	if( port )
	{
	    if( port->id >= 0 )
		snd_seq_delete_simple_port( d->handle, port->id );
	    bmem_free( port );
	    sd_port->device_specific = 0;
	}
    }
    
    return rv;
}

int device_midi_client_close_port( sundog_midi_client* c, int pnum )
{
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_close_port_jack( c, pnum );
#endif

    if( pnum < 0 ) return 0;
    
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    if( d->handle == 0 ) return -1;
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    if( port == 0 ) return 0;
    
    snd_seq_delete_simple_port( d->handle, port->id );
    
    snd_midi_event_free( port->event_decoder );

    char buf = 1; //reinit the thread
    write( d->thread_control_pipe[ 1 ], &buf, 1 );
    
    bmem_free( sd_port->device_specific );
    sd_port->device_specific = 0;
    return 0;
}

sundog_midi_event* device_midi_client_get_event( sundog_midi_client* c, int pnum )
{
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_get_event_jack( c, pnum );
#endif

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
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_next_event_jack( c, pnum );
#endif

    if( pnum < 0 ) return 0;
    
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    if( d->handle == 0 ) return -1;
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
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_send_event_jack( c, pnum, evt );
#endif

    if( pnum < 0 ) return 0;
    
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    if( d->handle == 0 ) return -1;
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    if( port == 0 ) return 0;
    
    g_last_midi_out_activity = time_ticks_hires();
    
    snd_seq_event_t alsa_event;
    size_t data_size = evt->size;
    size_t sent = 0;
    while( sent < data_size ) 
    { 
	size_t size = data_size - sent;
	snd_seq_ev_clear( &alsa_event );
	long s = snd_midi_event_encode( port->event_decoder, evt->data + sent, size, &alsa_event );
	if( s > 0 )
	{
	    if( alsa_event.type != SND_SEQ_EVENT_NONE ) 
	    {
		snd_seq_ev_set_source( &alsa_event, port->id );
		snd_seq_ev_set_subs( &alsa_event );
		snd_seq_real_time t;
		ticks_hr_t evt_t = evt->t - d->time_offset;
		long long nsec = ( (double)( evt_t % time_ticks_per_second_hires() ) / (double)time_ticks_per_second_hires() ) * (double)1000000;
		t.tv_sec = evt_t / time_ticks_per_second_hires();
		t.tv_nsec = (unsigned int)nsec;
		snd_seq_ev_schedule_real( &alsa_event, d->queue, 0, &t );
		snd_seq_event_output( d->handle, &alsa_event );
		snd_seq_drain_output( d->handle );
	    }
	    else
	    {
		blog( "send_event: SND_SEQ_EVENT_NONE\n" );
	    }
	    sent += s;
	} 
	else
	{
	    blog( "snd_midi_event_encode() error %d\n", (int)s );
	    break;
	}
    }

    return 0;
}

#endif
