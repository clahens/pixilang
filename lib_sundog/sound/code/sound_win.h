//################################
//## WINDOWS                    ##
//################################

enum 
{
#ifndef WINCE
    SDRIVER_DSOUND,
#ifndef NOASIO
    SDRIVER_ASIO,
#endif
#endif
    SDRIVER_MMSOUND,
    NUMBER_OF_SDRIVERS
};

const utf8_char* g_sdriver_infos[] =
{
#ifndef WINCE
    "DirectSound",
#ifndef NOASIO    
    "ASIO",
#endif
#endif
    "Waveform Audio"
};

const utf8_char* g_sdriver_names[] =
{
#ifndef WINCE
    "dsound",
#ifndef NOASIO
    "asio",
#endif
#endif
    "mmsound"
};

int g_sound_driver = 0;

int g_default_buffer_size = 4096;
int g_buffer_size = 0;
int g_frame_size = 0;
void* g_sound_buffer = 0;

//################################
//## WINDOWS DIRECT SOUND       ##
//################################

#ifndef WINCE

#include <dsound.h>
#define NUMEVENTS 2
LPDIRECTSOUND           	lpds = 0;
LPDIRECTSOUNDCAPTURE		lpdsCapture = 0;
LPDIRECTSOUNDNOTIFY     	lpdsNotify;
WAVEFORMATEX 			wfx;
DSBPOSITIONNOTIFY       	rgdsbpn[ NUMEVENTS ];
HANDLE                  	rghEvent[ NUMEVENTS ] = { 0 };
DSBUFFERDESC            	ds_buf_desc;
LPDIRECTSOUNDBUFFER     	ds_buf = 0;
LPDIRECTSOUNDBUFFER     	ds_buf_primary = 0;
DSCBUFFERDESC            	ds_input_buf_desc;
LPDIRECTSOUNDCAPTUREBUFFER	ds_input_buf = 0;
uint				ds_input_rp = 0;

//DirectSound output thread:

HANDLE ds_output_thread = 0;

void StreamToBuffer( uint evtNum )
{
    if( ds_buf == 0 ) return;

    uint offset = rgdsbpn[ ( evtNum + NUMEVENTS / 2 ) % NUMEVENTS ].dwOffset;
    uint size = ds_buf_desc.dwBufferBytes / NUMEVENTS;

    void* lockPtr1 = 0;
    void* lockPtr2 = 0;
    DWORD lockBytes1 = 0;
    DWORD lockBytes2 = 0;

    for( int a = 0; a < 2; a++ )
    {
	HRESULT res = ds_buf->Lock( 
    	    offset,           // Offset of lock start
    	    size,             // Number of bytes to lock
    	    &lockPtr1,        // Address of lock start
    	    &lockBytes1,      // Count of bytes locked
    	    &lockPtr2,        // Address of wrap around
    	    &lockBytes2,      // Count of wrap around bytes
    	    0 );              // Flags
	if( a == 0 && res == DSERR_BUFFERLOST )
	{
	    ds_buf->Restore();
	    continue;
	}
	if( res != DS_OK ) return;
	break;
    }
    
    //Write data to the locked buffer:

    g_snd.out_time = time_ticks_hires() + ( ( (uint64)g_buffer_size * (uint64)time_ticks_per_second_hires() ) / (uint64)g_snd.freq );
    g_snd.out_frames = lockBytes1 / ( 2 * g_snd.out_channels );
    get_input_data( g_snd.out_frames );
    if( g_snd.out_type == sound_buffer_int16 )
    {
        g_snd.out_buffer = lockPtr1;
        main_sound_output_callback( &g_snd, 0 );
    }
    if( g_snd.out_type == sound_buffer_float32 )
    {
        g_snd.out_buffer = g_sound_buffer;
        main_sound_output_callback( &g_snd, 0 );
        float* fb = (float*)g_sound_buffer;
        signed short* sb = (signed short*)lockPtr1;
        for( unsigned int i = 0; i < g_snd.out_frames * g_snd.out_channels; i++ )
        {
	    FLOAT32_TO_INT16( sb[ i ], fb[ i ] );
        }
    }
    
    ds_buf->Unlock( lockPtr1, lockBytes1, lockPtr2, lockBytes2 );

    return;
}

volatile int ds_output_exit_request = 0;

DWORD __stdcall ds_output_thread_body( void* par )
{
    uint evt = -1;
    while( ds_output_exit_request == 0 )
    {
	DWORD dwEvt = MsgWaitForMultipleObjects(
	    NUMEVENTS, // How many possible events
	    rghEvent, // Location of handles
	    FALSE, // Wait for all?
	    200, // How long to wait
	    QS_ALLINPUT ); // Any message is an event

	dwEvt -= WAIT_OBJECT_0;

	// If the event was set by the buffer, there's input
	// to process. 

	if( dwEvt < NUMEVENTS && dwEvt != evt ) 
	{
	    evt = dwEvt;
	    StreamToBuffer( evt ); // copy data to output stream
	}
    }
    ds_output_exit_request = 0;
    return 0;
}

HANDLE ds_input_thread = 0;
volatile int ds_input_exit_request = 0;

DWORD __stdcall ds_input_thread_body( void* par )
{
    while( ds_input_exit_request == 0 )
    {
	uint c, r;
        ds_input_buf->GetCurrentPosition( (DWORD*)&c, (DWORD*)&r );
        if( r != ds_input_rp )
        {
    	    int size = r - ds_input_rp;
    	    if( size < 0 ) size += ds_input_buf_desc.dwBufferBytes;

    	    void* lockInPtr1 = 0;
    	    void* lockInPtr2 = 0;
    	    DWORD lockInBytes1 = 0;
    	    DWORD lockInBytes2 = 0;
    	    HRESULT res = ds_input_buf->Lock(
        	ds_input_rp,      // Offset of lock start
        	size,             // Number of bytes to lock
        	&lockInPtr1,      // Address of lock start
        	&lockInBytes1,    // Count of bytes locked
        	&lockInPtr2,      // Address of wrap around
        	&lockInBytes2,    // Count of wrap around bytes
        	0 );              // Flags
    	    if( res == DS_OK )
    	    {
    		for( int lockNum = 0; lockNum < 2; lockNum++ )
    		{
    		    int size;
    		    char* ptr;
    		    if( lockNum == 0 )
    		    {
    			size = lockInBytes1;
    			ptr = (char*)lockInPtr1;
    		    }
    		    else
    		    {
    			size = lockInBytes2;
    			ptr = (char*)lockInPtr2;
    		    }
    		    if( size == 0 ) continue;
    		    int ds_frame_size = 2 * g_snd.in_channels;
    		    int frame_size = 0;
    		    if( g_snd.in_type == sound_buffer_int16 )
    			frame_size = 2 * g_snd.in_channels;
    		    if( g_snd.in_type == sound_buffer_float32 )
    			frame_size = 4 * g_snd.in_channels;
    		    size /= ds_frame_size;
    		    while( size > 0 )
    		    {
    			int size2 = size;
        		if( g_sound_input_buffer_wp + size2 > g_sound_input_buffer_size )
            		    size2 = g_sound_input_buffer_size - g_sound_input_buffer_wp;
        		char* buf_ptr = g_sound_input_buffer + g_sound_input_buffer_wp * frame_size;
        		bmem_copy( buf_ptr, ptr, size2 * ds_frame_size );
        		if( g_snd.in_type == sound_buffer_float32 )
        		{
            		    //Convert from INT16 to FLOAT32
            		    float* fb = (float*)buf_ptr;
            		    signed short* sb = (signed short*)buf_ptr;
            		    for( int i = size2 * g_snd.in_channels - 1; i >= 0; i-- )
            		    INT16_TO_FLOAT32( fb[ i ], sb[ i ] );
        		}
        		volatile uint new_wp = ( g_sound_input_buffer_wp + size2 ) & ( g_sound_input_buffer_size - 1 );
        		g_sound_input_buffer_wp = new_wp;
        		size -= size2;
        		ptr += size2 * ds_frame_size;
    		    }
    		}

    		ds_input_buf->Unlock( lockInPtr1, lockInBytes1, lockInPtr2, lockInBytes2 );
    	    }

    	    ds_input_rp = r;
        }
        else Sleep( 1 );
    }
    ds_input_exit_request = 0;
    return 0;
}

LPGUID guids[ 128 ];
int guids_num = 0;
LPGUID input_guids[ 128 ];
int input_guids_num = 0;

BOOL CALLBACK DSEnumCallback (
    LPGUID GUID,
    LPCSTR Description,
    LPCSTR Module,
    VOID* Context
)
{
    blog( "Found output device %d: %s\n", guids_num, Description );
    guids[ guids_num ] = GUID;
    guids_num++;
    return 1;
}

BOOL CALLBACK DSInputEnumCallback (
    LPGUID GUID,
    LPCSTR Description,
    LPCSTR Module,
    VOID* Context
)
{
    blog( "Found input device %d: %s\n", input_guids_num, Description );
    input_guids[ input_guids_num ] = GUID;
    input_guids_num++;
    return 1;
}

#endif

#ifndef WINCE
void dsound_close( bool input )
{
    if( input )
    {
	if( lpdsCapture ) lpdsCapture->Release(); lpdsCapture = 0;
    }
    else
    {
	if( lpdsNotify ) lpdsNotify->Release(); lpdsNotify = 0;
	if( ds_buf ) ds_buf->Release(); ds_buf = 0;
	if( ds_buf_primary ) ds_buf_primary->Release(); ds_buf_primary = 0;
	if( lpds ) lpds->Release(); lpds = 0;
    }
}
#endif

