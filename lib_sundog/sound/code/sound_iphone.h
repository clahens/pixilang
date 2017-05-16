//################################
//## IOS (IPHONE, IPAD)         ##
//################################

#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

enum
{
#ifdef JACK_AUDIO
    SDRIVER_JACK,
#endif
    SDRIVER_AUDIOUNIT,
    NUMBER_OF_SDRIVERS
};

const utf8_char* g_sdriver_infos[] =
{
#ifdef JACK_AUDIO
    "JACK",
#endif
#ifdef AUDIOBUS
    "Audio Unit / Audiobus",
#else
    "Audio Unit",
#endif
};

const utf8_char* g_sdriver_names[] =
{
#ifdef JACK_AUDIO
    "JACK",
#endif
    "audiounit",
};

int g_sound_driver = 0;

//MIDI:
#define MIDI_EVENTS ( 2048 )
#define MIDI_BYTES ( MIDI_EVENTS * 16 )
#ifdef JACK_AUDIO
    #include "sound_jack.h"
#endif
#include "sound_coremidi.h"

//
// Audio Unit / Audiobus
//

#define kOutputBus 0
#define kInputBus 1

#define INPUT_BUF_FRAMES ( 64 * 1024 )

AudioComponentInstance g_aunit = 0;
bool g_au_input = 0;
volatile int g_au_pause_request = 0;

char g_input_bufs[ 512 ];
void* g_input_buf_lowlevel;
uint* g_input_buf = 0;
void* g_input_buf2 = 0;
volatile uint g_input_buf_wp = 0;
uint g_input_buf_rp = 0;

ticks_hr_t g_sound_idle_start_time = 0;
int g_sound_idle_seconds = 0;

volatile bool g_sound_device_initialized = 0;

OSStatus au_playback_callback(
    void* inRefCon,
    AudioUnitRenderActionFlags* ioActionFlags,
    const AudioTimeStamp* inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList* ioData )
{
    double t = (double)inTimeStamp->mHostTime * g_timebase_nanosec / 20000;
    unsigned long long output_time = (unsigned long long)t;
    for( int b = 0; b < ioData->mNumberBuffers; b++ )
    {
	AudioBuffer* buf = &ioData->mBuffers[ b ];
	g_snd.out_buffer = buf->mData;
	g_snd.out_frames = buf->mDataByteSize / ( 2 * g_snd.out_channels );
	g_snd.out_time = (ticks_hr_t)output_time;
	g_snd.out_latency = g_snd.out_frames;
        g_snd.in_buffer = g_input_buf2;
        if( g_input_buf2 && ( ( g_input_buf_wp - g_input_buf_rp ) & ( INPUT_BUF_FRAMES - 1 ) ) >= g_snd.out_frames )
        {
            for( int i = 0; i < g_snd.out_frames; i++ )
            {
                ((uint*)g_input_buf2)[ i ] = g_input_buf[ g_input_buf_rp ];
                g_input_buf_rp++;
                g_input_buf_rp &= ( INPUT_BUF_FRAMES - 1 );
            }
        }
        	
	if( g_au_pause_request > 0 )
	{
	    g_au_pause_request = 2;
	    memset( g_snd.out_buffer, 0, buf->mDataByteSize );
            if( ioActionFlags ) *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
	}
	else
	{
	    if( main_sound_output_callback( &g_snd, 0 ) == 0 )
            {
		//Silence:
		if(
#ifdef AUDIOBUS
		   audiobus_connected() == 0
#else
		   1
#endif
		   && g_au_input == 0 )
		{
		    if( g_sound_idle_start_time == 0 )
		    {
			g_sound_idle_start_time = time_ticks_hires();
		    }
		    g_sound_idle_seconds = ( time_ticks_hires() - g_sound_idle_start_time ) / time_ticks_per_second_hires();
		}
                if( ioActionFlags ) *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
            }
	    else
	    {
		//Signal:
		g_sound_idle_start_time = 0;
		g_sound_idle_seconds = 0;
	    }
#ifdef AUDIOBUS_TRIGGER_PLAY
            audiobus_triggers_update();
#endif
	}
    }
    
    return noErr;
}

