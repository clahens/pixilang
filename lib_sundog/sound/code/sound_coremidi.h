//CoreMIDI for iOS and OSX

#include <CoreMIDI/CoreMIDI.h>

#ifndef kCFCoreFoundationVersionNumber_iPhoneOS_4_2
    // NOTE: THIS IS NOT THE FINAL NUMBER
    // 4.2 is not out of beta yet, so took the beta 1 build number
    #define kCFCoreFoundationVersionNumber_iPhoneOS_4_2 550.47
#endif

#ifndef NOMIDI

bool g_midi_info = 0;

struct device_midi_client
{
    MIDIClientRef client;
};

struct device_midi_port
{
    MIDIPortRef port;
    MIDIEndpointRef endpoint; //Source or destination
    uchar data[ MIDI_BYTES ];
    sundog_midi_event events[ MIDI_EVENTS ];
    volatile uint data_rp;
    volatile uint data_wp;
    volatile uint evt_rp;
    volatile uint evt_wp;
};

void midi_in_callback( const MIDIPacketList* packetList, void* readProcRefCon, void* srcConnRefCon )
{
    g_last_midi_in_activity = time_ticks_hires();
    
    sundog_midi_client* c = (sundog_midi_client*)readProcRefCon;
    size_t pnum = (size_t)srcConnRefCon;
    if( (int)pnum < 0 ) return;
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    if( port == 0 ) return;
    
    MIDIPacket* packet = (MIDIPacket*)packetList->packet;
    int count = packetList->numPackets;
    for( int i = 0; i < count; i++ ) 
    {
	unsigned int empty_data = 0;
	if( port->data_wp + packet->length > MIDI_BYTES )
	    empty_data = MIDI_BYTES - port->data_wp;
	unsigned int can_write = ( port->data_rp - port->data_wp ) & ( MIDI_BYTES - 1 );
	if( can_write == 0 ) can_write = MIDI_BYTES;
	if( ( ( port->data_wp + can_write ) & ( MIDI_BYTES - 1 ) ) == port->data_rp ) can_write--;
	if( empty_data + packet->length <= can_write )
	{
	    if( ( ( port->evt_rp - port->evt_wp ) & ( MIDI_EVENTS - 1 ) ) != 1 )
	    {
		for( int b = 0; b < packet->length; b++ )
		{
		    port->data[ ( port->data_wp + b + empty_data ) & ( MIDI_BYTES - 1 ) ] = packet->data[ b ];
		}
		sundog_midi_event* evt = &port->events[ port->evt_wp ];
		double t = (double)packet->timeStamp * g_timebase_nanosec / 20000;
		unsigned long long tl = (unsigned long long)t;
		evt->t = (ticks_hr_t)tl;
		evt->size = packet->length;
		evt->data = port->data + ( ( port->data_wp + empty_data ) & ( MIDI_BYTES - 1 ) );
		uint new_data_wp = ( port->data_wp + packet->length + empty_data ) & ( MIDI_BYTES - 1 );
		port->data_wp = new_data_wp;
		uint new_evt_wp = ( port->evt_wp + 1 ) & ( MIDI_EVENTS - 1 );
		COMPILER_MEMORY_BARRIER();
		port->evt_wp = new_evt_wp;
	    }
	}
	packet = MIDIPacketNext( packet );
    }
}

void midi_notify_callback( const MIDINotification* message, void* refCon )
{
    sundog_midi_client* c = (sundog_midi_client*)refCon;
    switch( message->messageID )
    {
	case kMIDIMsgSetupChanged:
	    blog( "MIDI Setup Changed\n" );
	    break;
	case kMIDIMsgObjectRemoved:
	    {
		blog( "MIDI Object Removed:\n" );
		MIDIObjectAddRemoveNotification* msg = (MIDIObjectAddRemoveNotification*)message;
		
		bmutex_lock( &g_sundog_midi_mutex );
		
		for( int i = 0; i < bmem_get_size( c->ports ) / sizeof( sundog_midi_port ); i++ )
		{
		    sundog_midi_port* sd_port = &c->ports[ i ];
		    if( sd_port->active == 0 ) continue;
		    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
		    if( port && port->endpoint == msg->child )
		    {
			blog( " * port %d: %s\n", i, sd_port->dev_name );
			device_midi_client_close_port( c, i );
			sd_port->flags |= MIDI_NO_DEVICE;
		    }
		}
		
		bmutex_unlock( &g_sundog_midi_mutex );
	    }
	    break;
	case kMIDIMsgObjectAdded:
	    {
		blog( "MIDI Object Added:\n" );

		bmutex_lock( &g_sundog_midi_mutex );
		
		for( int i = 0; i < bmem_get_size( c->ports ) / sizeof( sundog_midi_port ); i++ )
		{
		    sundog_midi_port* sd_port = &c->ports[ i ];
		    if( sd_port->active == 0 ) continue;
		    if( sd_port->flags & MIDI_NO_DEVICE )
		    {
			blog( " * port %d: %s. Trying to reopen...\n", i, sd_port->dev_name );
			sundog_midi_client_reopen_port( c, i );
		    }
		}

		bmutex_unlock( &g_sundog_midi_mutex );
	    }
	    break;
	default:
	    blog( "MIDI Notify %d\n", message->messageID );
	    break;
    }
}