int device_sound_stream_init_dsound( bool input )
{
#ifdef WINCE
    return 1;
#else
    int sound_dev;
    const utf8_char* input_label = "";
    HRESULT res;

    if( input == 0 ) blog( "DSOUND init...\n" );    

    if( input )
    {
	if( lpdsCapture ) return 0; //Already open
	input_label = " INPUT";
	input_guids_num = 0;
	sound_dev = profile_get_int_value( KEY_AUDIODEVICE_IN, -1, 0 );	
	ds_input_exit_request = 0;
	g_sound_input_buffer_wp = 0;
        g_sound_input_buffer_rp = 0;
    }
    else
    {
	if( lpds ) return 0; //Already open
	guids_num = 0;
	sound_dev = profile_get_int_value( KEY_AUDIODEVICE, -1, 0 );
	ds_output_exit_request = 0;
    }
    
    HWND hWnd = GetForegroundWindow();
    if( hWnd == NULL )
    {
        hWnd = GetDesktopWindow();
    }
    LPVOID EnumContext = 0;
    LPCGUID guid = 0;
    if( input )
    {
	DirectSoundCaptureEnumerate( DSInputEnumCallback, EnumContext );
	if( sound_dev >= 0 && sound_dev < input_guids_num )
	    guid = input_guids[ sound_dev ];
	res = DirectSoundCaptureCreate( guid, &lpdsCapture, NULL );
	if( res != DS_OK )
	{
	    blog( "DSOUND INPUT: DirectSoundCreate error %d\n", res );
    	    MessageBox( hWnd, "DSOUND INPUT: DirectSoundCreate error", "Error", MB_OK );
	    lpdsCapture = 0;
    	    return 1;
	}

	// Create capture buffer:

	memset( &ds_input_buf_desc, 0, sizeof( DSCBUFFERDESC ) ); 
	ds_input_buf_desc.dwSize = sizeof( DSCBUFFERDESC );
	ds_input_buf_desc.dwBufferBytes = ds_buf_desc.dwBufferBytes;
	ds_input_buf_desc.lpwfxFormat = &wfx;
	res = lpdsCapture->CreateCaptureBuffer( &ds_input_buf_desc, &ds_input_buf, NULL );
	if( res != DS_OK )
	{
    	    blog( "DSOUND INPUT: Create capture buffer error %d\n", res );
	    MessageBox( hWnd, "DSOUND INPUT: Create capture buffer error", "Error", MB_OK );
	    dsound_close( input );
	    return 1;
	}
	ds_input_buf->GetCurrentPosition( 0, (DWORD*)&ds_input_rp );

	// Create input thread:
    
	ds_input_thread = CreateThread( 0, 1024 * 16, &ds_input_thread_body, &g_snd, 0, 0 );
	SetThreadPriority( ds_input_thread, THREAD_PRIORITY_TIME_CRITICAL );

	// Start:
	
	ds_input_buf->Start( DSCBSTART_LOOPING );	
	
	if( 0 )
	{
	    ticks_t t = time_ticks();
	    int prev_c = -11111;
	    int prev_r = -11111;
	    while( 1 )
	    {
		int c, r;
                ds_input_buf->GetCurrentPosition( (DWORD*)&c, (DWORD*)&r );
            	int cc = ( c * 32 ) / ds_input_buf_desc.dwBufferBytes;
            	int rr = ( r * 32 ) / ds_input_buf_desc.dwBufferBytes;
                if( cc != prev_c || rr != prev_r )
                {
            	    char ts[ 34 ];
            	    ts[ 32 ] = 0;
            	    ts[ 33 ] = 0;
            	    bmem_set( ts, 32, '.' );
            	    ts[ cc ] = 'C';
            	    if( cc == rr ) ts[ cc ] = '*';
            		else ts[ rr ] = 'R';
            	    printf( "%s %d\n", ts );
            	    prev_c = cc;
            	    prev_r = rr;
                }
		if( time_ticks() - t > time_ticks_per_second() ) break;
	    }
	}
    }
    else
    {
	DirectSoundEnumerate( DSEnumCallback, EnumContext );    
	if( sound_dev >= 0 && sound_dev < guids_num )
	    guid = guids[ sound_dev ];
	blog( "DSOUND: selected device: %d\n", sound_dev );
	res = DirectSoundCreate( guid, &lpds, NULL );
	if( res != DS_OK )
	{
	    blog( "DSOUND: DirectSoundCreate error %d\n", res );
    	    MessageBox( hWnd, "DSOUND: DirectSoundCreate error", "Error", MB_OK );
	    lpds = 0;
    	    return 1;
	}
	res = lpds->SetCooperativeLevel( hWnd, DSSCL_PRIORITY );
	if( res != DS_OK )
	{
	    blog( "DSOUND: SetCooperativeLevel error %d\n", res );
	    MessageBox( hWnd, "DSOUND: SetCooperativeLevel error", "Error", MB_OK );
	    dsound_close( input );
	    return 1;
	}

	memset( &wfx, 0, sizeof( WAVEFORMATEX ) ); 
	wfx.wFormatTag = WAVE_FORMAT_PCM; 
        wfx.nChannels = g_snd.out_channels; 
	wfx.nSamplesPerSec = g_snd.freq; 
        wfx.wBitsPerSample = 16; 
        wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	// Primary buffer:
    
	memset( &ds_buf_desc, 0, sizeof( DSBUFFERDESC ) ); 
	ds_buf_desc.dwSize = sizeof( DSBUFFERDESC ); 
	ds_buf_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
	res = lpds->CreateSoundBuffer( &ds_buf_desc, &ds_buf_primary, NULL );
	if( res != DS_OK )
	{
	    blog( "DSOUND: Create primary buffer error %d\n", res );
	    MessageBox( hWnd, "DSOUND: Create primary buffer error", "Error", MB_OK );
	    dsound_close( input );
	    return 1;
	}    
	ds_buf_primary->SetFormat( &wfx );

	// Secondary buffer:
    
	memset( &ds_buf_desc, 0, sizeof( DSBUFFERDESC ) ); 
	ds_buf_desc.dwSize = sizeof( DSBUFFERDESC ); 
	ds_buf_desc.dwFlags = 
	    ( 0
	    | DSBCAPS_GETCURRENTPOSITION2 // Always a good idea
	    | DSBCAPS_STICKYFOCUS
	    | DSBCAPS_GLOBALFOCUS // Allows background playing
	    | DSBCAPS_CTRLPOSITIONNOTIFY
	    );
	ds_buf_desc.dwBufferBytes = g_buffer_size * 2 * g_snd.out_channels * 2;
	ds_buf_desc.lpwfxFormat = &wfx;
	res = lpds->CreateSoundBuffer( &ds_buf_desc, &ds_buf, NULL );
	if( res != DS_OK )
	{
    	    blog( "DSOUND: Create secondary buffer error %d\n", res );
	    MessageBox( hWnd, "DSOUND: Create secondary buffer error", "Error", MB_OK );
	    dsound_close( input );
	    return 1;
	}
    
        // Create buffer events:

        if( rghEvent[ 0 ] == 0 )
	{    
	    for( int i = 0; i < NUMEVENTS; i++ )
	    {
		rghEvent[ i ] = CreateEvent( NULL, FALSE, FALSE, NULL );
    		if( rghEvent[ i ] == NULL ) 
		{
		    blog( "DSOUND: Create event error\n" );
		    MessageBox( hWnd, "DSOUND: Create event error", "Error", MB_OK );
		    for( int ii = 0; ii < i; ii++ ) { CloseHandle( rghEvent[ ii ] ); rghEvent[ ii ] = 0; }
		    dsound_close( input );
		    return 1;
		}
	    }
	}
	for( int i = 0; i < NUMEVENTS; i++ )
	{
	    rgdsbpn[ i ].dwOffset = ( ds_buf_desc.dwBufferBytes / NUMEVENTS ) * i;
	    rgdsbpn[ i ].hEventNotify = rghEvent[ i ];
	}
	res = ds_buf->QueryInterface( IID_IDirectSoundNotify, (VOID **)&lpdsNotify );
	if( res != DS_OK )
	{
    	    blog( "DSOUND: QueryInterface error %d\n", res );
    	    MessageBox( hWnd, "DSOUND: QueryInterface error", "Error", MB_OK );
	    dsound_close( input );
    	    return 1;
	}    
	res = lpdsNotify->SetNotificationPositions( NUMEVENTS, rgdsbpn );
	if( res != DS_OK )
	{
	    blog( "DSOUND: SetNotificationPositions error %d\n", res );
	    MessageBox( hWnd, "DSOUND: SetNotificationPositions error", "Error", MB_OK );
	    dsound_close( input );
	    return 1;
	}

	// Create output thread:

	blog( "DSOUND thread starting...\n" );    
	ds_output_thread = CreateThread( 0, 1024 * 64, &ds_output_thread_body, &g_snd, 0, 0 );
	SetThreadPriority( ds_output_thread, THREAD_PRIORITY_TIME_CRITICAL );
    	
	// Play:

	ds_buf->Play( 0, 0, DSBPLAY_LOOPING );
	blog( "DSOUND play\n" );    
    }
    
    return 0;
#endif
}

//################################
//## Waveform Audio (MMSYSTEM)  ##
//################################

#define MM_USE_THREAD
#define MM_MAX_BUFFERS	2

HWAVEOUT		g_waveOutStream = 0;
HANDLE			g_waveOutThread = 0;
DWORD			g_waveOutThreadID = 0;
volatile int		g_waveOutExitRequest = 0;
WAVEHDR			g_outBuffersHdr[ MM_MAX_BUFFERS ];

HWAVEIN			g_waveInStream = 0;
HANDLE			g_waveInThread = 0;
DWORD			g_waveInThreadID = 0;
volatile int		g_waveInExitRequest = 0;
WAVEHDR			g_inBuffersHdr[ MM_MAX_BUFFERS ];

void WaveOutSendBuffer( WAVEHDR* waveHdr )
{
    g_snd.out_time = time_ticks_hires() + ( ( (uint64)g_buffer_size * (uint64)time_ticks_per_second_hires() ) / (uint64)g_snd.freq );
    g_snd.out_frames = waveHdr->dwBufferLength / ( 2 * g_snd.out_channels );
    get_input_data( g_snd.out_frames );
    if( g_snd.out_type == sound_buffer_int16 )
    {
	g_snd.out_buffer = waveHdr->lpData;
	main_sound_output_callback( &g_snd, 0 );
    }
    if( g_snd.out_type == sound_buffer_float32 )
    {
        g_snd.out_buffer = g_sound_buffer;
        main_sound_output_callback( &g_snd, 0 );
        float* fb = (float*)g_sound_buffer;
        signed short* sb = (signed short*)waveHdr->lpData;
        for( unsigned int i = 0; i < g_snd.out_frames * g_snd.out_channels; i++ )
        {
	    FLOAT32_TO_INT16( sb[ i ], fb[ i ] );
        }
    }
    MMRESULT mres = waveOutWrite( g_waveOutStream, waveHdr, sizeof( WAVEHDR ) );
    if( mres != MMSYSERR_NOERROR )
    {
        blog( "ERROR in waveOutWrite: %d\n", mres );
    }
}

void WaveInReceiveBuffer( WAVEHDR* waveHdr )
{
    if( waveHdr->dwBytesRecorded )
    {
	int mm_frame_size = 2 * g_snd.in_channels;
        int frame_size = 0;
        if( g_snd.in_type == sound_buffer_int16 )
    	    frame_size = 2 * g_snd.in_channels;
        if( g_snd.in_type == sound_buffer_float32 )
    	    frame_size = 4 * g_snd.in_channels;
        int size = waveHdr->dwBytesRecorded / mm_frame_size;
        int ptr = 0;
        while( size > 0 )
        {
    	    int size2 = size;
            if( g_sound_input_buffer_wp + size2 > g_sound_input_buffer_size )
                size2 = g_sound_input_buffer_size - g_sound_input_buffer_wp;
            char* buf_ptr = g_sound_input_buffer + g_sound_input_buffer_wp * frame_size;
            bmem_copy( buf_ptr, ((char*)waveHdr->lpData) + ptr, size2 * mm_frame_size );
            if( g_snd.in_type == sound_buffer_float32 )
            {
                //Convert from INT16 to FLOAT32
                float* fb = (float*)buf_ptr;
                signed short* sb = (signed short*)buf_ptr;
                for( int i = size2 * g_snd.in_channels - 1; i >= 0; i-- )
                INT16_TO_FLOAT32( fb[ i ], sb[ i ] );
            }
            volatile uint new_wp = ( g_sound_input_buffer_wp + size2 ) & ( g_sound_input_buffer_size - 1 );
            g_sound_input_buffer_wp = new_wp;
            size -= size2;
            ptr += size2 * mm_frame_size;
        }
    }
    MMRESULT mres = waveInAddBuffer( g_waveInStream, waveHdr, sizeof( WAVEHDR ) );
    if( mres != MMSYSERR_NOERROR )
    {
        blog( "MMSOUND INPUT ERROR: Can't add buffer (%d)\n", mres );
    }
}