OSStatus au_recording_callback(
    void* inRefCon, 
    AudioUnitRenderActionFlags* ioActionFlags, 
    const AudioTimeStamp* inTimeStamp, 
    UInt32 inBusNumber, 
    UInt32 inNumberFrames, 
    AudioBufferList* ioData ) 
{
    if( inNumberFrames > 8192 ) return noErr;
    
    AudioBufferList* bufs = (AudioBufferList*)g_input_bufs;
    bufs->mNumberBuffers = 1;
    AudioBuffer* buf = &bufs->mBuffers[ 0 ];
    buf->mNumberChannels = g_snd.in_channels;
    buf->mDataByteSize = inNumberFrames * ( 2 * g_snd.in_channels );
    buf->mData = g_input_buf_lowlevel;
    
    //Obtain recorded samples:
    AudioUnitRender(
        g_aunit, 
        ioActionFlags, 
        inTimeStamp, 
        inBusNumber, 
        inNumberFrames, 
        bufs );
    
    volatile uint wp = g_input_buf_wp;
    for( int i = 0; i < inNumberFrames; i++ )
    {
	g_input_buf[ wp ] = ((uint*)buf->mData)[ i ];
	wp++;
	wp &= ( INPUT_BUF_FRAMES - 1 );
    }
    g_input_buf_wp = wp;
    
    return noErr;
}

