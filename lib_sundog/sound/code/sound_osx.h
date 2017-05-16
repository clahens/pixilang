//################################
//## OSX                        ##
//################################

//MIDI:
#define MIDI_EVENTS ( 2048 )
#define MIDI_BYTES ( MIDI_EVENTS * 16 )
#ifdef JACK_AUDIO
    #include "sound_jack.h"
#endif
#include "sound_coremidi.h"

#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AudioComponent.h>

int g_default_buffer_size = 4096;
int g_buffer_size = 0;

#define kOutputBus 0
#define kInputBus 1

AudioUnit g_unit = 0;
AudioUnit g_unit_in = 0;
AudioBufferList* g_input_buffer_list;

static OSStatus output_callback(
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
	g_snd.out_frames = buf->mDataByteSize / ( 4 * g_snd.out_channels );
	g_snd.out_time = (ticks_hr_t)output_time;
	g_snd.out_latency = g_snd.out_frames;
        get_input_data( g_snd.out_frames );
	if( main_sound_output_callback( &g_snd, 0 ) )
	{
	    *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
	}
	else
	{
	    if( g_snd.out_type == sound_buffer_int16 )
	    {
		float* fb = (float*)g_snd.out_buffer;
		signed short* sb = (signed short*)g_snd.out_buffer;
		for( int i = ( g_snd.out_frames - 1 ) * g_snd.out_channels; i >= 0; i-- )
		    INT16_TO_FLOAT32( fb[ i ], sb[ i ] );
	    }
	}
    }
    return noErr;
}

static OSStatus input_callback(
    void* inRefCon,
    AudioUnitRenderActionFlags* ioActionFlags,
    const AudioTimeStamp* inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList* ioData )
{
    if( inNumberFrames > 8192 ) return noErr;
    g_input_buffer_list->mNumberBuffers = 1;
    g_input_buffer_list->mBuffers[ 0 ].mNumberChannels = g_snd.in_channels;
    g_input_buffer_list->mBuffers[ 0 ].mDataByteSize = inNumberFrames * 4 * g_snd.in_channels;
    OSStatus err = AudioUnitRender(
        g_unit_in,
        ioActionFlags,
        inTimeStamp,
        inBusNumber,
        inNumberFrames,
        g_input_buffer_list );
    if( err == noErr )
    {
        int size = inNumberFrames;
        char* ptr = (char*)g_input_buffer_list->mBuffers[ 0 ].mData;
        if( size == 0 ) return noErr;
        int au_frame_size = 4 * g_snd.in_channels;
        int frame_size = 0;
        if( g_snd.in_type == sound_buffer_int16 )
            frame_size = 2 * g_snd.in_channels;
        if( g_snd.in_type == sound_buffer_float32 )
            frame_size = 4 * g_snd.in_channels;
        while( size > 0 )
        {
            int size2 = size;
            if( g_sound_input_buffer_wp + size2 > g_sound_input_buffer_size )
                size2 = g_sound_input_buffer_size - g_sound_input_buffer_wp;
            char* buf_ptr = g_sound_input_buffer + g_sound_input_buffer_wp * frame_size;
            if( g_snd.in_type == sound_buffer_int16 )
            {
                signed short* sb = (signed short*)buf_ptr;
                float* fb = (float*)ptr;
                for( int i = 0; i < size2 * g_snd.in_channels; i++ )
                    FLOAT32_TO_INT16( sb[ i ], fb[ i ] );
            }
            if( g_snd.in_type == sound_buffer_float32 )
            {
                bmem_copy( buf_ptr, ptr, size2 * frame_size );
            }
            volatile uint new_wp = ( g_sound_input_buffer_wp + size2 ) & ( g_sound_input_buffer_size - 1 );
            g_sound_input_buffer_wp = new_wp;
            size -= size2;
            ptr += size2 * au_frame_size;
        }
    }
    return noErr;
}

