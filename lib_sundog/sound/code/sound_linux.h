//################################
//## LINUX                      ##
//################################

#include <pthread.h>

enum 
{
#ifndef NOALSA
    SDRIVER_ALSA,
#endif
#ifdef JACK_AUDIO
    SDRIVER_JACK,
#endif
#ifndef NOSDL
    SDRIVER_SDL,
#endif
#ifndef NOOSS
    SDRIVER_OSS,
#endif
    NUMBER_OF_SDRIVERS
};

const utf8_char* g_sdriver_infos[] =
{
#ifndef NOALSA
    "ALSA",
#endif
#ifdef JACK_AUDIO
    "JACK",
#endif
#ifndef NOSDL
    "SDL",
#endif
#ifndef NOOSS
    "OSS",
#endif
};

const utf8_char* g_sdriver_names[] =
{
#ifndef NOALSA
    "alsa",
#endif
#ifdef JACK_AUDIO
    "jack",
#endif
#ifndef NOSDL
    "sdl",
#endif
#ifndef NOOSS
    "oss",
#endif
};

int g_sound_driver = 0;

int g_default_buffer_size = 2048;
int g_buffer_size = 0;
void* g_sound_output_buffer = 0;
int dsp;
pthread_t pth;
volatile int g_sound_thread_exit_request = 0;
volatile int g_input_sound_thread_exit_request = 0;

//ALSA:
#ifndef NOALSA
#include <alsa/asoundlib.h>
#endif

//OSS:
#ifndef NOOSS
#include <linux/soundcard.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

//SDL:
#ifndef NOSDL
#include "SDL/SDL.h"
#endif

//MIDI:
#define MIDI_EVENTS ( 2048 )
#define MIDI_BYTES ( MIDI_EVENTS * 16 )
#ifdef JACK_AUDIO
    #include "sound_jack.h"
#endif
#include "sound_linux_alsa_midi.h"

//################################
//## SOUND                      ##
//################################

#ifndef NOALSA

snd_pcm_t* g_alsa_playback_handle = 0;
snd_pcm_t* g_alsa_capture_handle = 0;

int xrun_recovery( snd_pcm_t* handle, int err )
{
    if( err == -EPIPE ) 
    { //under-run:
	printf( "ALSA EPIPE (underrun)\n" );
	err = snd_pcm_prepare( handle );
	if( err < 0 )
	    printf( "ALSA Can't recovery from underrun, prepare failed: %s\n", snd_strerror( err ) );
	return 0;
    }
    else if( err == -ESTRPIPE )
    {
	printf( "ALSA ESTRPIPE\n" );
	while( ( err = snd_pcm_resume( handle ) ) == -EAGAIN )
	    sleep( 1 ); //wait until the suspend flag is released
	if( err < 0 )
	{
	    err = snd_pcm_prepare( handle );
	    if( err < 0 )
		 printf( "ALSA Can't recovery from suspend, prepare failed: %s\n", snd_strerror( err ) );
	}
	return 0;
    }
    return err;
}

#endif