DWORD WINAPI WaveOutThreadProc( LPVOID lpParameter )
{
    while( g_waveOutExitRequest == 0 )
    {
	MSG msg;
	WAVEHDR* waveHdr = NULL;
	int m = GetMessage( &msg, 0, 0, 0 );
        if( m == 0 ) break;
        if( m != -1 )
        {
	    switch( msg.message )
    	    {
    	        case MM_WOM_DONE:
    		    waveHdr = (WAVEHDR*)msg.lParam;
		    WaveOutSendBuffer( waveHdr );
		    break;
	    }
	}
	if( g_waveOutExitRequest ) break;
    }
    g_waveOutExitRequest = 0;
    return 0;
}

DWORD WINAPI WaveInThreadProc( LPVOID lpParameter )
{
    while( g_waveInExitRequest == 0 )
    {
	MSG msg;
	WAVEHDR* waveHdr = NULL;
	int m = GetMessage( &msg, 0, 0, 0 );
	if( m == 0 ) break;
	if( m != -1 )
	{
    	    switch( msg.message )
    	    {
    	        case MM_WIM_DATA:
    		    waveHdr = (WAVEHDR*)msg.lParam;
    		    WaveInReceiveBuffer( waveHdr );
		    break;
	    }
	}
	if( g_waveInExitRequest ) break;
    }
    g_waveInExitRequest = 0;
    return 0;
}

#ifndef MM_USE_THREAD
void CALLBACK WaveOut( uint hwo, unsigned short uMsg, uint dwInstance, uint dwParam1, uint dwParam2 )
{
    if( g_waveOutStream == 0 ) return;
    if( g_waveOutExitRequest )
    {
	g_waveOutExitRequest = 2;
	return;
    }
    if( uMsg == MM_WOM_DONE ) 
    {
	WAVEHDR* waveHdr = (WAVEHDR*)dwParam1;
	WaveOutSendBuffer( waveHdr );
    }
    return;
}
void CALLBACK WaveIn( uint hwo, unsigned short uMsg, uint dwInstance, uint dwParam1, uint dwParam2 )
{
    if( g_waveInStream == 0 ) return;
    if( g_waveInExitRequest )
    {
	g_waveInExitRequest = 2;
	return;
    }
    if( uMsg == MM_WIM_DATA )
    {
	WAVEHDR* waveHdr = (WAVEHDR*)dwParam1;
	WaveInReceiveBuffer( waveHdr );
    }
    return;
}
#endif

int device_sound_stream_init_mmsound( bool input )
{
    bool err = 0;
    int sound_dev;
    const utf8_char* input_label = "";
    
    if( input )
    {
	if( g_waveInStream ) return 0; //Already open
	sound_dev = profile_get_int_value( KEY_AUDIODEVICE_IN, -1, 0 );
	input_label = " INPUT";
	g_waveInStream = 0;
        g_waveInThread = 0;
	g_waveInThreadID = 0;
	g_waveInExitRequest = 0;
	g_sound_input_buffer_wp = 0;
        g_sound_input_buffer_rp = 0;
    }
    else
    {
	if( g_waveOutStream ) return 0; //Already open
	sound_dev = profile_get_int_value( KEY_AUDIODEVICE, -1, 0 );
	g_waveOutStream = 0;
        g_waveOutThread = 0;
	g_waveOutThreadID = 0;
	g_waveOutExitRequest = 0;
    }
    
    int soundDevices;
    if( input )
	soundDevices = waveInGetNumDevs();
    else
	soundDevices = waveOutGetNumDevs();
    if( soundDevices == 0 ) 
    { 
	blog( "MMSOUND%s ERROR: No sound devices :(\n", input_label ); 
	return 1; 
    } //No sound devices
    blog( "MMSOUND%s: Number of sound devices: %d\n", input_label, soundDevices );
    
    int ourDevice;
    int dev = -1;
    for( ourDevice = 0; ourDevice < soundDevices; ourDevice++ )
    {
    	if( input )
    	{
    	    WAVEINCAPS deviceCaps;
	    waveInGetDevCaps( ourDevice, &deviceCaps, sizeof( deviceCaps ) );
	    if( deviceCaps.dwFormats & WAVE_FORMAT_4S16 ) 
	    {
		dev = ourDevice;
		break;
	    }
	}
	else
	{
    	    WAVEOUTCAPS deviceCaps;
	    waveOutGetDevCaps( ourDevice, &deviceCaps, sizeof( deviceCaps ) );
	    if( deviceCaps.dwFormats & WAVE_FORMAT_4S16 ) 
	    {
		dev = ourDevice;
		break;
	    }
	}
    }
    if( sound_dev >= 0 && sound_dev < soundDevices )
    {
	dev = sound_dev;
    }
    else
    {
	if( dev == -1 )
	{
	    blog( "MMSOUND%s ERROR: Can't find compatible sound device\n", input_label );
	    return 1;
	}
    }
    blog( "MMSOUND%s: Dev: %d. Sound freq: %d\n", input_label, dev, g_snd.freq );
    
    WAVEFORMATEX waveFormat;
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = g_snd.out_channels;
    waveFormat.nSamplesPerSec = g_snd.freq;
    waveFormat.nAvgBytesPerSec = g_snd.freq * 4;
    waveFormat.nBlockAlign = 4;
    waveFormat.wBitsPerSample = 16;
    waveFormat.cbSize = 0;
    MMRESULT mres;
    if( input )
    {
#ifdef MM_USE_THREAD
	g_waveInThread = (HANDLE)CreateThread( NULL, 1024 * 16, WaveInThreadProc, NULL, 0, &g_waveInThreadID );
	SetThreadPriority( g_waveInThread, THREAD_PRIORITY_TIME_CRITICAL );
	mres = waveInOpen( &g_waveInStream, dev, &waveFormat, (uint)g_waveInThreadID, 0, CALLBACK_THREAD );
#else
	mres = waveInOpen( &g_waveInStream, dev, &waveFormat, (uint)&WaveIn, 0, CALLBACK_FUNCTION );
#endif
    }
    else
    {
#ifdef MM_USE_THREAD
	g_waveOutThread = (HANDLE)CreateThread( NULL, 1024 * 64, WaveOutThreadProc, NULL, 0, &g_waveOutThreadID );
	SetThreadPriority( g_waveOutThread, THREAD_PRIORITY_TIME_CRITICAL );
	mres = waveOutOpen( &g_waveOutStream, dev, &waveFormat, (uint)g_waveOutThreadID, 0, CALLBACK_THREAD );
#else
	mres = waveOutOpen( &g_waveOutStream, dev, &waveFormat, (uint)&WaveOut, 0, CALLBACK_FUNCTION );
#endif
    }
    if( mres != MMSYSERR_NOERROR )
    {
	blog( "MMSOUND%s ERROR: Can't open sound device (%d)\n", input_label, mres );
	switch( mres )
	{
	    case MMSYSERR_INVALHANDLE: blog( "MMSOUND%s ERROR: MMSYSERR_INVALHANDLE\n", input_label ); break;
	    case MMSYSERR_BADDEVICEID: blog( "MMSOUND%s ERROR: MMSYSERR_BADDEVICEID\n", input_label ); break;
	    case MMSYSERR_NODRIVER: blog( "MMSOUND%s ERROR: MMSYSERR_NODRIVER\n", input_label ); break;
	    case MMSYSERR_NOMEM: blog( "MMSOUND%s ERROR: MMSYSERR_NOMEM\n", input_label ); break;
	    case WAVERR_BADFORMAT: blog( "MMSOUND%s ERROR: WAVERR_BADFORMAT\n", input_label ); break;
	    case WAVERR_SYNC: blog( "MMSOUND%s ERROR: WAVERR_SYNC\n", input_label ); break;
	}
	return 1;
    }
    blog( "MMSOUND%s: waveout device opened\n", input_label );
    
    if( input )
    {
	for( int b = 0; b < MM_MAX_BUFFERS; b++ )
	{
    	    ZeroMemory( &g_inBuffersHdr[ b ], sizeof( WAVEHDR ) );
    	    g_inBuffersHdr[ b ].lpData = (char *)malloc( g_buffer_size * 2 * g_snd.out_channels );
    	    g_inBuffersHdr[ b ].dwBufferLength = g_buffer_size * 2 * g_snd.out_channels;
	    mres = waveInPrepareHeader( g_waveInStream, &g_inBuffersHdr[ b ], sizeof( WAVEHDR ) );
	    if( mres != MMSYSERR_NOERROR )
	    {
		blog( "MMSOUND%s ERROR: Can't prepare %d header (%d)\n", input_label, b, mres );
		err = 1;
		break;
	    }
	    mres = waveInAddBuffer( g_waveInStream, &g_inBuffersHdr[ b ], sizeof( WAVEHDR ) );
	    if( mres != MMSYSERR_NOERROR )
	    {
		blog( "MMSOUND%s ERROR: Can't add %d buffer (%d)\n", input_label, b, mres );
		err = 1;
		break;
	    }
	}
	if( err == 0 ) waveInStart( g_waveInStream );
    }
    else
    {
	for( int b = 0; b < MM_MAX_BUFFERS; b++ )
	{
    	    ZeroMemory( &g_outBuffersHdr[ b ], sizeof( WAVEHDR ) );
    	    g_outBuffersHdr[ b ].lpData = (char *)malloc( g_buffer_size * 2 * g_snd.out_channels );
    	    g_outBuffersHdr[ b ].dwBufferLength = g_buffer_size * 2 * g_snd.out_channels;
	    mres = waveOutPrepareHeader( g_waveOutStream, &g_outBuffersHdr[ b ], sizeof( WAVEHDR ) );
	    if( mres != MMSYSERR_NOERROR )
	    {
		blog( "MMSOUND ERROR: Can't prepare %d waveout header (%d)\n", b, mres );
		err = 1;
		break;
	    }
	    WaveOutSendBuffer( &g_outBuffersHdr[ b ] );
	}
    }
    
    if( err )
    {
	if( input && g_waveInStream )
	{
	    waveInClose( g_waveInStream );
	    g_waveInStream = 0;
	}
	if( !input && g_waveOutStream )
	{
	    waveOutClose( g_waveOutStream );
	    g_waveOutStream = 0;
	}
	return 1;
    }

    return 0;
}

//################################
//## WINDOWS ASIO               ##
//################################

#ifndef WINCE
#ifndef NOASIO

#include <iasiodrv.h>

#if !defined(_MSC_VER)

