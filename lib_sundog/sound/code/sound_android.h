//################################
//## ANDROID                    ##
//################################

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define MAX_NUMBER_INTERFACES 4
#define MAX_NUMBER_INPUT_DEVICES 4

int g_default_buffer_size = 1024;
int g_buffer_size = 0;
uint* g_sound_output_buffer = 0;
int g_sound_output_buffer_offset = 0;
volatile int g_sound_output_callback_stop_request = 0;

int g_sound_input_buffers_count = 8;
volatile uint g_sound_input_buffer_wp = 0;
volatile uint g_sound_input_buffer_rp = 0;
uint* g_sound_input_buffer = 0;
uint* g_sound_input_silent_buffer = 0;
bool g_sound_input_enabled = 0;
int g_sound_input_fadein = 0;

SLObjectItf g_sl = 0;
SLObjectItf g_sl_output_mix = 0;
SLObjectItf g_sl_player = 0;
SLObjectItf g_sl_recorder = 0;
SLDataFormat_PCM g_sl_pcm;

int sl_check_error( SLresult res, const utf8_char* fname )
{
    if( res != SL_RESULT_SUCCESS )
    {
	blog( "OpenSLES ERROR %d (%s)\n", (int)res, fname );
	return 1;
    }
    return 0;
}

void playback_callback( SLBufferQueueItf queueItf, void* context )
{
    sound_struct* ss = (sound_struct*)context;
    
    ss->out_buffer = g_sound_output_buffer + g_sound_output_buffer_offset;
    ss->out_frames = g_buffer_size;
    ss->out_time = time_ticks_hires() + ( ( ( ( g_buffer_size << 15 ) / ss->freq ) * time_ticks_per_second_hires() ) >> 15 );
    if( g_sound_input_fadein == 0 )
        ss->in_buffer = g_sound_input_silent_buffer;
    else
        ss->in_buffer = g_sound_input_buffer + g_sound_input_buffer_rp;
    if( g_sound_input_enabled )
    {
        if( g_sound_input_fadein > 0 )
        {
    	    //Input is ready and we can't lose input packets now:
	    int step = 10; //ms
	    int t = 0;
	    int timeout = 100;
	    while( g_sound_input_buffer_rp == g_sound_input_buffer_wp )
	    {
	        int cur_step = timeout - t;
	        if( cur_step > step ) cur_step = step;
	        time_sleep( cur_step );
	        t += cur_step;
	        if( t >= timeout ) break; //One input packet lost :(
	    }
	}
	if( g_sound_input_buffer_rp != g_sound_input_buffer_wp )
	{
	    g_sound_input_buffer_rp += g_buffer_size;
	    g_sound_input_buffer_rp %= g_buffer_size * g_sound_input_buffers_count;
	    if( g_sound_input_fadein < 32768 )
	    {
	        signed short* inbuf = (signed short*)ss->in_buffer;
	        for( int i = 0; i < ss->out_frames * 2; i++ )
	        {
	    	    int v = inbuf[ i ];
		    v *= g_sound_input_fadein;
		    v /= 32768;
		    inbuf[ i ] = (signed short)v;
		    g_sound_input_fadein++;
		    if( g_sound_input_fadein >= 32768 ) break;
		}
	    }
	}
    }
    main_sound_output_callback( ss, 0 );

    if( g_sound_output_callback_stop_request > 0 )
    {
	g_sound_output_callback_stop_request = 2;
	return;
    }

    (*queueItf)->Enqueue( queueItf, g_sound_output_buffer + g_sound_output_buffer_offset, g_buffer_size * 4 );
    g_sound_output_buffer_offset += g_buffer_size;
    g_sound_output_buffer_offset %= g_buffer_size * 2;
}

void recording_callback( SLAndroidSimpleBufferQueueItf queueItf, void* context )
{
    sound_struct* ss = (sound_struct*)context;

    //Re-use this buffer:
    (*queueItf)->Enqueue( 
	queueItf, 
	g_sound_input_buffer 
	    + ( g_sound_input_buffer_wp + ( g_sound_input_buffers_count / 2 ) * g_buffer_size ) % ( g_buffer_size * g_sound_input_buffers_count ), 
	g_buffer_size * 4 );

    volatile uint wp = g_sound_input_buffer_wp;    
    wp += g_buffer_size;
    wp %= g_buffer_size * g_sound_input_buffers_count;
    g_sound_input_buffer_wp = wp;
}