void* sound_thread( void* arg )
{
    sound_struct* ss = (sound_struct*)arg;
    void* buf = g_sound_output_buffer;
    volatile int err;
    while( 1 )
    {
	int len = g_buffer_size;
	ss->out_buffer = buf;
	ss->out_frames = len;
	ss->out_time = time_ticks_hires() + ( ( (uint64)g_buffer_size * (uint64)time_ticks_per_second_hires() ) / (uint64)ss->freq );
	get_input_data( ss->out_frames );
	main_sound_output_callback( ss, 0 );
	bool alsa = 0;
	bool oss = 0;
	int frame_size;
	if( g_snd.out_type == sound_buffer_int16 )
	    frame_size = 2 * g_snd.out_channels;
	if( g_snd.out_type == sound_buffer_float32 )
	    frame_size = 4 * g_snd.out_channels;
#ifndef NOALSA
	if( g_sound_driver == SDRIVER_ALSA )
	{
	    if( g_alsa_playback_handle != 0 && g_sound_thread_exit_request == 0 )
	    {
		alsa = 1;
	    }
	}
#endif
#ifndef NOOSS
	if( g_sound_driver == SDRIVER_OSS )
	{
	    if( dsp >= 0 && g_sound_thread_exit_request == 0 )
	    {
		oss = 1;
	    }
	}
#endif
#ifndef NOALSA
	if( alsa ) 
	{
	    if( g_snd.out_type == sound_buffer_float32 )
	    {
		float* fb = (float*)buf;
		signed short* sb = (signed short*)buf;
		for( int i = 0; i < g_buffer_size * g_snd.out_channels; i++ )
		    FLOAT32_TO_INT16( sb[ i ], fb[ i ] );
	    }
	    char* buf_ptr = (char*)buf;
	    int err;
	    while( len > 0 ) 
	    {
		err = snd_pcm_writei( g_alsa_playback_handle, buf_ptr, len );
		if( err == -EAGAIN )
                    continue;
		if( err < 0 ) 
		{
		    printf( "ALSA snd_pcm_writei error: %s\n", snd_strerror( err ) );
		    err = xrun_recovery( g_alsa_playback_handle, err );
		    if( err < 0 )
		    {
			printf( "ALSA xrun_recovery error: %s\n", snd_strerror( err ) );
			goto sound_thread_exit;
		    }
		}
		else
		{
		    len -= err;
		    buf_ptr += err * frame_size;
		}
	    }
	} 
#endif
#ifndef NOOSS
	if( oss )
	{
	    if( g_snd.out_type == sound_buffer_float32 )
	    {
		float* fb = (float*)buf;
		signed short* sb = (signed short*)buf;
		for( int i = 0; i < g_buffer_size * g_snd.out_channels; i++ )
		    FLOAT32_TO_INT16( sb[ i ], fb[ i ] );
	    }
	    err = write( dsp, buf, len * 4 ); 
	}
#endif

	if( alsa == 0 && oss == 0 ) 
	{
	    break;
	}
    }
sound_thread_exit:
    g_sound_thread_exit_request = 0;
    pthread_exit( 0 );
    return 0;
}

void* input_sound_thread( void* arg )
{
    sound_struct* ss = (sound_struct*)arg;
    while( g_input_sound_thread_exit_request == 0 ) 
    {
	if( 0
#ifndef NOALSA
	    || g_sound_driver == SDRIVER_ALSA
#endif
#ifndef NOOSS
	    || g_sound_driver == SDRIVER_OSS
#endif
#ifndef NOSDL
    	    || g_sound_driver == SDRIVER_SDL
#endif
	)
	{
	    int len = 128;
	    int frame_size = 0;
	    if( g_snd.in_type == sound_buffer_int16 ) frame_size = 2 * g_snd.in_channels;
	    if( g_snd.in_type == sound_buffer_float32 ) frame_size = 4 * g_snd.in_channels;
	    volatile uint new_wp = g_sound_input_buffer_wp & ( g_sound_input_buffer_size - 1 );
	    g_sound_input_buffer_wp = new_wp;
	    if( g_sound_input_buffer_wp + len > g_sound_input_buffer_size )
		len = g_sound_input_buffer_size - g_sound_input_buffer_wp;
    	    char* buf_ptr = (char*)g_sound_input_buffer + g_sound_input_buffer_wp * frame_size;
    	    while( len > 0 && g_input_sound_thread_exit_request == 0 )
    	    {
    		int err;
    		err = snd_pcm_readi( g_alsa_capture_handle, buf_ptr, len );
    		if( err < 0 )
    		{
    		    if( err == -EAGAIN )
                	continue;
            	    printf( "ALSA INPUT overrun\n" );
            	    snd_pcm_prepare( g_alsa_capture_handle );
                }
		else
		{
		    if( g_snd.in_type == sound_buffer_float32 )
        	    {
        		//Convert from INT16 to FLOAT32
            		float* fb = (float*)buf_ptr;
            		signed short* sb = (signed short*)buf_ptr;
            		for( int i = err * g_snd.in_channels - 1; i >= 0; i-- )
                	    INT16_TO_FLOAT32( fb[ i ], sb[ i ] );
        	    }
		    len -= err;
		    buf_ptr += err * frame_size;
		    new_wp = ( g_sound_input_buffer_wp + err ) & ( g_sound_input_buffer_size - 1 );
		    g_sound_input_buffer_wp = new_wp;
		}
    	    }
        }
    }
    g_input_sound_thread_exit_request = 0;
    pthread_exit( 0 );
    return 0;
}