CFStringRef device_midi_get_endpoint_name( MIDIEndpointRef endpoint )
{
    CFMutableStringRef name = CFStringCreateMutable( NULL, 0 );
    CFStringRef ts;

    MIDIEntityRef entity = 0;
    MIDIEndpointGetEntity( endpoint, &entity );
    
    MIDIDeviceRef device = 0;
    MIDIEntityGetDevice( entity, &device );
    
    bool text = 0;
    
    if( device )
    {
	ts = 0;
	MIDIObjectGetStringProperty( device, kMIDIPropertyName, &ts );
	if( ts )
	{
	    text = 1;
	    CFStringAppend( name, ts );
	    CFRelease( ts );
	}
    }
    
    if( entity )
    {
	ts = 0;
	MIDIObjectGetStringProperty( entity, kMIDIPropertyName, &ts );
	if( ts )
	{
	    bool ignore = 0;
	    if( CFStringGetLength( name ) > 0 )
	    {
		if( CFStringCompare( name, ts, 0 ) == 0 )
		{
		    ignore = 1;
		}
	    }
	    if( ignore == 0 )
	    {
		if( text )
		{
		    CFStringAppend( name, CFSTR( " " ) );
		}
		text = 1;
		CFStringAppend( name, ts );
	    }
	    CFRelease( ts );
	}
    }
    
    ts = 0;
    MIDIObjectGetStringProperty( endpoint, kMIDIPropertyName, &ts );
    if( ts ) 
    {
	if( text )
	{
	    CFStringAppend( name, CFSTR( " " ) );
	}
	text = 1;
	CFStringAppend( name, ts );
	CFRelease( ts );
    }
    
    if( CFStringGetLength( name ) == 0 )
    {
	CFStringAppend( name, CFSTR( "Unnamed" ) );
    }

    return name;
}

