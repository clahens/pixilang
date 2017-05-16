/*
    handlers.cpp. Standart window handlers
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

#include "core/core.h"
#include "dsp.h"

#ifdef IPHONE
    #include <string.h>
    #include "various/iphone/webserver.h"
#endif

#ifdef UNIX
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#include "handlers_dialog.h"
#include "handlers_files.h"
#include "handlers_list.h"
#include "handlers_popup.h"
#include "handlers_virtual_keyboard.h"

#define SIDEBTN 235 //Width of the side button. 1.0 = 256

int null_handler( sundog_event* evt, window_manager* wm )
{
    return 0;
}

int desktop_handler( sundog_event* evt, window_manager* wm )
{
    int retval = 0;
    WINDOWPTR win = evt->win;
    switch( evt->type )
    {
	case EVT_SCREENRESIZE:
	    win->x = 0;
	    win->y = 0;
	    win->xsize = wm->screen_xsize;
	    win->ysize = wm->screen_ysize;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	case EVT_MOUSEBUTTONUP:
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
    }
    return retval;
}

#define DIVIDER_BINDS 2

struct divider_data
{
    int start_x;
    int start_y;
    int start_wx;
    int start_wy;
    int start_vscroll;
    int pushed;
    COLOR color;
    int vscroll_min;
    int vscroll_max;
    int vscroll_cur;
    WINDOWPTR binds[ DIVIDER_BINDS ];
    int binds_x[ DIVIDER_BINDS ];
    int binds_y[ DIVIDER_BINDS ];
    WINDOWPTR push_win;
    char push_right;
    char push_bottom;
    
    bool vert;
    bool time;
    
    WINDOWPTR prev_focus_win;
};

void divider_push_start( divider_data *data, WINDOWPTR win )
{
    if( data->push_win )
    {
        if( data->push_win->x >= win->x + win->xsize )
    	    data->push_right = 1;
	else
	    data->push_right = 0;
	if( data->push_win->y >= win->y + win->ysize )
	    data->push_bottom = 1;
	else
	    data->push_bottom = 0;
    }
}

void divider_push( divider_data* data, WINDOWPTR win )
{
    if( data->push_win )
    {
        if( data->push_bottom )
        {
    	    if( win->y + win->ysize > data->push_win->y )
	    {
	        //Push down:
	        data->push_win->y = win->y + win->ysize;
	    }
	}
	else
	{
	    if( win->y < data->push_win->y + data->push_win->ysize )
	    {
		//Push up:
		data->push_win->y = win->y - data->push_win->ysize;
	    }
	}
    }
}

int divider_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    divider_data* data = (divider_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( divider_data );
	    break;
	case EVT_AFTERCREATE:
	    data->pushed = 0;
	    data->color = win->color;
	    data->vscroll_min = wm->opt_divider_vscroll_min;
	    data->vscroll_max = wm->opt_divider_vscroll_max;
	    data->vert = wm->opt_divider_vertical;
	    data->time = wm->opt_divider_with_time;
	    data->vscroll_cur = 0;
	    {
		for( int a = 0; a < DIVIDER_BINDS;a++ )
		    data->binds[ a ] = 0;
	    }
	    data->push_win = 0;
	    data->prev_focus_win = 0;
	    wm->opt_divider_vscroll_min = 0;
	    wm->opt_divider_vscroll_max = 0;
	    wm->opt_divider_vertical = false;
	    wm->opt_divider_with_time = false;
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	    if( evt->key == MOUSE_BUTTON_LEFT )
	    {
		data->start_x = evt->x;
		data->start_y = evt->y;
		data->start_wx = win->x;
		data->start_wy = win->y;
		data->start_vscroll = data->vscroll_cur;
		for( int a = 0; a < DIVIDER_BINDS; a++ )
		{
		    if( data->binds[ a ] )
		    {
			data->binds_x[ a ] = data->binds[ a ]->x;
			data->binds_y[ a ] = data->binds[ a ]->y;
			divider_push_start( (divider_data*)data->binds[ a ]->data, data->binds[ a ] );
		    }
		}
		divider_push_start( data, win );
		data->pushed = 1;
		win->color = PUSHED_COLOR( data->color );
		draw_window( win, wm );
		retval = 1;
	    }
	    break;
	case EVT_MOUSEMOVE:
	    if( evt->key == MOUSE_BUTTON_LEFT )
	    {
		int dx = evt->x - data->start_x;
		int dy = evt->y - data->start_y;

		win->x = data->start_wx + dx;
		win->y = data->start_wy + dy;
		for( int a = 0; a < DIVIDER_BINDS; a++ )
		{
		    if( data->binds[ a ] )
		    {
			data->binds[ a ]->x = data->binds_x[ a ] + dx;
			data->binds[ a ]->y = data->binds_y[ a ] + dy;
			divider_push( (divider_data*)data->binds[ a ]->data, data->binds[ a ] );
		    }
		}
		divider_push( data, win );

		if( data->vscroll_min != data->vscroll_max )
		{
		    data->vscroll_cur = data->start_vscroll + dy;
		    if( data->vscroll_cur < data->vscroll_min )
			data->vscroll_cur = data->vscroll_min;
		    if( data->vscroll_cur > data->vscroll_max )
			data->vscroll_cur = data->vscroll_max;
		    if( win->action_handler )
		    {
			win->action_handler( win->handler_data, win, wm );
		    }
		}
		
		recalc_regions( wm );
		draw_window( wm->root_win, wm );
		
		retval = 1;
	    }
	    break;
	case EVT_MOUSEBUTTONUP:
	    if( evt->key == MOUSE_BUTTON_LEFT && data->pushed )
	    {
		data->pushed = 0;
		set_focus_win( data->prev_focus_win, wm );
		win->color = data->color;
		draw_window( win, wm );
		retval = 1;
	    }
	    break;
	case EVT_FOCUS:
	    data->prev_focus_win = wm->prev_focus_win;
	    retval = 1;
	    break;
	case EVT_UNFOCUS:
	    if( data->pushed )
	    {
		data->pushed = 0;
		win->color = data->color;
		draw_window( win, wm );
		retval = 1;
	    }
	    break;
	case EVT_DRAW:
	    wbd_lock( win );
	    {
		int cx = win->xsize / 2;
		int cy = win->ysize / 2;
		COLOR color = win->color;
		draw_frect( 0, 0, win->xsize, win->ysize, color, wm );
		if( data->time )
		{
		    int h = time_hours();
		    int m = time_minutes();
		    utf8_char s[ 6 ];
		    s[ 0 ] = h / 10 + '0';
		    s[ 1 ] = h % 10 + '0';
		    s[ 2 ] = ':';
		    s[ 3 ] = m / 10 + '0';
		    s[ 4 ] = m % 10 + '0';
		    s[ 5 ] = 0;

		    int time_xsize = string_x_size( s, wm ) + 2;
		    draw_frect( time_xsize, 0, win->xsize, win->ysize, color, wm );
		    draw_frect( 0, 0, 2, win->ysize, color, wm );
		    
		    int y = ( win->ysize - char_y_size( wm ) ) / 2;
		    wm->cur_font_color = blend( wm->color2, color, 130 );
		    draw_string( s, 2, y, wm );
		    draw_frect( 0, 0, time_xsize, y, color, wm );
		    draw_frect( 0, y + char_y_size( wm ), time_xsize, win->ysize, color, wm );
		}
		if( 1 )
		{
		    draw_frect( cx - 2, cy - 2, 2, 2, BORDER_COLOR( color ), wm );
		    draw_frect( cx + 1, cy - 2, 2, 2, BORDER_COLOR( color ), wm );
		    draw_frect( cx - 2, cy + 1, 2, 2, BORDER_COLOR( color ), wm );
		    draw_frect( cx + 1, cy + 1, 2, 2, BORDER_COLOR( color ), wm );
		}
		int xx = 0;
		int yy = 0;
		if( data->vert ) yy = 1; else xx = 1;
		draw_rect( -xx, -yy, win->xsize + xx * 2 - 1, win->ysize + yy * 2 - 1, BORDER_COLOR( color ), wm );
	    }
	    wbd_draw( wm );
	    wbd_unlock( wm );
	    retval = 1;
	    break;
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
    }
    return retval;
}

void bind_divider_to( WINDOWPTR win, WINDOWPTR bind_to, int bind_num, window_manager* wm )
{
    divider_data* data = (divider_data*)win->data;
    data->binds[ bind_num ] = bind_to;
}

void set_divider_push( WINDOWPTR win, WINDOWPTR push_win, window_manager* wm )
{
    divider_data* data = (divider_data*)win->data;
    data->push_win = push_win;
}

void set_divider_vscroll_parameters( WINDOWPTR win, int min, int max, window_manager* wm )
{
    divider_data* data = (divider_data*)win->data;
    data->vscroll_min = min;
    data->vscroll_max = max;
}

int get_divider_vscroll_value( WINDOWPTR win, window_manager* wm )
{
    divider_data* data = (divider_data*)win->data;
    return data->vscroll_cur;
}

struct label_data 
{
    WINDOWPTR prev_focus_win;
};

int label_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    label_data* data = (label_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( label_data );
	    break;
	case EVT_AFTERCREATE:
	    data->prev_focus_win = 0;
	    break;
	case EVT_FOCUS:
	    data->prev_focus_win = wm->prev_focus_win;
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONUP:
	    set_focus_win( data->prev_focus_win, wm );
	    retval = 1;
	    break;
	case EVT_DRAW:
	    wbd_lock( win );
	    draw_frect( 0, 0, win->xsize, win->ysize, win->parent->color, wm );
	    //Draw name:
	    {
		int ty = ( win->ysize - char_y_size( wm ) ) / 2;
		wm->cur_font_color = blend( wm->color2, win->parent->color, LABEL_OPACITY );
		draw_string( win->name, 0, ty, wm );
	    }
	    wbd_draw( wm );
	    wbd_unlock( wm );
	    retval = 1;
	    break;
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
    }
    return retval;
}

struct text_data 
{
    WINDOWPTR win;
    WINDOWPTR clear_button;
    utf32_char* text;
    utf8_char* output_str;
    int cur_pos;
    int active;
    int call_handler_on_any_changes;
    int no_virtual_kbd;
    int zoom;
    int numeric;
    int min;
    int max;
    int step;
    bool hide_zero;
    bool ro;
    bool editing; //in focus and some data changed
    WINDOWPTR prev_focus_win;
    utf8_char show_char;
};

int text_dec_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    text_data* data = (text_data*)user_data;
    int val = text_get_value( data->win, wm );
    val -= data->step;
    text_set_value( data->win, val, wm );
    draw_window( data->win, wm );
    if( data->win->action_handler )
        data->win->action_handler( data->win->handler_data, data->win, wm );
    return 0;
}

int text_inc_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    text_data* data = (text_data*)user_data;
    int val = text_get_value( data->win, wm );
    val += data->step;
    text_set_value( data->win, val, wm );
    draw_window( data->win, wm );
    if( data->win->action_handler )
        data->win->action_handler( data->win->handler_data, data->win, wm );
    return 0;
}

int text_clear_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    text_data* data = (text_data*)user_data;
    text_set_text( data->win, "", wm );
    draw_window( data->win, wm );
    if( data->win->action_handler && data->call_handler_on_any_changes )
        data->win->action_handler( data->win->handler_data, data->win, wm );
    return 0;
}

void text_show_clear_button( text_data* data )
{
    window_manager* wm = data->win->wm;
    if( data->ro == false && data->clear_button && data->active )
    {
	if( data->clear_button->flags & WIN_FLAG_ALWAYS_INVISIBLE )
        {
            data->clear_button->flags &= ~WIN_FLAG_ALWAYS_INVISIBLE;
            show_window( data->clear_button, wm );
            recalc_regions( wm );
        }
    }
}

void text_hide_clear_button( text_data* data )
{
    window_manager* wm = data->win->wm;
    if( data->ro == false && data->clear_button )
    {
	if( !( data->clear_button->flags & WIN_FLAG_ALWAYS_INVISIBLE ) )
        {
            data->clear_button->flags |= WIN_FLAG_ALWAYS_INVISIBLE;
            hide_window( data->clear_button, wm );
            recalc_regions( wm );
        }
    }
}

int text_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    text_data* data = (text_data*)win->data;
    int xp;
    int rx = evt->x - win->screen_x;
    int changes = 0;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( text_data );
	    break;
	case EVT_AFTERCREATE:
	    data->win = win;
	    data->output_str = 0;
	    data->text = (utf32_char*)bmem_new( 32 * sizeof( utf32_char ) );
	    bmem_zero( data->text );
	    data->cur_pos = 0;
	    data->active = 0;
	    data->call_handler_on_any_changes = wm->opt_text_call_handler_on_any_changes;
	    data->no_virtual_kbd = wm->opt_text_no_virtual_keyboard;
	    data->zoom = 256;
	    data->numeric = wm->opt_text_numeric;
	    data->min = wm->opt_text_num_min;
	    data->max = wm->opt_text_num_max;
	    data->hide_zero = wm->opt_text_num_hide_zero;
	    data->ro = wm->opt_text_ro;
	    data->step = 1;
	    data->editing = false;
	    data->show_char = 0;
	    data->clear_button = 0;	    
	    if( data->numeric )
	    {
		wm->opt_button_flat = 1;
		wm->opt_button_autorepeat = true;
		WINDOWPTR b = new_window( "-", 1, 1, 1, 1, win->color, win, button_handler, wm );
		set_window_controller( b, 0, wm, CWIN, (WCMD)win, CYSIZE, CMULDIV256, (WCMD)SIDEBTN, CMAXVAL, (WCMD)wm->scrollbar_size, CPUTR0, CPERC, (WCMD)100, CSUB, CR0, CSUB, CR0, CEND, CEND );
		set_window_controller( b, 1, wm, (WCMD)0, CEND );
		set_window_controller( b, 2, wm, CWIN, (WCMD)win, CYSIZE, CMULDIV256, (WCMD)SIDEBTN, CMAXVAL, (WCMD)wm->scrollbar_size, CPUTR0, CPERC, (WCMD)100, CSUB, CR0, CEND );
	        set_window_controller( b, 3, wm, CPERC, (WCMD)100, CEND );
		set_handler( b, text_dec_handler, data, wm );
		wm->opt_button_flat = 1;
		wm->opt_button_autorepeat = true;
		b = new_window( "+", 1, 1, 1, 1, win->color, win, button_handler, wm );
		set_window_controller( b, 0, wm, CWIN, (WCMD)win, CYSIZE, CMULDIV256, (WCMD)SIDEBTN, CMAXVAL, (WCMD)wm->scrollbar_size, CPUTR0, CPERC, (WCMD)100, CSUB, CR0, CEND );
		set_window_controller( b, 1, wm, (WCMD)0, CEND );
		set_window_controller( b, 2, wm, CPERC, (WCMD)100, CEND );
	        set_window_controller( b, 3, wm, CPERC, (WCMD)100, CEND );
		set_handler( b, text_inc_handler, data, wm );
	    }
	    else
	    {
		wm->opt_button_flat = 1;
		data->clear_button = new_window( "x", 1, 1, 1, 1, win->color, win, button_handler, wm );
		data->clear_button->flags |= WIN_FLAG_ALWAYS_INVISIBLE;
		set_window_controller( data->clear_button, 0, wm, CWIN, (WCMD)win, CYSIZE, CMULDIV256, (WCMD)SIDEBTN, CMAXVAL, (WCMD)wm->scrollbar_size, CPUTR0, CPERC, (WCMD)100, CSUB, CR0, CEND );
		set_window_controller( data->clear_button, 1, wm, (WCMD)0, CEND );
		set_window_controller( data->clear_button, 2, wm, CPERC, (WCMD)100, CEND );
	        set_window_controller( data->clear_button, 3, wm, CPERC, (WCMD)100, CEND );
		set_handler( data->clear_button, text_clear_handler, data, wm );
	    }
	    data->prev_focus_win = 0;
	    wm->opt_text_no_virtual_keyboard = false;
	    wm->opt_text_call_handler_on_any_changes = false;
	    wm->opt_text_ro = false;
	    wm->opt_text_numeric = 0;
	    wm->opt_text_num_min = 0;
	    wm->opt_text_num_max = 0;
	    wm->opt_text_num_hide_zero = false;
	    retval = 1;
	    break;
	case EVT_FOCUS:
	    if( data->ro )
	    {
		retval = 1;
		break;
	    }
	    data->active = 1;
	    retval = 1;
	    break;
	case EVT_UNFOCUS:
	    if( data->ro )
	    {
		retval = 1;
		break;
	    }
	    if( data->active )
	    {
		data->active = 0;
		data->editing = false;
		draw_window( win, wm );
		retval = 1;
	    }
	    break;
	case EVT_MOUSEMOVE:
	case EVT_MOUSEBUTTONDOWN:
	    if( data->ro )
	    {
		retval = 1;
		break;
	    }
	    if( evt->key == MOUSE_BUTTON_LEFT )
	    {
		wbd_lock( win );
		wm->cur_font_scale = data->zoom;
		data->prev_focus_win = wm->prev_focus_win;
		data->active = 1;
		xp = 0;
		for( size_t p = 0; p < bmem_get_size( data->text ) / sizeof( utf32_char ); p++ )
		{
		    utf32_char c = data->text[ p ];
		    int charx = char_x_size( c, wm );
		    if( c == 0 ) charx = 600;
		    if( rx >= xp && rx < xp + charx )
		    {
		        data->cur_pos = p;
		        break;
		    }
		    xp += charx;
		    if( data->text[ p ] == 0 ) break;
		}
		wm->cur_font_scale = 256;
		wbd_unlock( wm );
		draw_window( win, wm );
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONUP:
	    if( data->ro )
	    {
		retval = 1;
		break;
	    }
	    if( evt->key == MOUSE_BUTTON_LEFT )
	    {
		if( data->active ) 
		{
		    if( data->no_virtual_kbd == 0 )
			show_keyboard_for_text_window( win, wm );
		}
	    }
	    retval = 1;
	    break;
	case EVT_BUTTONDOWN:
	    {
		if( data->active == 0 ) 
		{
	    	    retval = 0;
		    break;
		}
		if( data->ro )
		{
		    retval = 0;
		    break;
		}
		if( evt->flags & EVT_FLAG_CTRL )
		{
		    retval = 0;
		    break;
    		}
	    
		WINDOWPTR w2 = 0; //switch to...
		if( evt->key == KEY_TAB )
		{
		    WINDOWPTR parent = win->parent;
		    if( parent && parent->childs && parent->childs_num )
		    {
			if( evt->flags & EVT_FLAG_SHIFT )
			{
			    //Back:
			    for( int i = 0; i < parent->childs_num; i++ )
			    {
				WINDOWPTR w = parent->childs[ i ];
				if( w == 0 ) continue;
				if( w->win_handler != win->win_handler ) continue;
				if( text_is_readonly( w ) ) continue;
				int dx = w->x - win->x;
				int dy = w->y - win->y;
				if( dy <= 0 )
				{
				    if( !( dy == 0 && dx >= 0 ) )
				    {
				        if( w2 == 0 )
				        {
				    	    w2 = w;
					}
					else
					{
					    if( w->y > w2->y || ( w->y == w2->y && w->x > w2->x ) )
					    {
					        w2 = w;
					    }
					}
				    }
				}
			    }
			}
			else
			{
		    	    //Forward:
			    for( int i = 0; i < parent->childs_num; i++ )
			    {
				WINDOWPTR w = parent->childs[ i ];
				if( w == 0 ) continue;
				if( w->win_handler != win->win_handler ) continue;
				if( text_is_readonly( w ) ) continue;
			    	int dx = w->x - win->x;
				int dy = w->y - win->y;
				if( dy >= 0 )
			    	{
				    if( !( dy == 0 && dx <= 0 ) )
				    {
				        if( w2 == 0 )
				        {
				    	    w2 = w;
					}
					else
					{
					    if( w->y < w2->y || ( w->y == w2->y && w->x < w2->x ) )
					    {
					        w2 = w;
					    }
					}
				    }
				}
			    }
			}
		    }
		    if( w2 == win ) w2 = 0;
		    if( w2 == 0 )
		    {
			retval = 1;
			break;
		    }
		}
	    
		if( evt->key == KEY_ENTER || evt->key == KEY_ESCAPE || w2 )
		{
		    data->active = 0;
		    data->editing = false;
		    set_focus_win( data->prev_focus_win, wm );
		    draw_window( win, wm );
		    if( ( evt->key == KEY_ENTER || w2 ) && win->action_handler )
			win->action_handler( win->handler_data, win, wm );
		    if( w2 )
		    {
			set_focus_win( w2, wm );
			draw_window( w2, wm );
		    }
		    retval = 1;
		    break;
		}
	    
	        utf32_char c = 0;
		if( evt->key >= 0x20 && evt->key <= 0x7E ) c = evt->key;
		if( evt->flags & EVT_FLAG_SHIFT )
		{
		    if( evt->key >= 'a' && evt->key <= 'z' ) 
		    {
			c = evt->key - 0x20;
		    }
		    else
		    {
			switch( evt->key )
			{
			    case '0': c = ')'; break;
			    case '1': c = '!'; break;
			    case '2': c = '@'; break;
			    case '3': c = '#'; break;
			    case '4': c = '$'; break;
			    case '5': c = '%'; break;
			    case '6': c = '^'; break;
			    case '7': c = '&'; break;
			    case '8': c = '*'; break;
			    case '9': c = '('; break;
			    case '[': c = '{'; break;
			    case ']': c = '}'; break;
			    case ';': c = ':'; break;
			    case  39: c = '"'; break;
			    case ',': c = '<'; break;
			    case '.': c = '>'; break;
			    case '/': c = '?'; break;
			    case '-': c = '_'; break;
			    case '=': c = '+'; break;
			    case  92: c = '|'; break;
			    case '`': c = '~'; break;
			}
		    }
		}
	    
	        utf32_char* text_buf = data->text;
	        size_t text_size = bmem_get_size( text_buf ) / sizeof( utf32_char );
	    
		if( c == 0 )
	        {
		    if( evt->key == KEY_BACKSPACE )
		    {
			if( data->cur_pos >= 1 )
			{
			    data->cur_pos--;
			    for( size_t i = data->cur_pos; i < text_size - 1; i++ ) 
				text_buf[ i ] = text_buf[ i + 1 ];
			}
			changes = 1;
		    }
		    else
		    if( evt->key == KEY_DELETE )
		    {
			if( data->text[ data->cur_pos ] != 0 )
			    for( size_t i = data->cur_pos; i < text_size - 1; i++ ) 
			        text_buf[ i ] = text_buf[ i + 1 ];
			changes = 1;
		    }
		    else
		    if( evt->key == KEY_LEFT )
		    {
			if( data->cur_pos >= 1 )
			{
			    data->cur_pos--;
			}
		    }
		    else
		    if( evt->key == KEY_RIGHT )
		    {
			if( text_buf[ data->cur_pos ] != 0 )
			    data->cur_pos++;
		    }
		    else
		    if( evt->key == KEY_END )
		    {
			size_t i = 0;
			for( i = 0; i < text_size; i++ ) 
			    if( data->text[ i ] == 0 ) break;
			data->cur_pos = i;
		    }
		    else
		    if( evt->key == KEY_HOME )
		    {
			data->cur_pos = 0;
		    }
		    else 
		    {
			retval = 0;
			break;
		    }
		}
		else
		{
		    //Add new char:
		    if( bmem_strlen_utf32( (const utf32_char*)data->text ) + 16 > text_size )
		    {
			data->text = (utf32_char*)bmem_resize( data->text, ( text_size + 16 ) * sizeof( utf32_char ) );
			bmem_set( &data->text[ text_size ], 16 * sizeof( utf32_char ), 0 );
			text_size += 16;
			text_buf = data->text;
		    }
		    for( size_t i = text_size - 1; i >= (unsigned)data->cur_pos + 1; i-- )
			text_buf[ i ] = text_buf[ i - 1 ];
		    text_buf[ data->cur_pos ] = c;
		    data->cur_pos++;
		    changes = 1;
		}

		draw_window( win, wm );
	    
		if( changes ) data->editing = true;
	    
		if( data->call_handler_on_any_changes && changes && win->action_handler )
		    win->action_handler( win->handler_data, win, wm );
	    }
	    retval = 1;
	    break;
	case EVT_DRAW:
	    if( data->text && data->text[ 0 ] != 0 )
	    {
		text_show_clear_button( data );
	    }
	    else
	    {
    		text_hide_clear_button( data );
	    }
	    wbd_lock( win );
	    wm->cur_font_scale = data->zoom;
	    //Fill window:
	    wm->cur_opacity = 255;
	    wm->cur_font_color = wm->color3;
	    draw_frect( 0, 0, win->xsize, win->ysize, win->color, wm );
	    //Draw text and cursor:
	    xp = 0;
	    if( data->ro )
	    {
		int txsize = 0;
		for( size_t p = 0; p < bmem_get_size( data->text ) / sizeof( utf32_char ); p++ )
		{
		    utf32_char c = data->text[ p ];
		    if( c == 0 ) break;
		    txsize += char_x_size( c, wm );
		}
		if( txsize + xp >= ( win->xsize - 1 ) )
		{
		    xp -= txsize + xp - ( win->xsize - 1 );
		}
	    }
	    {
		size_t p = 0;
		int ty = ( win->ysize - char_y_size( wm ) ) / 2;
		for( p = 0; p < bmem_get_size( data->text ) / sizeof( utf32_char ); p++ )
		{
		    utf32_char c = data->text[ p ];
		    if( c == 0 ) c = ' ';
		    int charx = char_x_size( c, wm );
		    if( data->cur_pos == (int)p && data->active )
		    {
			draw_frect( xp, 0, charx, win->ysize, blend( wm->color3, win->color, 128 ), wm );
		    }
		    draw_char( c, xp, ty, wm );
		    xp += charx;
		    if( data->text[ p ] == 0 ) break;
		}
		if( p == 0 && !data->active )
		{
		    draw_string( "...", 0, ty, wm );
		}
	    }
	    if( data->show_char )
	    {
		//Show some symbol (defined in user request):
		wm->cur_opacity = 160;
		draw_frect( 0, 0, win->xsize, win->ysize, wm->color0, wm );
		wm->cur_opacity = 255;
		utf8_char sstr[ 8 ];
		sstr[ 0 ] = data->show_char;
		sstr[ 1 ] = 0;
		int len = string_x_size( (const utf8_char*)sstr, wm );
		wm->cur_font_color = wm->color3;
		draw_string( 
		    sstr, 
		    ( win->xsize - len ) / 2, 
		    ( win->ysize - char_y_size( wm ) ) / 2,
		    wm );
		data->show_char = 0;
	    }
	    wm->cur_font_scale = 256;
	    wbd_draw( wm );
	    wbd_unlock( wm );
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    hide_keyboard_for_text_window( wm );
	    if( data->output_str ) bmem_free( data->output_str );
	    if( data->text ) bmem_free( data->text );
	    retval = 1;
	    break;
	case EVT_USERDEFINED1:
	    {
		data->show_char = (utf8_char)evt->key;
		draw_window( win, wm );
	    }
	    retval = 1;
	    break;
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
    }
    return retval;
}

void text_set_text( WINDOWPTR win, const utf8_char* text, window_manager* wm )
{
    if( win && text )
    {
	if( bmem_strcmp( text, text_get_text( win, wm ) ) == 0 ) return;
	size_t len = bmem_strlen( text ) + 1;
	utf32_char* ts = (utf32_char*)bmem_new( len * sizeof( utf32_char ) );
	utf8_to_utf32( ts, len, text );
	size_t len2 = bmem_strlen_utf32( ts ) + 1;
	text_data* data = (text_data*)win->data;
	if( bmem_get_size( data->text ) / sizeof( utf32_char ) < len2 )
	{
	    //Resize text buffer:
	    data->text = (utf32_char*)bmem_resize( data->text, len2 * sizeof( utf32_char ) );
	}
	bmem_copy( data->text, ts, len2 * sizeof( utf32_char ) );
	if( data->cur_pos > (int)len2 - 1 )
	    data->cur_pos = (int)len2 - 1;
	text_hide_clear_button( data );
	draw_window( win, wm );
	bmem_free( ts );
    }
}

void text_set_cursor_position( WINDOWPTR win, int cur_pos, window_manager* wm )
{
    if( win )
    {
	text_data* data = (text_data*)win->data;
	data->cur_pos = cur_pos;
	draw_window( win, wm );
    }
}

void text_set_value( WINDOWPTR win, int val, window_manager* wm )
{
    if( win )
    {
	text_data* data = (text_data*)win->data;
	if( val < data->min ) val = data->min;
	if( val > data->max ) val = data->max;
	utf8_char ts[ 16 ];
	if( data->numeric == 1 )
	    int_to_string( val, ts );
	else
	    hex_int_to_string( val, ts );
	if( data->hide_zero && val == 0 )
	{
	    ts[ 0 ] = 0;
	}
	text_set_text( win, (const utf8_char*)ts, wm );
    }
}

utf8_char* text_get_text( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
	text_data* data = (text_data*)win->data;
	size_t size = bmem_get_size( data->text ) * 2;
	if( data->output_str ) 
	{
	    if( bmem_get_size( data->output_str ) < size )
	    {
		data->output_str = (utf8_char*)bmem_resize( data->output_str, size );
	    }
	}
	else
	{
	    data->output_str = (utf8_char*)bmem_new( size );
	}
	utf32_to_utf8( data->output_str, size, data->text );
	return data->output_str;
    }
    return 0;
}

int text_get_cursor_position( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
	text_data* data = (text_data*)win->data;
	return data->cur_pos;
    }
    return 0;
}

int text_get_value( WINDOWPTR win, window_manager* wm )
{
    int val = 0;
    if( win )
    {
	text_data* data = (text_data*)win->data;
	utf8_char* s = text_get_text( win, wm );
	if( data->numeric == 1 )
	    val = string_to_int( s );
	else
	    val = hex_string_to_int( s );
	if( val < data->min ) val = data->min;
	if( val > data->max ) val = data->max;
    }
    return val;
}

void text_changed( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
	text_data* data = (text_data*)win->data;
	if( data->win->action_handler )
    	    data->win->action_handler( data->win->handler_data, win, wm );
    }
}

void text_set_zoom( WINDOWPTR win, int zoom, window_manager* wm )
{
    if( win )
    {
	text_data* data = (text_data*)win->data;
        data->zoom = zoom;
    }
}

void text_set_range( WINDOWPTR win, int min, int max )
{
    if( win )
    {
	text_data* data = (text_data*)win->data;
	data->min = min;
	data->max = max;
	draw_window( win, win->wm );
    }
}

void text_set_step( WINDOWPTR win, int step )
{
    if( win )
    {
	text_data* data = (text_data*)win->data;
	data->step = step;
	draw_window( win, win->wm );
    }
}

bool text_get_editing_state( WINDOWPTR win )
{
    if( win )
    {
	text_data* data = (text_data*)win->data;
	return data->editing;
    }
    return 0;
}

bool text_is_readonly( WINDOWPTR win )
{
    if( win )
    {
	text_data* data = (text_data*)win->data;
	return data->ro;
    }
    return 0;
}

struct button_data
{
    WINDOWPTR win;
    sundog_image_scaled img1;
    sundog_image_scaled img2;
    utf8_char* menu;
    int menu_val;
    bool autorepeat;
    int timer;
    ticks_t autorepeat_base_time;
    uchar flags;
    bool pushed;
    bool pen_inside;
    char flat;
    int add_radius;
    
    utf8_char* text;
    COLOR text_color;
    uchar text_opacity;
    
    uint evt_flags;

    int (*end_handler)( void*, WINDOWPTR );
    void* end_handler_data;

    WINDOWPTR prev_focus_win;
};

void button_timer( void* vdata, sundog_timer* timer, window_manager* wm )
{
    //Autorepeat timer
    button_data* data = (button_data*)vdata;
    if( data->pushed && data->pen_inside )
    {
	if( data->win->action_handler )
	{
	    data->flags |= BUTTON_FLAG_HANDLER_CALLED_FROM_TIMER;
	    data->win->action_handler( data->win->handler_data, data->win, wm );
	    data->flags &= ~BUTTON_FLAG_HANDLER_CALLED_FROM_TIMER;
	}
    }
    ticks_t t = time_ticks();
    while( 1 )
    {
	if( t < data->autorepeat_base_time + time_ticks_per_second() * 3 )
	{
	    timer->delay = time_ticks_per_second() / 20;
	    break;
	}
	if( t < data->autorepeat_base_time + time_ticks_per_second() * 6 )
	{
	    timer->delay = time_ticks_per_second() / 70;
	    break;
	}
	if( t < data->autorepeat_base_time + time_ticks_per_second() * 9 )
	{
	    timer->delay = time_ticks_per_second() / 260;
	    break;
	}
	timer->delay = time_ticks_per_second() / 1000;
	break;
    }
}

int button_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    button_data* data = (button_data*)win->data;
    COLOR col;
    int rx = evt->x - win->screen_x;
    int ry = evt->y - win->screen_y;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( button_data );
	    break;
	case EVT_AFTERCREATE:
	    data->win = win;
	    data->menu = 0;
	    data->autorepeat = wm->opt_button_autorepeat;
	    data->flat = wm->opt_button_flat;
	    data->timer = -1;
	    data->flags = wm->opt_button_flags;
	    data->pushed = 0;
	    data->text = 0;
	    data->text_color = wm->color3;
	    data->text_opacity = 255;
	    data->evt_flags = 0;
	    data->end_handler = wm->opt_button_end_handler;
	    data->end_handler_data = wm->opt_button_end_handler_data;
	    if( wm->control_type == TOUCHCONTROL )
		data->add_radius = wm->scrollbar_size / 2;
	    else
		data->add_radius = 0;
	    bmem_set( &data->img1, sizeof( data->img1 ), 0 );
	    bmem_set( &data->img2, sizeof( data->img2 ), 0 );
	    if( wm->opt_button_image1.img )
	    {
		data->img1 = wm->opt_button_image1;
		data->img2 = wm->opt_button_image2;
	    }
	    if( data->flags & BUTTON_FLAG_SHOW_VALUE )
	    {
		win->font = 0;
		data->text_color = wm->color2;
		data->menu_val = 0;
	    }
	    data->prev_focus_win = 0;
	    bmem_set( &wm->opt_button_image1, sizeof( wm->opt_button_image1 ), 0 );
	    bmem_set( &wm->opt_button_image2, sizeof( wm->opt_button_image2 ), 0 );
	    wm->opt_button_autorepeat = false;
	    wm->opt_button_flat = 0;
	    wm->opt_button_end_handler = 0;
	    wm->opt_button_end_handler_data = 0;
	    wm->opt_button_flags = 0;
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    bmem_free( data->text );
	    bmem_free( data->menu );
	    retval = 1;
	    break;
	case EVT_FOCUS:
	    data->prev_focus_win = wm->prev_focus_win;
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	    if( evt->key == MOUSE_BUTTON_LEFT )
	    {
		if( rx >= 0 && rx < win->xsize &&
		    ry >= 0 && ry < win->ysize )
		{
		    data->evt_flags = evt->flags;
		    data->pushed = 1;
		    data->pen_inside = 1;
		    if( data->autorepeat )
		    {
			if( data->timer != -1 )
			{
			    remove_timer( data->timer, wm );
			}
			int autorepeat_delay = time_ticks_per_second() / 2;
			data->timer = add_timer( button_timer, (void*)data, autorepeat_delay, wm );
			data->autorepeat_base_time = time_ticks() + autorepeat_delay;
		    }
		    draw_window( win, wm );
		    retval = 1;
		}
	    }
	    break;
	case EVT_MOUSEMOVE:
	    if( data->pushed )
	    {
		if( rx >= -data->add_radius && rx < win->xsize + data->add_radius &&
		    ry >= -data->add_radius && ry < win->ysize + data->add_radius )
		{
		    data->pen_inside = 1;
		}
		else
		{
		    data->pen_inside = 0;
		}
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONUP:
	    if( evt->key == MOUSE_BUTTON_LEFT && data->pushed )
	    {
		data->pushed = 0;
		set_focus_win( data->prev_focus_win, wm );
		if( rx >= -data->add_radius && rx < win->xsize + data->add_radius &&
		    ry >= -data->add_radius && ry < win->ysize + data->add_radius )
		{
		    if( data->menu )
		    {
		        //Show popup with text menu:
		        win->action_result = popup_menu( win->name, data->menu, win->screen_x, win->screen_y, wm->menu_color, wm );
			if( win->action_result >= 0 )
			{
			    data->menu_val = win->action_result;
			}
		    }
		}
		if( data->autorepeat && data->timer != -1 )
		{
		    remove_timer( data->timer, wm );
		    data->timer = -1;
		}
		draw_window( win, wm );
		int (*end_handler)( void*, WINDOWPTR ) = data->end_handler;
		void* end_handler_data = data->end_handler_data;
		if( rx >= -data->add_radius && rx < win->xsize + data->add_radius &&
		    ry >= -data->add_radius && ry < win->ysize + data->add_radius &&
		    win->action_handler )
		{
		    win->action_handler( win->handler_data, win, wm );
		}
		if( end_handler )
		{
		    end_handler( end_handler_data, win );
		}
		retval = 1;
	    }
	    break;
	case EVT_UNFOCUS:
	    if( data->pushed )
	    {
		data->pushed = 0;
		if( data->autorepeat && data->timer != -1 )
		{
		    remove_timer( data->timer, wm );
		    data->timer = -1;
		}
		draw_window( win, wm );
		if( data->end_handler )
		{
		    data->end_handler( data->end_handler_data, win );
		}
		retval = 1;
	    }
	    break;
	case EVT_DRAW:
	    if( data->flags & BUTTON_FLAG_AUTOBACKCOLOR ) 
		col = win->parent->color;
	    else
		col = win->color;
	    if( data->pushed )
		col = PUSHED_COLOR( col );

	    wbd_lock( win );
		
	    draw_frect( 0, 0, win->xsize, win->ysize, col, wm );
		
	    if( data->img1.img )
	    {
		//Draw image:
		sundog_image_scaled* img = &data->img1;
		if( data->pushed && data->img2.img )
		    img = &data->img2;
		wm->cur_opacity = data->text_opacity;
		draw_image_scaled(
		    ( win->xsize - img->dest_xsize ) / 2,
		    ( win->ysize - img->dest_ysize ) / 2 + data->pushed,
		    img,
		    wm );
		wm->cur_opacity = 255;
	    }
	    else
	    {
		COLOR text_color = blend( col, data->text_color, data->text_opacity );
		wm->cur_font_color = text_color;
		
		utf8_char* text;
		if( data->text )
		    text = data->text;
		else
		    text = (utf8_char*)win->name;
		if( text )
		{
		    //Draw name:
		    int tx;
		    int ty;
		    if( data->flags & BUTTON_FLAG_SHOW_VALUE )
		    {
			//Show current selected item (data->menu_val) of the popup menu
			tx = 1;
			ty = win->ysize / 2 - char_y_size( wm );
			draw_string( text, tx, ty, wm );
			//int text_xsize = string_x_size( text, wm );
			//draw_string( ":", tx + text_xsize, ty, wm );
			
			//Draw value:
			if( data->menu )
			{
			    utf8_char val[ 64 ];
			    val[ 0 ] = 0;
			    val[ 63 ] = 0;
			    int i = 0;
			    utf8_char* m = (utf8_char*)data->menu;
			    for( ; *m != 0; m++ )
			    {
				if( *m == 0xA || *m == 0xD ) 
				{
				    i++;
				    continue;
				}
				if( i == data->menu_val )
				{
				    for( int i2 = 0; i2 < 63; i2++ )
				    {
					val[ i2 ] = *m;
					if( *m == 0 || *m == 0xD || *m == 0xA ) 
					{
					    val[ i2 ] = 0;
					    break;
					}
					m++;
				    }
				    break;
				}
			    }
			    ty = win->ysize / 2 + 1;
			    wm->cur_font_color = blend( wm->color2, wm->blue, 64 );
			    draw_string( val, tx, ty, wm );
			}			
		    }
		    else
		    {
			int xx = string_x_size( text, wm );
			tx = ( win->xsize - xx ) / 2;
			ty = ( win->ysize - char_y_size( wm ) ) / 2;
			if( xx > win->xsize )
			{
			    if( data->flags & BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW )
			    {
				tx = 1;
			    }
			}
			draw_string( text, tx, ty + data->pushed, wm );
		    }
		}
	    }
		    
	    //Draw border:
	    if( data->flat )
	    {
	        switch( data->flat )
	        {
	    	    case 1:
		        break;
		    case 2:
		        draw_line( 0, 0, win->xsize, 0, wm->color1, wm );
		        draw_line( win->xsize - 1, 0, win->xsize - 1, win->ysize, wm->color1, wm );
		        break;
		    case 3:
		        draw_line( 0, win->ysize - 1, win->xsize, win->ysize - 1, wm->color1, wm );
		        draw_line( win->xsize - 1, 0, win->xsize - 1, win->ysize, wm->color1, wm );
		        break;
		}
	    }
	    else
	    {
	        draw_rect( 0, 0, win->xsize - 1, win->ysize - 1, BORDER_COLOR( col ), wm );
	    }
		
	    wbd_draw( wm );
	    wbd_unlock( wm );

	    retval = 1;
	    break;
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
    }
    return retval;
}

void button_set_menu( WINDOWPTR win, const utf8_char* menu, window_manager* wm )
{
    if( win )
    {
        button_data* data = (button_data*)win->data;
	bmem_free( data->menu );
	data->menu = bmem_strdup( menu );
    }
}

void button_set_menu_val( WINDOWPTR win, int val, window_manager* wm )
{
    if( win )
    {
        button_data* data = (button_data*)win->data;
	data->menu_val = val;
    }
}

int button_get_menu_val( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
        button_data* data = (button_data*)win->data;
	return data->menu_val;
    }
    return -1;
}

void button_set_text( WINDOWPTR win, const utf8_char* text, window_manager* wm )
{
    if( win )
    {
	button_data* data = (button_data*)win->data;
	bmem_free( data->text );
	if( text )
	{
	    data->text = bmem_strdup( text );
	}
	else 
	{
	    data->text = 0;
	}
    }
}

void button_set_text_color( WINDOWPTR win, COLOR c, window_manager* wm )
{
    if( win )
    {
        button_data* data = (button_data*)win->data;
	data->text_color = c;
    }
}

void button_set_text_opacity( WINDOWPTR win, uchar opacity, window_manager* wm )
{
    if( win )
    {
        button_data* data = (button_data*)win->data;
	data->text_opacity = opacity;
    }
}

int button_get_text_opacity( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
        button_data* data = (button_data*)win->data;
	return data->text_opacity;
    }
    return 0;
}

void button_set_images( WINDOWPTR win, sundog_image_scaled* img1, sundog_image_scaled* img2 )
{
    if( win )
    {
        button_data* data = (button_data*)win->data;
        if( img1 )
    	    data->img1 = *img1;
        if( img2 )
    	    data->img2 = *img2;
    }
}

void button_set_flags( WINDOWPTR win, uchar flags )
{
    if( win )
    {
	button_data* data = (button_data*)win->data;
	data->flags = flags;
    }
}

uchar button_get_flags( WINDOWPTR win )
{
    if( win )
    {
	button_data* data = (button_data*)win->data;
	return data->flags;
    }
    return 0;
}

int button_get_evt_flags( WINDOWPTR win )
{
    if( win )
    {
	button_data* data = (button_data*)win->data;
	return data->evt_flags;
    }
    return 0;
}

int button_get_optimal_xsize( const utf8_char* button_name, int font, bool smallest_as_possible, window_manager* wm )
{
    int rv = 0;
    rv = font_string_x_size( button_name, font, wm ) + font_char_x_size( ' ', font, wm );
    if( smallest_as_possible )
    {
	if( rv < wm->small_button_xsize ) rv = wm->small_button_xsize;
    }
    else
    {
	if( rv < wm->button_xsize ) rv = wm->button_xsize;
    }
    return rv;
}

struct scrollbar_data
{
    WINDOWPTR win;
    WINDOWPTR but1;
    WINDOWPTR but2;
    bool but1_inc;
    bool vert;
    bool rev;
    bool compact_mode;
    bool flat;
    utf8_char* name;
    utf8_char* values;
    int max_value;
    int normal_value;
    int page_size;
    int step_size;
    int cur;
    bool bar_selected;
    int drag_start_x;
    int drag_start_y;
    int drag_start_val;
    int show_offset;
    uint flags;

    int working_area;
    int min_work_area;    
    int button_size;
    WINDOWPTR expanded;
    WINDOWPTR editor;

    utf8_char val_ts[ 64 ];
    utf8_char* name_str;
    utf8_char* value_str;
    
    int cur_x;
    int cur_y;

    int pos;
    int bar_size;
    float one_pixel_size;
    int move_region;

    scrollbar_hex hex;
    
    uint evt_flags;
    
    bool begin;
    int	(*begin_handler)( void*, WINDOWPTR, int );
    int	(*end_handler)( void*, WINDOWPTR, int );
    int	(*opt_handler)( void*, WINDOWPTR, int );
    void* begin_handler_data;
    void* end_handler_data;
    void* opt_handler_data;

    WINDOWPTR prev_focus_win;
};

int scrollbar_button_end( void* user_data, WINDOWPTR win )
{
    scrollbar_data* data = (scrollbar_data*)user_data;
    if( data->begin )
    {
	data->evt_flags = button_get_evt_flags( win );
	if( data->end_handler )
	    data->end_handler( data->end_handler_data, data->win, scrollbar_get_value( data->win, win->wm ) );
	data->begin = 0;
    }
    return 0;
}

int scrollbar_button( void* user_data, WINDOWPTR win, window_manager* wm )
{
    scrollbar_data* data = (scrollbar_data*)user_data;
    data->evt_flags = button_get_evt_flags( win );
    if( data->begin == 0 )
    {
	data->begin = 1;
	if( data->begin_handler )
	    data->begin_handler( data->begin_handler_data, data->win, scrollbar_get_value( data->win, wm ) );
    }
    bool inc = 0;
    if( data->but1 == win && data->but1_inc == 1 ) inc = 1;
    if( data->but2 == win && data->but1_inc == 0 ) inc = 1;
    if( inc )
    {
	data->cur += data->step_size;
	if( data->cur < 0 ) data->cur = 0;
	if( data->cur > data->max_value ) data->cur = data->max_value;
    }
    else 
    {
	data->cur -= data->step_size;
	if( data->cur < 0 ) data->cur = 0;
	if( data->cur > data->max_value ) data->cur = data->max_value;
    }
    draw_window( data->win, wm );
    if( data->win->action_handler )
	data->win->action_handler( data->win->handler_data, data->win, wm );
    return 0;
}

void draw_scrollbar_horizontal_selection( WINDOWPTR win, int x )
{
    window_manager* wm = win->wm;
    
    int xsize = wm->scrollbar_size / 2;

    wm->cur_opacity = 255;

    sundog_polygon p;
    sundog_vertex v[ 4 ];
    v[ 0 ].x = x;
    v[ 0 ].y = 0;
    v[ 0 ].c = wm->color3;
    v[ 0 ].t = 64;
    v[ 1 ].x = x + xsize;
    v[ 1 ].y = 0;
    v[ 1 ].t = 0;
    v[ 2 ].x = x + xsize;
    v[ 2 ].y = win->ysize;
    v[ 2 ].t = 0;
    v[ 3 ].x = x;
    v[ 3 ].y = win->ysize;
    v[ 3 ].t = 64;
    p.vnum = 4;
    p.v = v;
    wm->cur_flags = WBD_FLAG_ONE_COLOR;
    draw_polygon( &p, wm );

    v[ 1 ].x = x - xsize;
    v[ 2 ].x = x - xsize;
    draw_polygon( &p, wm );
}

void draw_scrollbar_vertical_selection( WINDOWPTR win, int y )
{
    window_manager* wm = win->wm;

    int ysize = wm->scrollbar_size / 2;
    
    wm->cur_opacity = 255;
    
    sundog_polygon p;
    sundog_vertex v[ 4 ];
    v[ 0 ].x = 0;
    v[ 0 ].y = y;
    v[ 0 ].c = wm->color3;
    v[ 0 ].t = 64;
    v[ 1 ].x = 0;
    v[ 1 ].y = y + ysize;
    v[ 1 ].t = 0;
    v[ 2 ].x = win->xsize;
    v[ 2 ].y = y + ysize;
    v[ 2 ].t = 0;
    v[ 3 ].x = win->xsize;
    v[ 3 ].y = y;
    v[ 3 ].t = 64;
    p.vnum = 4;
    p.v = v;
    wm->cur_flags = WBD_FLAG_ONE_COLOR;
    draw_polygon( &p, wm );
    
    v[ 1 ].y = y - ysize;
    v[ 2 ].y = y - ysize;
    draw_polygon( &p, wm );
}

void scrollbar_calc_button_size( WINDOWPTR win )
{
    scrollbar_data* data = (scrollbar_data*)win->data;
    if( data->vert )
    {
	data->button_size = data->but1->ysize;
    }
    else
    {
	data->button_size = data->but1->xsize;
    }
}

void scrollbar_calc_working_area( WINDOWPTR win )
{
    scrollbar_data* data = (scrollbar_data*)win->data;
    window_manager* wm = win->wm;

    scrollbar_calc_button_size( win );
		
    data->working_area = 0;
    if( data->vert )
        data->working_area = win->ysize - data->button_size * 2;
    else
        data->working_area = win->xsize - data->button_size * 2;
        
    if( data->compact_mode )
    {
	if( data->min_work_area == 0 )
	    data->min_work_area = wm->scrollbar_size * 2;
	if( data->working_area < data->min_work_area )
	{
	    if( ( data->but1->flags & WIN_FLAG_ALWAYS_INVISIBLE ) == 0 )
	    {
	        hide_window( data->but1, wm );
	        hide_window( data->but2, wm );
	        data->but1->flags |= WIN_FLAG_ALWAYS_INVISIBLE;
	        data->but2->flags |= WIN_FLAG_ALWAYS_INVISIBLE;
	        recalc_regions( wm );
	    }
	    data->working_area += data->button_size * 2;
	    data->button_size = 0;
	}
	else
	{
	    if( data->but1->flags & WIN_FLAG_ALWAYS_INVISIBLE )
	    {
	        data->but1->flags &= ~WIN_FLAG_ALWAYS_INVISIBLE;
	        data->but2->flags &= ~WIN_FLAG_ALWAYS_INVISIBLE;
	        show_window( data->but1, wm );
	        show_window( data->but2, wm );
	        recalc_regions( wm );
	    }
	}
    }
}

void scrollbar_get_strings( WINDOWPTR win )
{
    scrollbar_data* data = (scrollbar_data*)win->data;
    
    data->name_str = data->name;
    data->value_str = data->val_ts;
    data->value_str[ 0 ] = 0;
    
    if( data->compact_mode )
    {
	utf8_char text_val[ 256 ];
	text_val[ 0 ] = 0;
	text_val[ 255 ] = 0;
	if( data->values )
	{
	    const utf8_char* vv = data->values;
	    int v = 0;
	    while( 1 )
	    {
	        if( v == data->cur ) break;
	        if( *vv == '/' ) v++;
	        if( *vv == 0 ) break;
	        vv++;
	    }
	    for( size_t i = 0; i < sizeof( text_val ) - 1; i++ )
	    {
	        text_val[ i ] = vv[ i ];
	        if( text_val[ i ] == '/' ) text_val[ i ] = 0;
	        if( text_val[ i ] == 1 ) text_val[ i ] = '/';
	        if( text_val[ i ] == 0 ) break;
	    }
	}
	if( data->hex != scrollbar_hex_off && data->max_value )
	{
	    bool show_hex_val = false;
	    int val = data->cur + data->show_offset;
	    uint hex_val;
	    switch( data->hex )
	    {
		case scrollbar_hex_scaled:
		    //Scaled to 0x0000 (0%) ... 0x8000 (100%)
	    	    hex_val = ( data->cur * 32768 ) / data->max_value;
	    	    if( ( data->cur * 32768 ) % data->max_value ) hex_val++;
	    	    if( hex_val > 32768 ) hex_val = 32768;
	    	    break;
		case scrollbar_hex_normal:
		    //Normal hex value without show_offset:
	    	    hex_val = data->cur;
	    	    break;
	    	case scrollbar_hex_normal_with_offset:
		    //Normal hex value with show_offset:
	    	    hex_val = val;
	    	    break;
	    }
	    if( text_val[ 0 ] ) show_hex_val = true;
	    if( hex_val > 9 || (signed)hex_val != val ) show_hex_val = true;
	    if( show_hex_val )
	    {
		if( text_val[ 0 ] )
	    	    sprintf( data->value_str, "%s (%x)", text_val, (unsigned int)hex_val );
	    	else
	    	    sprintf( data->value_str, "%d (%x)", val, (unsigned int)hex_val );
	    }
	    else
	    {
		sprintf( data->value_str, "%d", val );
	    }
	}
	else
	{
	    if( text_val[ 0 ] )
		sprintf( data->value_str, "%s", text_val );
	    else
		sprintf( data->value_str, "%d", data->cur + data->show_offset );
	}
    }
}

struct scrollbar_editor_data
{
    WINDOWPTR win;
    scrollbar_data* sdata;
    WINDOWPTR text;
    WINDOWPTR close;
    int correct_ysize;
};

int scrollbar_editor_text_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    scrollbar_editor_data* data = (scrollbar_editor_data*)user_data;

    scrollbar_set_value( data->sdata->win, text_get_value( win, wm ) - data->sdata->show_offset, wm );
    scrollbar_call_handler( data->sdata->win );
    
    return 0;
}

int scrollbar_editor_close_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    scrollbar_editor_data* data = (scrollbar_editor_data*)user_data;
    
    remove_window( data->win, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    
    return 0;
}

int scrollbar_editor_handler( sundog_event* evt, window_manager* wm ) //host - scrollbar_data
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    scrollbar_editor_data* data = (scrollbar_editor_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( scrollbar_editor_data );
	    break;
	    
	case EVT_AFTERCREATE:
	    {
		data->win = win;
		data->sdata = (scrollbar_data*)win->host;
	    
		int y = wm->interelement_space;
		int x = wm->interelement_space;
		const utf8_char* bname = 0;
		int bxsize = 0;
	    
		wm->opt_text_numeric = 1;
        	wm->opt_text_num_min = data->sdata->show_offset;
        	wm->opt_text_num_max = data->sdata->max_value + data->sdata->show_offset;
        	data->text = new_window( "SBValue", 0, y, 1, wm->controller_ysize, wm->text_background, win, text_handler, wm );
        	text_set_value( data->text, data->sdata->cur + data->sdata->show_offset, wm );
        	set_handler( data->text, scrollbar_editor_text_handler, data, wm );
        	set_window_controller( data->text, 0, wm, (WCMD)wm->interelement_space, CEND );
        	set_window_controller( data->text, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		y += wm->controller_ysize + wm->interelement_space;
	    
		bname = wm_get_string( STR_WM_CLOSE ); bxsize = button_get_optimal_xsize( bname, win->font, false, wm );
        	data->close = new_window( bname, x, 0, bxsize, 10, wm->button_color, win, button_handler, wm );
        	set_handler( data->close, scrollbar_editor_close_handler, data, wm );
        	set_window_controller( data->close, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
        	set_window_controller( data->close, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->button_ysize + wm->interelement_space, CEND );
        	x += bxsize + wm->interelement_space2;
	    
		data->correct_ysize = y + wm->button_ysize + wm->interelement_space;
	    }
	    retval = 1;
	    break;

	case EVT_BEFORECLOSE:
	    data->sdata->editor = 0;
	    retval = 1;
	    break;
	    
	case EVT_SCREENRESIZE:
        case EVT_BEFORESHOW:
            resize_window_with_decorator( win, 0, data->correct_ysize, wm );
            break;

	case EVT_BUTTONDOWN:
	case EVT_BUTTONUP:            
        case EVT_MOUSEBUTTONDOWN:
        case EVT_MOUSEMOVE:
        case EVT_MOUSEBUTTONUP:
        case EVT_TOUCHBEGIN:
        case EVT_TOUCHEND:
        case EVT_TOUCHMOVE:
        case EVT_FOCUS:
        case EVT_UNFOCUS:
            retval = 1;
            break;
    }
    return retval;
}

struct scrollbar_expanded_data
{
    WINDOWPTR win;
    WINDOWPTR ctl;
};

int scrollbar_expanded_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    scrollbar_expanded_data* data = (scrollbar_expanded_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( scrollbar_expanded_data );
	    break;
	case EVT_AFTERCREATE:
	    data->win = win;
	    data->ctl = 0;
	    retval = 1;
	    break;
	case EVT_DRAW:
	    {
		scrollbar_data* cdata = (scrollbar_data*)data->ctl->data;
		scrollbar_get_strings( data->ctl );
		
		wbd_lock( win );

		wm->cur_opacity = 255;
		draw_frect( 0, 0, win->xsize, win->ysize, win->color, wm );
		
		int xc = 0;
                if( cdata->max_value != 0 )
                    xc = ( ( win->xsize - wm->interelement_space * 2 ) * ( ( cdata->cur << 10 ) / cdata->max_value ) ) >> 10;
		draw_frect( wm->interelement_space, wm->interelement_space, xc, win->ysize - wm->interelement_space * 2, blend( wm->color2, win->color, 128 ), wm );

		//Show normal value:
		if( cdata->normal_value && cdata->normal_value < cdata->max_value )
		{
		    wm->cur_opacity = 255;
		    int norm = ( ( win->xsize - wm->interelement_space * 2 ) * cdata->normal_value ) / cdata->max_value;
		    int cx = norm + wm->interelement_space;
		    int grad_size = ( wm->scrollbar_size * 3 ) / 4;
		    COLOR grad_color = blend( wm->color3, wm->red, 160 );
		    draw_hgradient( cx - grad_size, wm->interelement_space, grad_size, win->ysize - wm->interelement_space * 2, grad_color, 0, 48, wm );
		    draw_hgradient( cx, wm->interelement_space, grad_size, win->ysize - wm->interelement_space * 2, grad_color, 48, 0, wm );
		    wm->cur_opacity = 52;
		    draw_frect( cx, wm->interelement_space, 1, win->ysize - wm->interelement_space * 2, wm->color3, wm );
		    wm->cur_opacity = 255;
		    draw_frect( win->xsize - wm->interelement_space, wm->interelement_space, wm->interelement_space, win->ysize - wm->interelement_space * 2, win->color, wm );
		}
		
		int ychar = char_y_size( wm );

		wm->cur_font_color = wm->color3;

		if( cdata->name_str )
		{
		    draw_string( cdata->name_str, wm->interelement_space, win->ysize / 2 - ychar, wm );
		}

		if( cdata->value_str )
		{
		    draw_string( cdata->value_str, wm->interelement_space, win->ysize / 2, wm );
		}
		
		wbd_draw( wm );
    		wbd_unlock( wm );
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
        case EVT_MOUSEMOVE:
    	    hide_window( win, wm );
    	    retval = 1;
    	    break;
    }
    return retval;
}

int scrollbar_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    scrollbar_data* data = (scrollbar_data*)win->data;
    int rx = evt->x - win->screen_x;
    int ry = evt->y - win->screen_y;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( scrollbar_data );
	    break;
	case EVT_AFTERCREATE:
	    win->flags |= WIN_FLAG_ALWAYS_HANDLE_DRAW_EVT;
	    
	    data->win = win;
	    data->flat = wm->opt_scrollbar_flat;
	    data->begin_handler = wm->opt_scrollbar_begin_handler;
	    data->end_handler = wm->opt_scrollbar_end_handler;
	    data->opt_handler = wm->opt_scrollbar_opt_handler;
	    data->begin_handler_data = wm->opt_scrollbar_begin_handler_data;
	    data->end_handler_data = wm->opt_scrollbar_end_handler_data;
	    data->opt_handler_data = wm->opt_scrollbar_opt_handler_data;
	    data->begin = 0;
	    data->rev = wm->opt_scrollbar_reverse;
	    data->compact_mode = wm->opt_scrollbar_compact; 

	    if( data->compact_mode )
		data->flat = 1;

	    if( wm->opt_scrollbar_vertical )
	    {
		data->vert = 1;
		wm->opt_button_autorepeat = true;
		wm->opt_button_flat = 1;
		wm->opt_button_end_handler = scrollbar_button_end;
		wm->opt_button_end_handler_data = data;
		data->but1 = new_window( g_text_up, 0, 0, 1, 1, win->color, win, button_handler, wm );
		wm->opt_button_autorepeat = true;
		wm->opt_button_flat = 1;
		wm->opt_button_end_handler = scrollbar_button_end;
		wm->opt_button_end_handler_data = data;
		data->but2 = new_window( g_text_down, 0, 0, 1, 1, win->color, win, button_handler, wm );
		set_window_controller( data->but2, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->but2, 1, wm, CPERC, (WCMD)100, CEND );
		set_window_controller( data->but2, 2, wm, CPERC, (WCMD)100, CEND );
		set_window_controller( data->but1, 0, wm, (WCMD)0, CEND );
		set_window_controller( data->but1, 2, wm, CPERC, (WCMD)100, CEND );
		if( data->compact_mode )
		{
		    set_window_controller( data->but2, 3, wm, CWIN, (WCMD)win, CXSIZE, CMULDIV256, (WCMD)SIDEBTN, CMAXVAL, (WCMD)wm->scrollbar_size, CPUTR0, CPERC, (WCMD)100, CSUB, CR0, CEND );
		    set_window_controller( data->but1, 1, wm, CWIN, (WCMD)win, CXSIZE, CMULDIV256, (WCMD)SIDEBTN, CMAXVAL, (WCMD)wm->scrollbar_size, CPUTR0, CPERC, (WCMD)100, CSUB, CR0, CSUB, CR0, CEND );
		    set_window_controller( data->but1, 3, wm, CWIN, (WCMD)win, CXSIZE, CMULDIV256, (WCMD)SIDEBTN, CMAXVAL, (WCMD)wm->scrollbar_size, CPUTR0, CPERC, (WCMD)100, CSUB, CR0, CEND );
		}
		else
		{
		    set_window_controller( data->but2, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->scrollbar_size, CEND );
		    set_window_controller( data->but1, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->scrollbar_size * 2, CEND );
		    set_window_controller( data->but1, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->scrollbar_size, CEND );
		}
		if( wm->opt_scrollbar_reverse )
		{
		    data->but1_inc = 1;
		}
		else
		{
		    data->but1_inc = 0;
		}
	    }
	    else
	    {
		data->vert = 0;
		wm->opt_button_autorepeat = true;
		wm->opt_button_flat = 1;
		wm->opt_button_end_handler = scrollbar_button_end;
		wm->opt_button_end_handler_data = data;
		data->but1 = new_window( g_text_right, 0, 0, 1, 1, win->color, win, button_handler, wm );
		wm->opt_button_autorepeat = true;
		wm->opt_button_flat = 1;
		wm->opt_button_end_handler = scrollbar_button_end;
		wm->opt_button_end_handler_data = data;
		data->but2 = new_window( g_text_left, 0, 0, 1, 1, win->color, win, button_handler, wm );
		set_window_controller( data->but1, 0, wm, CPERC, (WCMD)100, CEND );
		set_window_controller( data->but1, 1, wm, CPERC, (WCMD)0, CEND );
		set_window_controller( data->but1, 3, wm, CPERC, (WCMD)100, CEND );
		set_window_controller( data->but2, 1, wm, (WCMD)0, CEND );
		set_window_controller( data->but2, 3, wm, CPERC, (WCMD)100, CEND );
		if( data->compact_mode )
		{
		    set_window_controller( data->but1, 2, wm, CWIN, (WCMD)win, CYSIZE, CMULDIV256, (WCMD)SIDEBTN, CMAXVAL, (WCMD)wm->scrollbar_size, CPUTR0, CPERC, (WCMD)100, CSUB, CR0, CEND );
		    set_window_controller( data->but2, 0, wm, CWIN, (WCMD)win, CYSIZE, CMULDIV256, (WCMD)SIDEBTN, CMAXVAL, (WCMD)wm->scrollbar_size, CPUTR0, CPERC, (WCMD)100, CSUB, CR0, CSUB, CR0, CEND );
		    set_window_controller( data->but2, 2, wm, CWIN, (WCMD)win, CYSIZE, CMULDIV256, (WCMD)SIDEBTN, CMAXVAL, (WCMD)wm->scrollbar_size, CPUTR0, CPERC, (WCMD)100, CSUB, CR0, CEND );
		}
		else
		{
		    set_window_controller( data->but1, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->scrollbar_size, CEND );
		    set_window_controller( data->but2, 0, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->scrollbar_size * 2, CEND );
		    set_window_controller( data->but2, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->scrollbar_size, CEND );
		}
		if( wm->opt_scrollbar_reverse )
		{
		    data->but1_inc = 0;
		}
		else
		{
		    data->but1_inc = 1;
		}
	    }
	    scrollbar_calc_button_size( win );
	    data->working_area = 0;
	    data->min_work_area = 0;
	    data->expanded = 0;
	    data->editor = 0;
	    set_handler( data->but1, scrollbar_button, data, wm );
	    set_handler( data->but2, scrollbar_button, data, wm );
	    if( data->compact_mode )
	    {
		data->but1->color = win->color;
		data->but2->color = win->color;
	    }
	    button_set_text_opacity( data->but1, 150, wm );
	    button_set_text_opacity( data->but2, 150, wm );
	    button_set_text_color( data->but1, wm->color2, wm );
	    button_set_text_color( data->but2, wm->color2, wm );
	    
	    data->name = 0;
	    data->values = 0;
	    data->cur = 0;
	    data->max_value = 0;
	    data->normal_value = 0;
	    data->page_size = 0;
	    data->step_size = 1;
	    data->bar_selected = 0;
	    data->drag_start_x = 0;
	    data->drag_start_y = 0;
	    data->show_offset = 0;
	    data->one_pixel_size = 0;
	    
	    data->hex = scrollbar_hex_off;
	    
	    data->prev_focus_win = 0;
	    
	    data->evt_flags = 0;
	    	    
	    wm->opt_scrollbar_vertical = false;
	    wm->opt_scrollbar_reverse = false;
	    wm->opt_scrollbar_compact = false;
	    wm->opt_scrollbar_flat = 0;
	    wm->opt_scrollbar_begin_handler = 0;
	    wm->opt_scrollbar_end_handler = 0;
	    wm->opt_scrollbar_opt_handler = 0;
	    wm->opt_scrollbar_begin_handler_data = 0;
	    wm->opt_scrollbar_end_handler_data = 0;
	    wm->opt_scrollbar_opt_handler_data = 0;
	    
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    if( data->name )
	    {
		bmem_free( data->name );
		data->name = 0;
	    }
	    if( data->values )
	    {
		bmem_free( data->values );
		data->values = 0;
	    }
	    remove_window( data->editor, wm );
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	    {
        	data->evt_flags = evt->flags;
		int inc = 0;
        	if( evt->key & MOUSE_BUTTON_SCROLLUP ) inc = 1;
        	if( evt->key & MOUSE_BUTTON_SCROLLDOWN ) inc = -1;
        	if( inc )
        	{
        	    if( data->vert ) inc = -inc;
		    if( data->begin == 0 )
		    {
		    	data->begin = 1;
			if( data->begin_handler )
			    data->begin_handler( data->begin_handler_data, win, scrollbar_get_value( win, wm ) );
		    }
		    if( inc > 0 )
		    {
			data->cur += data->step_size;
			if( data->cur < 0 ) data->cur = 0;
			if( data->cur > data->max_value ) data->cur = data->max_value;
		    }
		    else 
		    {
			data->cur -= data->step_size;
			if( data->cur < 0 ) data->cur = 0;
			if( data->cur > data->max_value ) data->cur = data->max_value;
		    }
		    draw_window( win, wm );
		    if( win->action_handler )
			win->action_handler( win->handler_data, win, wm );
		    if( data->begin )
		    {
		    	if( data->end_handler )
			    data->end_handler( data->end_handler_data, win, scrollbar_get_value( win, wm ) );
			data->begin = 0;
		    }
        	}
            }
	    if( evt->key == MOUSE_BUTTON_LEFT )
	    {
		if( evt->type == EVT_MOUSEBUTTONDOWN )
		{
		    if( data->begin == 0 )
		    {
			data->begin = 1;
			if( data->begin_handler )
			    data->begin_handler( data->begin_handler_data, win, scrollbar_get_value( win, wm ) );
		    }
		}
		
		int value_changed = 1;

		data->cur_x = rx;
		data->cur_y = ry;
		if( data->compact_mode == 0 )
		{
		    //Normal mode:
		    if( evt->type == EVT_MOUSEBUTTONDOWN && 
			rx >= 0 && rx < win->xsize && ry >= 0 && ry < win->ysize )
		    {
			data->bar_selected = 1;
			data->drag_start_x = rx;
			data->drag_start_y = ry;
			data->drag_start_val = data->cur;
		    }
		    if( evt->type == EVT_MOUSEMOVE )
		    {
			//Move:
			if( data->bar_selected )
			{
			    int d = 0;
			    if( data->vert ) 
				d = ry - data->drag_start_y;
			    else
				d = rx - data->drag_start_x;
			    if( data->rev )
				data->cur = data->drag_start_val - (int)( (float)d * data->one_pixel_size );
			    else
				data->cur = data->drag_start_val + (int)( (float)d * data->one_pixel_size );
			}
		    }
		}
		else
		{
		    //Compact mode:
		    if( evt->type == EVT_MOUSEBUTTONDOWN &&
			rx >= 0 && ry >= 0 &&
			rx < win->xsize && ry < win->ysize )
		    {
			if( data->but1 && data->but1->visible == 0 )
			{
			    int new_xsize = wm->scrollbar_size * 10;
			    int new_ysize = wm->scrollbar_size * 2;
			    if( new_xsize > wm->screen_xsize )
				new_xsize = wm->screen_xsize;
			    int new_x = win->screen_x + ( win->xsize - new_xsize ) / 2;
			    int new_y = win->screen_y + ( win->ysize - new_ysize ) / 2;
			    if( new_x + new_xsize > wm->screen_xsize ) new_x -= new_x + new_xsize - wm->screen_xsize;
			    if( new_y + new_ysize > wm->screen_ysize ) new_y -= new_y + new_ysize - wm->screen_ysize;
			    if( new_x < 0 ) new_x = 0;
			    if( new_y < 0 ) new_y = 0;
			    data->expanded = new_window( "Expanded controller", new_x, new_y, new_xsize, new_ysize, wm->color0, wm->root_win, scrollbar_expanded_handler, wm );
			    scrollbar_expanded_data* edata = (scrollbar_expanded_data*)data->expanded->data;
			    edata->ctl = win;
			    show_window( data->expanded, wm );
			    recalc_regions( wm );
			    draw_window( data->expanded, wm );
			}
			data->bar_selected = 1;
			data->drag_start_x = rx;
			data->drag_start_y = ry;
			data->drag_start_val = data->cur;
		    }
		    if( data->bar_selected == 1 && evt->type == EVT_MOUSEMOVE )
		    {
			if( data->working_area > 1 )
			{
			    int new_val;
			    int dx = rx - data->drag_start_x;
			    int dy = ry - data->drag_start_y;
			    int s;
			    if( data->expanded == 0 )
			    {
				s = data->working_area - 1;
				new_val = ( ( dx << 12 ) / s ) * data->max_value;
			    }
			    else
			    {
				s = data->expanded->xsize - wm->interelement_space * 2;
				if( data->values )
				{
				    if( s / ( wm->scrollbar_size / 2 ) > data->max_value + 1 )
					s = ( data->max_value + 1 ) * ( wm->scrollbar_size / 2 );
				}
				new_val = ( ( dx << 12 ) / s ) * data->max_value;
				new_val -= ( ( dy << 12 ) / s ) * data->max_value;
			    }
			    new_val >>= 12;
			    new_val += data->drag_start_val;
			    if( data->expanded && ( data->max_value + 1 > s ) && data->normal_value && data->normal_value < data->max_value )
			    {
				/*printf( "%d -> ", new_val );
				int c = data->normal_value - 1;
				if( new_val < data->normal_value )
				{
				    new_val = ( new_val * data->normal_value ) / c;
				    if( new_val > data->normal_value ) 
					new_val = data->normal_value;
				}
				printf( "%d\n", new_val );*/
			    }
			    if( data->cur == new_val ) 
				value_changed = 0;
			    else
				data->cur = new_val;
			}
		    }
		    else
		    {
			value_changed = 0;
		    }
		}
		//Bounds control:
		if( data->cur < 0 ) data->cur = 0;
		if( data->cur > data->max_value ) data->cur = data->max_value;
		//Redraw it:
		draw_window( win, wm );
		if( data->expanded )
		    draw_window( data->expanded, wm );
		//User handler:
		if( win->action_handler && value_changed )
		    win->action_handler( win->handler_data, win, wm );
	    }
	    retval = 1;
	    break;
	case EVT_FOCUS:
	    data->prev_focus_win = wm->prev_focus_win;
	    retval = 1;
	    break;
	case EVT_UNFOCUS:
	    if( data->bar_selected )
	    {
		data->bar_selected = 0;
		if( data->expanded )
                {
                    remove_window( data->expanded, wm );
                    data->expanded = 0;
                    recalc_regions( wm );
                    draw_window( wm->root_win, wm );
                }
		draw_window( win, wm );
		if( data->begin )
		{
		    data->begin = 0;
		    if( data->end_handler )
			data->end_handler( data->end_handler_data, win, scrollbar_get_value( win, wm ) );
		}
		retval = 1;
	    }
	    break;
	case EVT_MOUSEBUTTONUP:
	    {
		if( evt->key == MOUSE_BUTTON_LEFT )
		{
		    data->bar_selected = 0;
		    if( data->expanded )
		    {
			remove_window( data->expanded, wm );
			data->expanded = 0;
			recalc_regions( wm );
			draw_window( wm->root_win, wm );
		    }
		    set_focus_win( data->prev_focus_win, wm );
		    draw_window( win, wm );
		    //User handler:
		    if( win->action_handler )
			win->action_handler( win->handler_data, win, wm );
		    if( data->begin )
		    {
			data->begin = 0;
			if( data->end_handler )
			    data->end_handler( data->end_handler_data, win, scrollbar_get_value( win, wm ) );
		    }
		}
		int dx = rx - data->drag_start_x;
		int dy = ry - data->drag_start_y;
		if( dx < 0 ) dx = -dx;
		if( dy < 0 ) dy = -dy;
		int d;
		if( dx > dy ) d = dx; else d = dy;
		if( evt->key == MOUSE_BUTTON_RIGHT || ( ( evt->flags & EVT_FLAG_DOUBLECLICK ) && d < wm->scrollbar_size / 3 ) )
		{
		    if( data->opt_handler )
		    {
			data->opt_handler( data->opt_handler_data, win, scrollbar_get_value( win, wm ) );
		    }
		    else
		    {
			if( data->compact_mode )
			{
			    if( data->editor == 0 )
			    {
    				data->editor = new_window_with_decorator(
        			    data->name,
        			    0, 0,
        			    wm->normal_window_xsize, wm->normal_window_ysize,
        			    wm->dialog_color,
        			    wm->root_win,
        			    data,
        			    scrollbar_editor_handler,
        			    DECOR_FLAG_CENTERED | DECOR_FLAG_CHECK_SIZE,
        			    wm );
        		    }
    			    show_window( data->editor, wm );
    			    bring_to_front( data->editor, wm );
    			    recalc_regions( wm );
    			    draw_window( wm->root_win, wm );
    			}
		    }
		}
	    }
	    retval = 1;
	    break;
	case EVT_DRAW:
	    {
		scrollbar_calc_working_area( win );
		scrollbar_get_strings( win );
		
		if( win->reg->numRects == 0 ) { retval = 1; break; } //Because WIN_FLAG_ALWAYS_HANDLE_DRAW_EVT is set
		
		utf8_char* name_str = data->name_str;
		if( data->working_area < data->min_work_area )
	        {
    		    if( name_str )
		    {
	    		int p = 0;
	    		while( 1 )
	    		{
			    int c = name_str[ p ];
			    if( c == 0 ) break;
			    if( c == '.' )
			    {
		    		name_str += p + 1;
				break;
			    }
			    if( ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'F' ) )
			    {
			    }
			    else break;
			    p++;
			}
		    }
		}

		wbd_lock( win );

		if( data->compact_mode == 0 )
		{
		    //Normal mode:
		    
		    if( data->max_value == 0 || data->page_size == 0 )
		    {
			data->bar_size = data->working_area;
			data->pos = 0;
		    }
		    else
		    {
			//Calculate move-region (in pixels)
			float ppage;
			if( data->max_value + 1 == 0 ) 
			    ppage = 1.0F;
			else
			    ppage = (float)data->page_size / (float)( data->max_value + 1 );
			if( ppage == 0 ) ppage = 1.0F;
			data->move_region = (int)( (float)data->working_area / ( ppage + 1.0F ) );
			if( data->move_region == 0 ) data->move_region = 1;

			//Caclulate slider size (in pixels)
			data->bar_size = data->working_area - data->move_region;

			//Calculate one pixel size
			data->one_pixel_size = (float)( data->max_value + 1 ) / (float)data->move_region;

			//Calculate slider position (in pixels)
			data->pos = (int)( ( (float)data->cur / (float)data->max_value ) * (float)data->move_region );

			if( data->bar_size < 2 ) data->bar_size = 2;
			if( data->rev )
			    data->pos = ( data->working_area - data->pos ) - data->bar_size;

			if( data->pos + data->bar_size > data->working_area )
			{
			    data->bar_size -= ( data->pos + data->bar_size ) - data->working_area;
			}
		    }

		    bool frozen = 0;
		    if( data->bar_size >= data->working_area ) frozen = 1;
		    COLOR bar_color;
		    COLOR bg_color;
		    COLOR marker_color;
		    bg_color = win->parent->color;
		    bar_color = blend( wm->color2, bg_color, 220 );
		    marker_color = blend( bar_color, bg_color, 128 );
		    if( frozen )
		    {
			button_set_text_opacity( data->but1, 64, wm );
			button_set_text_opacity( data->but2, 64, wm );
			draw_frect( 0, 0, win->xsize, win->ysize, bg_color, wm );
		    }
		    else 
		    {
			button_set_text_opacity( data->but1, 150, wm );
			button_set_text_opacity( data->but2, 150, wm );
			if( data->vert )
			{
			    draw_frect( 0, 0, win->xsize, data->pos, bg_color, wm );
			    draw_frect( 0, data->pos + data->bar_size, win->xsize, data->working_area - ( data->pos + data->bar_size ), bg_color, wm );

			    int ss = 2;
			    draw_frect( 0, 0, win->xsize, ss, marker_color, wm );
			    draw_frect( 0, data->working_area - ss, win->xsize, ss, marker_color, wm );
			    
			    draw_frect( 0, data->pos, win->xsize, data->bar_size, bar_color, wm );
			    draw_frect( 0, data->pos - 1, win->xsize, 1, BORDER_COLOR( bar_color ), wm );
			    draw_frect( 0, data->pos + data->bar_size, win->xsize, 1, BORDER_COLOR( bar_color ), wm );
			}
			else
			{
			    draw_frect( 0, 0, data->pos, win->ysize, bg_color, wm );
			    draw_frect( data->pos + data->bar_size, 0, data->working_area - ( data->pos + data->bar_size ), win->ysize, bg_color, wm );
			    
			    int ss = 2;
			    draw_frect( 0, 0, ss, win->xsize, marker_color, wm );
			    draw_frect( data->working_area - ss, 0, ss, win->xsize, marker_color, wm );
			    
			    draw_frect( data->pos, 0, data->bar_size, win->ysize, bar_color, wm );
			    draw_frect( data->pos - 1, 0, 1, win->ysize, BORDER_COLOR( bar_color ), wm );
			    draw_frect( data->pos + data->bar_size, 0, 1, win->ysize, BORDER_COLOR( bar_color ), wm );
			}
			if( data->bar_selected )
			{
			    wm->cur_opacity = 64;
			    draw_frect( 0, 0, win->xsize, win->ysize, wm->green, wm );
			    if( data->vert )
				draw_scrollbar_vertical_selection( win, data->cur_y );
			    else 
				draw_scrollbar_horizontal_selection( win, data->cur_x );
			    bg_color = blend( bg_color, wm->green, 64 );
			}
		    }
		    		    
		    data->but1->color = bg_color;
		    data->but2->color = bg_color;
		}
		else
		{
		    //Compact mode:
		    		    
		    int start_x = 0;
		    COLOR bgcolor = win->color;
		    if( data->bar_selected )
		    {
			bgcolor = blend( bgcolor, wm->green, 64 );
		    }
		    data->but1->color = bgcolor;
		    data->but2->color = bgcolor;
		    draw_frect( 0, 0, win->xsize, win->ysize, bgcolor, wm );
		    
		    int ychar = char_y_size( wm );

#define SCROLL_XBORDER 0
#define SCROLL_YBORDER 0
		    
		    int xc = 0;
		    if( data->max_value != 0 )
			xc = ( ( data->working_area - SCROLL_XBORDER * 2 ) * ( ( data->cur << 10 ) / data->max_value ) ) >> 10;

		    COLOR fgcolor = wm->color2;
		    
		    wm->cur_opacity = 255;
		    if( data->but1->visible == 0 )
		    {
			COLOR lr_color = blend( fgcolor, bgcolor, 230 );
			draw_frect( start_x + SCROLL_XBORDER, SCROLL_YBORDER, 1, win->ysize - SCROLL_YBORDER * 2, lr_color, wm );
			draw_frect( start_x + SCROLL_XBORDER + data->working_area - 1, SCROLL_YBORDER, 1, win->ysize - SCROLL_YBORDER * 2, lr_color, wm );
		    }
		    draw_frect( start_x + SCROLL_XBORDER, SCROLL_YBORDER, xc, win->ysize - SCROLL_YBORDER * 2, blend( fgcolor, bgcolor, 180 ), wm );
		    
		    if( data->bar_selected )
		    {
			//Show selection:
			draw_scrollbar_horizontal_selection( win, data->cur_x );
		    }

		    //Show normal value:
		    if( data->normal_value && data->normal_value < data->max_value )
		    {
			wm->cur_opacity = 255;
			int norm = ( data->working_area * data->normal_value ) / data->max_value;
			int cx = SCROLL_XBORDER + norm;
			int grad_size = ( wm->scrollbar_size * 3 ) / 4;
			COLOR grad_color = blend( wm->color3, wm->red, 160 );
			draw_hgradient( cx - grad_size, SCROLL_YBORDER, grad_size, win->ysize - SCROLL_YBORDER * 2, grad_color, 0, 48, wm );
			draw_hgradient( cx, SCROLL_YBORDER, grad_size, win->ysize - SCROLL_YBORDER * 2, grad_color, 48, 0, wm );
			wm->cur_opacity = 52;
			draw_frect( cx, SCROLL_YBORDER, 1, win->ysize - SCROLL_YBORDER * 2, wm->color3, wm );
			wm->cur_opacity = 255;
		    }

		    if( name_str )
		    {
			wm->cur_font_color = wm->color2;
			wm->cur_opacity = 255;
			draw_string( name_str, start_x + SCROLL_XBORDER, win->ysize / 2 - ychar, wm );
		    }
		    
		    if( data->bar_selected )
			wm->cur_font_color = wm->color3;
		    else 
			wm->cur_font_color = blend( wm->color2, wm->blue, 64 );
		    wm->cur_opacity = 255;
		    draw_string( data->value_str, start_x + SCROLL_XBORDER, win->ysize / 2 + 1, wm );
		    
		    if( data->flags & SCROLLBAR_FLAG_INPUT )		    
		    {
			wm->cur_font_color = bgcolor;
    			wm->cur_opacity = 200;
    			draw_string( "\x18", 0, win->ysize / 2 - char_y_size( wm ), wm );
    			draw_string( "\x19", 0, win->ysize / 2, wm );
    			wm->cur_opacity = 255;

			wm->cur_font_color = wm->color3;
    			wm->cur_opacity = 255;
    			draw_string( "\x1A", 0, win->ysize / 2 - char_y_size( wm ), wm );
    			draw_string( "\x1B", 0, win->ysize / 2, wm );
    			
    			draw_hgradient( 0, 0, win->ysize, win->ysize, wm->color3, 100, 0, wm );
		    }
		}
		
		wbd_draw( wm );
    		wbd_unlock( wm );
	    }
	    retval = 1;
	    break;
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
    }
    return retval;
}