const utf8_char* asio_error_str( ASIOError err )
{
    const utf8_char* rv = "Unknown";
    switch( err )
    {
	case ASE_NotPresent: rv = "ASE_NotPresent"; break;
	case ASE_HWMalfunction: rv = "ASE_HWMalfunction"; break;
	case ASE_InvalidParameter: rv = "ASE_InvalidParameter"; break;
	case ASE_InvalidMode: rv = "ASE_InvalidMode"; break;
	case ASE_SPNotAdvancing: rv = "ASE_SPNotAdvancing"; break;
	case ASE_NoClock: rv = "ASE_NoClock"; break;
	case ASE_NoMemory: rv = "ASE_NoMemory"; break;
    }
    return rv;
}

const utf8_char* asio_sampletype_str( ASIOSampleType type )
{
    const utf8_char* rv = "Unknown";
    switch( type )
    {
	case ASIOSTInt16MSB: rv = "ASIOSTInt16MSB"; break;
	case ASIOSTInt24MSB: rv = "ASIOSTInt24MSB"; break;
	case ASIOSTInt32MSB: rv = "ASIOSTInt32MSB"; break;
	case ASIOSTFloat32MSB: rv = "ASIOSTFloat32MSB"; break;
	case ASIOSTFloat64MSB: rv = "ASIOSTFloat64MSB"; break;
	case ASIOSTInt32MSB16: rv = "ASIOSTInt32MSB16"; break;
	case ASIOSTInt32MSB18: rv = "ASIOSTInt32MSB18"; break;
	case ASIOSTInt32MSB20: rv = "ASIOSTInt32MSB20"; break;
	case ASIOSTInt32MSB24: rv = "ASIOSTInt32MSB24"; break;
	case ASIOSTInt16LSB: rv = "ASIOSTInt16LSB"; break;
	case ASIOSTInt24LSB: rv = "ASIOSTInt24LSB"; break;
	case ASIOSTInt32LSB: rv = "ASIOSTInt32LSB"; break;
	case ASIOSTFloat32LSB: rv = "ASIOSTFloat32LSB"; break;
	case ASIOSTFloat64LSB: rv = "ASIOSTFloat64LSB"; break;
	case ASIOSTInt32LSB16: rv = "ASIOSTInt32LSB16"; break;
	case ASIOSTInt32LSB18: rv = "ASIOSTInt32LSB18"; break;
	case ASIOSTInt32LSB20: rv = "ASIOSTInt32LSB20"; break;
	case ASIOSTInt32LSB24: rv = "ASIOSTInt32LSB24"; break;
	case ASIOSTDSDInt8LSB1: rv = "ASIOSTDSDInt8LSB1"; break;
	case ASIOSTDSDInt8MSB1: rv = "ASIOSTDSDInt8MSB1"; break;
	case ASIOSTDSDInt8NER8: rv = "ASIOSTDSDInt8NER8"; break;	
    }
    return rv;
}

IASIO* g_iasio = 0; // Points to the real IASIO

typedef void (*iasio_release_t) ( IASIO* ) __attribute__((stdcall));
typedef ASIOBool (*iasio_init_t) ( IASIO*, int, void* sysHandle ) __attribute__((fastcall));
typedef void (*iasio_getDriverName_t) ( IASIO*, int, char* name ) __attribute__((fastcall));
typedef long (*iasio_getDriverVersion_t) ( IASIO*, int ) __attribute__((fastcall));
typedef void (*iasio_getErrorMessage_t) ( IASIO*, int, char* string ) __attribute__((fastcall));
typedef ASIOError (*iasio_start_t) ( IASIO*, int ) __attribute__((fastcall));
typedef ASIOError (*iasio_stop_t) ( IASIO*, int ) __attribute__((fastcall));
typedef ASIOError (*iasio_getChannels_t) ( IASIO*, int, long* numInputChannels, long* numOutputChannels ) __attribute__((fastcall));
typedef ASIOError (*iasio_getLatencies_t) ( IASIO*, int, long* inputLatency, long* outputLatency ) __attribute__((fastcall));
typedef ASIOError (*iasio_getBufferSize_t) ( IASIO*, int, long* minSize, long* maxSize, long* preferredSize, long* granularity ) __attribute__((fastcall));
typedef ASIOError (*iasio_canSampleRate_t) ( IASIO*, int, ASIOSampleRate sampleRate ) __attribute__((fastcall));
typedef ASIOError (*iasio_getSampleRate_t) ( IASIO*, int, ASIOSampleRate* sampleRate ) __attribute__((fastcall));
typedef ASIOError (*iasio_setSampleRate_t) ( IASIO*, int, ASIOSampleRate sampleRate ) __attribute__((fastcall));
typedef ASIOError (*iasio_getClockSources_t) ( IASIO*, int, ASIOClockSource* clocks, long* numSources ) __attribute__((fastcall));
typedef ASIOError (*iasio_setClockSource_t) ( IASIO*, int, long reference ) __attribute__((fastcall));
typedef ASIOError (*iasio_getSamplePosition_t) ( IASIO*, int, ASIOSamples* sPos, ASIOTimeStamp* tStamp ) __attribute__((fastcall));
typedef ASIOError (*iasio_getChannelInfo_t) ( IASIO*, int, ASIOChannelInfo* info ) __attribute__((fastcall));
typedef ASIOError (*iasio_createBuffers_t) ( IASIO*, int, ASIOBufferInfo* bufferInfos, long numChannels, long bufferSize, ASIOCallbacks* callbacks ) __attribute__((fastcall));
typedef ASIOError (*iasio_disposeBuffers_t) ( IASIO*, int ) __attribute__((fastcall));
typedef ASIOError (*iasio_controlPanel_t) ( IASIO*, int ) __attribute__((fastcall));
typedef ASIOError (*iasio_future_t)( IASIO*, int, long selector, void* opt ) __attribute__((fastcall));
typedef ASIOError (*iasio_outputReady_t) ( IASIO*, int ) __attribute__((fastcall));

iasio_release_t g_iasio_release = 0;
iasio_init_t g_iasio_init = 0;
iasio_getDriverName_t g_iasio_getDriverName = 0;
iasio_getDriverVersion_t g_iasio_getDriverVersion = 0;
iasio_getErrorMessage_t g_iasio_getErrorMessage = 0;
iasio_start_t g_iasio_start = 0;
iasio_stop_t g_iasio_stop = 0;
iasio_getChannels_t g_iasio_getChannels = 0;
iasio_getLatencies_t g_iasio_getLatencies = 0;
iasio_getBufferSize_t g_iasio_getBufferSize = 0;
iasio_canSampleRate_t g_iasio_canSampleRate = 0;
iasio_getSampleRate_t g_iasio_getSampleRate = 0;
iasio_setSampleRate_t g_iasio_setSampleRate = 0;
iasio_getClockSources_t g_iasio_getClockSources = 0;
iasio_setClockSource_t g_iasio_setClockSource = 0;
iasio_getSamplePosition_t g_iasio_getSamplePosition = 0;
iasio_getChannelInfo_t g_iasio_getChannelInfo = 0;
iasio_createBuffers_t g_iasio_createBuffers = 0;
iasio_disposeBuffers_t g_iasio_disposeBuffers = 0;
iasio_controlPanel_t g_iasio_controlPanel = 0;
iasio_future_t g_iasio_future = 0;
iasio_outputReady_t g_iasio_outputReady = 0;

void iasio_load( void )
{
    char* p = ((char**)g_iasio)[ 0 ];
    g_iasio_release = (iasio_release_t)( ((void**)( p + 8 ))[ 0 ] );
    g_iasio_init = (iasio_init_t)( ((void**)( p + 12 ))[ 0 ] );
    g_iasio_getDriverName = (iasio_getDriverName_t)( ((void**)( p + 16 ))[ 0 ] );
    g_iasio_getDriverVersion = (iasio_getDriverVersion_t)( ((void**)( p + 20 ))[ 0 ] );
    g_iasio_getErrorMessage = (iasio_getErrorMessage_t)( ((void**)( p + 24 ))[ 0 ] );
    g_iasio_start = (iasio_start_t)( ((void**)( p + 28 ))[ 0 ] );
    g_iasio_stop = (iasio_stop_t)( ((void**)( p + 32 ))[ 0 ] );
    g_iasio_getChannels = (iasio_getChannels_t)( ((void**)( p + 36 ))[ 0 ] );
    g_iasio_getLatencies = (iasio_getLatencies_t)( ((void**)( p + 40 ))[ 0 ] );
    g_iasio_getBufferSize = (iasio_getBufferSize_t)( ((void**)( p + 44 ))[ 0 ] );
    g_iasio_canSampleRate = (iasio_canSampleRate_t)( ((void**)( p + 48 ))[ 0 ] );
    g_iasio_getSampleRate = (iasio_getSampleRate_t)( ((void**)( p + 52 ))[ 0 ] );
    g_iasio_setSampleRate = (iasio_setSampleRate_t)( ((void**)( p + 56 ))[ 0 ] );
    g_iasio_getClockSources = (iasio_getClockSources_t)( ((void**)( p + 60 ))[ 0 ] );
    g_iasio_setClockSource = (iasio_setClockSource_t)( ((void**)( p + 64 ))[ 0 ] );
    g_iasio_getSamplePosition = (iasio_getSamplePosition_t)( ((void**)( p + 68 ))[ 0 ] );
    g_iasio_getChannelInfo = (iasio_getChannelInfo_t)( ((void**)( p + 72 ))[ 0 ] );
    g_iasio_createBuffers = (iasio_createBuffers_t)( ((void**)( p + 76 ))[ 0 ] );
    g_iasio_disposeBuffers = (iasio_disposeBuffers_t)( ((void**)( p + 80 ))[ 0 ] );
    g_iasio_controlPanel = (iasio_controlPanel_t)( ((void**)( p + 84 ))[ 0 ] );
    g_iasio_future = (iasio_future_t)( ((void**)( p + 88 ))[ 0 ] );
    g_iasio_outputReady = (iasio_outputReady_t)( ((void**)( p + 92 ))[ 0 ] );
}

void iasio_release( void )
{
    g_iasio_release( g_iasio );
}

ASIOBool iasio_init( void* sysHandle )
{
    return g_iasio_init( g_iasio, 0, sysHandle );
}

void iasio_getDriverName( char* name )
{
    g_iasio_getDriverName( g_iasio, 0, name );
}

long iasio_getDriverVersion()
{
    return g_iasio_getDriverVersion( g_iasio, 0 );
}

void iasio_getErrorMessage( char* string )
{
    g_iasio_getErrorMessage( g_iasio, 0, string );
}

ASIOError iasio_start()
{
    return g_iasio_start( g_iasio, 0 );
}

ASIOError iasio_stop()
{
    return g_iasio_stop( g_iasio, 0 );
}

ASIOError iasio_getChannels( long* numInputChannels, long* numOutputChannels )
{
    return g_iasio_getChannels( g_iasio, 0, numInputChannels, numOutputChannels );
}

ASIOError iasio_getLatencies( long* inputLatency, long* outputLatency )
{
    return g_iasio_getLatencies( g_iasio, 0, inputLatency, outputLatency );
}