int device_sound_stream_init( void )
{
    SLresult res = SL_RESULT_SUCCESS;
    
    blog( "device_sound_stream_init() begin\n" );
    
    g_sl = 0;
    
    g_buffer_size = profile_get_int_value( KEY_SOUNDBUFFER, g_default_buffer_size, 0 );
    g_sound_output_buffer = (uint*)bmem_new( g_buffer_size * 4 * 2 ); // *2 - because it is double buffered
    bmem_zero( g_sound_output_buffer );
    g_snd.out_latency = g_buffer_size;
    g_snd.in_type = sound_buffer_int16;
    g_snd.in_channels = g_snd.out_channels;
    g_sound_output_callback_stop_request = 0;
    g_sound_input_buffer_wp = 0;
    g_sound_input_buffer_rp = 0;
    g_sound_input_enabled = 0;
    
    while( 1 )
    {
	//SLEngineOption EngineOption[] = { (SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE };

	blog( "SLES: slCreateEngine\n" );	
	//res = slCreateEngine( &g_sl, 1, EngineOption, 0, NULL, NULL );
	res = slCreateEngine( &g_sl, 0, NULL, 0, NULL, NULL );
	if( sl_check_error( res, "slCreateEngine" ) ) break;
	
	// Realizing the SL Engine in synchronous mode:
	blog( "SLES: Realizing the SL Engine in synchronous mode\n" );
	res = (*g_sl)->Realize( g_sl, SL_BOOLEAN_FALSE );
	if( sl_check_error( res, "Realizing the SL Engine" ) ) break;
	
	// Get the SL Engine Interface which is implicit:
	SLEngineItf EngineItf;
	blog( "SLES: Get the SL Engine Interface\n" );
	res = (*g_sl)->GetInterface( g_sl, SL_IID_ENGINE, (void*)&EngineItf );
	if( sl_check_error( res, "Get the SL Engine Interface which is implicit" ) ) break;
	
	// Initialize arrays required[] and iidArray[]:
	SLboolean required[ MAX_NUMBER_INTERFACES ];
	SLInterfaceID iidArray[ MAX_NUMBER_INTERFACES ];
	for( int i = 0; i < MAX_NUMBER_INTERFACES; i++ )
	{
	    required[ i ] = SL_BOOLEAN_FALSE;
	    iidArray[ i ] = SL_IID_NULL;
	}
	
	// Create Output Mix object to be used by player:
	blog( "SLES: Create Output Mix object\n" );
	res = (*EngineItf)->CreateOutputMix( EngineItf, &g_sl_output_mix, 0, 0, 0 );
	if( sl_check_error( res, "Create Output Mix object" ) ) break;
	
	// Realizing the Output Mix object in synchronous mode.
	blog( "SLES: Realizing the Output Mix object\n" );
	res = (*g_sl_output_mix)->Realize( g_sl_output_mix, SL_BOOLEAN_FALSE );
	if( sl_check_error( res, "Realizing the Output Mix object" ) ) break;
	
	// Setup the data source structure for the buffer queue:
	SLDataLocator_BufferQueue bufferQueue;
	bufferQueue.locatorType = SL_DATALOCATOR_BUFFERQUEUE;
	bufferQueue.numBuffers = 2;
	
	// Setup the format of the content in the buffer queue:
	g_sl_pcm.formatType = SL_DATAFORMAT_PCM;
	g_sl_pcm.numChannels = 2;
	g_sl_pcm.samplesPerSec = g_snd.freq * 1000;
	g_sl_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	g_sl_pcm.containerSize = 16;
	g_sl_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	g_sl_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
	
	SLDataSource audioSource;
	audioSource.pFormat = (void*)&g_sl_pcm;
	audioSource.pLocator = (void*)&bufferQueue;
	
	SLDataSink audioSink;
	SLDataLocator_OutputMix locator_outputmix;
	
	// Setup the data sink structure:
	locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	locator_outputmix.outputMix = g_sl_output_mix;
	audioSink.pLocator = (void*)&locator_outputmix;
	audioSink.pFormat = NULL;
	
	// Set arrays required[] and iidArray[] for SEEK interface (PlayItf is implicit):
	required[ 0 ] = SL_BOOLEAN_TRUE;
	iidArray[ 0 ] = SL_IID_BUFFERQUEUE;
	
	// Create the music player:
	blog( "SLES: Create the music player\n" );
	res = (*EngineItf)->CreateAudioPlayer( EngineItf, &g_sl_player, &audioSource, &audioSink, 1, iidArray, required );
	if( sl_check_error( res, "Create the music player" ) ) break;
	
	// Realizing the player in synchronous mode:
	blog( "SLES: Realizing the player\n" );
	res = (*g_sl_player)->Realize( g_sl_player, SL_BOOLEAN_FALSE );
	if( sl_check_error( res, "Realizing the player" ) ) break;
	
	// Get seek and play interfaces:
	SLPlayItf playItf;
	blog( "SLES: Get seek and play interfaces\n" );
	res = (*g_sl_player)->GetInterface( g_sl_player, SL_IID_PLAY, (void*)&playItf );
	if( sl_check_error( res, "Get seek and play interfaces" ) ) break;
	
	SLBufferQueueItf bufferQueueItf;
	blog( "SLES: Get buffer queue interface\n" );
	res = (*g_sl_player)->GetInterface( g_sl_player, SL_IID_BUFFERQUEUE, (void*)&bufferQueueItf );
	if( sl_check_error( res, "bufferQueueItf" ) ) break;
	
	// Setup to receive buffer queue event callbacks:
	blog( "SLES: Setup to receive buffer queue event callbacks\n" );
	res = (*bufferQueueItf)->RegisterCallback( bufferQueueItf, playback_callback, &g_snd );
	if( sl_check_error( res, "Setup to receive buffer queue event callback" ) ) break;
	
	blog( "SLES: Enqueue...\n" );
	res = (*bufferQueueItf)->Enqueue( bufferQueueItf, g_sound_output_buffer, g_buffer_size * 4 );
	if( sl_check_error( res, "Start Enqueue 1" ) ) break;
	res = (*bufferQueueItf)->Enqueue( bufferQueueItf, g_sound_output_buffer + g_buffer_size, g_buffer_size * 4 );
	if( sl_check_error( res, "Start Enqueue 2" ) ) break;
		
	// Play the PCM samples using a buffer queue:
	blog( "SLES: Play\n" );
	res = (*playItf)->SetPlayState( playItf, SL_PLAYSTATE_PLAYING );
	if( sl_check_error( res, "Play" ) ) break;
	
	break;
    }

    blog( "device_sound_stream_init() end\n" );
    
    if( res != SL_RESULT_SUCCESS )
    {
	if( g_sl )
	{
	    (*g_sl)->Destroy( g_sl );
	    g_sl = 0;
	}
	return 1;
    }
    else
    {
	return 0;
    }
}