void scrollbar_set_parameters( WINDOWPTR win, int cur, int max_value, int page_size, int step_size, window_manager* wm )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{
	    data->cur = cur;
	    if( max_value >= 0 ) data->max_value = max_value;
	    if( page_size >= 0 ) data->page_size = page_size;
	    if( step_size >= 0 ) data->step_size = step_size;
	}
    }
}

void scrollbar_set_value( WINDOWPTR win, int val, window_manager* wm )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{
	    data->cur = val;
	    if( data->cur < 0 ) data->cur = 0;
	    if( data->cur > data->max_value ) data->cur = data->max_value;
	}
    }
}

int scrollbar_get_value( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{
	    return data->cur;
	}
    }
    return 0;
}

int scrollbar_get_evt_flags( WINDOWPTR win )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{
	    return data->evt_flags;
	}
    }
    return 0;
}

int scrollbar_get_step( WINDOWPTR win )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{
	    return data->step_size;
	}
    }
    return 0;
}

void scrollbar_set_name( WINDOWPTR win, const utf8_char* name, window_manager* wm )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{
	    if( name == 0 )
	    {
		bmem_free( data->name );
		data->name = 0;
		return;
	    }
	    if( data->name == 0 )
	    {
		data->name = (utf8_char*)bmem_new( bmem_strlen( name ) + 1 );
	    }
	    else
	    {
		if( bmem_strlen( name ) + 1 > bmem_get_size( data->name ) )
		{
		    data->name = (utf8_char*)bmem_resize( data->name, bmem_strlen( name ) + 1 );
		}
	    }
	    bmem_copy( data->name, name, bmem_strlen( name ) + 1 );
	}
    }
}