ASIOError iasio_getBufferSize( long* minSize, long* maxSize, long* preferredSize, long* granularity )
{
    return g_iasio_getBufferSize( g_iasio, 0, minSize, maxSize, preferredSize, granularity );
}

ASIOError iasio_canSampleRate( ASIOSampleRate sampleRate )
{
    return g_iasio_canSampleRate( g_iasio, 0, sampleRate );
}

ASIOError iasio_getSampleRate( ASIOSampleRate* sampleRate )
{
    return g_iasio_getSampleRate( g_iasio, 0, sampleRate );
}

ASIOError iasio_setSampleRate( ASIOSampleRate sampleRate )
{    
    return g_iasio_setSampleRate( g_iasio, 0, sampleRate );
}

ASIOError iasio_getClockSources( ASIOClockSource* clocks, long* numSources )
{
    return g_iasio_getClockSources( g_iasio, 0, clocks, numSources );
}

ASIOError iasio_setClockSource( long reference )
{
    return g_iasio_setClockSource( g_iasio, 0, reference );
}

ASIOError iasio_getSamplePosition( ASIOSamples* sPos, ASIOTimeStamp* tStamp )
{
    return g_iasio_getSamplePosition( g_iasio, 0, sPos, tStamp );
}

ASIOError iasio_getChannelInfo( ASIOChannelInfo* info )
{
    return g_iasio_getChannelInfo( g_iasio, 0, info );
}

ASIOError iasio_createBuffers( ASIOBufferInfo* bufferInfos, long numChannels, long bufferSize, ASIOCallbacks* callbacks )
{
    return g_iasio_createBuffers( g_iasio, 0, bufferInfos, numChannels, bufferSize, callbacks );
}

ASIOError iasio_disposeBuffers()
{
    return g_iasio_disposeBuffers( g_iasio, 0 );
}

ASIOError iasio_controlPanel()
{
    return g_iasio_controlPanel( g_iasio, 0 );
}

ASIOError iasio_future( long selector, void* opt )
{
    return g_iasio_future( g_iasio, 0, selector, opt );
}

ASIOError iasio_outputReady()
{
    return g_iasio_outputReady( g_iasio, 0 );
}

#endif

#define ASIO_DEVS 32

struct asio_device
{
    CLSID clsid;    
};

asio_device g_asio_devs[ ASIO_DEVS ];
ASIOChannelInfo g_asio_channels[ 32 ];
ASIOBufferInfo g_asio_bufs[ 32 ];
ASIOCallbacks g_asio_callbacks;
int g_asio_sample_size = 0;
int g_asio_input_sample_size = 0;

ASIOTime* asio_buffer_switch_time_info( ASIOTime* timeInfo, long index, ASIOBool processNow )
{
    //Indicates that both input and output are to be processed. 
    //It also passes extended time information (time code for synchronization purposes) to the host application and back to the driver.

    //Input:
    if( g_sound_input_enabled )
    {
	g_sound_input_buffer2_is_empty = 0;
        g_snd.in_buffer = g_sound_input_buffer2;
	signed short* dest16 = (signed short*)g_snd.in_buffer;
	float* dest32 = (float*)g_snd.in_buffer;
	for( int c = g_snd.out_channels; c < g_snd.out_channels + g_snd.in_channels; c++ )
	{
	    if( g_asio_bufs[ c ].isInput == 0 ) continue;
	    int cnum = c - g_snd.out_channels;
	    unsigned char* buf = (unsigned char*)g_asio_bufs[ c ].buffers[ index ];
	    ASIOSampleType sample_type = g_asio_channels[ c ].type;
	    switch( sample_type )
	    {
		case ASIOSTInt16MSB:
		case ASIOSTInt16LSB:
		    {
			signed short* src16 = (signed short*)buf;
			if( g_snd.in_type == sound_buffer_int16 )
			{
			    if( sample_type == ASIOSTInt16MSB )
			    {
				for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
				{
				    uint16 v = (uint16)src16[ i ];
				    uint16 temp = ( v >> 8 ) & 255;
				    v = ( ( v << 8 ) & 0xFF00 ) | temp;
				    dest16[ i2 + cnum ] = (signed short)v;
				}
			    }
			    else
			    {
				for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
				    dest16[ i2 + cnum ] = src16[ i ];
			    }
			}
			if( g_snd.in_type == sound_buffer_float32 )
			{
			    if( sample_type == ASIOSTInt16MSB )
			    {
				for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
				{
				    uint16 v = (uint16)src16[ i ];
				    uint16 temp = ( v >> 8 ) & 255;
				    v = ( ( v << 8 ) & 0xFF00 ) | temp;
				    INT16_TO_FLOAT32( dest32[ i2 + cnum ], (signed short)v );
				}
			    }
			    else
			    {
				for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
				    INT16_TO_FLOAT32( dest32[ i2 + cnum ], src16[ i ] );
			    }
			}
		    }
		    break;
		case ASIOSTInt24LSB:
		case ASIOSTInt24MSB:
		    {
			for( int i = 0, i2 = 0, i3 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels, i3 += 3 )
			{
			    int v;
			    if( sample_type == ASIOSTInt24LSB )
				v = buf[ i3 + 0 ] | ( buf[ i3 + 1 ] << 8 ) | ( buf[ i3 + 2 ] << 16 );
			    else
				v = buf[ i3 + 2 ] | ( buf[ i3 + 1 ] << 8 ) | ( buf[ i3 + 0 ] << 16 );
			    if( v & 0x800000 ) v |= 0xFF000000; //sign
			    if( g_snd.in_type == sound_buffer_int16 )
				dest16[ i2 + cnum ] = v >> 8;
			    if( g_snd.in_type == sound_buffer_float32 )
				INT16_TO_FLOAT32( dest32[ i2 + cnum ], v >> 8 );
			}
		    }
		    break;
		case ASIOSTInt32MSB:
        	case ASIOSTInt32LSB:
		    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
			    int v;
			    if( sample_type == ASIOSTInt32LSB )
				v = buf[ i * 4 + 0 ] | ( buf[ i * 4 + 1 ] << 8 ) | ( buf[ i * 4 + 2 ] << 16 ) | ( buf[ i * 4 + 3 ] << 24 );
			    else
				v = buf[ i * 4 + 3 ] | ( buf[ i * 4 + 2 ] << 8 ) | ( buf[ i * 4 + 1 ] << 16 ) | ( buf[ i * 4 + 0 ] << 24 );
			    if( g_snd.in_type == sound_buffer_int16 )
				dest16[ i2 + cnum ] = v >> 16;
			    if( g_snd.in_type == sound_buffer_float32 )
				INT16_TO_FLOAT32( dest32[ i2 + cnum ], v >> 16 );
			}
		    }
    		    break;
    		case ASIOSTFloat32MSB:
        	case ASIOSTFloat32LSB:
        	    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
        		    float f;
			    uint* v = (uint*)&f;
			    if( sample_type == ASIOSTFloat32LSB )
				f = ((float*)buf)[ i ];
			    else
				*v = buf[ i * 4 + 3 ] | ( buf[ i * 4 + 2 ] << 8 ) | ( buf[ i * 4 + 1 ] << 16 ) | ( buf[ i * 4 + 0 ] << 24 );
			    if( g_snd.in_type == sound_buffer_int16 )
				FLOAT32_TO_INT16( dest16[ i2 + cnum ], f );
			    if( g_snd.in_type == sound_buffer_float32 )
				dest32[ i2 + cnum ] = f;
			}
        	    }
    		    break;
	    }
	}
    }
    else
    {
        if( g_sound_input_buffer2_is_empty == 0 )
        {
            bmem_zero( g_sound_input_buffer2 );
            g_sound_input_buffer2_is_empty = 1;
        }
        g_snd.in_buffer = g_sound_input_buffer2;
    }

    g_snd.out_time = time_ticks_hires() + ( ( (uint64)g_buffer_size * (uint64)time_ticks_per_second_hires() ) / (uint64)g_snd.freq );
    g_snd.out_buffer = g_sound_buffer;
    g_snd.out_frames = g_buffer_size;
    main_sound_output_callback( &g_snd, 0 );
    
    //Output:
    signed short* src16 = (signed short*)g_snd.out_buffer;
    float* src32 = (float*)g_snd.out_buffer;
    for( int ch = 0; ch < g_snd.out_channels; ch++ )
    {
	if( g_asio_bufs[ ch ].isInput ) continue;
	unsigned char* buf = (unsigned char*)g_asio_bufs[ ch ].buffers[ index ];
	switch( g_asio_channels[ ch ].type )
	{
	    case ASIOSTInt16MSB:
	    case ASIOSTInt16LSB:
		{
		    signed short* dest16 = (signed short*)buf;
		    if( g_snd.out_type == sound_buffer_int16 )
		    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
			    dest16[ i ] = src16[ i2 + ch ];
			}
		    }
		    if( g_snd.out_type == sound_buffer_float32 )
		    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
			    FLOAT32_TO_INT16( dest16[ i ], src32[ i2 + ch ] );
			}
		    }
		    if( g_asio_channels[ ch ].type == ASIOSTInt16MSB )
		    {
			for( int i = 0; i < g_buffer_size * 2; i += 2 )
			{
			    unsigned char temp = buf[ i + 1 ];
			    buf[ i ] = buf[ i + 1 ];
			    buf[ i + 1 ] = temp;
			}
		    }
		}
		break;
	    case ASIOSTInt24LSB:
		{
		    signed short* dest16 = (signed short*)buf;
		    if( g_snd.out_type == sound_buffer_int16 )
		    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
			    int v = src16[ i2 + ch ] << 8;
			    buf[ i * 3 + 0 ] = v & 0xFF;
			    buf[ i * 3 + 1 ] = ( v >> 8 ) & 0xFF;
			    buf[ i * 3 + 2 ] = ( v >> 16 ) & 0xFF;
			}
		    }
		    if( g_snd.out_type == sound_buffer_float32 )
		    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
			    int v = (int)( src32[ i2 + ch ] * (float)( 1 << 23 ) );
			    buf[ i * 3 + 0 ] = v & 0xFF;
			    buf[ i * 3 + 1 ] = ( v >> 8 ) & 0xFF;
			    buf[ i * 3 + 2 ] = ( v >> 16 ) & 0xFF;
			}
		    }
		}
		break;
	    case ASIOSTInt24MSB:
		{
		    if( g_snd.out_type == sound_buffer_int16 )
		    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
			    int v = src16[ i2 + ch ] << 8;
			    buf[ i * 3 + 2 ] = v & 0xFF;
			    buf[ i * 3 + 1 ] = ( v >> 8 ) & 0xFF;
			    buf[ i * 3 + 0 ] = ( v >> 16 ) & 0xFF;
			}
		    }
		    if( g_snd.out_type == sound_buffer_float32 )
		    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
			    int v = (int)( src32[ i2 + ch ] * (float)( 1 << 23 ) );
			    buf[ i * 3 + 2 ] = v & 0xFF;
			    buf[ i * 3 + 1 ] = ( v >> 8 ) & 0xFF;
			    buf[ i * 3 + 0 ] = ( v >> 16 ) & 0xFF;
			}
		    }
		}
		break;
	    case ASIOSTInt32MSB:
	    case ASIOSTInt32LSB:
		{
		    signed int* dest32 = (signed int*)buf;
		    if( g_snd.out_type == sound_buffer_int16 )
		    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
			    dest32[ i ] = (signed int)src16[ i2 + ch ] << 16;
			}
		    }
		    if( g_snd.out_type == sound_buffer_float32 )
		    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
			    FLOAT32_TO_INT32( dest32[ i ], src32[ i2 + ch ] );
			}
		    }
		    if( g_asio_channels[ ch ].type == ASIOSTInt32MSB )
		    {
			for( int i = 0; i < g_buffer_size * 4; i += 4 )
			{
			    unsigned char temp1 = buf[ i + 1 ];
			    unsigned char temp2 = buf[ i + 0 ];
			    buf[ i + 0 ] = buf[ i + 3 ];
			    buf[ i + 1 ] = buf[ i + 2 ];
			    buf[ i + 2 ] = temp1;
			    buf[ i + 3 ] = temp2;
			}
		    }
		}
		break;
	    case ASIOSTFloat32MSB:
	    case ASIOSTFloat32LSB:
		{
		    float* dest32 = (float*)buf;
		    if( g_snd.out_type == sound_buffer_int16 )
		    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
			    INT16_TO_FLOAT32( dest32[ i ], src16[ i2 + ch ] );
			}
		    }
		    if( g_snd.out_type == sound_buffer_float32 )
		    {
			for( int i = 0, i2 = 0; i < g_buffer_size; i++, i2 += g_snd.out_channels )
			{
			    dest32[ i ] = src32[ i2 + ch ];
			}
		    }
		    if( g_asio_channels[ ch ].type == ASIOSTFloat32MSB )
		    {
			for( int i = 0; i < g_buffer_size * 4; i += 4 )
			{
			    unsigned char temp1 = buf[ i + 1 ];
			    unsigned char temp2 = buf[ i + 0 ];
			    buf[ i + 0 ] = buf[ i + 3 ];
			    buf[ i + 1 ] = buf[ i + 2 ];
			    buf[ i + 2 ] = temp1;
			    buf[ i + 3 ] = temp2;
			}
		    }
		}
		break;
	}
    }
    
    iasio_outputReady();
    
    return 0;
}