int device_midi_client_open( sundog_midi_client* c, const utf8_char* name )
{
#ifdef IPHONE
    if( kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber_iPhoneOS_4_2 )
	return 0;
#endif
    
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_open_jack( c, name );
#endif
    
    if( g_midi_info )
    {
	{
	    CFStringRef endpoint_name;
	    char name_utf8[ 512 ];
	    int sources = MIDIGetNumberOfSources();
	    int dests = MIDIGetNumberOfDestinations();
	    blog( "ONLINE SOURCES: %d\n", sources );
	    blog( "ONLINE DESTINATIONS: %d\n", dests );
	    for( int j = 0; j < sources; j++ )
	    {
		MIDIEndpointRef source = MIDIGetSource( j );
		if( source )
		{
		    endpoint_name = 0;
		    endpoint_name = device_midi_get_endpoint_name( source );
		    if( endpoint_name == 0 ) continue;
		    CFStringGetCString( endpoint_name, name_utf8, 512, kCFStringEncodingUTF8 );
		    CFRelease( endpoint_name );
		    blog( "SOURCE %d: %s\n", j, name_utf8 );
		}
	    }
	    for( int j = 0; j < dests; j++ )
	    {
		MIDIEndpointRef dest = MIDIGetDestination( j );
		if( dest )
		{
		    endpoint_name = 0;
		    endpoint_name = device_midi_get_endpoint_name( dest );
		    if( endpoint_name == 0 ) continue;
		    CFStringGetCString( endpoint_name, name_utf8, 512, kCFStringEncodingUTF8 );
		    CFRelease( name );
		    blog( "DESTINATION %d: %s\n", j, name_utf8 );
		}
	    }
	}
	int devices = MIDIGetNumberOfDevices();
	for( int i = 0; i < devices; i++ )
	{
	    MIDIDeviceRef dev = MIDIGetDevice( i );
	    if( dev )
	    {
		CFStringRef endpoint_name;
		char name_utf8[ 512 ];
		MIDIObjectGetStringProperty( dev, kMIDIPropertyName, &endpoint_name );
		CFStringGetCString( endpoint_name, name_utf8, 512, kCFStringEncodingUTF8 );
		blog( "DEV %d: %s\n", i, name_utf8 );
		int ents = MIDIDeviceGetNumberOfEntities( dev );
		for( int e = 0; e < ents; e++ )
		{
		    MIDIEntityRef ent = MIDIDeviceGetEntity( dev, e );
		    if( ent )
		    {
			MIDIObjectGetStringProperty( ent, kMIDIPropertyName, &endpoint_name );
			if( endpoint_name == 0 ) continue;
			CFStringGetCString( endpoint_name, name_utf8, 512, kCFStringEncodingUTF8 );
			blog( "  ENTITY %d: %s\n", e, name_utf8 );
			int sources = MIDIEntityGetNumberOfSources( ent );
			for( int j = 0; j < sources; j++ )
			{
			    MIDIEndpointRef source = MIDIEntityGetSource( ent, j );
			    if( source )
			    {
				MIDIObjectGetStringProperty( source, kMIDIPropertyName, &endpoint_name );
				if( endpoint_name == 0 ) continue;
				CFStringGetCString( endpoint_name, name_utf8, 512, kCFStringEncodingUTF8 );
				CFRelease( endpoint_name );
				blog( "    SOURCE %d: %s\n", j, name_utf8 );
			    }
			}
			int dests = MIDIEntityGetNumberOfDestinations( ent );
			for( int j = 0; j < dests; j++ )
			{
			    MIDIEndpointRef dest = MIDIEntityGetSource( ent, j );
			    if( dest )
			    {
				MIDIObjectGetStringProperty( dest, kMIDIPropertyDisplayName, &endpoint_name );
				if( endpoint_name == 0 ) continue;
				CFStringGetCString( endpoint_name, name_utf8, 512, kCFStringEncodingUTF8 );
				CFRelease( endpoint_name );
				blog( "    DESTINATION %d: %s\n", j, name_utf8 );
			    }
			}
		    }
		}
	    }
	}
	g_midi_info = 0;
    }
    
    c->device_specific = bmem_new( sizeof( device_midi_client ) );
    device_midi_client* d = (device_midi_client*)c->device_specific;
    bmem_zero( d );

    OSStatus status = MIDIClientCreate( CFStringCreateWithCString( NULL, name, kCFStringEncodingUTF8 ), &midi_notify_callback, c, &d->client );
    if( status )
    {
	blog( "Error trying to create MIDI Client structure: %d\n", status );
	return -1;
    }
    
    return 0;
}

int device_midi_client_close( sundog_midi_client* c )
{
#ifdef IPHONE
    if( kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber_iPhoneOS_4_2 )
	return 0;
#endif
    
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_close_jack( c );
#endif
    
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    
    MIDIClientDispose( d->client );
    
    bmem_free( d );
    c->device_specific = 0;
    return 0;
}