int device_sound_stream_init( void )
{
    OSStatus status;
    UInt32 enableIO;
    bool err = 0;
    
    if( g_unit ) return -1; //Already open
    
    g_buffer_size = profile_get_int_value( KEY_SOUNDBUFFER, g_default_buffer_size, 0 );
    g_snd.out_latency = g_buffer_size;
    
    while( 1 )
    {
        //Describe audio component:
        AudioComponentDescription desc;
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_HALOutput;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;

        //Get component:
        AudioComponent comp = AudioComponentFindNext( NULL, &desc );
        if( comp == 0 )
        {
            blog( "AUDIO UNIT ERROR: AudioComponent not found\n" );
            err = 1;
            break;
        }
    
        //Get audio unit:
        status = AudioComponentInstanceNew( comp, &g_unit );
        if( status != noErr )
        {
            blog( "AUDIO UNIT ERROR: OpenAComponent failed with code %d\n", (int)status );
            err = 1;
            break;
        }
    
        //Disable input:
        enableIO = 0;
        status = AudioUnitSetProperty(
            g_unit,
            kAudioOutputUnitProperty_EnableIO,
            kAudioUnitScope_Input,
            kInputBus,
            &enableIO,
            sizeof( enableIO ) );
        if( status != noErr )
        {
            blog( "AUDIO UNIT ERROR: AudioUnitSetProperty (disable input) failed with code %d\n", (int)status );
        }
    
        //Set output device:
        AudioDeviceID outputDevice = 0;
        UInt32 size = sizeof( AudioDeviceID );
        AudioObjectPropertyAddress property_address =
        {
            kAudioHardwarePropertyDefaultOutputDevice,  // mSelector
            kAudioObjectPropertyScopeGlobal,           // mScope
            kAudioObjectPropertyElementMaster          // mElement
        };
        status = AudioObjectGetPropertyData( kAudioObjectSystemObject, &property_address, 0, 0, &size, &outputDevice );
        if( status != noErr )
        {
            blog( "AUDIO UNIT ERROR: AudioHardwareGetProperty failed with code %d\n", (int)status );
            err = 1;
            break;
        }
        status = AudioUnitSetProperty(
            g_unit,
            kAudioOutputUnitProperty_CurrentDevice,
            kAudioUnitScope_Global,
            kOutputBus,
            &outputDevice,
            sizeof( outputDevice ) );
        if( status != noErr )
        {
            blog( "AUDIO UNIT ERROR: AudioUnitSetProperty (set output device) failed with code %d\n", (int)status );
            err = 1;
            break;
        }

        //Describe output format:
        AudioStreamBasicDescription af;
        af.mSampleRate = g_snd.freq;
        af.mFormatID = kAudioFormatLinearPCM;
        af.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        af.mFramesPerPacket = 1;
        af.mChannelsPerFrame = g_snd.out_channels;
        af.mBitsPerChannel = 32;
        af.mBytesPerPacket = 4 * g_snd.out_channels;
        af.mBytesPerFrame = 4 * g_snd.out_channels;

        //Apply format:
        status = AudioUnitSetProperty(
            g_unit,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Input,
            kOutputBus,
            &af,
            sizeof( af ) );
        if( status != noErr )
        {
            blog( "AUDIO UNIT ERROR: AudioUnitSetProperty (StreamFormat for output) failed with code %d\n", (int)status );
            err = 1;
            break;
        }
    
        //Set output callback:
        AURenderCallbackStruct cs;
        cs.inputProc = output_callback;
        cs.inputProcRefCon = g_unit;
        status = AudioUnitSetProperty(
            g_unit,
            kAudioUnitProperty_SetRenderCallback,
            kAudioUnitScope_Global,
            kOutputBus,
            &cs,
            sizeof( cs ) );
        if( status != noErr )
        {
            blog( "AUDIO UNIT ERROR: AudioUnitSetProperty (SetRenderCallback,Global) failed with code %d\n", (int)status );
            err = 1;
            break;
        }
    
        //Initialize:
        status = AudioUnitInitialize( g_unit );
        if( status != noErr )
        {
            blog( "AUDIO UNIT ERROR: AudioUnitInitialize failed with code %d\n", (int)status );
            err = 1;
            break;
        }
    
        //Start:
        status = AudioOutputUnitStart( g_unit );
        if( status != noErr )
        {
            blog( "AUDIO UNIT ERROR: AudioOutputUnitStart failed with code %d\n", (int)status );
            err = 1;
            break;
        }
        
        break;
    }
    
    if( err )
    {
        if( g_unit )
        {
            AudioUnitUninitialize( g_unit );
            AudioComponentInstanceDispose( g_unit );
            g_unit = 0;
        }
        return -1;
    }
    
    return 0;
}