#ifndef NOSDL

void sdl_audio_callback( void* udata, Uint8* stream, int len )
{
    sound_struct* ss = (sound_struct*)udata;
    
    ss->out_buffer = stream;
    ss->out_frames = len / 4;
    ss->out_time = time_ticks_hires() + ( ( (uint64)g_buffer_size * (uint64)time_ticks_per_second_hires() ) / (uint64)ss->freq );
    get_input_data( ss->out_frames );
    main_sound_output_callback( ss, 0 );
}

#endif

int device_sound_stream_init_alsa( bool input )
{
#ifndef NOALSA
    int rv = 1;
    int err;
    snd_pcm_hw_params_t* hw_params;
    snd_pcm_sw_params_t* sw_params;
    
    snd_pcm_t** handle;
    const utf8_char* key;
    const utf8_char* input_label = "";
    snd_pcm_stream_t direction;
    if( input )
    {
	handle = &g_alsa_capture_handle;
	key = KEY_AUDIODEVICE_IN;
	input_label = " INPUT";
	direction = SND_PCM_STREAM_CAPTURE;
    }
    else
    {
	handle = &g_alsa_playback_handle;
	key = KEY_AUDIODEVICE;
	direction = SND_PCM_STREAM_PLAYBACK;
    }
    if( *handle ) return 0; //Already open

    while( 1 )
    {    
	snd_pcm_hw_params_malloc( &hw_params );
	snd_pcm_sw_params_malloc( &sw_params );
    
	const utf8_char* device_name = 0;
	const utf8_char* next_device_name = 0;
	bool device_err = 1;
	device_name = profile_get_str_value( key, "", 0 );
	if( device_name[ 0 ] != 0 )
	{
	    err = snd_pcm_open( handle, device_name, direction, 0 );
	    if( err < 0 ) 
	    {
		blog( "ALSA%s ERROR: Can't open audio device %s: %s\n", input_label, device_name, snd_strerror( err ) );
		device_err = 1;
	    }
	    else
	    {
		blog( "ALSA%s: %s\n", input_label, device_name );
		device_err = 0;
	    }
	}
	next_device_name = "pulse";
	if( device_err && strcmp( device_name, next_device_name ) ) 
	{
	    err = snd_pcm_open( handle, next_device_name, direction, 0 );
	    if( err < 0 ) 
	    {
	        blog( "ALSA%s ERROR: Can't open audio device %s: %s\n", input_label, next_device_name, snd_strerror( err ) );
		device_err = 1;
	    }
	    else 
	    {
		blog( "ALSA%s: %s\n", input_label, next_device_name );
		device_err = 0;
	    }
	}
	next_device_name = "default";
	if( device_err && strcmp( device_name, next_device_name ) ) 
	{
	    err = snd_pcm_open( handle, next_device_name, direction, 0 );
	    if( err < 0 )
	    {
		blog( "ALSA%s ERROR: Can't open audio device %s: %s\n", input_label, next_device_name, snd_strerror( err ) );
		device_err = 1;
	    }
	    else 
	    {
		blog( "ALSA%s: %s\n", input_label, next_device_name );
		device_err = 0;
	    }
	}
	if( device_err ) break;
    
	err = snd_pcm_hw_params_any( *handle, hw_params );
	if( err < 0 ) 
	{
	    blog( "ALSA%s ERROR: Can't initialize hardware parameter structure: %s\n", input_label, snd_strerror( err ) );
	    break;
	}
	err = snd_pcm_hw_params_set_access( *handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED );
	if( err < 0 ) 
	{
	    blog( "ALSA%s ERROR: Can't set access type: %s\n", input_label, snd_strerror( err ) );
	    break;
	}
	err = snd_pcm_hw_params_set_format( *handle, hw_params, SND_PCM_FORMAT_S16_LE );
	if( err < 0 ) 
	{
	    blog( "ALSA%s ERROR: Can't set sample format: %s\n", input_label, snd_strerror( err ) );
	    break;
	}
	err = snd_pcm_hw_params_set_rate_near( *handle, hw_params, (unsigned int*)&g_snd.freq, 0 );
	if( err < 0 ) 
	{
	    blog( "ALSA%s ERROR: Can't set rate: %s\n", input_label, snd_strerror( err ) );
	    break;
	}
	blog( "ALSA%s HW Default rate: %d\n", input_label, g_snd.freq );
	err = snd_pcm_hw_params_set_channels( *handle, hw_params, g_snd.out_channels );
	if( err < 0 ) 
	{
    	    blog( "ALSA%s ERROR: Can't set channel count: %s\n", input_label, snd_strerror( err ) );
	    break;
	}
	snd_pcm_uframes_t frames = g_buffer_size * 2;
	err = snd_pcm_hw_params_set_buffer_size_near( *handle, hw_params, &frames );
	if( err < 0 ) 
	{
	    blog( "ALSA%s ERROR: Can't set buffer size: %s\n", input_label, snd_strerror( err ) );
	    break;
	}
	err = snd_pcm_hw_params( *handle, hw_params );
	if( err < 0 ) 
	{
	    blog( "ALSA%s ERROR: Can't set parameters: %s\n", input_label, snd_strerror( err ) );
	    break;
	}

	snd_pcm_hw_params_current( *handle, hw_params );
	snd_pcm_hw_params_get_rate( hw_params, (unsigned int*)&g_snd.freq, 0 );
	blog( "ALSA%s HW Rate: %d frames\n", input_label, g_snd.freq );
	snd_pcm_hw_params_get_buffer_size( hw_params, &frames );
	blog( "ALSA%s HW Buffer size: %d frames\n", input_label, frames );
	snd_pcm_hw_params_get_period_size( hw_params, &frames, NULL );
	blog( "ALSA%s HW Period size: %d\n", input_label, (int)frames );
	unsigned int v;
	snd_pcm_hw_params_get_periods( hw_params, &v, NULL );
	blog( "ALSA%s HW Periods: %d\n", input_label, v );
    
	snd_pcm_sw_params_current( *handle, sw_params );
	snd_pcm_sw_params_get_avail_min( sw_params, &frames );
	blog( "ALSA%s SW Avail min: %d\n", input_label, (int)frames );
	snd_pcm_sw_params_get_start_threshold( sw_params, &frames );
	blog( "ALSA%s SW Start threshold: %d\n", input_label, (int)frames );
	snd_pcm_sw_params_get_stop_threshold( sw_params, &frames );
	blog( "ALSA%s SW Stop threshold: %d\n", input_label, (int)frames );
    
	//snd_pcm_sw_params_set_start_threshold( *handle, sw_params, g_buffer_size - 32 );
	//snd_pcm_sw_params_set_avail_min( *handle, sw_params, frames / 8 );
	//snd_pcm_sw_params( *handle, sw_params );
    
	snd_pcm_hw_params_free( hw_params );
	snd_pcm_sw_params_free( sw_params );
	
	if( input )
	{
	    g_sound_input_buffer_wp = 0;
            g_sound_input_buffer_rp = 0;
	}
	else
	{
	    int frame_size;
	    if( g_snd.out_type == sound_buffer_int16 )
	        frame_size = 2 * g_snd.out_channels;
	    if( g_snd.out_type == sound_buffer_float32 )
    		frame_size = 4 * g_snd.out_channels;    
    	    bmem_free( g_sound_output_buffer );
	    g_sound_output_buffer = bmem_new( g_buffer_size * frame_size );
	}
    
	err = snd_pcm_prepare( *handle );
	if( err < 0 ) 
	{
	    blog( "ALSA%s ERROR: Can't prepare audio interface for use: %s\n", input_label, snd_strerror( err ) );
	    break;
	}    
	//Create sound thread:
	if( input )
	{
	    if( pthread_create( &pth, NULL, input_sound_thread, &g_snd ) != 0 )
	    {
    		blog( "ALSA%s ERROR: Can't create sound thread!\n", input_label );
    		break;
    	    }
	}
	else
	{
	    if( pthread_create( &pth, NULL, sound_thread, &g_snd ) != 0 )
	    {
    		blog( "ALSA%s ERROR: Can't create sound thread!\n", input_label );
    		break;
    	    }
	}
    
	if( input )
	{
	}
	else
	{
#ifndef NOALSA
	    g_sound_driver = SDRIVER_ALSA;
#endif
	}
	
	rv = 0; //Successful
	
	break;
    }
    
    if( rv && handle[ 0 ] )
    {
	snd_pcm_close( *handle );
	*handle = 0;
    }

    return rv;
#else
    return 1;
#endif
}