void asio_buffer_switch( long index, ASIOBool processNow )
{
    //Indicates that both input and output are to be processed.
    
    //As this is a "back door" into the bufferSwitchTimeInfo a timeInfo needs to be created
    //though it will only set the timeInfo.samplePosition and timeInfo.systemTime fields and the according flags
    ASIOTime timeInfo;
    memset( &timeInfo, 0, sizeof( timeInfo ) );
    
    //get the time stamp of the buffer, not necessary if no
    //synchronization to other media is required
    if( iasio_getSamplePosition( &timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime ) == ASE_OK )
	timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;
    
    asio_buffer_switch_time_info( &timeInfo, index, processNow );
}

void asio_sample_rate_changed( ASIOSampleRate sRate )
{
    blog( "ASIO Sample rate changed: %d\n", (int)sRate );
}

long asio_messages( long selector, long value, void* message, double* opt )
{
    long ret = 0;
    switch( selector )
    {
	case kAsioSelectorSupported:
	    if( value == kAsioEngineVersion || value == kAsioSupportsTimeInfo ) ret = 1;
	    break;
	case kAsioEngineVersion: 
            // return the supported ASIO version of the host application
            // If a host applications does not implement this selector, ASIO 1.0 is assumed
            // by the driver
	    ret = 2;
	    break;
        case kAsioSupportsTimeInfo:
            // informs the driver wether the asioCallbacks.bufferSwitchTimeInfo() callback
            // is supported.
            // For compatibility with ASIO 1.0 drivers the host application should always support
            // the "old" bufferSwitch method, too.
            ret = 1;
            break;
    }
    return ret;
}

#endif
#endif

int device_sound_stream_init_asio( void )
{
#ifndef WINCE
#ifndef NOASIO

    //ASIO Input is enabled by default:
    set_input_defaults();

    ASIOError err;    
    int old_buf_size = g_buffer_size;
    int sound_dev = profile_get_int_value( KEY_AUDIODEVICE, -1, 0 );
    
    //Enumerate devices:
    int devs = 0;
    HKEY hk = NULL;
    CHAR keyname[ 128 ];
    int rv = RegOpenKey( HKEY_LOCAL_MACHINE, "software\\asio", &hk );
    int d = 0;
    while( ( rv == ERROR_SUCCESS ) && ( devs < ASIO_DEVS ) )
    {
	rv = RegEnumKey( hk, d, (LPTSTR)keyname, 128 );
	d++;
	if( rv == ERROR_SUCCESS )
	{
	    blog( "ASIO Device %d: %s\n", devs, keyname );
	    HKEY hk2;
	    int rv2 = RegOpenKeyEx( hk, (LPCTSTR)keyname, 0, KEY_READ, &hk2 );
	    if( rv2 == ERROR_SUCCESS )
	    {
		CHAR s[ 256 ];
		WCHAR w[ 128 ];
		DWORD datatype = REG_SZ;
		DWORD datasize = 256;
		rv2 = RegQueryValueEx( hk2, "clsid", 0, &datatype, (LPBYTE)s, &datasize );
		if( rv2 == ERROR_SUCCESS )
		{
		    MultiByteToWideChar( CP_ACP, 0, (LPCSTR)s, -1, (LPWSTR)w, 128 );
		    if( CLSIDFromString( (LPOLESTR)w, (LPCLSID)&g_asio_devs[ devs ].clsid ) == S_OK )
		    {
		    }
		}
	    }
	    devs++;
	}
    }

    //Set device number:
    if( devs == 0 )
    {
	blog( "ASIO ERROR: No devices\n" );
	return 1;
    }
    int dev = 0;
    if( sound_dev >= 0 && sound_dev < devs )
    {
	dev = sound_dev;
    }
    blog( "ASIO Selected device: %d\n", dev ); 
    
    //Open device:
    HWND hWnd = GetForegroundWindow();
    if( hWnd == NULL )
    {
        hWnd = GetDesktopWindow();
    }
    CoInitialize( NULL );
    g_iasio = 0;
    rv = CoCreateInstance( g_asio_devs[ dev ].clsid, 0, CLSCTX_INPROC_SERVER, g_asio_devs[ dev ].clsid, (void**)&g_iasio );
    if( rv == S_OK )
    {
	iasio_load();
	blog( "ASIO init\n" );
	if( !iasio_init( (void *)hWnd ) )
	{
	    blog( "ASIO ERROR in init()\n" );
	    return 1;
	}
    }
    else 
    {
	blog( "ASIO ERROR in CoCreateInstance() %x\n", rv );
	return 1;
    }
    if( g_iasio == 0 )
    {
	blog( "ASIO ERROR: can't open the driver\n" );
	return 1;
    }
    
    //Device init:
    long max_in_channels = 0;
    long max_out_channels = 0;
    long in_channels = 0;
    long out_channels = 0;
    long minSize = 0;
    long maxSize = 0;
    long preferredSize = 0;
    long granularity = 0;
    blog( "ASIO getChannels\n" );
    err = iasio_getChannels( &in_channels, &out_channels );
    if( err != ASE_OK )
    {
	blog( "ASIO ERROR in getChannels() %X %s\n", err, asio_error_str( err ) );
	return 1;
    }
    max_in_channels = in_channels;
    max_out_channels = out_channels;
    blog( "ASIO CHANNELS: IN:%d OUT:%d\n", (int)in_channels, (int)out_channels );
    if( g_snd.out_channels > out_channels )
    {
	blog( "ASIO ERROR: Can't set %d channels\n", g_snd.out_channels );
	return 1;
    }
    out_channels = g_snd.out_channels;
    if( g_snd.in_channels > in_channels )
	g_snd.in_channels = in_channels;
    in_channels = g_snd.in_channels;
    ASIOSampleRate asio_srate = 0;
    iasio_getSampleRate( &asio_srate );
    blog( "ASIO Initial sample rate: %d\n", (int)asio_srate );
    if( asio_srate != g_snd.freq )
    {
	while( 1 )
	{
	    blog( "ASIO Trying to set %d ...\n", g_snd.freq );
	    err = iasio_setSampleRate( g_snd.freq );
	    if( err != ASE_OK )
	    {
		blog( "ASIO ERROR: setSampleRate(%d) failed with code %X %s\n", g_snd.freq, err, asio_error_str( err ) );
		while( 1 )
		{
		    if( g_snd.freq == 44100 ) { g_snd.freq = 48000; break; }
		    if( g_snd.freq == 48000 ) { g_snd.freq = 96000; break; }
		    if( g_snd.freq == 96000 ) { g_snd.freq = 192000; break; }
		    if( g_snd.freq == 192000 ) { return 1; }
		    break;
		}
	    }
	    else 
	    {
		break;
	    }
	}
    }
    iasio_getSampleRate( &asio_srate );
    if( asio_srate != g_snd.freq )
    {
	blog( "ASIO Real sample rate (!=g_snd.freq): %d\n", (int)asio_srate );
    }
    int in_ch_offset = profile_get_int_value( "audio_ch_in", 0, 0 );
    int out_ch_offset = profile_get_int_value( "audio_ch", 0, 0 );
    g_asio_sample_size = 0;
    g_asio_input_sample_size = 0;
    for( int ch = 0; ch < out_channels + in_channels; ch++ )
    {
	int ch_num = ch;
	bool input = 0;
	if( ch_num >= out_channels ) 
	{
	    //Input:
	    ch_num -= out_channels; //Switch to Input channels
	    ch_num += in_ch_offset;
	    if( ch_num >= max_in_channels )
		ch_num %= max_in_channels;
	    input = 1;
	}
	else
	{
	    //Output:
	    ch_num += out_ch_offset;
	    if( ch_num >= max_out_channels )
		ch_num %= max_out_channels;
	}
	g_asio_channels[ ch ].channel = ch_num;
	g_asio_channels[ ch ].isInput = input;
	iasio_getChannelInfo( &g_asio_channels[ ch ] );
	blog( "ASIO CH %d: ", ch_num );
	if( input ) blog( "input; " ); else blog( "output; " );
	blog( "type=%s (%d);", asio_sampletype_str( g_asio_channels[ ch ].type ), g_asio_channels[ ch ].type );
	blog( "\n" );
	g_asio_bufs[ ch ].isInput = input;
	g_asio_bufs[ ch ].channelNum = ch_num;
	g_asio_bufs[ ch ].buffers[ 0 ] = NULL;
	g_asio_bufs[ ch ].buffers[ 1 ] = NULL;
	int sample_size = 0;
	switch( g_asio_channels[ ch ].type & 0xF )
	{
	    case ASIOSTInt16MSB: 
	    case ASIOSTInt16LSB: 
		sample_size = 2; 
		break;
	    case ASIOSTInt24MSB: 
	    case ASIOSTInt24LSB: 
		sample_size = 3; 
		break;
	    case ASIOSTFloat64MSB: 
	    case ASIOSTFloat64LSB: 
		sample_size = 8; 
		break;
	    default: 
		sample_size = 4;
		break;
	}
	if( input )
	{
	    if( g_asio_input_sample_size == 0 ) g_asio_input_sample_size = sample_size;
	}
	else
	{
	    if( g_asio_sample_size == 0 ) g_asio_sample_size = sample_size;
	}
    }
    printf( "ASIO getBufferSize\n" );
    err = iasio_getBufferSize( &minSize, &maxSize, &preferredSize, &granularity );
    if( err != ASE_OK )
    {
	blog( "ASIO ERROR in getBufferSize() %X %s\n", err, asio_error_str( err ) );
	return 1;
    }
    blog( "ASIO buffer: minSize=%d maxSize=%d preferredSize=%d granularity=%d\n; SampleSize=%d inputSampleSize=%d\n", (int)minSize, (int)maxSize, (int)preferredSize, (int)granularity, g_asio_sample_size, g_asio_input_sample_size );
    if( granularity == -1 )
    {
	//The buffer size will be a power of 2:
	int size = minSize;
	while( size < g_buffer_size )
	{
	    size *= 2;
	}
	g_buffer_size = size;
    }
    if( granularity > 0 )
    {
	int size = minSize;
	while( size < g_buffer_size )
	{
	    size += granularity;
	}
	g_buffer_size = size;
    }
    if( granularity == 0 )
    {
	g_buffer_size = minSize;
    }
    if( g_buffer_size > maxSize )
	g_buffer_size = maxSize;
    if( g_buffer_size < minSize )
	g_buffer_size = minSize;
    blog( "ASIO buffer size: %d\n", (int)g_buffer_size );
    //Set callbacks:
    g_asio_callbacks.bufferSwitch = asio_buffer_switch;
    g_asio_callbacks.sampleRateDidChange = asio_sample_rate_changed;
    g_asio_callbacks.asioMessage = asio_messages;
    g_asio_callbacks.bufferSwitchTimeInfo = asio_buffer_switch_time_info;
    //Create buffers:
create_bufs_again:
    blog( "ASIO createBuffers\n" );
    err = iasio_createBuffers( g_asio_bufs, in_channels + out_channels, g_buffer_size, &g_asio_callbacks );
    if( err == ASE_OK )
    {
	for( int i = 0; i < in_channels + out_channels; i++ )
	{
	    int sample_size;
	    if( g_asio_bufs[ i ].isInput )
		sample_size = g_asio_input_sample_size;
	    else
		sample_size = g_asio_sample_size;
	    if( g_asio_bufs[ i ].buffers[ 0 ] )
	    {
		memset( g_asio_bufs[ i ].buffers[ 0 ], 0, g_buffer_size * sample_size );
	    }
	    if( g_asio_bufs[ i ].buffers[ 1 ] )
	    {
		memset( g_asio_bufs[ i ].buffers[ 1 ], 0, g_buffer_size * sample_size );
	    }
	}
	blog( "ASIO outputReady\n" );
	iasio_outputReady();
    }
    else 
    {
	blog( "ASIO ERROR: createBuffers() failed with code %X %s\n", err, asio_error_str( err ) );
	if( err == ASE_InvalidMode )
	{
	    if( g_buffer_size != preferredSize )
	    {
		blog( "ASIO trying to switch to preferred buffer size %d ...\n", preferredSize );
		g_buffer_size = preferredSize;
		goto create_bufs_again;
    	    }
    	}
    	return 1;
    }
    
    if( old_buf_size < g_buffer_size )
    {
	bmem_free( g_sound_buffer );
	g_sound_buffer = bmem_new( g_buffer_size * g_frame_size );
    }
    
    //ASIO Input is enabled by default. So create the buffers:
    create_input_buffers( g_buffer_size );
    
    blog( "ASIO start\n" );
    iasio_start();

    return 0;
    
#endif
#endif
    return 1;
}