int device_midi_client_get_devices( sundog_midi_client* c, utf8_char*** devices, uint flags )
{
#ifdef IPHONE
    if( kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber_iPhoneOS_4_2 )
	return 0;
#endif
    
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_get_devices_jack( c, devices, flags );
#endif
    
    int rv = 0;
    
    *devices = 0;
    
    CFStringRef name;
    char name_utf8[ 512 ];
    int sources = MIDIGetNumberOfSources();
    int dests = MIDIGetNumberOfDestinations();
    if( flags & MIDI_PORT_READ )
    {
	for( int j = 0; j < sources; j++ )
	{
	    MIDIEndpointRef source = MIDIGetSource( j );
	    if( source )
	    {
		name = 0;
		name = device_midi_get_endpoint_name( source );
		if( name == 0 ) continue;
		CFStringGetCString( name, name_utf8, 512, kCFStringEncodingUTF8 );
		CFRelease( name );
		if( *devices == 0 )
		{
		    *devices = (utf8_char**)bmem_new( sizeof( void* ) * 128 );
		}
		else
		{
		    if( rv + 1 > bmem_get_size( *devices ) / sizeof( void* ) )
		    {
			*devices = (utf8_char**)bmem_resize( *devices, sizeof( void* ) * ( rv + 1 ) * 2 );
		    }
		}
		(*devices)[ rv ] = (utf8_char*)bmem_new( bmem_strlen( name_utf8 ) + 1 );
		(*devices)[ rv ][ 0 ] = 0;
		bmem_strcat_resize( (*devices)[ rv ], name_utf8 );
		rv++;
	    }
	}
    }
    if( flags & MIDI_PORT_WRITE )
    {
	for( int j = 0; j < dests; j++ )
	{
	    MIDIEndpointRef dest = MIDIGetDestination( j );
	    if( dest )
	    {
		name = 0;
		name = device_midi_get_endpoint_name( dest );
		if( name == 0 ) continue;
		CFStringGetCString( name, name_utf8, 512, kCFStringEncodingUTF8 );
		CFRelease( name );
		if( *devices == 0 )
		{
		    *devices = (utf8_char**)bmem_new( sizeof( void* ) * 128 );
		}
		else
		{
		    if( ( rv + 1 ) > bmem_get_size( *devices ) / sizeof( void* ) )
		    {
			*devices = (utf8_char**)bmem_resize( *devices, sizeof( void* ) * ( rv + 1 ) * 2 );
		    }
		}
		(*devices)[ rv ] = (utf8_char*)bmem_new( bmem_strlen( name_utf8 ) + 1 );
		(*devices)[ rv ][ 0 ] = 0;
		bmem_strcat_resize( (*devices)[ rv ], name_utf8 );
		rv++;
	    }
	}
    }
        
    return rv;
}

int device_midi_client_open_port( sundog_midi_client* c, int pnum, const utf8_char* port_name, const utf8_char* dev_name, uint flags )
{
#ifdef IPHONE
    if( kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber_iPhoneOS_4_2 )
	return 0;
#endif
    
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK )
        return device_midi_client_open_port_jack( c, pnum, port_name, dev_name, flags );