int device_sound_stream_init_oss( void )
{
#ifndef NOOSS
    if( g_snd.out_channels != 2 )
    {
	blog( "OSS ERROR: channels must be 2\n" );
	return 1;
    }
    //Start first time:
    int temp;
    dsp = open ( profile_get_str_value( KEY_AUDIODEVICE, "/dev/dsp", 0 ), O_WRONLY, 0 );
    if( dsp == -1 )
	dsp = open ( "/dev/.static/dev/dsp", O_WRONLY, 0 );
    if( dsp == -1 )
    {
        blog( "OSS ERROR: Can't open sound device\n" );
        return 1;
    }
    temp = 1;
    ioctl( dsp, SNDCTL_DSP_STEREO, &temp );
    temp = 16;
    ioctl( dsp, SNDCTL_DSP_SAMPLESIZE, &temp );
    temp = g_snd.freq;
    ioctl( dsp, SNDCTL_DSP_SPEED, &temp );
    temp = 16 << 16 | 8;
    ioctl( dsp, SNDCTL_DSP_SETFRAGMENT, &temp );
    ioctl( dsp, SNDCTL_DSP_GETBLKSIZE, &temp );
    
    int frame_size;
    if( g_snd.out_type == sound_buffer_int16 )
        frame_size = 2 * g_snd.out_channels;
    if( g_snd.out_type == sound_buffer_float32 )
	frame_size = 4 * g_snd.out_channels;
    bmem_free( g_sound_output_buffer );
    g_sound_output_buffer = bmem_new( g_buffer_size * frame_size );
    
    //Create sound thread:
    if( pthread_create( &pth, NULL, sound_thread, &g_snd ) != 0 )
    {
        blog( "OSS ERROR: Can't create sound thread!\n" );
        return 1;
    }
    
    g_sound_driver = SDRIVER_OSS;
    
    return 0;
#else
    return 1;
#endif
}

