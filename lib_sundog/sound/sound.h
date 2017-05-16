#pragma once

//Structures and defines:

#define SOUND_SLOTS			    	4

#define SOUND_STREAM_FLAG_USER_CONTROLLED	( 1 << 0 )
#define SOUND_STREAM_FLAG_ONE_THREAD		( 1 << 1 )

#define FLOAT32_TO_INT16( res, val ) \
{ \
    float temp = val; \
    temp *= 32767; \
    if( temp > 32767 ) \
	temp = 32767; \
    if( temp < -32767 ) \
	temp = -32767; \
    res = (signed short)temp; \
}

#define FLOAT32_TO_INT32( res, val ) \
{ \
    float temp = val; \
    temp *= 0x80000000; \
    if( temp > 0x80000000 - 1 ) \
    temp = 0x80000000 - 1; \
    if( temp < -( 0x80000000 - 1 ) ) \
    temp = -( 0x80000000 - 1 ); \
    res = (signed int)temp; \
}

#define INT16_TO_FLOAT32( res, val ) { res = (float)(val) / 32768.0F; }

//MIDI flags:
#define MIDI_PORT_READ ( 1 << 0 )
#define MIDI_PORT_WRITE ( 1 << 1 )
#define MIDI_NO_DEVICE ( 1 << 2 )

enum sound_buffer_type
{
    sound_buffer_default,
    sound_buffer_int16,
    sound_buffer_float32,
    sound_buffer_max
};
extern int g_sample_size[ sound_buffer_max ];

struct sound_slot
{
    void*		callback;
    void*		user_data;

    int			status;

    volatile int	event;
    volatile int	event_answer;

    void*		in_buffer;
    void*		buffer;
    int 		frames;
    ticks_hr_t 		time; //output time
};

struct sound_struct
{
    uint 		flags;
    int			freq;

    sound_slot		slots[ SOUND_SLOTS ];
    void*		slot_buffer;
    int			slot_buffer_size;

    sound_buffer_type	in_type;
    int			in_channels;
    void*		in_buffer;
    int			in_enabled;
    bmutex		in_mutex;
    int                 in_request; // >0 - enable INPUT; ==0 - disable INPUT
    int                 in_request_answer; //Must be equal to input_request
    
    sound_buffer_type	out_type;
    int			out_channels;
    int                 out_latency; //delay between the sound rendering and its output (in frames)
    void*		out_buffer;
    int 		out_frames;
    ticks_hr_t 		out_time; //output time
    
    bfs_file		out_file;
    uint		out_file_size; //File size without WAV headers (in bytes)
    uchar*		out_file_buf;
    volatile size_t	out_file_buf_wp;
    volatile size_t	out_file_buf_rp;
    bthread		out_file_thread;
    volatile int	out_file_exit_request;
    
    bmutex		mutex;
};

struct sundog_midi_event
{
    ticks_hr_t		t;
    size_t		size;
    uchar*		data;
};

struct sundog_midi_port
{
    bool		active;
    uint		flags;
    utf8_char*		port_name;
    utf8_char*		dev_name;
    void*		device_specific;
};

struct sundog_midi_client
{
    sundog_midi_port*	ports;
    utf8_char*		name;
    void*		device_specific;
};

//Variables:

extern sound_struct g_snd;
extern volatile bool g_snd_initialized;

extern volatile int g_snd_play_request; //optional; play request from some audio framework (for example - Audiobus)
extern volatile int g_snd_stop_request; //optional
extern volatile int g_snd_rewind_request; //optional
extern volatile bool g_snd_play_status; //optional

#ifndef NOMIDI
    extern ticks_hr_t g_last_midi_in_activity;
    extern ticks_hr_t g_last_midi_out_activity;
    extern bmutex g_sundog_midi_mutex;
#endif

#ifdef IPHONE
    extern int g_sound_idle_seconds;
#endif

//Functions (Audio):

void user_controlled_audio_callback( void* buffer, int frames, int latency, ticks_hr_t output_time );
int main_sound_output_callback( sound_struct* ss, uint flags );

int sound_stream_init( sound_buffer_type type, int freq, int channels, uint flags );
void sound_stream_set_slot_callback( int slot, void* callback, void* user_data );
void sound_stream_remove_slot_callback( int slot );
void sound_stream_lock( void );
void sound_stream_unlock( void );
void sound_stream_play( int slot );
void sound_stream_stop( int slot );
int sound_stream_device_play( void );
void sound_stream_device_stop( void );
void sound_stream_input( bool enable ); //Call this function in the main app thread, where the sound stream is not locked. Or...
void sound_stream_input_request( bool enable ); //... or use this function + sound_stream_handle_input_requests() in the main thread
void sound_stream_handle_input_requests( void );
const utf8_char* sound_stream_get_driver_name( void );
const utf8_char* sound_stream_get_driver_info( void );
int sound_stream_get_drivers( utf8_char*** names, utf8_char*** infos );
int sound_stream_get_devices( const utf8_char* driver, utf8_char*** names, utf8_char*** infos, bool input );
int sound_stream_capture_start( const utf8_char* filename ); //capture sound to the WAV file
void sound_stream_capture_stop( void );
void sound_stream_deinit( void );

int device_sound_stream_init( int mode, int freq, int channels );
void device_sound_stream_input( bool enable );
const utf8_char* device_sound_stream_get_driver_name( void );
void device_sound_stream_deinit( void );

#ifdef IPHONE
    int audio_session_init( bool record, int preferred_sample_rate, int preferred_buffer_size );
    int audio_session_deinit( void );
    int get_audio_session_freq();
    #ifdef AUDIOBUS
	int audiobus_init( void ); //Called from sundog_bridge.m (main thread)
        void audiobus_deinit( void ); //Called from sundog_bridge.m (main thread)
        void audiobus_enable_ports( bool enable );
	bool audiobus_connected( void );
        void audiobus_triggers_update( void );
    #endif
#endif

//Functions (MIDI):

int sundog_midi_init( void );
int sundog_midi_deinit( void );
int sundog_midi_client_open( sundog_midi_client* c, const utf8_char* name );
int sundog_midi_client_close( sundog_midi_client* c );
int sundog_midi_client_get_devices( sundog_midi_client* c, utf8_char*** devices, uint flags );
int sundog_midi_client_open_port( sundog_midi_client* c, const utf8_char* port_name, const utf8_char* dev_name, uint flags );
int sundog_midi_client_reopen_port( sundog_midi_client* c, int pnum );
int sundog_midi_client_close_port( sundog_midi_client* c, int pnum );
sundog_midi_event* sundog_midi_client_get_event( sundog_midi_client* c, int pnum );
int sundog_midi_client_next_event( sundog_midi_client* c, int pnum );
int sundog_midi_client_send_event( sundog_midi_client* c, int pnum, sundog_midi_event* evt );

int device_midi_client_open( sundog_midi_client* c, const utf8_char* name );
int device_midi_client_close( sundog_midi_client* c );
int device_midi_client_get_devices( sundog_midi_client* c, utf8_char*** devices, uint flags );
int device_midi_client_open_port( sundog_midi_client* c, int pnum, const utf8_char* port_name, const utf8_char* dev_name, uint flags );
int device_midi_client_close_port( sundog_midi_client* c, int pnum );
sundog_midi_event* device_midi_client_get_event( sundog_midi_client* c, int pnum );
int device_midi_client_next_event( sundog_midi_client* c, int pnum );
int device_midi_client_send_event( sundog_midi_client* c, int pnum, sundog_midi_event* evt );