int au_new( bool with_input, int sample_rate, int channels )
{
    OSStatus status;
    bool input = 0;
    
    size_t size;
    size = 8192 * 4; g_input_buf_lowlevel = malloc( size ); memset( g_input_buf_lowlevel, 0, size );
    size = INPUT_BUF_FRAMES * 4; g_input_buf = (uint*)malloc( size ); memset( g_input_buf, 0, size );
    size = 8192 * 4; g_input_buf2 = malloc( size ); memset( g_input_buf2, 0, size );
    g_input_buf_wp = 0;
    g_input_buf_rp = 0;

    if( g_aunit == 0 )
    {    
	//Create new RemoteIO Audio Unit:
	AudioComponentDescription desc;
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_RemoteIO;
	desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        //Get component:
        AudioComponent comp = AudioComponentFindNext( NULL, &desc );
        if( comp == 0 )
        {
            blog( "AUDIO UNIT ERROR: AudioComponent not found\n" );
            return -2;
        }
        //Get audio unit:
        status = AudioComponentInstanceNew( comp, &g_aunit );
        if( status != noErr )
	{
    	    blog( "AUDIO UNIT ERROR: AudioComponentInstanceNew failed with code %d\n", status );
	    g_aunit = 0;
            return -3;
        }
    }
        
    //Enable IO for playback:
    UInt32 val = 1;
    status = AudioUnitSetProperty(
	g_aunit,
	kAudioOutputUnitProperty_EnableIO,
	kAudioUnitScope_Output,
	kOutputBus,
	&val,
	sizeof( val ) );
    if( status != noErr )
    {
	blog( "AUDIO UNIT ERROR: AudioUnitSetProperty (output) failed with code %d\n", status );
	if( status == kAudioUnitErr_Initialized )
	{
	    blog( "AU is already initialized\n" ); //but not by the SunDog... Maybe by IAA host?
#ifdef AUDIOBUS
	    audiobus_enable_ports( 0 );
#endif
	    AudioOutputUnitStop( g_aunit );
	    AudioUnitUninitialize( g_aunit );
	    status = AudioUnitSetProperty(
					  g_aunit,
					  kAudioOutputUnitProperty_EnableIO,
					  kAudioUnitScope_Output,
					  kOutputBus,
					  &val,
					  sizeof( val ) );
	    if( status != noErr )
	    {
		blog( "failed\n" );
		return -4;
	    }
	}
	else
	    return -4;
    }
        
    //Enable IO for recording:
    if( with_input )
    {
	val = 1;
	input = 1;
    }
    else
    {
	val = 0;
	input = 0;
    }
    status = AudioUnitSetProperty(
	g_aunit,
	kAudioOutputUnitProperty_EnableIO,
	kAudioUnitScope_Input,
	kInputBus,
	&val,
	sizeof( val ) );
    if( status != noErr )
    {
	blog( "AUDIO UNIT ERROR: AudioUnitSetProperty (input) failed with code %d\n", status );
	input = 0;
    }
    
    //Describe audio stream format:
    AudioStreamBasicDescription af;
    af.mSampleRate = sample_rate;
    af.mFormatID = kAudioFormatLinearPCM;
    af.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    af.mFramesPerPacket = 1;
    af.mChannelsPerFrame = channels;
    af.mBitsPerChannel = 16;
    af.mBytesPerPacket = 2 * channels;
    af.mBytesPerFrame = 2 * channels;
    
    //Set output stream format:
    status = AudioUnitSetProperty(
	g_aunit,
        kAudioUnitProperty_StreamFormat,
        kAudioUnitScope_Input,
        kOutputBus,
        &af,
        sizeof( af ) );
    if( status != noErr )
    {
	blog( "AUDIO UNIT ERROR: AudioUnitSetProperty (output,streamformat) failed with code %d\n", status );
	return -5;
    }
    
    //Set input stream format:
    if( input )
    {
	AudioStreamBasicDescription input_af;
	UInt32 input_af_size = sizeof( input_af );
	status = AudioUnitGetProperty(
	    g_aunit,
	    kAudioUnitProperty_StreamFormat,
	    kAudioUnitScope_Output,
	    kInputBus,
	    &input_af,
	    &input_af_size );
	if( status != noErr )
	{
	    blog( "AUDIO UNIT ERROR: AudioUnitGetProperty (input,streamformat) failed with code %d\n", status );
	}
	else
	{
	    blog( "AUDIO INPUT Default Sample Rate: %d\n", (int)input_af.mSampleRate );
	    blog( "AUDIO INPUT Channels: %d\n", (int)input_af.mChannelsPerFrame );
	    blog( "AUDIO INPUT Bits Per Channel: %d\n", (int)input_af.mBitsPerChannel );
	}
	status = AudioUnitSetProperty(
	    g_aunit,
	    kAudioUnitProperty_StreamFormat,
	    kAudioUnitScope_Output,
	    kInputBus,
	    &af,
	    sizeof( af ) );
	if( status != noErr )
	{
	    blog( "AUDIO UNIT ERROR: AudioUnitSetProperty (input,streamformat) failed with code %d\n", status );
	    input = 0;
	}
    }
    
    //Set output callback:
    AURenderCallbackStruct cs;
    cs.inputProc = au_playback_callback;
    cs.inputProcRefCon = g_aunit;
    status = AudioUnitSetProperty(
	g_aunit,
	kAudioUnitProperty_SetRenderCallback,
	kAudioUnitScope_Global,
	kOutputBus,
	&cs,
	sizeof( cs ) );
    if( status != noErr )
    {
	blog( "AUDIO UNIT ERROR: AudioUnitSetProperty (output,setrendercallback) failed with code %d\n", status );
	return -6;
    }
    
    //Set input callback:
    if( input )
    {
	AURenderCallbackStruct cs;
	cs.inputProc = au_recording_callback;
	cs.inputProcRefCon = g_aunit;
	status = AudioUnitSetProperty(
	    g_aunit,
	    kAudioOutputUnitProperty_SetInputCallback,
	    kAudioUnitScope_Global,
	    kInputBus,
	    &cs,
	    sizeof( cs ) );
	if( status != noErr )
	{
	    blog( "AUDIO UNIT ERROR: AudioUnitSetProperty (input,setinputcallback) failed with code %d\n", status );
	    input = 0;
	}
    }
    
    //Initialize:
    status = AudioUnitInitialize( g_aunit );
    if( status != noErr )
    {
	blog( "AUDIO UNIT ERROR: AudioUnitInitialize failed with code %d\n", status );
	return -7;
    }
    
    g_au_pause_request = 0;
    
    status = AudioOutputUnitStart( g_aunit );
    if( status != noErr )
    {
	blog( "AUDIO UNIT ERROR: AudioOutputUnitStart failed with code %d\n", status );
	return -8;
    }

    HANDLE_THREAD_EVENTS;

#ifdef AUDIOBUS
    audiobus_enable_ports( 1 );
#endif

    return 0;
}

void au_remove( void )
{
    if( g_aunit == 0 ) return;
    
    g_au_pause_request = 1;
    ticks_t t = time_ticks();
    while( g_au_pause_request == 1 )
    {
        if( ( time_ticks() - t ) > time_ticks_per_second() / 2 ) break;
        time_sleep( 1 );
        HANDLE_THREAD_EVENTS;
    }
                
    blog( "AUDIO UNIT: deinit...\n" );
#ifdef AUDIOBUS
    audiobus_enable_ports( 0 );
#endif
    AudioOutputUnitStop( g_aunit );
    HANDLE_THREAD_EVENTS;
    AudioUnitUninitialize( g_aunit );
    HANDLE_THREAD_EVENTS;
#ifndef AUDIOBUS
    AudioComponentInstanceDispose( g_aunit );
    HANDLE_THREAD_EVENTS;
    g_aunit = 0;
#endif
    
    free( g_input_buf_lowlevel );
    free( g_input_buf );
    free( g_input_buf2 );
    g_input_buf_lowlevel = 0;
    g_input_buf = 0;
    g_input_buf2 = 0;
                
    HANDLE_THREAD_EVENTS;                
}