void device_sound_stream_input( bool enable )
{
    if( enable )
    {
	if( g_sound_input_buffer == 0 )
	{
	    g_sound_input_buffer = (uint*)bmem_new( g_buffer_size * 4 * g_sound_input_buffers_count );
	    bmem_zero( g_sound_input_buffer );
	    g_sound_input_silent_buffer = (uint*)bmem_new( g_buffer_size * 4 );
	    bmem_zero( g_sound_input_silent_buffer );
	}
	
	if( g_sl_recorder == 0 )
	{
	    SLresult res = SL_RESULT_SUCCESS;
	    
	    while( 1 )
	    {
		// Get the SL Engine Interface which is implicit:
		SLEngineItf EngineItf;
		res = (*g_sl)->GetInterface( g_sl, SL_IID_ENGINE, (void*)&EngineItf );
		if( sl_check_error( res, "Get the SL Engine Interface which is implicit" ) ) break;
	
		// Initialize arrays required[] and iidArray[]:
		SLboolean required[ MAX_NUMBER_INTERFACES ];
		SLInterfaceID iidArray[ MAX_NUMBER_INTERFACES ];
		for( int i = 0; i < MAX_NUMBER_INTERFACES; i++ )
		{
		    required[ i ] = SL_BOOLEAN_FALSE;
		    iidArray[ i ] = SL_IID_NULL;
		}
	
		// Setup the data source structure:
		SLDataLocator_IODevice locator_mic;
		locator_mic.locatorType = SL_DATALOCATOR_IODEVICE;
		locator_mic.deviceType = SL_IODEVICE_AUDIOINPUT;
		locator_mic.deviceID = SL_DEFAULTDEVICEID_AUDIOINPUT;
		locator_mic.device = NULL;
		
		SLDataSource audioSource;
		audioSource.pLocator = &locator_mic;
		audioSource.pFormat = &g_sl_pcm;
		
		SLDataLocator_AndroidSimpleBufferQueue locator_bufferqueue;
		locator_bufferqueue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
		locator_bufferqueue.numBuffers = 2;
		
		SLDataSink audioSink;
		audioSink.pLocator = &locator_bufferqueue;
		audioSink.pFormat = &g_sl_pcm;
		
		// Create audio recorder:
		required[ 0 ] = SL_BOOLEAN_TRUE;
		iidArray[ 0 ] = SL_IID_ANDROIDSIMPLEBUFFERQUEUE;
		res = (*EngineItf)->CreateAudioRecorder( EngineItf, &g_sl_recorder, &audioSource, &audioSink, 1, iidArray, required );
		if( sl_check_error( res, "CreateAudioRecorder" ) ) break;
		
		res = (*g_sl_recorder)->Realize( g_sl_recorder, SL_BOOLEAN_FALSE );
		if( sl_check_error( res, "Recorder Realize" ) ) break;
		
		// Register callback:
		SLRecordItf recorderRecord;
		res = (*g_sl_recorder)->GetInterface( g_sl_recorder, SL_IID_RECORD, &recorderRecord );
		if( sl_check_error( res, "Get SL_IID_RECORD interface" ) ) break;
		SLAndroidSimpleBufferQueueItf recorderBufferQueue;
		res = (*g_sl_recorder)->GetInterface( g_sl_recorder, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recorderBufferQueue );
		if( sl_check_error( res, "Get SL_IID_ANDROIDSIMPLEBUFFERQUEUE interface" ) ) break;
		res = (*recorderBufferQueue)->RegisterCallback( recorderBufferQueue, recording_callback, &g_snd );
		if( sl_check_error( res, "Recorder RegisterCallback" ) ) break;
		
		for( int i = 0; i < g_sound_input_buffers_count / 2; i++ )
		{
		    (*recorderBufferQueue)->Enqueue( recorderBufferQueue, g_sound_input_buffer + g_buffer_size * i, g_buffer_size * 4 );
		}
		
		// Start recording:
		g_sound_input_buffer_wp = 0;
		g_sound_input_buffer_rp = 0;
		g_sound_input_fadein = 0;
		g_sound_input_enabled = 1;
		res = (*recorderRecord)->SetRecordState( recorderRecord, SL_RECORDSTATE_RECORDING );
		if( sl_check_error( res, "Start recording" ) ) break;
		
		break;
	    }
	    
	    if( res != SL_RESULT_SUCCESS )
	    {
		if( g_sl_recorder )
		{
		    g_sound_input_enabled = 0;
		    (*g_sl_recorder)->Destroy( g_sl_recorder );
		    g_sl_recorder = 0;
		}
	    }
	}
    }
    else 
    {
	if( g_sl_recorder )
	{
	    g_sound_input_enabled = 0;
	    (*g_sl_recorder)->Destroy( g_sl_recorder );
	    g_sl_recorder = 0;
	}
    }
}

