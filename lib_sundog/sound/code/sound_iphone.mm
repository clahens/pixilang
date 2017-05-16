#import <UIKit/UIKit.h>

#include "core/core.h"
#include "../sound.h"

//
// Possible options (defines): 
//   JACK_AUDIO;
//   AUDIOBUS;
//   AUDIOBUS_API_KEY;
//   AUDIOBUS_SENDER;
//   AUDIOBUS_FILTER;
//   AUDIOBUS_TRIGGER_PLAY;
//   AUDIOBUS_TRIGGER_REWIND;
//

//
// AVAudioSession
//

#import <AVFoundation/AVAudioSession.h>

int audio_session_init( bool record, int preferred_sample_rate, int preferred_buffer_size )
{
    int rv = -1;

    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];

    while( 1 )
    {
	NSError* error = nil;
	NSString* category;
	if( record )
	{
	    category = AVAudioSessionCategoryPlayAndRecord;
	}
	else
	{
	    category = AVAudioSessionCategoryPlayback;
	}
	AVAudioSessionCategoryOptions options = AVAudioSessionCategoryOptionMixWithOthers;
	if( ![ [ AVAudioSession sharedInstance ] setCategory:category withOptions:options error:&error ] ) 
	{
	    blog( "Can't set audio session category: %s\n", [ [ error localizedDescription ] UTF8String ] );
	    if( record )
	    {
		error = nil;
		category = AVAudioSessionCategoryPlayback;
		if( ![ [ AVAudioSession sharedInstance ] setCategory:category withOptions:options error:&error ] ) 
		{
		    blog( "Can't set audio session category: %s\n", [ [ error localizedDescription ] UTF8String ] );
		    break;
		}
	    }
	    else
	    {
		break;
	    }
	}

	if( preferred_sample_rate > 0 )
	{
	    error = nil;
	    if( ![ [ AVAudioSession sharedInstance ] setPreferredSampleRate:preferred_sample_rate error:&error ] )
	    {
		blog( "Can't set audio session sample rate: %s\n", [ [ error localizedDescription ] UTF8String ] );
	    }
	}
	
	if( preferred_buffer_size > 0 )
	{
	    error = nil;
	    if( ![ [ AVAudioSession sharedInstance ] setPreferredIOBufferDuration: ( (double)preferred_buffer_size / [ AVAudioSession sharedInstance ].preferredSampleRate ) error:&error ] )
	    {
		blog( "Can't set audio session buffer size: %s\n", [ [ error localizedDescription ] UTF8String ] );
	    }
	}

	error = nil;
	if( ![ [ AVAudioSession sharedInstance ] setActive:YES error:&error ] ) 
	{
	    blog( "Can't activate audio session: %s\n", [ [ error localizedDescription ] UTF8String ] );
	    break;
	}

	rv = 0;
	break;
    }

    [ pool release ];
    
    HANDLE_THREAD_EVENTS;
    
    return rv;
}

int audio_session_deinit( void )
{
    int rv = -1;

    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];

    while( 1 )
    {
	NSError* error = nil;
	if( ![ [ AVAudioSession sharedInstance ] setActive:NO error:&error ] ) 
	{
	    blog( "Can't deactivate audio session: %s\n", [ [ error localizedDescription ] UTF8String ] );
	    break;
	}

	rv = 0;
	break;
    }

    [ pool release ];
    
    HANDLE_THREAD_EVENTS;
    
    return rv;
}

int get_audio_session_freq()
{
    int rv = -1;
    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];
    rv = [ AVAudioSession sharedInstance ].sampleRate;
    [ pool release ];
    return rv;
}

//
// JACK
//

#ifdef JACK_AUDIO

#import <jack/custom.h>

void jack_publish_client_icon( jack_client_t* client )
{
    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];
    NSString* iconFile = [ [ NSBundle mainBundle ] pathForResource:@"icon@2x" ofType:@"png" ];
    NSFileHandle* fileHandle = [ NSFileHandle fileHandleForReadingAtPath:iconFile ];
    NSData* data = [ fileHandle readDataToEndOfFile ];
    const void* rawData = [ data bytes ];
    const size_t size   = [ data length ];
    jack_custom_publish_data( client, "icon.png", rawData, size );
    [ fileHandle closeFile ];
    [ pool release ];
}

#endif

//
// Audiobus
//

#ifdef AUDIOBUS

#import "Audiobus.h"
#import <AudioUnit/AudioUnit.h>

extern AudioComponentInstance g_aunit; //Defined in sound_iphone.h

ABAudiobusController* g_ab_controller = 0;
ABSenderPort* g_ab_sender = 0;
ABFilterPort* g_ab_filter = 0;
ABTrigger* g_ab_trigger_play = 0;
ABTrigger* g_ab_trigger_rewind = 0;