int device_sound_stream_init_sdl( void )
{
#ifndef NOSDL
    SDL_AudioSpec a;
    
    a.freq = g_snd.freq;
    a.format = AUDIO_S16;
    a.channels = g_snd.out_channels;
    a.samples = g_buffer_size;
    a.callback = sdl_audio_callback;
    a.userdata = &g_snd;
    
    if( SDL_OpenAudio( &a, NULL ) < 0 ) 
    {
	blog( "SDL AUDIO ERROR: Couldn't open audio: %s\n", SDL_GetError() );
	return -1;
    }

    //16bit mode supported only:
    g_snd.out_type = sound_buffer_int16;
    g_snd.in_type = sound_buffer_int16;
    
    SDL_PauseAudio( 0 );
    
    return 0;
#else
    return 1;
#endif
}

int device_sound_stream_init( void )
{
    g_sound_driver = 0;
    g_sound_thread_exit_request = 0;

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
    blog( "Audio buffer size: %d frames\n", g_buffer_size );

    bool sdriver_checked[ NUMBER_OF_SDRIVERS ];
    bmem_set( sdriver_checked, sizeof( sdriver_checked ), 0 );
    
    while( 1 )
    {
	int prev_buffer_size = g_buffer_size;
	bool sdriver_ok = 0;
	switch( g_sound_driver )
	{
#ifdef JACK_AUDIO
	    case SDRIVER_JACK:
		if( device_sound_stream_init_jack() == 0 ) sdriver_ok = 1;
		break;
#endif
#ifndef NOALSA
	    case SDRIVER_ALSA:
		if( device_sound_stream_init_alsa( 0 ) == 0 ) sdriver_ok = 1;
		break;
#endif
#ifndef NOOSS
	    case SDRIVER_OSS:
		if( device_sound_stream_init_oss() == 0 ) sdriver_ok = 1;
		break;
#endif
#ifndef NOSDL
	    case SDRIVER_SDL:
		if( device_sound_stream_init_sdl() == 0 ) sdriver_ok = 1;
		break;
#endif
	}
	if( sdriver_ok ) break;
	g_buffer_size = prev_buffer_size;
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
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK ) return;
#endif
    int input_driver = g_sound_driver;
#ifndef NOALSA
#ifndef NOOSS
    if( g_sound_driver == SDRIVER_OSS ) input_driver = SDRIVER_ALSA;
#endif
#endif
#ifndef NOALSA
#ifndef NOSDL
    if( g_sound_driver == SDRIVER_SDL ) input_driver = SDRIVER_ALSA;
#endif
#endif
    if( enable )
    {
#ifndef NOALSA
	set_input_defaults();
	create_input_buffers( g_buffer_size );
	if( input_driver == SDRIVER_ALSA )
	{
	    if( device_sound_stream_init_alsa( 1 ) == 0 )
		g_sound_input_enabled = 1;
	}
#endif
    }
    else
    {
#ifndef NOALSA
	if( input_driver == SDRIVER_ALSA )
	{
	    if( g_sound_input_enabled && g_alsa_capture_handle )
    	    {
    	        g_input_sound_thread_exit_request = 1;
            	while( g_input_sound_thread_exit_request ) { time_sleep( 20 ); }
        	snd_pcm_close( g_alsa_capture_handle );
            	g_alsa_capture_handle = 0;
            	g_sound_input_enabled = 0;
    	    }
	}
#endif
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
	n[ d ] = (utf8_char*)bmem_new( bmem_strlen( g_sdriver_names[ d ] ) + 1 ); 
	n[ d ][ 0 ] = 0;
	bmem_strcat_resize( n[ d ], g_sdriver_names[ d ] );
	i[ d ] = (utf8_char*)bmem_new( bmem_strlen( g_sdriver_infos[ d ] ) + 1 ); 
	i[ d ][ 0 ] = 0;
	bmem_strcat_resize( i[ d ], g_sdriver_infos[ d ] );
    }
    *names = n;
    *infos = i;
    return NUMBER_OF_SDRIVERS;
}

