/*
    handlers_preferences.cpp
    This file is part of the SunDog engine.
    Copyright (C) 2011 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

#include "core/core.h"

struct prefs_data
{
    WINDOWPTR win;
    WINDOWPTR close;
    WINDOWPTR sections;
    int list_xsize;
    int cur_section;
    WINDOWPTR cur_section_window;
    
    int correct_ysize;
};

int prefs_close_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_data* data = (prefs_data*)user_data;

    remove_window( data->win, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    
    if( wm->prefs_restart_request )
    {
	if( dialog( wm_get_string( STR_WM_PREFS_CHANGED ), wm_get_string( STR_WM_YESNO ), wm ) == 0 )
	{
	    wm->exit_request = 1;
	    wm->restart_request = 1;
	}
    }
    
    return 0;
}

int prefs_sections_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_data* data = (prefs_data*)user_data;
    list_data* ldata = list_get_data( data->sections, wm );
    
    if( ldata )
    {
	if( list_get_last_action( win, wm ) == LIST_ACTION_ESCAPE )
	{
	    //ESCAPE KEY:
	    prefs_close_handler( data, 0, wm );
	    return 0;
	}
	int sel = list_get_selected_num( ldata );
	if( (unsigned)sel < (unsigned)wm->prefs_sections )
	{
	    if( sel != data->cur_section )
	    {
		data->cur_section = sel;
		//Close old section:
		remove_window( data->cur_section_window, wm );
		//Open new section:
		data->cur_section_window = new_window( "Section", 0, 0, 1, 1, data->win->color, data->win, (int(*)(sundog_event*,window_manager*)) wm->prefs_section_handlers[ data->cur_section ], wm );
		set_window_controller( data->cur_section_window, 0, wm, (WCMD)wm->interelement_space * 2 + data->list_xsize, CEND );
		set_window_controller( data->cur_section_window, 1, wm, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->cur_section_window, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->cur_section_window, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		show_window( data->cur_section_window, wm );
		bring_to_front( data->close, wm );
		//Resize prefs window:
		int new_ysize = wm->prefs_section_ysize + wm->interelement_space * 3 + wm->button_ysize;
		if( new_ysize < wm->large_window_ysize )
		    new_ysize = wm->large_window_ysize;
		resize_window_with_decorator( data->win, 0, new_ysize, wm );
		//Show it:
		recalc_regions( wm );
		draw_window( wm->root_win, wm );
	    }
	}
    }
    
    return 0;
}

int prefs_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    prefs_data* data = (prefs_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( prefs_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		data->win = win;
		
		wm->prefs_restart_request = false;

		data->list_xsize = 16;
		wm->prefs_sections = 0;
		while( 1 )
		{
		    const utf8_char* section_name = wm->prefs_section_names[ wm->prefs_sections ];
		    if( section_name == 0 ) break;
		    int x = font_string_x_size( section_name, win->font, wm ) + wm->interelement_space2;
		    if( x > data->list_xsize ) data->list_xsize = x;
		    wm->prefs_sections++;
		}
		
		wm->opt_list_without_scrollbar = true;
		data->sections = new_window( "Sections", 0, 0, 1, 1, wm->list_background, win, list_handler, wm );
		set_window_controller( data->sections, 0, wm, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->sections, 1, wm, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->sections, 2, wm, (WCMD)wm->interelement_space + data->list_xsize, CEND );
		set_window_controller( data->sections, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->button_ysize + wm->interelement_space * 2, CEND );
		set_handler( data->sections, prefs_sections_handler, data, wm );
		list_data* l = list_get_data( data->sections, wm );
		for( int i = 0; i < wm->prefs_sections; i++ )
		{
		    const utf8_char* section_name = wm->prefs_section_names[ i ];
		    list_add_item( section_name, 0, l );
		}
		list_set_selected_num( 0, l );

		data->cur_section = 0;
		data->cur_section_window = new_window( "Section", 0, 0, 1, 1, win->color, win, (int(*)(sundog_event*,window_manager*)) wm->prefs_section_handlers[ 0 ], wm );
		set_window_controller( data->cur_section_window, 0, wm, (WCMD)wm->interelement_space * 2 + data->list_xsize, CEND );
		set_window_controller( data->cur_section_window, 1, wm, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->cur_section_window, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->cur_section_window, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		
		int x = wm->interelement_space;
		const utf8_char* bname;
		int bxsize;
		
		bname = wm_get_string( STR_WM_CLOSE ); bxsize = button_get_optimal_xsize( bname, win->font, false, wm );
		data->close = new_window( bname, x, 0, bxsize, 1, wm->button_color, win, button_handler, wm );
		set_handler( data->close, prefs_close_handler, data, wm );
		set_window_controller( data->close, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->close, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->button_ysize + wm->interelement_space, CEND );
		
		//data->correct_ysize = win->ysize;
	    }
	    retval = 1;
	    break;
	case EVT_DRAW:
	    break;
	case EVT_BUTTONDOWN:
	    if( evt->key == KEY_ESCAPE )
	    {
		prefs_close_handler( data, 0, wm );
		retval = 1;
	    }
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    retval = 1;
	    wm->prefs_win = 0;
	    break;
	    
	case EVT_SCREENRESIZE:
	case EVT_BEFORESHOW:
	    //resize_window_with_decorator( win, 0, data->correct_ysize, wm );
	    break;
    }
    return retval;
}

//
//
//

struct prefs_ui_data
{
    WINDOWPTR win;
    WINDOWPTR window_pars;
    WINDOWPTR maxfps;
    WINDOWPTR angle;
    WINDOWPTR control;
    WINDOWPTR dclick;
    WINDOWPTR color;
    WINDOWPTR scale;
    WINDOWPTR virt_kbd;
    WINDOWPTR keymap;
    WINDOWPTR lang;
};

void prefs_ui_reinit( WINDOWPTR win )
{
    prefs_ui_data* data = (prefs_ui_data*)win->data;
    window_manager* wm = data->win->wm;
    
    utf8_char ts[ 512 ];
    const utf8_char* v = "";
    
    if( data->maxfps )
    {
	sprintf( ts, "%s = %d", wm_get_string( STR_WM_MAXFPS ), profile_get_int_value( "maxfps", wm->max_fps, 0 ) );
	button_set_text( data->maxfps, ts, wm );
    }

    if( data->angle )
    {
	sprintf( ts, "%s = %d", wm_get_string( STR_WM_ANGLE ), profile_get_int_value( KEY_ROTATE, 0, 0 ) );
	button_set_text( data->angle, ts, wm );
    }
    
    if( data->control )
    {
	int tc = profile_get_int_value( KEY_TOUCHCONTROL, -1, 0 );
	int pc = profile_get_int_value( KEY_PENCONTROL, -1, 0 );
	if( tc == -1 && pc == -1 ) v = wm_get_string( STR_WM_AUTO );
	if( tc >= 0 ) v = wm_get_string( STR_WM_CTL_FINGERS );
	if( pc >= 0 ) v = wm_get_string( STR_WM_CTL_PEN );
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_CTL_TYPE ), v );
	button_set_text( data->control, ts, wm );
    }
    
    if( data->dclick )
    {
	int t = profile_get_int_value( KEY_DOUBLECLICK, -1, 0 );	
	if( t == -1 ) 
	    sprintf( ts, "%s = %s (%d%s)", wm_get_string( STR_WM_DOUBLE_CLICK_TIME ), wm_get_string( STR_WM_AUTO ), wm->double_click_time, wm_get_string( STR_WM_MS ) );
	else
	    sprintf( ts, "%s = %d%s", wm_get_string( STR_WM_DOUBLE_CLICK_TIME ), t, wm_get_string( STR_WM_MS ) );
	button_set_text( data->dclick, ts, wm );
    }
    
    if( data->virt_kbd )
    {
	int vk = -1;
	if( profile_get_int_value( KEY_SHOW_VIRT_KBD, -1, 0 ) != -1 ) vk = 1;
	if( profile_get_int_value( KEY_HIDE_VIRT_KBD, -1, 0 ) != -1 ) vk = 0;
	if( vk == -1 ) v = wm_get_string( STR_WM_AUTO );
	if( vk == 0 ) v = wm_get_string( STR_WM_NO_CAP );
	if( vk == 1 ) v = wm_get_string( STR_WM_YES_CAP );
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_SHOW_KBD ), v );
	button_set_text( data->virt_kbd, ts, wm );
    }
    
    if( data->lang )
    {
	utf8_char* lang = profile_get_str_value( KEY_LANG, 0, 0 );
	v = wm_get_string( STR_WM_AUTO );
	if( bmem_strstr( lang, "ru_" ) ) v = "русский";
	if( bmem_strstr( lang, "en_" ) ) v = "English";
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_LANG ), v );
	button_set_text( data->lang, ts, wm );
    }
}

int prefs_ui_fps_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_ui_data* data = (prefs_ui_data*)user_data;
    
    int new_fps = -1;
    int v = win->action_result;
    switch( v )
    {
	case 0: new_fps = 0; break;
	case 1: new_fps = 5; break;
	case 2: new_fps = 8; break;
	case 3: new_fps = 10; break;
	case 4: new_fps = 15; break;
	case 5: new_fps = 20; break;
	case 6: new_fps = 25; break;
	case 7: new_fps = 30; break;
	case 8: new_fps = 40; break;
	case 9: new_fps = 50; break;
	case 10: new_fps = 60; break;
	case 11: new_fps = 120; break;
	default: new_fps = -1; break;
    }
    if( new_fps == 0 )
	profile_remove_key( "maxfps", 0 );
    if( new_fps > 0 )
	profile_set_int_value( "maxfps", new_fps, 0 );
    if( new_fps != -1 )
    {
	profile_save( 0 );
	wm->prefs_restart_request = true;
    }
    
    prefs_ui_reinit( data->win );
    draw_window( data->win, wm );
    
    return 0;
}

int prefs_ui_angle_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_ui_data* data = (prefs_ui_data*)user_data;

    int prev_angle = profile_get_int_value( KEY_ROTATE, 0, 0 );    
    int angle = win->action_result * 90;
    if( angle >= 0 && angle <= 270 )
    {
	if( prev_angle != angle )
	{
	    if( angle )
		profile_set_int_value( KEY_ROTATE, angle, 0 );
	    else
		profile_remove_key( KEY_ROTATE, 0 );
	    profile_save( 0 );
	    wm->prefs_restart_request = true;
	}
    }
    
    prefs_ui_reinit( data->win );
    draw_window( data->win, wm );
    
    return 0;
}

int prefs_ui_control_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_ui_data* data = (prefs_ui_data*)user_data;

    int v = win->action_result;
    switch( v )
    {
	case 0:
	    //Auto:
	    profile_remove_key( KEY_TOUCHCONTROL, 0 );
	    profile_remove_key( KEY_PENCONTROL, 0 );
	    break;
	case 1:
	    //Fingers:
	    profile_set_int_value( KEY_TOUCHCONTROL, 1, 0 );
	    profile_remove_key( KEY_PENCONTROL, 0 );
	    break;
	case 2:
	    //Pen/Mouse:
	    profile_set_int_value( KEY_PENCONTROL, 1, 0 );
	    profile_remove_key( KEY_TOUCHCONTROL, 0 );
	    break;
	default:
	    return 0;
	    break;
    }
    profile_save( 0 );
    wm->prefs_restart_request = true;

    prefs_ui_reinit( data->win );
    draw_window( data->win, wm );
    
    return 0;
}

int prefs_ui_dclick_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_ui_data* data = (prefs_ui_data*)user_data;

    int v = win->action_result;
    if( v == 0 )
    {
	//Auto:
	profile_remove_key( KEY_DOUBLECLICK, 0 );
	wm->double_click_time = DEFAULT_DOUBLE_CLICK_TIME;
    }
    else
    {
	if( v >= 1 )
	{
	    v++;
	    profile_set_int_value( KEY_DOUBLECLICK, v * 50, 0 );
	    wm->double_click_time = v * 50;
	}
    }
    profile_save( 0 );

    prefs_ui_reinit( data->win );
    draw_window( data->win, wm );

    return 0;
}

int prefs_ui_color_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    colortheme_open( wm );
    return 0;
}

int prefs_ui_scale_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    ui_scale_open( wm );
    return 0;
}

int prefs_ui_virt_kbd_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_ui_data* data = (prefs_ui_data*)user_data;

    int v = win->action_result;
    switch( v )
    {
	case 0:
	    //Auto:
	    profile_remove_key( KEY_SHOW_VIRT_KBD, 0 );
	    profile_remove_key( KEY_HIDE_VIRT_KBD, 0 );
	    break;
	case 1:
	    //Yes:
	    profile_set_int_value( KEY_SHOW_VIRT_KBD, 1, 0 );
	    profile_remove_key( KEY_HIDE_VIRT_KBD, 0 );
	    break;
	case 2:
	    //No:
	    profile_set_int_value( KEY_HIDE_VIRT_KBD, 1, 0 );
	    profile_remove_key( KEY_SHOW_VIRT_KBD, 0 );
	    break;
	default:
	    return 0;
	    break;
    }
    profile_save( 0 );
    wm->prefs_restart_request = true;

    prefs_ui_reinit( data->win );
    draw_window( data->win, wm );
    
    return 0;
}

int prefs_ui_keymap_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    keymap_open( wm );
    return 0;
}

int prefs_ui_lang_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_ui_data* data = (prefs_ui_data*)user_data;

    int v = win->action_result;
    switch( v )
    {
	case 0:
	    //Auto:
	    profile_remove_key( KEY_LANG, 0 );
	    break;
	case 1:
	    //English:
	    profile_set_str_value( KEY_LANG, "en_US", 0 );
	    break;
	case 2:
	    //Russian:
	    profile_set_str_value( KEY_LANG, "ru_RU", 0 );
	    break;
	default:
	    return 0;
	    break;
    }
    profile_save( 0 );
    wm->prefs_restart_request = true;

    prefs_ui_reinit( data->win );
    draw_window( data->win, wm );
        
    return 0;
}

int prefs_ui_window_pars_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    int screen_xsize = profile_get_int_value( KEY_SCREENX, wm->real_window_width, 0 );
    int screen_ysize = profile_get_int_value( KEY_SCREENY, wm->real_window_height, 0 );
    int fullscreen = 0;
    if( profile_get_int_value( KEY_FULLSCREEN, -1, 0 ) != -1 ) fullscreen = 1;
    dialog_item di[ 6 ];
    bmem_set( &di, sizeof( di ), 0 );
    di[ 0 ].type = DIALOG_ITEM_LABEL;
    di[ 0 ].str_val = (utf8_char*)wm_get_string( STR_WM_WINDOW_WIDTH );
    di[ 1 ].type = DIALOG_ITEM_NUMBER;
    di[ 1 ].min = 0;
    di[ 1 ].max = 8000;
    di[ 1 ].int_val = screen_xsize;
    di[ 2 ].type = DIALOG_ITEM_LABEL;
    di[ 2 ].str_val = (utf8_char*)wm_get_string( STR_WM_WINDOW_HEIGHT );
    di[ 3 ].type = DIALOG_ITEM_NUMBER;
    di[ 3 ].min = 0;
    di[ 3 ].max = 8000;
    di[ 3 ].int_val = screen_ysize;
    di[ 4 ].type = DIALOG_ITEM_POPUP;
    di[ 4 ].str_val = (utf8_char*)wm_get_string( STR_WM_WINDOW_FULLSCREEN );
    di[ 4 ].int_val = fullscreen;
    di[ 4 ].menu = wm_get_string( STR_WM_DISABLED_ENABLED_MENU );
    di[ 5 ].type = DIALOG_ITEM_NONE;
    wm->opt_dialog_items = di;
    int d = dialog( wm_get_string( STR_WM_WINDOW_PARS ), wm_get_string( STR_WM_OKCANCEL ), wm );
    if( d == 0 )
    {
        bool changed = 0;
    	if( di[ 1 ].int_val != screen_xsize || di[ 3 ].int_val != screen_ysize )
    	{
    	    profile_set_int_value( KEY_SCREENX, di[ 1 ].int_val, 0 );
    	    profile_set_int_value( KEY_SCREENY, di[ 3 ].int_val, 0 );
    	    changed = 1;
    	}
    	if( di[ 4 ].int_val != fullscreen )
    	{
    	    if( di[ 4 ].int_val )
    		profile_set_int_value( KEY_FULLSCREEN, 1, 0 );
    	    else
    		profile_remove_key( KEY_FULLSCREEN, 0 );
    	    changed = 1;
    	}
        if( changed )
        {
    	    wm->prefs_restart_request = true;
    	    profile_save( 0 );
    	}
    }
    return 0;
}

int prefs_ui_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    prefs_ui_data* data = (prefs_ui_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( prefs_ui_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		data->win = win;
		
		utf8_char ts[ 256 ];
		int y = 0;

#if !defined(IPHONE) && !defined(ANDROID) && !defined(WINCE) && !defined(OSX)
		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->window_pars = new_window( wm_get_string( STR_WM_WINDOW_PARS ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->window_pars, prefs_ui_window_pars_handler, data, wm );
		set_window_controller( data->window_pars, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->window_pars, 2, wm, CPERC, (WCMD)50, CSUB, 1, CEND );
#else
		data->window_pars = 0;
#endif

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->scale = new_window( wm_get_string( STR_WM_UI_SCALE ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->scale, prefs_ui_scale_handler, data, wm );
		if( data->window_pars )
		    set_window_controller( data->scale, 0, wm, CPERC, (WCMD)50, CEND );
		else
		    set_window_controller( data->scale, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->scale, 2, wm, CPERC, (WCMD)100, CEND );
		y += wm->text_ysize + wm->interelement_space;

#ifdef SCREEN_ROTATE_SUPPORTED
		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->angle = new_window( wm_get_string( STR_WM_ANGLE ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->angle, prefs_ui_angle_handler, data, wm );
		button_set_menu( data->angle, "0\n90\n180\n270", wm );
		set_window_controller( data->angle, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->angle, 2, wm, CPERC, (WCMD)50, CSUB, 1, CEND );
#else
		data->angle = 0;
#endif

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->maxfps = new_window( wm_get_string( STR_WM_MAXFPS ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->maxfps, prefs_ui_fps_handler, data, wm );
		ts[ 0 ] = 0;
		bmem_strcat( ts, sizeof( ts ), wm_get_string( STR_WM_AUTO ) );
		bmem_strcat( ts, sizeof( ts ), "\n5\n8\n10\n15\n20\n25\n30\n40\n50\n60\n120" );
		button_set_menu( data->maxfps, ts, wm );
		if( data->angle )
		    set_window_controller( data->maxfps, 0, wm, CPERC, (WCMD)50, CEND );
		else
		    set_window_controller( data->maxfps, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->maxfps, 2, wm, CPERC, (WCMD)100, CEND );
		y += wm->text_ysize + wm->interelement_space;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->color = new_window( wm_get_string( STR_WM_COLOR_THEME ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->color, prefs_ui_color_handler, data, wm );
		set_window_controller( data->color, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->color, 2, wm, CPERC, (WCMD)100, CEND );
		if( wm->prefs_flags & PREFS_FLAG_NO_COLOR_THEME )
		    data->color->flags |= WIN_FLAG_ALWAYS_INVISIBLE;
		else
		    y += wm->text_ysize + wm->interelement_space;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->control = new_window( wm_get_string( STR_WM_CTL_TYPE ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->control, prefs_ui_control_handler, data, wm );
		button_set_menu( data->control, wm_get_string( STR_WM_CTL_TYPE_MENU ), wm );
		set_window_controller( data->control, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->control, 2, wm, CPERC, (WCMD)100, CEND );
		if( wm->prefs_flags & PREFS_FLAG_NO_CONTROL_TYPE )
		    data->control->flags |= WIN_FLAG_ALWAYS_INVISIBLE;
		else
		    y += wm->text_ysize + wm->interelement_space;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->dclick = new_window( wm_get_string( STR_WM_DOUBLE_CLICK_TIME ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->dclick, prefs_ui_dclick_handler, data, wm );
		ts[ 0 ] = 0;
		bmem_strcat( ts, sizeof( ts ), wm_get_string( STR_WM_AUTO ) );
		bmem_strcat( ts, sizeof( ts ), "\n100\n150\n200\n250\n300\n350\n400\n450\n500" );
		button_set_menu( data->dclick, ts, wm );
		set_window_controller( data->dclick, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->dclick, 2, wm, CPERC, (WCMD)100, CEND );
		y += wm->text_ysize + wm->interelement_space;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->virt_kbd = new_window( wm_get_string( STR_WM_SHOW_KBD ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->virt_kbd, prefs_ui_virt_kbd_handler, data, wm );
		button_set_menu( data->virt_kbd, wm_get_string( STR_WM_AUTO_YES_NO_MENU ), wm );
		set_window_controller( data->virt_kbd, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->virt_kbd, 2, wm, CPERC, (WCMD)50, CSUB, 1, CEND );

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->keymap = new_window( wm_get_string( STR_WM_SHORTCUTS_SHORT ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->keymap, prefs_ui_keymap_handler, data, wm );
		set_window_controller( data->keymap, 0, wm, CPERC, (WCMD)50, CEND );
		set_window_controller( data->keymap, 2, wm, CPERC, (WCMD)100, CEND );
		if( wm->prefs_flags & PREFS_FLAG_NO_KEYMAP )
		{
		    data->keymap->flags |= WIN_FLAG_ALWAYS_INVISIBLE;
		    set_window_controller( data->virt_kbd, 2, wm, CPERC, (WCMD)100, CEND );
		}
		y += wm->text_ysize + wm->interelement_space;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->lang = new_window( wm_get_string( STR_WM_LANG ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->lang, prefs_ui_lang_handler, data, wm );
		ts[ 0 ] = 0;
		bmem_strcat( ts, sizeof( ts ), wm_get_string( STR_WM_AUTO ) );
		bmem_strcat( ts, sizeof( ts ), "\nEnglish\nРусский" );
		button_set_menu( data->lang, ts, wm );
		set_window_controller( data->lang, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->lang, 2, wm, CPERC, (WCMD)100, CEND );
		y += wm->text_ysize + wm->interelement_space;
		
		prefs_ui_reinit( win );
		
		wm->prefs_section_ysize = y;
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    retval = 1;
	    break;
    }
    return retval;
}

//
//
//

struct prefs_video_data
{
    WINDOWPTR win;
    WINDOWPTR cam;
};

void prefs_video_reinit( WINDOWPTR win )
{
    prefs_video_data* data = (prefs_video_data*)win->data;
    window_manager* wm = data->win->wm;

    utf8_char ts[ 512 ];

    int cam = profile_get_int_value( KEY_CAMERA, -1111, 0 );
    if( cam == -1111 )
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_CAMERA ), wm_get_string( STR_WM_AUTO ) );
    else
    {
#if defined(IPHONE) || defined(ANDROID)
	if( cam >= 0 && cam <= 1 )
	{
	    const utf8_char* n;
	    if( cam == 0 ) n = wm_get_string( STR_WM_BACK_CAM );
	    if( cam == 1 ) n = wm_get_string( STR_WM_FRONT_CAM );
	    sprintf( ts, "%s = %d (%s)", wm_get_string( STR_WM_CAMERA ), cam, n );
	}
	else
#endif
	{
	    sprintf( ts, "%s = %d", wm_get_string( STR_WM_CAMERA ), cam );
	}
    }
    button_set_text( data->cam, ts, wm );
}

int prefs_video_cam_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_video_data* data = (prefs_video_data*)user_data;

    int cam = win->action_result;
    if( cam < 0 || cam > 10 ) return 0;
    
    if( cam == 0 )
    {
	if( profile_get_int_value( KEY_CAMERA, -1, 0 ) != -1 )
	{
	    profile_remove_key( KEY_CAMERA, 0 );
	    wm->prefs_restart_request = true;
	    profile_save( 0 );
	}
    }
    if( cam > 0 )
    {
	cam--;
	if( profile_get_int_value( KEY_CAMERA, -1, 0 ) != cam )
	{
	    profile_set_int_value( KEY_CAMERA, cam, 0 );
	    wm->prefs_restart_request = true;
	    profile_save( 0 );
	}
    }

    prefs_video_reinit( data->win );
    draw_window( data->win, wm );

    return 0;
}

int prefs_video_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    prefs_video_data* data = (prefs_video_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( prefs_video_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		data->win = win;
		
		utf8_char ts[ 512 ];
		int y = 0;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->cam = new_window( wm_get_string( STR_WM_CAMERA ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->cam, prefs_video_cam_handler, data, wm );
#if defined(IPHONE) || defined(ANDROID)
		sprintf( ts, "%s\n0 (%s)\n1 (%s)\n2\n3\n4\n5\n6\n7", wm_get_string( STR_WM_AUTO ), wm_get_string( STR_WM_BACK_CAM ), wm_get_string( STR_WM_FRONT_CAM ) );
#else
		sprintf( ts, "%s\n0\n1\n2\n3\n4\n5\n6\n7", wm_get_string( STR_WM_AUTO ) );
#endif
		button_set_menu( data->cam, ts, wm );
		set_window_controller( data->cam, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->cam, 2, wm, CPERC, (WCMD)100, CEND );
		y += wm->text_ysize + wm->interelement_space;

		prefs_video_reinit( win );
		
		wm->prefs_section_ysize = y;
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    retval = 1;
	    break;
    }
    return retval;
}

//
//
//

struct prefs_audio_data
{
    WINDOWPTR win;
    WINDOWPTR buf;
    WINDOWPTR freq;
    WINDOWPTR driver;
    WINDOWPTR device;
    WINDOWPTR device_in;
    WINDOWPTR more;
};

void prefs_audio_reinit( WINDOWPTR win )
{
    prefs_audio_data* data = (prefs_audio_data*)win->data;
    window_manager* wm = data->win->wm;
    
    utf8_char ts[ 512 ];

    utf8_char* drv = profile_get_str_value( KEY_AUDIODRIVER, 0, 0 );
    if( bmem_strstr( drv, "asio" ) )
    {
	if( data->more->flags & WIN_FLAG_ALWAYS_INVISIBLE )
	{
	    data->more->flags &= ~WIN_FLAG_ALWAYS_INVISIBLE;
	    show_window( data->more, wm );
	    recalc_regions( wm );
        }
    }
    else
    {
	if( ( data->more->flags & WIN_FLAG_ALWAYS_INVISIBLE ) == 0 )
	{
	    data->more->flags |= WIN_FLAG_ALWAYS_INVISIBLE;
	    hide_window( data->more, wm );
	    recalc_regions( wm );
	}
    }
    if( drv == 0 )
    {
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_DRIVER ), wm_get_string( STR_WM_AUTO ) );
    }
    else
    {
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_DRIVER ), drv );
	utf8_char** names = 0;
	utf8_char** infos = 0;
	int drivers = sound_stream_get_drivers( &names, &infos );
	if( ( drivers > 0 ) && names && infos )
	{
	    for( int d = 0; d < drivers; d++ )
	    {
		if( bmem_strcmp( names[ d ], drv ) == 0 )
		{
		    sprintf( ts, "%s = %s", wm_get_string( STR_WM_DRIVER ), infos[ d ] );
		    break;
		}
	    }
	    for( int d = 0; d < drivers; d++ )
	    {
		bmem_free( names[ d ] );
		bmem_free( infos[ d ] );
	    }
	    bmem_free( names );
	    bmem_free( infos );
	}
    }
    button_set_text( data->driver, ts, wm );

    utf8_char* dev = profile_get_str_value( KEY_AUDIODEVICE, 0, 0 );
    if( dev == 0 )
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_OUTPUT ), wm_get_string( STR_WM_AUTO ) );
    else
    {
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_OUTPUT ), dev );
	utf8_char** names = 0;
	utf8_char** infos = 0;
	int devices = sound_stream_get_devices( profile_get_str_value( KEY_AUDIODRIVER, 0, 0 ), &names, &infos, 0 );
	if( ( devices > 0 ) && names && infos )
	{
	    for( int d = 0; d < devices; d++ )
	    {
		if( bmem_strcmp( names[ d ], dev ) == 0 )
		{
		    ts[ 0 ] = 0;
		    bmem_strcat( ts, sizeof( ts ), wm_get_string( STR_WM_OUTPUT ) );
		    bmem_strcat( ts, sizeof( ts ), " = " );
		    bmem_strcat( ts, sizeof( ts ), infos[ d ] );
		    break;
		}
	    }
	    for( int d = 0; d < devices; d++ )
	    {
		bmem_free( names[ d ] );
		bmem_free( infos[ d ] );
	    }
	    bmem_free( names );
	    bmem_free( infos );
	}
    }
    button_set_text( data->device, ts, wm );

    dev = profile_get_str_value( KEY_AUDIODEVICE_IN, 0, 0 );
    if( dev == 0 )
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_INPUT ), wm_get_string( STR_WM_AUTO ) );
    else
    {
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_INPUT ), dev );
	utf8_char** names = 0;
	utf8_char** infos = 0;
	int devices = sound_stream_get_devices( profile_get_str_value( KEY_AUDIODRIVER, 0, 0 ), &names, &infos, 1 );
	if( ( devices > 0 ) && names && infos )
	{
	    for( int d = 0; d < devices; d++ )
	    {
		if( bmem_strcmp( names[ d ], dev ) == 0 )
		{
		    ts[ 0 ] = 0;
		    bmem_strcat( ts, sizeof( ts ), wm_get_string( STR_WM_INPUT ) );
		    bmem_strcat( ts, sizeof( ts ), " = " );
		    bmem_strcat( ts, sizeof( ts ), infos[ d ] );
		    break;
		}
	    }
	    for( int d = 0; d < devices; d++ )
	    {
		bmem_free( names[ d ] );
		bmem_free( infos[ d ] );
	    }
	    bmem_free( names );
	    bmem_free( infos );
	}
    }
    button_set_text( data->device_in, ts, wm );
    
    int freq = 44100;
    if( g_snd_initialized )
        freq = g_snd.freq;
    int size = profile_get_int_value( KEY_SOUNDBUFFER, 0, 0 );
    if( size == 0 )
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_BUFFER ), wm_get_string( STR_WM_AUTO ) );
    else
	sprintf( ts, "%s = %d; %d %s", wm_get_string( STR_WM_BUFFER ), size, ( size * 1000 ) / freq, wm_get_string( STR_WM_MS ) );
    button_set_text( data->buf, ts, wm );

    freq = profile_get_int_value( KEY_FREQ, 0, 0 );
    if( freq == 0 )
	sprintf( ts, "%s = %s", wm_get_string( STR_WM_FREQ ), wm_get_string( STR_WM_AUTO ) );
    else
	sprintf( ts, "%s = %d %s", wm_get_string( STR_WM_FREQ ), freq, wm_get_string( STR_WM_HZ ) );
    button_set_text( data->freq, ts, wm );
}

int prefs_audio_driver_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_audio_data* data = (prefs_audio_data*)user_data;
    
    size_t menu_size = 8192;
    utf8_char* menu = (utf8_char*)bmem_new( menu_size );
    menu[ 0 ] = 0;
    
    bmem_strcat( menu, menu_size, wm_get_string( STR_WM_AUTO ) );
    
    utf8_char** names = 0;
    utf8_char** infos = 0;
    int drivers = sound_stream_get_drivers( &names, &infos );
    if( ( drivers > 0 ) && names && infos )
    {
	bmem_strcat( menu, menu_size, "\n" );
	
	for( int d = 0; d < drivers; d++ )
	{
	    bmem_strcat( menu, menu_size, infos[ d ] );
	    if( d != drivers - 1 )
		bmem_strcat( menu, menu_size, "\n" );
	}

	int sel = popup_menu( wm_get_string( STR_WM_DRIVER ), menu, win->screen_x, win->screen_y, wm->menu_color, wm );
	if( (unsigned)sel < (unsigned)drivers + 1 )
	{
	    if( sel == 0 )
	    {
		//Auto:
		if( profile_get_str_value( KEY_AUDIODRIVER, 0, 0 ) != 0 )
		{
		    profile_remove_key( KEY_AUDIODRIVER, 0 );
		    profile_remove_key( KEY_AUDIODEVICE, 0 );
		    profile_remove_key( KEY_AUDIODEVICE_IN, 0 );
            	    wm->prefs_restart_request = true;
        	    profile_save( 0 );
		}
	    }
	    else
	    {
		if( bmem_strcmp( profile_get_str_value( KEY_AUDIODRIVER, "", 0 ), names[ sel - 1 ] ) )
		{
		    profile_set_str_value( KEY_AUDIODRIVER, names[ sel - 1 ], 0 );
		    profile_remove_key( KEY_AUDIODEVICE, 0 );
		    profile_remove_key( KEY_AUDIODEVICE_IN, 0 );
		    wm->prefs_restart_request = true;
            	    profile_save( 0 );
		}
	    }
	}

	for( int d = 0; d < drivers; d++ )
	{
	    bmem_free( names[ d ] );
	    bmem_free( infos[ d ] );
	}
	bmem_free( names );
	bmem_free( infos );
    }
    
    bmem_free( menu );    

    prefs_audio_reinit( data->win );
    draw_window( data->win, wm );

    return 0;
}

int prefs_audio_device_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_audio_data* data = (prefs_audio_data*)user_data;
    
    bool input = 0;
    const utf8_char* key = KEY_AUDIODEVICE;
    if( win == data->device_in )
    {
	input = 1;
	key = KEY_AUDIODEVICE_IN;
    }
    
    size_t menu_size = 8192;
    utf8_char* menu = (utf8_char*)bmem_new( menu_size );
    menu[ 0 ] = 0;
    
    bmem_strcat( menu, menu_size, wm_get_string( STR_WM_AUTO ) );
    
    utf8_char** names = 0;
    utf8_char** infos = 0;
    int devices = sound_stream_get_devices( profile_get_str_value( KEY_AUDIODRIVER, 0, 0 ), &names, &infos, input );
    if( ( devices > 0 ) && names && infos )
    {
	bmem_strcat( menu, menu_size, "\n" );
	
	for( int d = 0; d < devices; d++ )
	{
	    bmem_strcat( menu, menu_size, infos[ d ] );
	    if( d != devices - 1 )
		bmem_strcat( menu, menu_size, "\n" );
	}

	int sel;
	if( input )
	    sel = popup_menu( wm_get_string( STR_WM_INPUT_DEVICE ), menu, win->screen_x, win->screen_y, wm->menu_color, wm );
	else
	    sel = popup_menu( wm_get_string( STR_WM_DEVICE ), menu, win->screen_x, win->screen_y, wm->menu_color, wm );
	if( (unsigned)sel < (unsigned)devices + 1 )
	{
	    if( sel == 0 )
	    {
		//Auto:
		if( profile_get_str_value( key, 0, 0 ) != 0 )
		{
		    profile_remove_key( key, 0 );
            	    wm->prefs_restart_request = true;
        	    profile_save( 0 );
		}
	    }
	    else
	    {
		if( bmem_strcmp( profile_get_str_value( key, "", 0 ), names[ sel - 1 ] ) )
		{
		    profile_set_str_value( key, names[ sel - 1 ], 0 );
		    wm->prefs_restart_request = true;
            	    profile_save( 0 );
		}
	    }
	}

	for( int d = 0; d < devices; d++ )
	{
	    bmem_free( names[ d ] );
	    bmem_free( infos[ d ] );
	}
	bmem_free( names );
	bmem_free( infos );
    }
    
    bmem_free( menu );    

    prefs_audio_reinit( data->win );
    draw_window( data->win, wm );

    return 0;
}

static const int g_sound_buf_size_table[] = 
{
    128,
    256, 
    512,
    768,
    1024,
    1280,
    1536,
    1792,
    2048,
    2560,
    3072,
    4096,
    -1
};

int prefs_audio_buf_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_audio_data* data = (prefs_audio_data*)user_data;
    
    size_t menu_size = 8192;
    utf8_char* menu = (utf8_char*)bmem_new( menu_size );
    menu[ 0 ] = 0;
    bmem_strcat( menu, menu_size, wm_get_string( STR_WM_AUTO ) );
    
    int i = 0;
    int size = 0;

#if defined(OSX)
#else
    int freq = 44100;
    if( g_snd_initialized )
        freq = g_snd.freq;
    utf8_char ts[ 512 ];
    while( 1 )
    {
	size = g_sound_buf_size_table[ i ];
	if( size == -1 ) break;
	sprintf( ts, "\n%d; %d %s", size, ( size * 1000 ) / freq, wm_get_string( STR_WM_MS ) ); 
	bmem_strcat( menu, menu_size, ts );
	i++;
    }
#endif
    
    size = 0;
    int v = popup_menu( wm_get_string( STR_WM_BUFFER_SIZE ), menu, win->screen_x, win->screen_y, wm->menu_color, wm );
    if( v > 0 && v < i + 1 )
	size = g_sound_buf_size_table[ v - 1 ];
    
    bmem_free( menu );
    
    if( v == 0 || size > 0 )
    {
	if( v == 0 )
	{
	    if( profile_get_int_value( KEY_SOUNDBUFFER, -1, 0 ) != -1 )
	    {
		profile_remove_key( KEY_SOUNDBUFFER, 0 );
		wm->prefs_restart_request = true;
		profile_save( 0 );
	    }
	}
	if( size > 0 )
	{
	    if( profile_get_int_value( KEY_SOUNDBUFFER, -1, 0 ) != size )
	    {
		profile_set_int_value( KEY_SOUNDBUFFER, size, 0 );
		wm->prefs_restart_request = true;
		profile_save( 0 );
	    }
	}
    }

    prefs_audio_reinit( data->win );
    draw_window( data->win, wm );
    
    return 0;
}

int prefs_audio_freq_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_audio_data* data = (prefs_audio_data*)user_data;
    
    int freq = 0;
    int v = win->action_result;
    switch( v )
    {
	case 1: freq = 44100; break;
	case 2: freq = 48000; break;
	case 3: freq = 96000; break;
	case 4: freq = 192000; break;
    }
    
    if( v == 0 || freq > 0 )
    {
	if( v == 0 )
	{
	    if( profile_get_int_value( KEY_FREQ, -1, 0 ) != -1 )
	    {
		profile_remove_key( KEY_FREQ, 0 );
		wm->prefs_restart_request = true;
		profile_save( 0 );
	    }
	}
	if( freq > 0 )
	{
	    if( profile_get_int_value( KEY_FREQ, -1, 0 ) != freq )
	    {
		profile_set_int_value( KEY_FREQ, freq, 0 );
		wm->prefs_restart_request = true;
		profile_save( 0 );
	    }
	}
    }

    prefs_audio_reinit( data->win );
    draw_window( data->win, wm );
    
    return 0;
}

int prefs_audio_more_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_audio_data* data = (prefs_audio_data*)user_data;

    utf8_char* drv = profile_get_str_value( KEY_AUDIODRIVER, 0, 0 );
    if( bmem_strstr( drv, "asio" ) )
    {
	dialog_item di[ 5 ];
	bmem_set( &di, sizeof( di ), 0 );
        di[ 0 ].type = DIALOG_ITEM_LABEL;
        di[ 0 ].str_val = (utf8_char*)wm_get_string( STR_WM_FIRST_OUT_CH );
        di[ 1 ].type = DIALOG_ITEM_NUMBER;
        di[ 1 ].min = 0;
        di[ 1 ].max = 64;
        di[ 1 ].int_val = profile_get_int_value( "audio_ch", 0, 0 );
        di[ 2 ].type = DIALOG_ITEM_LABEL;
        di[ 2 ].str_val = (utf8_char*)wm_get_string( STR_WM_FIRST_IN_CH );
        di[ 3 ].type = DIALOG_ITEM_NUMBER;
        di[ 3 ].min = 0;
        di[ 3 ].max = 64;
        di[ 3 ].int_val = profile_get_int_value( "audio_ch_in", 0, 0 );
        di[ 4 ].type = DIALOG_ITEM_NONE;
        wm->opt_dialog_items = di;
        int d = dialog( wm_get_string( STR_WM_ASIO_OPTIONS ), wm_get_string( STR_WM_OKCANCEL ), wm );
        if( d == 0 )
        {
    	    bool changed = 0;
    	    if( profile_get_int_value( "audio_ch", 0, 0 ) != di[ 1 ].int_val )
    	    {
    		profile_set_int_value( "audio_ch", di[ 1 ].int_val, 0 );
    		changed = 1;
    	    }
    	    if( profile_get_int_value( "audio_ch_in", 0, 0 ) != di[ 3 ].int_val )
    	    {
    		profile_set_int_value( "audio_ch_in", di[ 3 ].int_val, 0 );
    		changed = 1;
    	    }
    	    if( changed )
    	    {
    		wm->prefs_restart_request = true;
        	profile_save( 0 );
    	    }
        }
    }
    
    return 0;
}   

int prefs_audio_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    prefs_audio_data* data = (prefs_audio_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( prefs_audio_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		data->win = win;
		
		utf8_char ts[ 512 ];
		int y = 0;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->driver = new_window( "Driver", 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->driver, prefs_audio_driver_handler, data, wm );
		set_window_controller( data->driver, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->driver, 2, wm, CPERC, (WCMD)100, CEND );
		y += wm->text_ysize + wm->interelement_space;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->device = new_window( "Output device", 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->device, prefs_audio_device_handler, data, wm );
		set_window_controller( data->device, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->device, 2, wm, CPERC, (WCMD)100, CEND );
		y += wm->text_ysize + wm->interelement_space;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->device_in = new_window( "Input device", 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->device_in, prefs_audio_device_handler, data, wm );
		set_window_controller( data->device_in, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->device_in, 2, wm, CPERC, (WCMD)100, CEND );
		y += wm->text_ysize + wm->interelement_space;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->buf = new_window( "Audio buffer size", 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->buf, prefs_audio_buf_handler, data, wm );
		set_window_controller( data->buf, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->buf, 2, wm, CPERC, (WCMD)100, CEND );
		y += wm->text_ysize + wm->interelement_space;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->freq = new_window( wm_get_string( STR_WM_FREQ ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->freq, prefs_audio_freq_handler, data, wm );
#if defined(ONLY44100)
		sprintf( ts, "%s", wm_get_string( STR_WM_AUTO ) );
#else
		sprintf( ts, "%s\n44100\n48000\n96000\n192000", wm_get_string( STR_WM_AUTO ) );
#endif
		button_set_menu( data->freq, ts, wm );
		set_window_controller( data->freq, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->freq, 2, wm, CPERC, (WCMD)100, CEND );
		y += wm->text_ysize + wm->interelement_space;

		wm->opt_button_flags = BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW;
		data->more = new_window( wm_get_string( STR_WM_MORE_OPTIONS ), 0, y, 1, wm->text_ysize, wm->button_color, win, button_handler, wm );
		set_handler( data->more, prefs_audio_more_handler, data, wm );
		set_window_controller( data->more, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->more, 2, wm, CPERC, (WCMD)100, CEND );
		data->more->flags |= WIN_FLAG_ALWAYS_INVISIBLE;
		y += wm->text_ysize + wm->interelement_space;

		prefs_audio_reinit( win );
		
		wm->prefs_section_ysize = y;
	    }
	    retval = 1;
	    break;
	case EVT_DRAW:
	    {
		wbd_lock( win );
		wm->cur_font_color = wm->color2;
		draw_frect( 0, 0, win->xsize, win->ysize, win->color, wm );
                if( g_snd_initialized )
                {
                    utf8_char ts[ 512 ];
                    int y = data->freq->y + data->freq->ysize + wm->interelement_space;
                    if( data->more->visible )
                        y = data->more->y + data->more->ysize + wm->interelement_space;
                    ts[ 0 ] = 0;
                    bmem_strcat( ts, sizeof( ts ), wm_get_string( STR_WM_CUR_DRIVER ) );
                    bmem_strcat( ts, sizeof( ts ), ": " );
                    bmem_strcat( ts, sizeof( ts ), sound_stream_get_driver_info() );
                    draw_string( ts, 0, y, wm );
                    sprintf( ts, "%s: %d %s", wm_get_string( STR_WM_CUR_FREQ ), g_snd.freq, wm_get_string( STR_WM_HZ ) );
                    draw_string( ts, 0, y + char_y_size( wm ), wm );
                    sprintf( ts, "%s: %d; %d %s", wm_get_string( STR_WM_CUR_LATENCY ), g_snd.out_latency, ( g_snd.out_latency * 1000 ) / g_snd.freq, wm_get_string( STR_WM_MS ) );
                    draw_string( ts, 0, y + char_y_size( wm ) * 2, wm );
                }
                wbd_draw( wm );
                wbd_unlock( wm );
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    retval = 1;
	    break;
    }
    return retval;
}