void scrollbar_set_values( WINDOWPTR win, const utf8_char* values, window_manager* wm )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{
	    if( values == 0 )
	    {
		bmem_free( data->values );
		data->values = 0;
		return;
	    }
	    size_t len = bmem_strlen( values );
	    if( data->values == 0 )
	    {
		data->values = (utf8_char*)bmem_new( len + 1 );
	    }
	    else
	    {
		if( len + 1 > bmem_get_size( data->values ) )
		{
		    data->values = (utf8_char*)bmem_resize( data->values, len + 1 );
		}
	    }
	    const utf8_char* src = values;
	    utf8_char* dest = data->values;
	    bool slash = 0;
	    while( 1 )
	    {
		utf8_char c = *src;
		*dest = c;
		if( c == '/' ) 
		{
		    if( slash )
		    {
			dest--;
			*dest = 1;
			slash = 0;
		    }
		    else
		    {
			slash = 1;
		    }
		}
		else
		{
		    slash = 0;
		}
		if( c == 0 ) break;
		dest++;
		src++;
	    }
	}
    }
}

void scrollbar_set_showing_offset( WINDOWPTR win, int offset, window_manager* wm )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{
	    data->show_offset = offset;
	}
    }
}

void scrollbar_set_hex_mode( WINDOWPTR win, scrollbar_hex hex, window_manager* wm )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{
	    data->hex = hex;
	}
    }
}