int device_sound_stream_get_devices( const utf8_char* driver, utf8_char*** names, utf8_char*** infos, bool input )
{
#ifdef JACK_AUDIO
    if( g_sound_driver == SDRIVER_JACK ) return 0;
#endif

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
    
    if( input )
    {
#ifndef NOALSA
#ifndef NOOSS
	if( drv_num == SDRIVER_OSS ) drv_num = SDRIVER_ALSA;
#endif
#ifndef NOSDL
	if( drv_num == SDRIVER_SDL ) drv_num = SDRIVER_ALSA;
#endif
#endif
    }

    *names = 0;
    *infos = 0;
    utf8_char* ts = (utf8_char*)bmem_new( 2048 );

    switch( drv_num )
    {
#ifndef NOALSA
	case SDRIVER_ALSA:
	    {
		snd_ctl_card_info_t* info;
                snd_pcm_info_t* pcminfo;
		snd_ctl_card_info_alloca( &info );
		snd_pcm_info_alloca( &pcminfo );
		int card = -1;
		while( 1 )
		{
		    if( snd_card_next( &card ) != 0 )
			break; //Error
		    if( card < 0 )
			break; //No more cards
		    while( 1 )
		    {
			snd_ctl_t* handle;
			int err;
			char name[ 32 ];
            		sprintf( name, "hw:%d", card );
			err = snd_ctl_open( &handle, name, 0 );
			if( err < 0 )
			{
			    blog( "ALSA ERROR: control open (%d): %s\n", card, snd_strerror( err ) );
			    break;
			}
			err = snd_ctl_card_info( handle, info );
			if( err < 0 )
			{
			    blog( "ALSA ERROR: control hardware info (%d): %s\n", card, snd_strerror( err ) );
			    snd_ctl_close( handle );
			    break;
			}
			int dev = -1;
			while( 1 )
			{
			    if( snd_ctl_pcm_next_device( handle, &dev ) < 0 )
			    {
                                blog( "ALSA ERROR: snd_ctl_pcm_next_device\n" );
                                break;
                            }
                            if( dev < 0 ) break;
                            snd_pcm_info_set_device( pcminfo, dev );
                            snd_pcm_info_set_subdevice( pcminfo, 0 );
                            if( input )
                        	snd_pcm_info_set_stream( pcminfo, SND_PCM_STREAM_CAPTURE );
                    	    else
                        	snd_pcm_info_set_stream( pcminfo, SND_PCM_STREAM_PLAYBACK );
                            err = snd_ctl_pcm_info( handle, pcminfo );
                            if( err < 0 )
                            {
                        	if( err != -ENOENT )
                            	    blog( "ALSA ERROR: control digital audio info (%d): %s\n", card, snd_strerror( err ) );
                        	continue;
                            }
                            if( *names == 0 )
                            {
                        	*names = (utf8_char**)bmem_new( 512 * sizeof( void* ) );
                        	*infos = (utf8_char**)bmem_new( 512 * sizeof( void* ) );
                            }
                            //Device name:
                            sprintf( ts, "hw:%d,%d", card, dev );
                            (*names)[ rv ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*names)[ rv ][ 0 ] = 0; bmem_strcat_resize( (*names)[ rv ], ts );
                            //Device info:
                            sprintf( ts, "hw:%d,%d %s, %s", card, dev, snd_ctl_card_info_get_name( info ), snd_pcm_info_get_name( pcminfo ) );
                            (*infos)[ rv ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*infos)[ rv ][ 0 ] = 0; bmem_strcat_resize( (*infos)[ rv ], ts );
                            rv++;
                            //Subdevices:
                            if( 0 )
                            {
                        	uint count = snd_pcm_info_get_subdevices_count( pcminfo );
                        	for( int idx = 0; idx < (int)count; idx++ ) 
                        	{
                        	    snd_pcm_info_set_subdevice( pcminfo, idx );
                        	    err = snd_ctl_pcm_info( handle, pcminfo );
                        	    if( err < 0 )
                        		blog( "ALSA ERROR: control digital audio playback info (%d): %s\n", card, snd_strerror( err ) );
                        	    else
                        	    {
                        		sprintf( ts, "hw:%d,%d,%d", card, dev, idx );
                        		(*names)[ rv ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*names)[ rv ][ 0 ] = 0; bmem_strcat_resize( (*names)[ rv ], ts );
                        		//Device info:
                        		sprintf( ts, "hw:%d,%d,%d %s", card, dev, idx, snd_pcm_info_get_subdevice_name( pcminfo ) );
                        		(*infos)[ rv ] = (utf8_char*)bmem_new( bmem_strlen( ts ) + 1 ); (*infos)[ rv ][ 0 ] = 0; bmem_strcat_resize( (*infos)[ rv ], ts );
                        		rv++;
                        	    }
                        	}
                    	    }
			}
			snd_ctl_close( handle );
			break;
		    }
		}
	    }
	    break;
#endif
#ifndef NOOSS
	case SDRIVER_OSS:
	    break;
#endif
#ifndef NOSDL
	case SDRIVER_SDL:
	    break;
#endif
	default:
	    break;
    }

    bmem_free( ts );
    
    return rv;
}

void device_sound_stream_deinit( void )
{
    device_sound_stream_input( 0 );
    switch( g_sound_driver )
    {
#ifdef JACK_AUDIO
	case SDRIVER_JACK:
	    device_sound_stream_deinit_jack();
	    break;
#endif
#ifndef NOALSA
	case SDRIVER_ALSA:
	    if( g_alsa_playback_handle )
	    {
		g_sound_thread_exit_request = 1;
		while( g_sound_thread_exit_request ) { time_sleep( 20 ); }
		snd_pcm_close( g_alsa_playback_handle );
		g_alsa_playback_handle = 0;
	    }
	    break;
#endif
#ifndef NOOSS
	case SDRIVER_OSS:
	    if( dsp >= 0 )
	    {
		g_sound_thread_exit_request = 1;
		while( g_sound_thread_exit_request ) { time_sleep( 20 ); }
		close( dsp );
		dsp = -1;
	    }
	    break;
#endif
#ifndef NOSDL
	case SDRIVER_SDL:
	    SDL_CloseAudio();
	    break;
#endif
	default:
	    break;
    }
    bmem_free( g_sound_output_buffer );
    g_sound_output_buffer = 0;
    remove_input_buffers();
}