int device_sound_stream_init_audiounit( void )
{
    return au_new( g_au_input, g_snd.freq, g_snd.out_channels );
}

void device_sound_stream_deinit_audiounit( void )
{
    au_remove();
}

int device_sound_stream_reinit_audiounit( void )
{
    if( g_aunit == 0 ) return -1;
    OSStatus status;
    status = AudioOutputUnitStart( g_aunit );
    if( status != noErr )
    {
        blog( "AUDIO UNIT ERROR (restart): AudioOutputUnitStart failed with code %d\n", status );
        return -1;
    }	
    return 0;
}

int device_sound_stream_init( void )
{
    audio_session_init( g_au_input, g_snd.freq, profile_get_int_value( KEY_SOUNDBUFFER, 0, 0 ) );

    if( g_sound_device_initialized )
    {
        //Audio device is already initialized. We just need to restart it (from the sundog_bridge.mm -> iphone_sundog_event_handler):
	switch( g_sound_driver )
	{
	    case SDRIVER_AUDIOUNIT:
	        device_sound_stream_reinit_audiounit();
	        break;
	    default:
		break;
	}
	return 0;
    }
    
    g_snd.in_type = sound_buffer_int16;
    g_snd.in_channels = g_snd.out_channels;
    g_snd.freq = get_audio_session_freq();
    
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
    
    bool sdriver_checked[ NUMBER_OF_SDRIVERS ];
    bmem_set( sdriver_checked, sizeof( sdriver_checked ), 0 );
    
    while( 1 )
    {
	bool sdriver_ok = 0;
	switch( g_sound_driver )
	{
#ifdef JACK_AUDIO
	    case SDRIVER_JACK:
	        if( device_sound_stream_init_jack() == 0 ) sdriver_ok = 1;
	        break;
#endif
	    case SDRIVER_AUDIOUNIT:
	        if( device_sound_stream_init_audiounit() == 0 ) sdriver_ok = 1;
	        break;
	    default:
	        break;
	}
	if( sdriver_ok ) break;
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
    
    g_sound_device_initialized = 1;
    
    return 0;
}

void device_sound_stream_input( bool enable )
{
    switch( g_sound_driver )
    {
        case SDRIVER_AUDIOUNIT:
            device_sound_stream_deinit();
            if( enable )
            {
                g_input_buf_wp = 0;
                g_input_buf_rp = 0;
                g_au_input = 1;
            }
            else 
            {
                g_input_buf_wp = 0;
                g_input_buf_rp = 0;
                g_au_input = 0;
            }
            device_sound_stream_init();
            break;
    }
}

const utf8_char* device_sound_stream_get_driver_info( void )
{
    if( (unsigned)g_sound_driver < NUMBER_OF_SDRIVERS )
	return g_sdriver_infos[ g_sound_driver ];
    return 0;
}

const utf8_char* device_sound_stream_get_driver_name( void )
{
    if( (unsigned)g_sound_driver < NUMBER_OF_SDRIVERS )
	return g_sdriver_names[ g_sound_driver ];
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

int device_sound_stream_get_devices( const utf8_char* driver, utf8_char*** names, utf8_char*** infos, bool input )
{
    return 0;
}

void device_sound_stream_deinit( void )
{
    switch( g_sound_driver )
    {
#ifdef JACK_AUDIO
        case SDRIVER_JACK:
            {
                device_sound_stream_deinit_jack();
            }
            break;
#endif
        case SDRIVER_AUDIOUNIT:
            {
        	device_sound_stream_deinit_audiounit();
            }
            break;
    }

    blog( "Disabling audio session...\n" );
#ifndef AUDIOBUS
    audio_session_deinit();
    //When Audiobus enabled, audio session will be closed in the main thread (in audiobus_deinit())
#endif
    
    blog( "device_sound_stream_deinit() ok\n" );

    HANDLE_THREAD_EVENTS;
    
    g_sound_device_initialized = 0;
}