int audiobus_init( void ) //Called from sundog_bridge.m (main thread)
{
    blog( "audiobus_init(): session init...\n" );
    audio_session_init( 0, 0, 0 );

    //Create new RemoteIO Audio Unit:
    blog( "audiobus_init(): Audio Unit creation...\n" );
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    AudioComponent comp = AudioComponentFindNext( NULL, &desc );
    if( comp )
	AudioComponentInstanceNew( comp, &g_aunit );
    else
    {
	blog( "audiobus_init(): can't create the new Audio Unit\n" );
	return -1;
    }
    //Audio Unit will be initialized in the SunDog thread
    
    blog( "audiobus_init(): AB controller init...\n" );
    g_ab_controller = [ [ ABAudiobusController alloc ] initWithApiKey:@AUDIOBUS_API_KEY ];
    if( g_ab_controller == 0 )
    {
	blog( "Can't init ABAudiobusController\n" );
	return -2;
    }

    blog( "audiobus_init(): AB ports init...\n" );
#ifdef AUDIOBUS_SENDER
    g_ab_sender = [ [ ABSenderPort alloc ] initWithName:@"Output"
                                                  title:@"Output"
                              audioComponentDescription:( AudioComponentDescription ) {
                                .componentType = kAudioUnitType_RemoteGenerator,
                                .componentSubType = AUDIOBUS_SENDER_SUBTYPE, // Note single quotes
                                .componentManufacturer = 'nira' }
                                              audioUnit:g_aunit ];
    if( g_ab_sender == 0 )
        blog( "Can't create AB sender port\n" );
    else
        [ g_ab_controller addSenderPort:g_ab_sender ];
#endif
#ifdef AUDIOBUS_FILTER
    g_ab_filter = [ [ ABFilterPort alloc ] initWithName:@"Filter"
                                                  title:@"Filter"
                              audioComponentDescription:(AudioComponentDescription) {
    	            		.componentType = kAudioUnitType_RemoteEffect,
	                        .componentSubType = AUDIOBUS_FILTER_SUBTYPE,
	                        .componentManufacturer = 'nira' }
                                              audioUnit:g_aunit ];
    if( g_ab_filter == 0 )
        blog( "Can't create AB filter port\n" );
    else
        [ g_ab_controller addFilterPort:g_ab_filter ];
#endif
    
    blog( "audiobus_init(): AB triggers init...\n" );
#ifdef AUDIOBUS_TRIGGER_PLAY
    g_ab_trigger_play = [ ABTrigger triggerWithSystemType:ABTriggerTypePlayToggle block:^( ABTrigger* trigger, NSSet* ports )
    {
        if( g_snd_play_status )
            g_snd_stop_request++;
        else
            g_snd_play_request++;
    } ];
    [ g_ab_controller addTrigger:g_ab_trigger_play ];
#endif
#ifdef AUDIOBUS_TRIGGER_REWIND
    g_ab_trigger_rewind = [ ABTrigger triggerWithSystemType:ABTriggerTypeRewind block:^( ABTrigger* trigger, NSSet* ports )
    {
        g_snd_rewind_request++;
    } ];
    [ g_ab_controller addTrigger:g_ab_trigger_rewind ];
#endif

    blog( "audiobus_init(): ok.\n" );
    return 0;
}

void audiobus_deinit( void ) //Called from sundog_bridge.m (main thread)
{
    blog( "audiobus_deinit(): AB controller deinit...\n" );
    //[ g_ab_controller release ];
    g_ab_controller = 0;
    g_ab_sender = 0;
    g_ab_filter = 0;
    
    blog( "audiobus_deinit(): Audio Unit deinit...\n" );
    AudioComponentInstanceDispose( g_aunit );
    g_aunit = 0;

    blog( "audiobus_deinit(): audio session deinit...\n" );
    audio_session_deinit();

    blog( "audiobus_deinit(): ok.\n" );
}

void audiobus_enable_ports( bool enable )
{
    if( g_ab_controller == 0 ) return;
    
    NSAutoreleasePool* pool = [ [ NSAutoreleasePool alloc ] init ];

#ifdef AUDIOBUS_SENDER
    if( g_ab_sender )
    {
	if( enable )
	    g_ab_sender.audioUnit = g_aunit;
	else
	    g_ab_sender.audioUnit = 0;
    }
#endif

#ifdef AUDIOBUS_FILTER
    if( g_ab_filter )
    {
	if( enable )
	    g_ab_filter.audioUnit = g_aunit;
	else
	    g_ab_filter.audioUnit = 0;
    }
#endif

    [ pool release ];
}

bool audiobus_connected( void )
{
    if( g_ab_controller )
    {
	return g_ab_controller.connected || g_ab_controller.memberOfActiveAudiobusSession;
    }
    return 0;
}

void audiobus_triggers_update( void )
{
#ifdef AUDIOBUS_TRIGGER_PLAY
    if( g_ab_controller )
    {
        NSAutoreleasePool* pool = 0;
        if( g_snd_play_status )
        {
            if( g_ab_trigger_play.state != ABTriggerStateSelected )
            {
                pool = [ [ NSAutoreleasePool alloc ] init ];
                g_ab_trigger_play.state = ABTriggerStateSelected;
            }
        }
        else
        {
            if( g_ab_trigger_play.state != ABTriggerStateNormal )
            {
                pool = [ [ NSAutoreleasePool alloc ] init ];
                g_ab_trigger_play.state = ABTriggerStateNormal;
            }
        }
        [ pool release ];
    }
#endif
}

#endif