//################################
//## MAIN SOUND FUNCTIONS       ##
//################################

int device_sound_stream_init( void )
{
    g_sound_driver = 0;
    g_frame_size = 0;
    g_sound_buffer = 0;
#ifndef WINCE
#ifndef NOASIO
    g_iasio = 0;
    g_asio_sample_size = 0;
#endif
#endif

    const utf8_char* audio_driver = 0;
    audio_driver = profile_get_str_value( KEY_AUDIODRIVER, "", 0 );
    if( audio_driver )
    {
	for( int i = 0; i < NUMBER_OF_SDRIVERS; i++ )
        {
            if( strcmp( audio_driver, g_sdriver_names[ i ] ) == 0 )
            {
                g_sound_driver = i;
                break;
            }
        }
    }

    g_buffer_size = profile_get_int_value( KEY_SOUNDBUFFER, g_default_buffer_size, 0 );
    g_snd.out_latency = g_buffer_size;

    if( g_snd.out_type == sound_buffer_int16 )
	g_frame_size = 2 * g_snd.out_channels;
    if( g_snd.out_type == sound_buffer_float32 )
	g_frame_size = 4 * g_snd.out_channels;
    g_sound_buffer = bmem_new( g_buffer_size * g_frame_size );

    bool sdriver_checked[ NUMBER_OF_SDRIVERS ];
    bmem_set( sdriver_checked, sizeof( sdriver_checked ), 0 );
    
    while( 1 )
    {
	int prev_buf_size = g_buffer_size;
	bool sdriver_ok = 0;
	switch( g_sound_driver )
	{
	    case SDRIVER_MMSOUND:
		if( device_sound_stream_init_mmsound( 0 ) == 0 ) sdriver_ok = 1;
		break;
#ifndef WINCE
	    case SDRIVER_DSOUND:
		if( device_sound_stream_init_dsound( 0 ) == 0 ) sdriver_ok = 1;
		break;
#ifndef NOASIO
	    case SDRIVER_ASIO:
		if( device_sound_stream_init_asio() == 0 ) sdriver_ok = 1;
		break;
#endif
#endif
	}
	if( sdriver_ok ) 
	{
	    //Driver found:
	    break;
	}
	//Driver not found:
	g_buffer_size = prev_buf_size;
	if( g_sound_driver < NUMBER_OF_SDRIVERS ) sdriver_checked[ g_sound_driver ] = 1;
	g_sound_driver = 0;
	for( g_sound_driver = 0; g_sound_driver < NUMBER_OF_SDRIVERS; g_sound_driver++ )
	{
	    if( sdriver_checked[ g_sound_driver ] == 0 ) 
	    {
		blog( "Switching to %s\n", g_sdriver_names[ g_sound_driver ] );
		break;
	    }
	}
	if( g_sound_driver == NUMBER_OF_SDRIVERS )
	{
	    //No sound driver found:
	    return 1;
	}
    }
    
    g_snd.out_latency = g_buffer_size;
    
    return 0;
}

void device_sound_stream_input( bool enable )
{
    if( enable )
    {	
	set_input_defaults();
        create_input_buffers( g_buffer_size );
        switch( g_sound_driver )
        {
            case SDRIVER_MMSOUND:
        	if( device_sound_stream_init_mmsound( 1 ) == 0 )
                    g_sound_input_enabled = 1;
        	break;
#ifndef WINCE
            case SDRIVER_DSOUND:
        	if( device_sound_stream_init_dsound( 1 ) == 0 )
                    g_sound_input_enabled = 1;
        	break;
#ifndef NOASIO
            case SDRIVER_ASIO:
    		Sleep( 1 );
                g_sound_input_enabled = 1;
        	break;
#endif
#endif
        }
    }
    else
    {
	if( g_sound_input_enabled == 0 ) return;
        switch( g_sound_driver )
        {
            case SDRIVER_MMSOUND:
        	if( g_waveInStream )
        	{
            	    g_waveInExitRequest = 1;
            	    int step = 20; //ms
            	    int timeout = 300;
            	    int t = 0;
#ifdef MM_USE_THREAD
            	    while( g_waveInExitRequest ) 
#else
            	    while( g_waveInExitRequest == 1 )
#endif
            	    { 
            		Sleep( step ); //Waiting for thread stop
            		t += step;
            		if( t > timeout ) 
            		{
            		    blog( "MMSOUND INPUT: Timeout\n" );
            		    break;
            		}
            	    }
#ifdef MM_USE_THREAD
            	    CloseHandle( g_waveInThread ); g_waveInThread = 0;
#endif
		    g_waveInExitRequest = 0;
            	    blog( "MMSOUND INPUT: CloseHandle (soundThread) ok\n" );

            	    MMRESULT mres;
            	    mres = waveInReset( g_waveInStream );
            	    if( mres != MMSYSERR_NOERROR )
            	    {
                	blog( "MMSOUND INPUT ERROR in waveInReset (%d)\n", mres );
            	    }

            	    for( int b = 0; b < MM_MAX_BUFFERS; b++ )
            	    {
                	mres = waveInUnprepareHeader( g_waveInStream, &g_inBuffersHdr[ b ], sizeof( WAVEHDR ) );
                	if( mres != MMSYSERR_NOERROR )
                    	    blog( "MMSOUND INPUT ERROR: Can't unprepare header %d (%d)\n", b, mres );
                	free( g_inBuffersHdr[ b ].lpData );
            	    }

        	    mres = waveInClose( g_waveInStream );
        	    if( mres != MMSYSERR_NOERROR )
            	        blog( "MMSOUND INPUT ERROR in waveInClose: %d\n", mres );
            	    else
                	blog( "MMSOUND INPUT: waveInClose ok\n" );
            	    
            	    g_waveInStream = 0;
            	    g_sound_input_enabled = 0;
        	}
        	break;
#ifndef WINCE
            case SDRIVER_DSOUND:
        	{
		    ds_input_exit_request = 1;
		    int step = 20; //ms
            	    int timeout = 300;
            	    int t = 0;
            	    while( ds_input_exit_request )
            	    {
                	Sleep( step ); //Waiting for thread stop
                	t += step;
                	if( t > timeout )
                	{
                    	    blog( "DSOUND INPUT: Timeout\n" );
                    	    break;
                	}
            	    }
		    CloseHandle( ds_input_thread ); ds_input_thread = 0;
		    ds_input_exit_request = 0;
		    
		    if( ds_input_buf ) ds_input_buf->Release(); ds_input_buf = 0;
		    if( lpdsCapture ) lpdsCapture->Release(); lpdsCapture = 0;
            	    g_sound_input_enabled = 0;
		}
        	break;
#ifndef NOASIO
            case SDRIVER_ASIO:
                g_sound_input_enabled = 0;
        	break;
#endif
#endif
        }
    }
}

const utf8_char* device_sound_stream_get_driver_name( void )
{
    if( (unsigned)g_sound_driver < NUMBER_OF_SDRIVERS )
        return g_sdriver_names[ g_sound_driver ];
    return 0;
}

