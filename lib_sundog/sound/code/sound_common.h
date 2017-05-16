#if ( defined(WIN) || defined(WINCE) || defined(LINUX) || defined(OSX) ) && !defined(ANDROID)

int g_sound_input_buffers_count = 8;
int g_sound_input_buffer_size = 0; //in frames
volatile uint g_sound_input_buffer_wp = 0;
volatile uint g_sound_input_buffer_rp = 0;
char* g_sound_input_buffer = 0;
char* g_sound_input_buffer2 = 0;
bool g_sound_input_buffer2_is_empty = 1;
bool g_sound_input_enabled = 0;

void get_input_data( int frames )
{
    if( g_sound_input_enabled )
    {
        g_sound_input_buffer2_is_empty = 0;
        g_snd.in_buffer = g_sound_input_buffer2;
        if( ( ( g_sound_input_buffer_wp - g_sound_input_buffer_rp ) & ( g_sound_input_buffer_size - 1 ) ) >= frames )
        {
            int src_ptr = g_sound_input_buffer_rp * g_snd.in_channels;
            int src_mask = ( g_sound_input_buffer_size * g_snd.in_channels ) - 1;
            if( g_snd.in_type == sound_buffer_float32 )
            {
                float* src = (float*)g_sound_input_buffer;
                float* dest = (float*)g_sound_input_buffer2;
                for( int i = 0; i < frames * g_snd.in_channels; i++, src_ptr++ )
                {
                    dest[ i ] = src[ src_ptr & src_mask ];
                }
            }
            if( g_snd.in_type == sound_buffer_int16 )
            {
                int16* src = (int16*)g_sound_input_buffer;
                int16* dest = (int16*)g_sound_input_buffer2;
                for( int i = 0; i < frames * g_snd.in_channels; i++, src_ptr++ )
                {
                    dest[ i ] = src[ src_ptr & src_mask ];
                }
            }
            g_sound_input_buffer_rp += frames;
            g_sound_input_buffer_rp &= ( g_sound_input_buffer_size - 1 );
        }
    }   
    else
    {
	if( g_sound_input_buffer2 )
	{
    	    if( g_sound_input_buffer2_is_empty == 0 )
    	    {
        	bmem_zero( g_sound_input_buffer2 );
        	g_sound_input_buffer2_is_empty = 1;
    	    }
    	}
        g_snd.in_buffer = g_sound_input_buffer2;
    }
}

void set_input_defaults( void )
{
    g_snd.in_channels = g_snd.out_channels;
    if( g_snd.in_channels > 2 ) g_snd.in_channels = 2;
    g_snd.in_type = g_snd.out_type;
    g_sound_input_buffer_wp = 0;
    g_sound_input_buffer_rp = 0;
    g_sound_input_enabled = 0;
}

void create_input_buffers( int frames )
{
    if( g_sound_input_buffer == 0 )
    {
        int frame_size = 0;
        if( g_snd.in_type == sound_buffer_int16 )
            frame_size = 2 * g_snd.in_channels;
        if( g_snd.in_type == sound_buffer_float32 )
            frame_size = 4 * g_snd.in_channels;
        g_sound_input_buffer_size = round_to_power_of_two( frames * g_sound_input_buffers_count );
        g_sound_input_buffer = (char*)bmem_new( g_sound_input_buffer_size * frame_size );
        bmem_zero( g_sound_input_buffer );
        g_sound_input_buffer2 = (char*)bmem_new( frames * frame_size );
        bmem_zero( g_sound_input_buffer2 );
        g_sound_input_buffer2_is_empty = 1;
    }
}

void remove_input_buffers( void )
{
    bmem_free( g_sound_input_buffer );
    bmem_free( g_sound_input_buffer2 );
    g_sound_input_buffer = 0;
    g_sound_input_buffer2 = 0;
}

#endif