const utf8_char* device_sound_stream_get_driver_info( void )
{
    return "OpenSL ES";
}

const utf8_char* device_sound_stream_get_driver_name( void )
{
    return "opensles";
}

int device_sound_stream_get_drivers( utf8_char*** names, utf8_char*** infos )
{
    return 0;
}

int device_sound_stream_get_devices( const utf8_char* driver, utf8_char*** names, utf8_char*** infos, bool input )
{
    return 0;
}

void device_sound_stream_deinit( void )
{
    SLresult res;
    
    if( g_sl )
    {
	device_sound_stream_input( 0 );

	if( g_sound_output_callback_stop_request == 0 )
	{
	    //SetPlayState( playItf, SL_PLAYSTATE_STOPPED ) does not stop the callback immediately - tested on LG Optimus Hub.
	    //But we must be sure that this callback is not uses SL functions anymore (Enqueue() in particular).
	    g_sound_output_callback_stop_request = 1;
	    int step = 50;
	    int timeout = 1000;
	    int t = 0;
	    while( g_sound_output_callback_stop_request != 2 )
	    {
		time_sleep( step );
		t += step;
		if( t >= timeout ) break;
	    }
	}

	if( g_sl_player )
	{
	    SLPlayItf playItf;
	    res = (*g_sl_player)->GetInterface( g_sl_player, SL_IID_PLAY, (void*)&playItf );
	    if( res == SL_RESULT_SUCCESS )
	    {
		(*playItf)->SetPlayState( playItf, SL_PLAYSTATE_STOPPED );
	    }
	    (*g_sl_player)->Destroy( g_sl_player );
	    g_sl_player = 0;
	}

	if( g_sl_output_mix )
	{
	    (*g_sl_output_mix)->Destroy( g_sl_output_mix );  
	    g_sl_output_mix = 0;
	}
	
	(*g_sl)->Destroy( g_sl );
	g_sl = 0;
    }
    
    bmem_free( g_sound_output_buffer );
    g_sound_output_buffer = 0;
    
    bmem_free( g_sound_input_buffer );
    g_sound_input_buffer = 0;
    
    bmem_free( g_sound_input_silent_buffer );
    g_sound_input_silent_buffer = 0;
}