const utf8_char* device_sound_stream_get_driver_info( void )
{
    if( (unsigned)g_sound_driver < NUMBER_OF_SDRIVERS )
        return g_sdriver_infos[ g_sound_driver ];
    return 0;
}

int device_sound_stream_get_drivers( utf8_char*** names, utf8_char*** infos )
{
    utf8_char** n = (utf8_char**)bmem_new( NUMBER_OF_SDRIVERS * sizeof( void* ) );
    utf8_char** i = (utf8_char**)bmem_new( NUMBER_OF_SDRIVERS * sizeof( void* ) );
    for( int d = 0; d < NUMBER_OF_SDRIVERS; d++ )
    {
        n[ d ] = (utf8_char*)bmem_new( bmem_strlen( g_sdriver_names[ d ] ) + 1 ); n[ d ][ 0 ] = 0;
        bmem_strcat_resize( n[ d ], g_sdriver_names[ d ] );
        i[ d ] = (utf8_char*)bmem_new( bmem_strlen( g_sdriver_infos[ d ] ) + 1 ); i[ d ][ 0 ] = 0;
        bmem_strcat_resize( i[ d ], g_sdriver_infos[ d ] );
    }
    *names = n;
    *infos = i;
    return NUMBER_OF_SDRIVERS;
}

#ifndef WINCE
BOOL CALLBACK DSEnumCallback2 (
    LPGUID GUID,
    LPCSTR Description,
    LPCSTR Module,
    VOID* Context
)
{
    utf8_char*** infos = (utf8_char***)Context;
    if( *infos == 0 )
    {
        *infos = (utf8_char**)bmem_new( 512 * sizeof( void* ) );
    }
    utf8_char* ts = (utf8_char*)bmem_new( 2048 );
    sprintf( ts, "%s (%d)", Description, guids_num );
    (*infos)[ guids_num ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*infos)[ guids_num ][ 0 ] = 0; bmem_strcat_resize( (*infos)[ guids_num ], ts );
    bmem_free( ts );
    guids[ guids_num ] = GUID;
    guids_num++;
    return 1;
}
BOOL CALLBACK DSInputEnumCallback2 (
    LPGUID GUID,
    LPCSTR Description,
    LPCSTR Module,
    VOID* Context
)
{
    utf8_char*** infos = (utf8_char***)Context;
    if( *infos == 0 )
    {
        *infos = (utf8_char**)bmem_new( 512 * sizeof( void* ) );
    }
    utf8_char* ts = (utf8_char*)bmem_new( 2048 );
    sprintf( ts, "%s (%d)", Description, input_guids_num );
    (*infos)[ input_guids_num ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*infos)[ input_guids_num ][ 0 ] = 0; bmem_strcat_resize( (*infos)[ input_guids_num ], ts );
    bmem_free( ts );
    guids[ input_guids_num ] = GUID;
    input_guids_num++;
    return 1;
}
#endif

int device_sound_stream_get_devices( const utf8_char* driver, utf8_char*** names, utf8_char*** infos, bool input )
{
    int rv = 0;

    if( driver == 0 )
    {
        //Default:
        driver = g_sdriver_names[ g_sound_driver ];
    }

    int drv_num = -1;
    for( int drv = 0; drv < NUMBER_OF_SDRIVERS; drv++ )
    {
        if( bmem_strcmp( g_sdriver_names[ drv ], driver ) == 0 )
        {
            drv_num = drv;
            break;
        }
        if( bmem_strcmp( g_sdriver_infos[ drv ], driver ) == 0 )
        {
            drv_num = drv;
            break;
        }
    }
    if( drv_num == -1 ) return 0;

    *names = 0;
    *infos = 0;
    utf8_char* ts = (utf8_char*)bmem_new( 2048 );
    
    switch( drv_num )
    {
	case SDRIVER_MMSOUND:
	    {
		int devices;
		if( input )
		    devices = waveInGetNumDevs();
		else
		    devices = waveOutGetNumDevs();
	        if( devices == 0 ) break;	        
	        *names = (utf8_char**)bmem_new( devices * sizeof( void* ) );
	        *infos = (utf8_char**)bmem_new( devices * sizeof( void* ) );
		for( int d = 0; d < devices; d++ )
		{
    		    sprintf( ts, "%d", d );
    		    (*names)[ rv ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*names)[ rv ][ 0 ] = 0; bmem_strcat_resize( (*names)[ rv ], ts );
		    if( input )
		    {
    			WAVEINCAPS deviceCaps;
    			bmem_set( &deviceCaps, sizeof( deviceCaps ), 0 );
    			waveInGetDevCaps( d, &deviceCaps, sizeof( deviceCaps ) );
    			sprintf( ts, "%s (%d)", deviceCaps.szPname, d );
    			(*infos)[ rv ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*infos)[ rv ][ 0 ] = 0; bmem_strcat_resize( (*infos)[ rv ], ts );
		    }
		    else
		    {
    			WAVEOUTCAPS deviceCaps;
    			bmem_set( &deviceCaps, sizeof( deviceCaps ), 0 );
    			waveOutGetDevCaps( d, &deviceCaps, sizeof( deviceCaps ) );
    			sprintf( ts, "%s (%d)", deviceCaps.szPname, d );
    			(*infos)[ rv ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*infos)[ rv ][ 0 ] = 0; bmem_strcat_resize( (*infos)[ rv ], ts );
    		    }
    		    rv++;
    		}
	    }
	    break;
#ifndef WINCE
	case SDRIVER_DSOUND:
	    {
		if( input )
		{
		    input_guids_num = 0;
		    DirectSoundCaptureEnumerate( DSInputEnumCallback2, (void*)infos );
		    rv = input_guids_num;
		}
		else
		{
		    guids_num = 0;
		    DirectSoundEnumerate( DSEnumCallback2, (void*)infos );
		    rv = guids_num;
		}
		if( rv == 0 ) break;
		*names = (utf8_char**)bmem_new( rv * sizeof( void* ) );
		for( int d = 0; d < rv; d++ )
		{
    		    sprintf( ts, "%d", d );
    		    (*names)[ d ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*names)[ d ][ 0 ] = 0; bmem_strcat_resize( (*names)[ d ], ts );
		}
	    }
	    break;
#ifndef NOASIO
        case SDRIVER_ASIO:
    	    if( !input )
	    {
		int d = 0;
		HKEY hk = NULL;
		CHAR keyname[ 128 ];
		int err = RegOpenKey( HKEY_LOCAL_MACHINE, "software\\asio", &hk );
		while( ( err == ERROR_SUCCESS ) && ( rv < ASIO_DEVS ) )
		{
    		    err = RegEnumKey( hk, d, (LPTSTR)keyname, 128 );
    		    d++;
    		    if( err == ERROR_SUCCESS )
    		    {
    			if( *names == 0 )
                        {
                            *names = (utf8_char**)bmem_new( 512 * sizeof( void* ) );
                            *infos = (utf8_char**)bmem_new( 512 * sizeof( void* ) );
                        }
                        sprintf( ts, "%d", rv );
                        (*names)[ rv ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*names)[ rv ][ 0 ] = 0; bmem_strcat_resize( (*names)[ rv ], ts );
                        sprintf( ts, "%s (%d)", keyname, rv );
                        (*infos)[ rv ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*infos)[ rv ][ 0 ] = 0; bmem_strcat_resize( (*infos)[ rv ], ts );
        		rv++;
    		    }
		}
	    }
    	    break;
#endif
#endif
    }
    
    bmem_free( ts );
    
    return rv;
}

void device_sound_stream_deinit( void )
{
    device_sound_stream_input( 0 );
    switch( g_sound_driver )
    {
	case SDRIVER_MMSOUND:
	    if( g_waveOutStream )
	    {
            	g_waveOutExitRequest = 1;
            	int step = 20; //ms
            	int timeout = 300;
            	int t = 0;
#ifdef MM_USE_THREAD
            	while( g_waveOutExitRequest ) 
#else
            	while( g_waveOutExitRequest == 1 )
#endif
            	{ 
            	    Sleep( step ); //Waiting for thread stop
            	    t += step;
            	    if( t > timeout ) 
            	    {
            	        blog( "MMSOUND: Timeout\n" );
            		break;
            	    }
            	}
#ifdef MM_USE_THREAD
            	CloseHandle( g_waveOutThread ); g_waveOutThread = 0;
#endif
		g_waveInExitRequest = 0;
            	blog( "MMSOUND: CloseHandle (soundThread) ok\n" );
		
		MMRESULT mres;
		mres = waveOutReset( g_waveOutStream );
		if( mres != MMSYSERR_NOERROR )
		{
		    blog( "MMSOUND ERROR in waveOutReset (%d)\n", mres );
		}
		
		for( int b = 0; b < MM_MAX_BUFFERS; b++ )
		{
		    mres = waveOutUnprepareHeader( g_waveOutStream, &g_outBuffersHdr[ b ], sizeof( WAVEHDR ) );
		    if( mres != MMSYSERR_NOERROR )
		    {
			blog( "MMSOUND ERROR: Can't unprepare waveout header %d (%d)\n", b, mres );
		    }
		    free( g_outBuffersHdr[ b ].lpData );
		}
		
		mres = waveOutClose( g_waveOutStream );
		if( mres != MMSYSERR_NOERROR ) 
		    blog( "MMSOUND ERROR in waveOutClose: %d\n", mres );
		else
		    blog( "MMSOUND: waveOutClose ok\n" );
		
		g_waveOutStream = 0;
	    }
	    break;
#ifndef WINCE
	case SDRIVER_DSOUND:
	    if( lpds )
	    {
		ds_output_exit_request = 1;
		int step = 20; //ms
                int timeout = 300;
                int t = 0;
                while( ds_output_exit_request )
                {
                    Sleep( step ); //Waiting for thread stop
                    t += step;
                    if( t > timeout )
                    {
                        blog( "DSOUND: Timeout\n" );
                        break;
                    }
                }
		CloseHandle( ds_output_thread ); ds_output_thread = 0;
		ds_output_exit_request = 0;
		
		if( lpdsNotify ) lpdsNotify->Release(); lpdsNotify = 0;
		if( ds_buf ) ds_buf->Release(); ds_buf = 0;
		if( ds_buf_primary ) ds_buf_primary->Release(); ds_buf_primary = 0;
		if( ds_input_buf ) ds_input_buf->Release(); ds_input_buf = 0;
		if( lpds ) lpds->Release(); lpds = 0;
		if( lpdsCapture ) lpdsCapture->Release(); lpdsCapture = 0;
	    }
	    break;
#ifndef NOASIO
	case SDRIVER_ASIO:
	    if( g_iasio )
	    {
		blog( "ASIO stop...\n" );
		iasio_stop();
		blog( "ASIO disposeBuffers...\n" );
		iasio_disposeBuffers();
		blog( "ASIO release...\n" );
		iasio_release();
		blog( "ASIO closed.\n" );
		g_iasio = 0;
	    }
	    break;
#endif
#endif
    }

    bmem_free( g_sound_buffer );
    g_sound_buffer = 0;
    remove_input_buffers();    
}

