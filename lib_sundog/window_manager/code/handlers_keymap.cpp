/*
    handlers_keymap.cpp
    This file is part of the SunDog engine.
    Copyright (C) 2014 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

#include "core/core.h"

#define SHORTCUT_NAME_SIZE	128

struct keymap_data
{
    WINDOWPTR win;
    WINDOWPTR list_actions;
    WINDOWPTR shortcuts[ KEYMAP_ACTION_KEYS ];
    utf8_char shortcut_names[ KEYMAP_ACTION_KEYS * SHORTCUT_NAME_SIZE ];
    WINDOWPTR reset;
    WINDOWPTR ok;
    int action_num;
    bool capturing;
    int capture_num;
    int capture_key_counter;
    int capture_key;
    uint capture_flags;
    int capture_x;
    int capture_y;
    int capture_scancode;
};

utf8_char g_ascii_names[ 128 * 2 ];

const utf8_char* get_key_name( int key )
{
    const utf8_char* rv = 0;
    if( key > ' ' && key <= '~' )
    {
	if( g_ascii_names[ '0' * 2 ] != '0' )
	{
	    for( int i = 0; i < 128; i++ )
	    {
		int c = i;
		if( i >= 0x61 && i <= 0x7A ) c -= 0x20;
		g_ascii_names[ i * 2 ] = c;
		g_ascii_names[ i * 2 + 1 ] = 0;
	    }
	}
	return (const utf8_char*)&g_ascii_names[ key * 2 ];
    }
    switch( key )
    {
	case KEY_BACKSPACE: rv = "Backspace"; break;
	case KEY_TAB: rv = "Tab"; break;
	case KEY_ENTER: rv = "Enter"; break;
	case KEY_ESCAPE: rv = "Escape"; break;
	case KEY_SPACE: rv = "Space"; break;
	case KEY_F1: rv = "F1"; break;
	case KEY_F2: rv = "F2"; break;
	case KEY_F3: rv = "F3"; break;
	case KEY_F4: rv = "F4"; break;
	case KEY_F5: rv = "F5"; break;
	case KEY_F6: rv = "F6"; break;
	case KEY_F7: rv = "F7"; break;
	case KEY_F8: rv = "F8"; break;
	case KEY_F9: rv = "F9"; break;
	case KEY_F10: rv = "F10"; break;
	case KEY_F11: rv = "F11"; break;
	case KEY_F12: rv = "F12"; break;
	case KEY_UP: rv = "Up"; break;
	case KEY_DOWN: rv = "Down"; break;
	case KEY_LEFT: rv = "Left"; break;
	case KEY_RIGHT: rv = "Right"; break;
	case KEY_INSERT: rv = "Insert"; break;
	case KEY_DELETE: rv = "Delete"; break;
	case KEY_HOME: rv = "Home"; break;
	case KEY_END: rv = "End"; break;
	case KEY_PAGEUP: rv = "PageUp"; break;
	case KEY_PAGEDOWN: rv = "PageDown"; break;
	case KEY_CAPS: rv = "CapsLock"; break;
	case KEY_SHIFT: rv = "Shift"; break;
	case KEY_CTRL: rv = "Ctrl"; break;
	case KEY_ALT: rv = "Alt"; break;
	case KEY_MENU: rv = "Menu"; break;
	case KEY_CMD: rv = "Cmd"; break;
	case KEY_FN: rv = "Fn"; break;
	case KEY_MIDI_NOTE:
	case KEY_MIDI_CTL:
	case KEY_MIDI_NRPN:
	case KEY_MIDI_RPN:
	case KEY_MIDI_PROG:
	    rv = "MIDI: "; 
	    break;
	default:
	    break;
    }
    return rv;
}

void keymap_refresh_shortcut_info( keymap_data* data )
{
    window_manager* wm = data->win->wm;
    list_data* ldata = list_get_data( data->list_actions, wm );
    int action_num = list_get_selected_num( ldata );
    data->action_num = action_num;
    if( action_num >= 0 )
    {
	for( int i = 0; i < KEYMAP_ACTION_KEYS; i++ )
	{
	    data->shortcuts[ i ]->name = "...";
	    data->shortcuts[ i ]->color = wm->button_color;
	}
	for( int i = 0; i < KEYMAP_ACTION_KEYS; i++ )
	{
	    sundog_keymap_key* k = keymap_get_key( 0, 0, action_num, i, wm );
	    if( k == 0 ) continue;
	    utf8_char* name = &data->shortcut_names[ i * SHORTCUT_NAME_SIZE ];
	    name[ 0 ] = 0;
	    bool n = 0;
	    if( k->flags & EVT_FLAG_CTRL ) { bmem_strcat( name, SHORTCUT_NAME_SIZE, "Ctrl" ); n = 1; }
	    if( k->flags & EVT_FLAG_ALT ) { if( n ) bmem_strcat( name, SHORTCUT_NAME_SIZE, "+" ); bmem_strcat( name, SHORTCUT_NAME_SIZE, "Alt" ); n = 1; }
	    if( k->flags & EVT_FLAG_SHIFT ) { if( n ) bmem_strcat( name, SHORTCUT_NAME_SIZE, "+" ); bmem_strcat( name, SHORTCUT_NAME_SIZE, "Shift" ); n = 1; }
	    if( k->flags & EVT_FLAG_MODE ) { if( n ) bmem_strcat( name, SHORTCUT_NAME_SIZE, "+" ); bmem_strcat( name, SHORTCUT_NAME_SIZE, "Mode" ); n = 1; }
	    if( k->flags & EVT_FLAG_CMD ) { if( n ) bmem_strcat( name, SHORTCUT_NAME_SIZE, "+" ); bmem_strcat( name, SHORTCUT_NAME_SIZE, "Cmd" ); n = 1; }
	    if( n ) bmem_strcat( name, SHORTCUT_NAME_SIZE, "+" );
	    const utf8_char* key_name = get_key_name( k->key );
	    if( key_name )
	    {
		bmem_strcat( name, SHORTCUT_NAME_SIZE, key_name );
	    }
	    else
	    {
		utf8_char key_code[ 16 ];
		int_to_string( k->key, key_code );
		bmem_strcat( name, SHORTCUT_NAME_SIZE, key_code );
	    }
	    if( k->key >= KEY_MIDI_NOTE && k->key <= KEY_MIDI_PROG )
	    {
		utf8_char ts[ SHORTCUT_NAME_SIZE ];
		ts[ 0 ] = 0;
		int ch = ( ( k->pars1 >> 16 ) & 0xFFFF ) + 1;
		int val = k->pars1 & 0xFFFF;
		switch( k->key )
		{
		    case KEY_MIDI_NOTE: sprintf( ts, "%d:Note%d", ch, val ); break;
            	    case KEY_MIDI_CTL: sprintf( ts, "%d:CC%d", ch, val ); break;
            	    case KEY_MIDI_NRPN: sprintf( ts, "%d:NRPN%d", ch, val ); break;
            	    case KEY_MIDI_RPN: sprintf( ts, "%d:RPN%d", ch, val ); break;
            	    case KEY_MIDI_PROG: sprintf( ts, "%d:Prog%d", ch, val ); break;
            	}
		bmem_strcat( name, SHORTCUT_NAME_SIZE, ts );
	    }
	    data->shortcuts[ i ]->name = (const utf8_char*)name;
	    data->shortcuts[ i ]->color = wm->button_color;
	}
	for( int i = 0; i < KEYMAP_ACTION_KEYS; i++ )
	{
	    draw_window( data->shortcuts[ i ], wm );
	}
    }    
}

int keymap_actions_list_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    keymap_data* data = (keymap_data*)user_data;
    switch( list_get_last_action( win, wm ) )
    {
        case LIST_ACTION_ESCAPE:
	    remove_window( wm->keymap_win, wm );
	    recalc_regions( wm );
	    draw_window( wm->root_win, wm );
    	    break;
    	default:
	    keymap_refresh_shortcut_info( data );
    	    break;
    }
    return 0;
}

int keymap_shortcut_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    keymap_data* data = (keymap_data*)user_data;
    if( data->capturing )
    {
	data->capturing = 0;
	keymap_refresh_shortcut_info( data );
    }
    else
    {
	data->capturing = 1;
	data->capture_key_counter = 0;
	data->capture_key = 0;
	data->capture_flags = 0;
	data->capture_x = 0;
	data->capture_y = 0;
	data->capture_scancode = 0;
	for( int i = 0; i < KEYMAP_ACTION_KEYS; i++ )
	{
	    if( data->shortcuts[ i ] == win )
	    {
		data->capture_num = i;
		break;
	    }
	}
	set_focus_win( data->win, wm );
	win->name = wm_get_string( STR_WM_ENTER_SHORTCUT );
	win->color = SELECTED_BUTTON_COLOR;
	draw_window( win, wm );
    }
    return 0;
}

int keymap_reset_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    keymap_data* data = (keymap_data*)user_data;
    for( int i = 0; i < KEYMAP_ACTION_KEYS; i++ )
    {
	keymap_bind( 0, 0, 0, 0, data->action_num, i, KEYMAP_BIND_RESET_TO_DEFAULT, wm );
    }
    keymap_save( 0, 0, wm );
    keymap_refresh_shortcut_info( data );
    return 0;
}

int keymap_ok_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    keymap_data* data = (keymap_data*)user_data;
    remove_window( wm->keymap_win, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    return 0;
}

int keymap_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    keymap_data* data = (keymap_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( keymap_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		data->win = win;
		
		data->capturing = 0;
		
		int static_text_size = font_char_y_size( win->font, wm ) + 2;
		
		int y = wm->interelement_space;
		WINDOWPTR label = new_window( wm_get_string( STR_WM_ACTION ), 2, y, 32, static_text_size, win->color, win, label_handler, wm );
                set_window_controller( label, 0, wm, (WCMD)wm->interelement_space, CEND );
                set_window_controller( label, 2, wm, CPERC, (WCMD)100, CEND );
                label = new_window( wm_get_string( STR_WM_SHORTCUTS2 ), 2, y, 32, static_text_size, win->color, win, label_handler, wm );
                set_window_controller( label, 0, wm, CPERC, (WCMD)100 / 2, CEND );
                set_window_controller( label, 2, wm, CPERC, (WCMD)100, CEND );
                y += static_text_size + wm->interelement_space;
                
                data->list_actions = new_window( "", 0, 0, 1, 1, wm->list_background, win, list_handler, wm );
                set_handler( data->list_actions, keymap_actions_list_handler, data, wm );
                set_window_controller( data->list_actions, 0, wm, (WCMD)wm->interelement_space, CEND );
                set_window_controller( data->list_actions, 1, wm, (WCMD)y, CEND );
                set_window_controller( data->list_actions, 2, wm, CPERC, (WCMD)100 / 2, CSUB, (WCMD)wm->interelement_space, CEND );
                set_window_controller( data->list_actions, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->button_ysize + wm->interelement_space, CEND );
                list_data* ldata = list_get_data( data->list_actions, wm );
		list_reset_items( ldata );
		for( int i = 0; ; i++ )
		{
		    const utf8_char* action_name = keymap_get_action_name( 0, 0, i, wm );
		    if( action_name == 0 ) break;
            	    int attr = 0;
            	    list_add_item( action_name, attr, ldata );
            	}
            	list_set_selected_num( 0, ldata );
                
                WINDOWPTR w;
                for( int i = 0; i < KEYMAP_ACTION_KEYS; i++ )
                {
            	    int yy = y + i * ( wm->scrollbar_size + wm->interelement_space );
            	    wm->opt_button_flat = 1;
            	    w = new_window( wm_get_string( STR_WM_SHORTCUT ), 0, yy, 1, wm->scrollbar_size, wm->button_color, win, button_handler, wm );
		    set_window_controller( w, 0, wm, CPERC, (WCMD)100 / 2, CEND );            	    
		    set_window_controller( w, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		    set_handler( w, keymap_shortcut_handler, data, wm );
		    data->shortcuts[ i ] = w;
                }
            	wm->opt_button_flat = 1;
            	w = new_window( wm_get_string( STR_WM_RESET_TO_DEF ), 0, y + KEYMAP_ACTION_KEYS * ( wm->scrollbar_size + wm->interelement_space ), 1, wm->scrollbar_size, wm->button_color, win, button_handler, wm );
		set_window_controller( w, 0, wm, CPERC, (WCMD)100 / 2, CEND );            	    
		set_window_controller( w, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		set_handler( w, keymap_reset_handler, data, wm );
		data->reset = w;
                
                keymap_refresh_shortcut_info( data );

		data->ok = new_window( wm_get_string( STR_WM_CLOSE ), 0, 0, 1, 1, wm->button_color, win, button_handler, wm );
		set_handler( data->ok, keymap_ok_handler, data, wm );
		int x = wm->interelement_space;
		set_window_controller( data->ok, 0, wm, (WCMD)x, CEND );
		set_window_controller( data->ok, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		x += wm->button_xsize;
		set_window_controller( data->ok, 2, wm, (WCMD)x, CEND );
		set_window_controller( data->ok, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->button_ysize + wm->interelement_space, CEND );
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	case EVT_MOUSEBUTTONUP:
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
	case EVT_BUTTONDOWN:
	    if( data->capturing )
	    {
		if( evt->key < KEY_SHIFT || evt->key > KEY_FN )
		{
		    data->capture_key = evt->key;
		    data->capture_flags |= evt->flags;
		    data->capture_x = evt->x;
		    data->capture_y = evt->y;
		    data->capture_scancode = evt->scancode;
		    data->capture_key_counter++;
		}
	    }
	    retval = 1;
	    break;
	case EVT_BUTTONUP:
	    if( data->capturing )
	    {
		if( evt->key < KEY_SHIFT || evt->key > KEY_FN )
		{
		    data->capture_flags |= evt->flags;
		    data->capture_key_counter--;
		    if( data->capture_key_counter <= 0 )
		    {
			data->capturing = 0;
			uint pars1 = 0;
			uint pars2 = 0;
			if( data->capture_key >= KEY_MIDI_NOTE && data->capture_key <= KEY_MIDI_PROG )
			{
			    //Currently the x,y parameters are used for the MIDI key events only
			    pars1 = data->capture_x | ( data->capture_y << 16 );
			}
			keymap_bind2( 0, 0, data->capture_key, data->capture_flags, pars1, pars2, data->action_num, data->capture_num, KEYMAP_BIND_OVERWRITE, wm );
			keymap_save( 0, 0, wm );
			keymap_refresh_shortcut_info( data );
		    }
		}
	    }
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    retval = 1;
	    wm->keymap_win = 0;
	    break;
    }
    return retval;
}