#endif

    int rv = -1;
    
    OSStatus status;
    device_midi_client* d = (device_midi_client*)c->device_specific;
    if( d == 0 )
    {
	return -1;
    }
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    sd_port->device_specific = bmem_new( sizeof( device_midi_port ) );
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    bmem_zero( port );
    
    if( flags & MIDI_PORT_READ )
	status = MIDIInputPortCreate( d->client, CFStringCreateWithCString( NULL, port_name, kCFStringEncodingUTF8 ), midi_in_callback, c, &port->port );
    else 
	status = MIDIOutputPortCreate( d->client, CFStringCreateWithCString( NULL, port_name, kCFStringEncodingUTF8 ), &port->port );
    if( status )
    {
	blog( "Error trying to create MIDI port: %d\n", status );
	goto open_end;
    }
    port->endpoint = 0;
    if( bmem_strcmp( dev_name, "public port" ) == 0 )
    {
        //Connect this port to the port visible to others:
        utf8_char* ts = (utf8_char*)bmem_new( bmem_strlen( port_name ) + 128 );
        if( flags & MIDI_PORT_READ )
        {
            sprintf( ts, "%s", port_name );
            int idd = 'SDIn';
            for( int i = 0; i < bmem_strlen( ts ); i++ ) { idd += ts[ i ]; idd ^= ts[ i ]; }
            MIDIEndpointRef dest;
            status = MIDIDestinationCreate( d->client, CFStringCreateWithCString( NULL, ts, kCFStringEncodingUTF8 ), midi_in_callback, c, &dest );
            if( status )
            {
                blog( "Error trying to create MIDI Virtual Input: %d\n", status );
            }
            else 
            {
                MIDIObjectSetIntegerProperty( dest, kMIDIPropertyUniqueID, idd );
                rv = 0;
                port->endpoint = dest;
            }
        }
        else 
        {
            sprintf( ts, "%s", port_name );
            int idd = 'SDOu';
            for( int i = 0; i < bmem_strlen( ts ); i++ ) { idd += ts[ i ]; idd ^= ts[ i ]; }
            MIDIEndpointRef source;
            status = MIDISourceCreate( d->client, CFStringCreateWithCString( NULL, ts, kCFStringEncodingUTF8 ), &source );
            if( status )
            {
                blog( "Error trying to create MIDI Virtual Output: %d\n", status );
            }
            else 
            {
                MIDIObjectSetIntegerProperty( source, kMIDIPropertyUniqueID, idd );
                rv = 0;
                port->endpoint = source;
            }
        }
        bmem_free( ts );
    }
    else
    {
        //Connect this port to selected source or destination:
	CFStringRef name;
	char name_utf8[ 512 ];
	int sources = MIDIGetNumberOfSources();
	int dests = MIDIGetNumberOfDestinations();
	if( flags & MIDI_PORT_READ )
	{
	    for( int j = 0; j < sources; j++ )
	    {
		MIDIEndpointRef source = MIDIGetSource( j );
		if( source )
		{
		    name = 0;
		    name = device_midi_get_endpoint_name( source );
		    if( name == 0 ) continue;
		    CFStringGetCString( name, name_utf8, 512, kCFStringEncodingUTF8 );
		    CFRelease( name );
		    if( bmem_strcmp( name_utf8, dev_name ) == 0 )
		    {
			status = MIDIPortConnectSource( port->port, source, (void*)pnum );
			if( status )
			{
			    blog( "Error trying to connect source: %d\n", status );
			    goto open_end;
			}
			rv = 0;
			port->endpoint = source;
			break;
		    }
		}
	    }
	}
	else 
	{
	    for( int j = 0; j < dests; j++ )
	    {
		MIDIEndpointRef dest = MIDIGetDestination( j );
		if( dest )
		{
		    name = 0;
		    name = device_midi_get_endpoint_name( dest );
		    if( name == 0 ) continue;
		    CFStringGetCString( name, name_utf8, 512, kCFStringEncodingUTF8 );
		    CFRelease( name );
		    if( bmem_strcmp( name_utf8, dev_name ) == 0 )
		    {
			rv = 0;
			port->endpoint = dest;
			break;
		    }
		}
	    }
	}
    }
    
open_end:
    
    if( rv != 0 )
    {
	//Open error:
	bmem_free( port );
    }
    
    return rv;
}

int device_midi_client_close_port( sundog_midi_client* c, int pnum )
{
#ifdef IPHONE
    if( kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber_iPhoneOS_4_2 )
	return 0;
#endif
    
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
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    if( port == 0 ) return 0;

    MIDIPortDispose( port->port );
    
    if( bmem_strcmp( sd_port->dev_name, "public port" ) == 0 )
        MIDIEndpointDispose( port->endpoint );
    
    bmem_free( sd_port->device_specific );
    sd_port->device_specific = 0;
    return 0;
}

sundog_midi_event* device_midi_client_get_event( sundog_midi_client* c, int pnum )
{
#ifdef IPHONE
    if( kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber_iPhoneOS_4_2 )
	return 0;
#endif
    
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
#ifdef IPHONE
    if( kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber_iPhoneOS_4_2 )
	return 0;
#endif
    
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
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    if( port == 0 ) return -1;
    
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
#ifdef IPHONE
    if( kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber_iPhoneOS_4_2 )
	return 0;
#endif
    
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
    sundog_midi_port* sd_port = &c->ports[ pnum ];
    device_midi_port* port = (device_midi_port*)sd_port->device_specific;
    if( port == 0 ) return 0;
    
    g_last_midi_out_activity = time_ticks_hires();
    
    MIDIPacketList list;
    list.numPackets = 1;
    double cc = 1000000 / g_timebase_nanosec / (double)time_ticks_per_second_hires();
    double t = (double)evt->t * cc;
    unsigned long long tl = (unsigned long long)t;
    list.packet[ 0 ].timeStamp = (MIDITimeStamp)tl;
    size_t data_size = evt->size;
    size_t sent = 0;
    while( sent < data_size ) 
    { 
	size_t size = data_size - sent;
	if( size > 256 ) size = 256;
	list.packet[ 0 ].length = size;
	bmem_copy( list.packet[ 0 ].data, evt->data + sent, size );
	sent += size;
	MIDISend( port->port, port->endpoint, &list );
    }
    
    return 0;
}

#endif