void scrollbar_set_normal_value( WINDOWPTR win, int normal_value, window_manager* wm )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{
	    data->normal_value = normal_value;
	}
    }
}

void scrollbar_set_flags( WINDOWPTR win, uint flags )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{
	    data->flags = flags;
	}
    }
}

uint scrollbar_get_flags( WINDOWPTR win )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	if( data )
	{	
	    return data->flags;
	}
    }
    return 0;
}

void scrollbar_call_handler( WINDOWPTR win )
{
    if( win == 0 ) return;
    window_manager* wm = win->wm;
    scrollbar_data* data = (scrollbar_data*)win->data;
    if( data->begin == 0 )
    {
	data->begin = 1;
	if( data->begin_handler )
	    data->begin_handler( data->begin_handler_data, win, scrollbar_get_value( win, wm ) );
    }
    if( win->action_handler )
	win->action_handler( win->handler_data, win, wm );
    if( data->begin )
    {
	data->evt_flags = 0;
	if( data->end_handler )
	    data->end_handler( data->end_handler_data, win, scrollbar_get_value( win, wm ) );
	data->begin = 0;
    }
}

bool scrollbar_get_editing_state( WINDOWPTR win )
{
    if( win )
    {
	scrollbar_data* data = (scrollbar_data*)win->data;
	return data->begin;
    }
    return 0;
}