void device_sound_stream_input( bool enable )
{
    OSStatus status;
    UInt32 enableIO;
    bool err = 0;
    
    if( enable )
    {
        while( 1 )
        {
            if( g_sound_input_enabled ) break;
            if( g_unit_in ) break; //Already open
         
            set_input_defaults();
            create_input_buffers( 4096 );
            if( g_input_buffer_list == 0 )
            {
                g_input_buffer_list = (AudioBufferList*)bmem_new( sizeof( AudioBufferList ) );
                bmem_zero( g_input_buffer_list );
                g_input_buffer_list->mNumberBuffers = 1;
                g_input_buffer_list->mBuffers[ 0 ].mNumberChannels = 2;
                g_input_buffer_list->mBuffers[ 0 ].mDataByteSize = 8192 * 4 * 2;
                g_input_buffer_list->mBuffers[ 0 ].mData = bmem_new( 8192 * 4 * 2 );
                bmem_zero( g_input_buffer_list->mBuffers[ 0 ].mData );
            }
            
            //Describe audio component:
            AudioComponentDescription desc;
            desc.componentType = kAudioUnitType_Output;
            desc.componentSubType = kAudioUnitSubType_HALOutput;
            desc.componentFlags = 0;
            desc.componentFlagsMask = 0;
            desc.componentManufacturer = kAudioUnitManufacturer_Apple;
            
            //Get component:
            AudioComponent comp = AudioComponentFindNext( NULL, &desc );
            if( comp == 0 )
            {
                blog( "AUDIO UNIT INPUT ERROR: AudioComponent not found\n" );
                err = 1;
                break;
            }
            
            //Get audio unit:
            status = AudioComponentInstanceNew( comp, &g_unit_in );
            if( status != noErr )
            {
                blog( "AUDIO UNIT INPUT ERROR: OpenAComponent failed with code %d\n", (int)status );
                err = 1;
                break;
            }
            
            //Enable input:
            enableIO = 1;
            status = AudioUnitSetProperty(
                g_unit_in,
                kAudioOutputUnitProperty_EnableIO,
                kAudioUnitScope_Input,
                kInputBus,
                &enableIO,
                sizeof( enableIO ) );
            if( status != noErr )
            {
                blog( "AUDIO UNIT INPUT ERROR: AudioUnitSetProperty (enable input) failed with code %d\n", (int)status );
                err = 1;
                break;
            }
            
            //Disable output:
            enableIO = 0;
            status = AudioUnitSetProperty(
                g_unit_in,
                kAudioOutputUnitProperty_EnableIO,
                kAudioUnitScope_Output,
                kOutputBus,
                &enableIO,
                sizeof( enableIO ) );
            if( status != noErr )
            {
                blog( "AUDIO UNIT INPUT ERROR: AudioUnitSetProperty (disable output) failed with code %d\n", (int)status );
                err = 1;
                break;
            }
            
            //Get input device:
            AudioDeviceID inputDevice = 0;
            UInt32 size = sizeof( AudioDeviceID );
            AudioObjectPropertyAddress property_address =
            {
                kAudioHardwarePropertyDefaultInputDevice,  // mSelector
                kAudioObjectPropertyScopeGlobal,           // mScope
                kAudioObjectPropertyElementMaster          // mElement
            };
            status = AudioObjectGetPropertyData( kAudioObjectSystemObject, &property_address, 0, 0, &size, &inputDevice );
            if( status != noErr )
            {
                blog( "AUDIO UNIT INPUT ERROR: AudioObjectGetPropertyData failed with code %d\n", (int)status );
                err = 1;
                break;
            }
	    //Get available freqs:
	    property_address.mSelector = kAudioDevicePropertyAvailableNominalSampleRates;
	    status = AudioObjectGetPropertyDataSize( inputDevice, &property_address, 0, 0, &size );
	    if( status != noErr )
	    {
		blog( "AUDIO UNIT INPUT ERROR: AudioObjectGetPropertyDataSize failed with code %d\n", (int)status );
		err = 1;
		break;
	    }
	    int val_count = size / (int)sizeof( AudioValueRange );
	    blog( "AUDIO UNIT INPUT: Available %d Sample Rates\n", val_count );
	    AudioValueRange* freqs = (AudioValueRange*)bmem_new( size );
	    status = AudioObjectGetPropertyData( inputDevice, &property_address, 0, 0, &size, freqs );
	    if( status != noErr )
	    {
		blog( "AUDIO UNIT INPUT ERROR: AudioObjectGetPropertyData (2) failed with code %d\n", (int)status );
		err = 1;
		break;
	    }
	    for( int i = 0; i < val_count; i++ )
	    {
		blog( "%d\n", (int)freqs[ i ].mMinimum );
	    }
	    bmem_free( freqs );
	    //Set device sample rate:
	    AudioValueRange srate;
	    srate.mMinimum = g_snd.freq;
	    srate.mMaximum = g_snd.freq;
	    property_address.mSelector = kAudioDevicePropertyNominalSampleRate;
	    AudioObjectSetPropertyData( inputDevice, &property_address, 0, 0, sizeof( srate ), &srate );
	    property_address.mSelector = kAudioDevicePropertyLatency;
	    //Set input device:
            status = AudioUnitSetProperty(
                g_unit_in,
                kAudioOutputUnitProperty_CurrentDevice,
                kAudioUnitScope_Global,
                kInputBus,
                &inputDevice,
                sizeof( inputDevice ) );
            if( status != noErr )
            {
                blog( "AUDIO UNIT INPUT ERROR: AudioUnitSetProperty (set input device) failed with code %d\n", (int)status );
                err = 1;
                break;
            }
            
            //Describe input format:
            AudioStreamBasicDescription af;
            af.mSampleRate = g_snd.freq;
            af.mFormatID = kAudioFormatLinearPCM;
            af.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
            af.mFramesPerPacket = 1;
            af.mChannelsPerFrame = g_snd.in_channels;
            af.mBitsPerChannel = 32;
            af.mBytesPerPacket = 4 * g_snd.in_channels;
            af.mBytesPerFrame = 4 * g_snd.in_channels;
            
            //Apply input format:
            status = AudioUnitSetProperty(
                g_unit_in,
                kAudioUnitProperty_StreamFormat,
                kAudioUnitScope_Output,
                kInputBus,
                &af,
                sizeof( af ) );
            if( status != noErr )
            {
                blog( "AUDIO UNIT INPUT ERROR: AudioUnitSetProperty (StreamFormat) failed with code %d\n", (int)status );
                err = 1;
                break;
            }
            
            //Set input callback:
            AURenderCallbackStruct cs;
            cs.inputProc = input_callback;
            cs.inputProcRefCon = g_unit;
            status = AudioUnitSetProperty(
                g_unit_in,
                kAudioOutputUnitProperty_SetInputCallback,
                kAudioUnitScope_Global,
                kInputBus,
                &cs,
                sizeof( cs ) );
            if( status != noErr )
            {
                blog( "AUDIO UNIT INPUT ERROR: AudioUnitSetProperty (SetInputCallback) failed with code %d\n", (int)status );
                err = 1;
                break;
            }
            
            //Initialize:
            status = AudioUnitInitialize( g_unit_in );
            if( status != noErr )
            {
                blog( "AUDIO UNIT INPUT ERROR: AudioUnitInitialize failed with code %d\n", (int)status );
                err = 1;
                break;
            }
            
            //Start:
            status = AudioOutputUnitStart( g_unit_in );
            if( status != noErr )
            {
                blog( "AUDIO UNIT INPUT ERROR: AudioOutputUnitStart failed with code %d\n", (int)status );
                err = 1;
                break;
            }
            
            g_sound_input_enabled = 1;
            
            break;
        }
        if( err )
        {
            if( g_unit_in )
            {
                AudioUnitUninitialize( g_unit_in );
                AudioComponentInstanceDispose( g_unit_in );
                g_unit_in = 0;
            }
        }
    }
    else
    {
        if( g_sound_input_enabled == 0 ) return;
        if( g_unit_in == 0 ) return;
        AudioUnitUninitialize( g_unit_in );
        AudioComponentInstanceDispose( g_unit_in );
        g_unit_in = 0;
        if( g_input_buffer_list )
        {
            bmem_free( g_input_buffer_list->mBuffers[ 0 ].mData );
            bmem_free( g_input_buffer_list );
            g_input_buffer_list = 0;
        }
        g_sound_input_enabled = 0;
    }
}

const utf8_char* device_sound_stream_get_driver_info( void )
{
    return "Audio Unit";
}

const utf8_char* device_sound_stream_get_driver_name( void )
{
    return "audiounit";
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
    device_sound_stream_input( 0 );
    AudioUnitUninitialize( g_unit );
    AudioComponentInstanceDispose( g_unit );
    g_unit = 0;
    remove_input_buffers();
}
