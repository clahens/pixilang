/*
    keymap.cpp. Key redefinition
    This file is part of the SunDog engine.
    Copyright (C) 2014 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

#include "core/core.h"

inline void keymap_make_codename( utf8_char* name, int key, uint flags, uint pars1, uint pars2 )
{
    if( pars1 || pars2 )
    {
	sprintf( name, "%x %x %x %x", key, flags, pars1, pars2 );
    }
    else
    {
	name[ 0 ] = int_to_hchar( flags >> 4 );
	name[ 1 ] = int_to_hchar( flags & 15 );
	hex_int_to_string( key, &name[ 2 ] );
    }
}

sundog_keymap* keymap_new( window_manager* wm )
{	
    sundog_keymap* km = (sundog_keymap*)bmem_new( sizeof( sundog_keymap ) );
    bmem_zero( km );
    return km;
}

void keymap_remove( sundog_keymap* km, window_manager* wm )
{
    if( km == 0 ) 
    {
	if( wm ) km = wm->km;
	if( km == 0 ) return;
    }
    if( km->sections )
    {
	for( size_t i = 0; i < bmem_get_size( km->sections ) / sizeof( sundog_keymap_section ); i++ )
	{
	    sundog_keymap_section* section = &km->sections[ i ];
	    for( size_t i2 = 0; i2 < bmem_get_size( section->actions ) / sizeof( sundog_keymap_action ); i2++ )
	    {
		sundog_keymap_action* action = &section->actions[ i2 ];
	    }
	    bmem_free( section->actions );
	    bsymtab_remove( section->bindings );
	}
	bmem_free( km->sections );
    }
    bmem_free( km );
}

void keymap_silent( sundog_keymap* km, bool silent, window_manager* wm )
{
    if( km == 0 ) 
    {
	if( wm ) km = wm->km;
	if( km == 0 ) return;
    }
    km->silent = silent;
}

int keymap_add_section( sundog_keymap* km, const utf8_char* section_name, const utf8_char* section_id, int section_num, window_manager* wm )
{
    if( km == 0 ) 
    {
	if( wm ) km = wm->km;
	if( km == 0 ) return -1;
    }
    int rv = -1;
    while( 1 )
    {	
	if( km->sections == 0 )
	{
	    km->sections = (sundog_keymap_section*)bmem_new( sizeof( sundog_keymap_section ) * ( section_num + 1 ) );
	    if( km->sections )
	    {
		bmem_zero( km->sections );
		rv = 0;
	    }
	}
	else
	{
	    size_t count = bmem_get_size( km->sections ) / sizeof( sundog_keymap_section );
	    if( section_num >= count )
	    {
		km->sections = (sundog_keymap_section*)bmem_resize( km->sections, ( section_num + 1 ) * sizeof( sundog_keymap_section ) );
    		bmem_set( &km->sections[ count ], sizeof( sundog_keymap_section ) * ( section_num - count ), 0 );
		rv = 0;
	    }
	}
	if( rv != -1 )
	{
	    sundog_keymap_section* section = &km->sections[ section_num ];
	    section->name = section_name;
	    section->id = section_id;
	    section->bindings = bsymtab_new( 1543 );
	}
	break;
    }
    return rv;
}

int keymap_add_action( sundog_keymap* km, int section_num, const utf8_char* action_name, const utf8_char* action_id, int action_num, window_manager* wm )
{
    if( km == 0 ) 
    {
	if( wm ) km = wm->km;
	if( km == 0 ) return -1;
    }
    int rv = -1;
    while( 1 )
    {	
	if( section_num < 0 ) break;
	if( section_num >= bmem_get_size( km->sections ) / sizeof( sundog_keymap_section ) ) break;
	sundog_keymap_section* section = &km->sections[ section_num ];
	if( section->actions == 0 )
	{
	    section->actions = (sundog_keymap_action*)bmem_new( sizeof( sundog_keymap_action ) * ( action_num + 1 ) );
	    if( section->actions )
	    {
		bmem_zero( section->actions );
		rv = 0;
	    }
	}
	else
	{
	    size_t count = bmem_get_size( section->actions ) / sizeof( sundog_keymap_action );
	    if( action_num >= count )
	    {
		section->actions = (sundog_keymap_action*)bmem_resize( section->actions, ( action_num + 1 ) * sizeof( sundog_keymap_action ) );
		bmem_set( &section->actions[ count ], sizeof( sundog_keymap_action ) * ( action_num - count ), 0 );
		rv = 0;
	    }
	}
	if( rv != -1 )
	{
	    sundog_keymap_action* action = &section->actions[ action_num ];
	    action->name = action_name;
	    action->id = action_id;
	}
	break;
    }
    return rv;
}

const utf8_char* keymap_get_action_name( sundog_keymap* km, int section_num, int action_num, window_manager* wm )
{
    if( km == 0 ) 
    {
	if( wm ) km = wm->km;
	if( km == 0 ) return 0;
    }
    const utf8_char* rv = 0;
    while( 1 )
    {	
	if( section_num < 0 ) break;
	if( section_num >= bmem_get_size( km->sections ) / sizeof( sundog_keymap_section ) ) break;
	sundog_keymap_section* section = &km->sections[ section_num ];
	if( action_num < 0 ) break;
	if( action_num >= bmem_get_size( section->actions ) / sizeof( sundog_keymap_action ) ) break;
	sundog_keymap_action* action = &section->actions[ action_num ];

	rv = action->name;
	
	break;
    }
    return rv;
}

int keymap_bind2( sundog_keymap* km, int section_num, int key, uint flags, uint pars1, uint pars2, int action_num, int bind_num, int bind_flags, window_manager* wm )
{
    if( km == 0 ) 
    {
	if( wm ) km = wm->km;
	if( km == 0 ) return -1;
    }
    int rv = -1;
    while( 1 )
    {	
	if( section_num < 0 ) break;
	if( section_num >= bmem_get_size( km->sections ) / sizeof( sundog_keymap_section ) ) break;
	sundog_keymap_section* section = &km->sections[ section_num ];
	if( action_num < 0 ) break;
	if( action_num >= bmem_get_size( section->actions ) / sizeof( sundog_keymap_action ) ) break;
	sundog_keymap_action* action = &section->actions[ action_num ];
	if( (unsigned)bind_num >= KEYMAP_ACTION_KEYS ) break;
	
	flags &= EVT_FLAG_MODS;

	BSYMTAB_VAL val;
	utf8_char name[ 128 ];

	if( ( bind_flags & KEYMAP_BIND_OVERWRITE ) || 
	    ( bind_flags & KEYMAP_BIND_RESET_TO_DEFAULT ) )
	{
	    keymap_make_codename( name, action->keys[ bind_num ].key, action->keys[ bind_num ].flags, action->keys[ bind_num ].pars1, action->keys[ bind_num ].pars2 );
	    bsymtab_item* sym = bsymtab_lookup( (const utf8_char*)name, -1, 0, 0, val, 0, section->bindings );
	    if( sym )
	    {
		if( sym->val.i == action_num )
		{
		    sym->val.i = -1;
		    if( action->keys[ bind_num ].key == KEY_MIDI_NOTE )
		    {
			int midi_note = action->keys[ bind_num ].pars1 & 255;
			km->midi_notes[ midi_note / 32 ] &= ~( 1 << ( midi_note & 31 ) );
		    }
		}
	    }
	}
	
	if( bind_flags & KEYMAP_BIND_RESET_TO_DEFAULT )
	{
	    int prev_key = action->keys[ bind_num ].key;
	    uint prev_flags = action->keys[ bind_num ].flags;
	    int prev_pars1 = action->keys[ bind_num ].pars1;
	    int prev_pars2 = action->keys[ bind_num ].pars2;
	    if( prev_key )
	    {
		//Selected key+flags will be removed from selected action.
		//We should check if there are some actions where this key is default.
		size_t actions_count = bmem_get_size( section->actions ) / sizeof( sundog_keymap_action );
		for( size_t a = 0; a < actions_count; a++ )
		{
		    if( a == action_num ) continue;
		    sundog_keymap_action* action2 = &section->actions[ a ];
		    for( int n = 0; n < KEYMAP_ACTION_KEYS; n++ )
		    {
			sundog_keymap_key* k = &action2->default_keys[ n ];
			if( k->key == prev_key &&
		    	    k->flags == prev_flags &&
		    	    k->pars1 == prev_pars1 &&
		    	    k->pars2 == prev_pars2 )
			{
			    keymap_bind( km, section_num, 0, 0, a, n, KEYMAP_BIND_RESET_TO_DEFAULT, wm );
			}
		    }
		}
	    }

	    key = action->default_keys[ bind_num ].key;
	    flags = action->default_keys[ bind_num ].flags;	    
	    pars1 = action->default_keys[ bind_num ].pars1;
	    pars2 = action->default_keys[ bind_num ].pars2;
	}
	
	action->keys[ bind_num ].key = key;
	action->keys[ bind_num ].flags = flags;
	action->keys[ bind_num ].pars1 = pars1;
	action->keys[ bind_num ].pars2 = pars2;
	if( bind_flags & KEYMAP_BIND_DEFAULT )
	{
	    action->default_keys[ bind_num ].key = key;
	    action->default_keys[ bind_num ].flags = flags;
	    action->default_keys[ bind_num ].pars1 = pars1;
	    action->default_keys[ bind_num ].pars2 = pars2;
	}

	if( key || flags || pars1 || pars2 )
	{		
	    val.i = action_num;
	    keymap_make_codename( name, key, flags, pars1, pars2 );
	    bool created = 0;
	    bsymtab_item* sym = bsymtab_lookup( (const utf8_char*)name, -1, 1, 0, val, &created, section->bindings );
	    if( sym )
	    {
		if( created == 0 )
		{
		    if( sym->val.i >= 0 )
			if( km->silent == 0 )
			    blog( "keymap: binding (key:%x flags:%x) added to the action \"%s\" and removed from the \"%s\"\n", key, flags, section->actions[ action_num ].name, section->actions[ sym->val.i ].name );
		}
		sym->val = val;
		if( key == KEY_MIDI_NOTE )
		{
		    int midi_note = pars1 & 255;
		    km->midi_notes[ midi_note / 32 ] |= 1 << ( midi_note & 31 );
		}
	    }
	}
	
	rv = 0;
	break;
    }
    return rv;
}

int keymap_bind( sundog_keymap* km, int section_num, int key, uint flags, int action_num, int bind_num, int bind_flags, window_manager* wm )
{
    return keymap_bind2( km, section_num, key, flags, 0, 0, action_num, bind_num, bind_flags, wm );
}

int keymap_get_action( sundog_keymap* km, int section_num, int key, uint flags, uint pars1, uint pars2, window_manager* wm )
{
    if( km == 0 ) 
    {
	if( wm ) km = wm->km;
	if( km == 0 ) return -1;
    }
    int rv = -1;
    while( 1 )
    {	
	if( section_num < 0 ) break;
	if( section_num >= bmem_get_size( km->sections ) / sizeof( sundog_keymap_section ) ) break;
	sundog_keymap_section* section = &km->sections[ section_num ];

	flags &= EVT_FLAG_MODS;
	
	if( key < KEY_MIDI_NOTE || key > KEY_MIDI_PROG )
	{
	    pars1 = 0;
	    pars2 = 0;
	}

	BSYMTAB_VAL val;
	val.i = 0;
	utf8_char name[ 128 ];
	keymap_make_codename( name, key, flags, pars1, pars2 );
	
	bool created = 0;
	bsymtab_item* sym = bsymtab_lookup( name, -1, 0, 0, val, 0, section->bindings );
	if( sym )
	{
	    rv = (int)sym->val.i;
	}

	break;
    }

    return rv;
}

sundog_keymap_key* keymap_get_key( sundog_keymap* km, int section_num, int action_num, int key_num, window_manager* wm )
{
    if( km == 0 ) 
    {
	if( wm ) km = wm->km;
	if( km == 0 ) return 0;
    }
    sundog_keymap_key* rv = 0;
    while( 1 )
    {	
	if( section_num < 0 ) break;
	if( section_num >= bmem_get_size( km->sections ) / sizeof( sundog_keymap_section ) ) break;
	sundog_keymap_section* section = &km->sections[ section_num ];
	if( action_num < 0 ) break;
	if( action_num >= bmem_get_size( section->actions ) / sizeof( sundog_keymap_action ) ) break;
	sundog_keymap_action* action = &section->actions[ action_num ];
	if( key_num < 0 ) break;
	if( key_num >= KEYMAP_ACTION_KEYS ) break;

	if( action->keys[ key_num ].key || action->keys[ key_num ].flags )
	{
	    rv = &action->keys[ key_num ];
	}
	
	break;
    }
    return rv;
}

bool keymap_midi_note_assigned( sundog_keymap* km, int note, window_manager* wm )
{
    if( km == 0 ) 
    {
	if( wm ) km = wm->km;
	if( km == 0 ) return false;
    }
    if( km->midi_notes[ note / 32 ] & ( 1 << ( note & 31 ) ) )
	return true;
    return false;
}

int keymap_save( sundog_keymap* km, profile_data* profile, window_manager* wm )
{
    if( km == 0 ) 
    {
	if( wm ) km = wm->km;
	if( km == 0 ) return -1;
    }
    int rv = -1;
    while( 1 )
    {
	size_t sections_count = bmem_get_size( km->sections ) / sizeof( sundog_keymap_section );
	utf8_char* keys = (utf8_char*)bmem_new( 8 );
	for( size_t snum = 0; snum < sections_count; snum++ )
	{
	    sundog_keymap_section* section = &km->sections[ snum ];
	    size_t actions_count = bmem_get_size( section->actions ) / sizeof( sundog_keymap_action );
	    for( size_t anum = 0; anum < actions_count; anum++ )
	    {
		sundog_keymap_action* action = &section->actions[ anum ];
		utf8_char action_name[ 128 ];
		sprintf( action_name, "%s_%s", section->id, action->id );
		utf8_char key[ 10 * 5 ];
		keys[ 0 ] = 0;
		for( int i = 0; i < KEYMAP_ACTION_KEYS; i++ )
		{
		    if( action->keys[ i ].key != action->default_keys[ i ].key ||
			action->keys[ i ].flags != action->default_keys[ i ].flags ||
			action->keys[ i ].pars1 != action->default_keys[ i ].pars1 ||
			action->keys[ i ].pars2 != action->default_keys[ i ].pars2 )
		    {			
			if( action->keys[ i ].pars1 || action->keys[ i ].pars2 )
			{
			    sprintf( key, "%x %x %x %x %x ", 100 + i, action->keys[ i ].key, action->keys[ i ].flags, action->keys[ i ].pars1, action->keys[ i ].pars2 );
			}
			else
			{
			    sprintf( key, "%x %x %x ", i, action->keys[ i ].key, action->keys[ i ].flags );
			}
			bmem_strcat_resize( keys, key );
		    }
		}
		if( keys[ 0 ] == 0 )
		{
		    profile_remove_key( action_name, profile );
		}
		else
		{
		    profile_set_str_value( action_name, keys, profile );
		}
	    }
	}
	bmem_free( keys );
	rv = 0;
	break;
    }
    if( rv == 0 )
    {
	profile_save( profile );
    }
    return rv;
}

int keymap_load( sundog_keymap* km, profile_data* profile, window_manager* wm )
{
    if( km == 0 ) 
    {
	if( wm ) km = wm->km;
	if( km == 0 ) return -1;
    }
    int rv = -1;
    while( 1 )
    {
	size_t sections_count = bmem_get_size( km->sections ) / sizeof( sundog_keymap_section );
	for( size_t snum = 0; snum < sections_count; snum++ )
	{
	    sundog_keymap_section* section = &km->sections[ snum ];
	    size_t actions_count = bmem_get_size( section->actions ) / sizeof( sundog_keymap_action );
	    for( size_t anum = 0; anum < actions_count; anum++ )
	    {
		sundog_keymap_action* action = &section->actions[ anum ];
		utf8_char action_name[ 128 ];
		sprintf( action_name, "%s_%s", section->id, action->id );
		utf8_char* keys = profile_get_str_value( (const utf8_char*)action_name, 0, profile );
		if( keys )
		{
		    keys = bmem_strdup( keys );
		    int nums[ 5 * KEYMAP_ACTION_KEYS ];
		    int nums_count = 0;
		    bmem_set( nums, sizeof( nums ), 0 );
		    size_t len = bmem_strlen( keys );
		    utf8_char* num_ptr = &keys[ 0 ];
		    for( size_t i = 0; i < len; i++ )
		    {
			if( keys[ i ] == ' ' )
			{
			    keys[ i ] = 0;
			    nums[ nums_count ] = hex_string_to_int( num_ptr );
			    num_ptr = &keys[ i + 1 ];
			    nums_count++;
			    if( nums_count >= 5 * KEYMAP_ACTION_KEYS ) break;
			}
		    }
		    bmem_free( keys );
		    for( int i = 0; i < nums_count; )
		    {
			int bind_num = nums[ i ]; i++;
			int key = nums[ i ]; i++;
			uint flags = nums[ i ]; i++;
			uint pars1 = 0;
			uint pars2 = 0;
			if( bind_num >= 100 )
			{
			    bind_num -= 100;
			    pars1 = nums[ i ]; i++;
			    pars2 = nums[ i ]; i++;
			}
			keymap_bind2( km, snum, key, flags, pars1, pars2, anum, bind_num, KEYMAP_BIND_OVERWRITE, wm );
		    }
		}
	    }
	}
	rv = 0;
	break;
    }
    return rv;
}
