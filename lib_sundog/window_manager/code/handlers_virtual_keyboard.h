/*
    handlers_virtual_keyboard.h.
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

struct kbd_data
{
    int 		mx, my;
    int 		selected_key;
    int 		shift;
    WINDOWPTR		send_event_to;
    int			result;
};

#define XKEYS 14
#define YKEYS 4
#define KEYSCALE 1.5

const utf8_char* kbd_text1[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "BCK", "~" };
const utf8_char* kbd__text1[]= { "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "BCK", "~" };
int    kbd_key1[] =            { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', KEY_BACKSPACE };
int    kbd__key1[] =           { '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', KEY_BACKSPACE };
int  kbd_texts1[] =            { 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   3 };

const utf8_char* kbd_text2[] = { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "=", "SHIFT", "~" };
const utf8_char* kbd__text2[] ={ "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "+", "SHIFT", "~" };
int    kbd_key2[] =            { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '=', 4 };
int    kbd__key2[] =           { 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '+', 4 };
int  kbd_texts2[] =            { 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   3 };

const utf8_char* kbd_text3[] = { "a", "s", "d", "f", "g", "h", "j", "k", "l", ":", "|", "CANCEL", "~" };
const utf8_char* kbd__text3[] ={ "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "\"", "CANCEL", "~" };
int    kbd_key3[] =            { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '|', 2 };
int    kbd__key3[] =           { 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '"', 2 };
int  kbd_texts3[] =            { 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   3 };

const utf8_char* kbd_text4[] = { "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "SP", "ENTER", "~" };
const utf8_char* kbd__text4[] ={ "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "SP", "ENTER", "~" };
int    kbd_key4[] =            { 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', ' ', 3 };
int    kbd__key4[] =           { 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', ' ', 3 };
int  kbd_texts4[] =            { 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   3 };

void kbd_draw_line( const utf8_char** text, int* size, int* key_codes, int y, int mx, int my, kbd_data* data, WINDOWPTR win, window_manager* wm )
{
    int cell_xsize = ( win->xsize << 15 ) / XKEYS;
    int cell_ysize = win->ysize / YKEYS;
    int p = 0;
    int x = 0;
    y += ( win->ysize - ( cell_ysize * 4 ) ) / 2;
    while( 1 )
    {
	const utf8_char* str = text[ p ];
	if( str[ 0 ] == '~' ) break;
	int button_size = cell_xsize * size[ p ];
	int button_size2 = ( ( x + button_size ) >> 15 ) - ( x >> 15 );
	
	//Bounds correction:
	if( ( x >> 15 ) + button_size2 + ( cell_xsize >> 15 ) > win->xsize )
	    button_size2 = win->xsize - ( x >> 15 ) + 1;
	if( y + cell_ysize + 4 > win->ysize )
	    cell_ysize = win->ysize - y + 1;
	
	int str_size = string_x_size( str, wm );
	int light = 0;
	int kc = key_codes[ p ];
	if( kc == 4 && data->shift )
	    light = 1;
	if( mx >= (x>>15) && mx < (x>>15) + button_size2 &&
	    my >= y && my < y + cell_ysize )
	{
	    data->selected_key = (unsigned)key_codes[ p ];
	    light = 1;
	}
	if( light )
	{
	    draw_vgradient( ( x >> 15 ), y, button_size2 - 1, cell_ysize - 1, wm->color2, 255, 150, wm );
	}
	else 
	{
	    if( kc == 2 || kc == 3 )
		draw_vgradient( ( x >> 15 ), y, button_size2 - 1, cell_ysize - 1, wm->color2, 150, 100, wm );
	    else
		draw_vgradient( ( x >> 15 ), y, button_size2 - 1, cell_ysize - 1, wm->color2, 100, 64, wm );
	}
	wm->cur_font_color = wm->color3;
	draw_string( 
	    str, 
	    ( x >> 15 ) + ( button_size2 - str_size ) / 2, 
	    y + ( cell_ysize - char_y_size( wm ) ) / 2,
	    wm );
	x += button_size;
	p++;
    }
}

int keyboard_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    kbd_data* data = (kbd_data*)win->data;
    int cell_ysize = win->ysize / YKEYS;
    int rx = evt->x - win->screen_x;
    int ry = evt->y - win->screen_y;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( kbd_data );
	    break;
	case EVT_AFTERCREATE:
	    data->mx = -1;
	    data->my = -1;
	    data->selected_key = 0;
	    data->shift = 0;
	    data->send_event_to = 0;
	    data->result = 0;
	    retval = 1;
	    break;
	case EVT_DRAW:
	    {
		wbd_lock( win );

		wm->cur_opacity = 255;
                if( ( win->xsize / XKEYS ) * 3 >= string_x_size( "######", wm ) * 2 &&
                    win->ysize / YKEYS >= char_y_size( wm ) * 2 )
		    wm->cur_font_scale = 512;
		else
		    wm->cur_font_scale = 256;
		draw_frect( 0, 0, win->xsize, win->ysize, win->color, wm );

		int prev_selected_key = data->selected_key;
		data->selected_key = 0;
		if( data->shift == 0 )
		{
		    kbd_draw_line( kbd_text1, kbd_texts1, kbd_key1, 0, data->mx, data->my, data, win, wm );
		    kbd_draw_line( kbd_text2, kbd_texts2, kbd_key2, cell_ysize, data->mx, data->my, data, win, wm );
		    kbd_draw_line( kbd_text3, kbd_texts3, kbd_key3, cell_ysize * 2, data->mx, data->my, data, win, wm );
		    kbd_draw_line( kbd_text4, kbd_texts4, kbd_key4, cell_ysize * 3, data->mx, data->my, data, win, wm );
		}
		else
		{
		    kbd_draw_line( kbd__text1, kbd_texts1, kbd__key1, 0, data->mx, data->my, data, win, wm );
		    kbd_draw_line( kbd__text2, kbd_texts2, kbd__key2, cell_ysize, data->mx, data->my, data, win, wm );
		    kbd_draw_line( kbd__text3, kbd_texts3, kbd__key3, cell_ysize * 2, data->mx, data->my, data, win, wm );
		    kbd_draw_line( kbd__text4, kbd_texts4, kbd__key4, cell_ysize * 3, data->mx, data->my, data, win, wm );
		}
		wm->cur_opacity = BORDER_OPACITY;
		draw_frect( 0, 0, 1, win->ysize, BORDER_COLOR_WITHOUT_OPACITY, wm );
		draw_frect( win->xsize - 1, 0, 1, win->ysize, BORDER_COLOR_WITHOUT_OPACITY, wm );
		draw_frect( 0, win->ysize - 1, win->xsize, 1, BORDER_COLOR_WITHOUT_OPACITY, wm );

		wbd_draw( wm );
		wbd_unlock( wm );
	    
		if( prev_selected_key != data->selected_key )
		{
		    if( data->selected_key > 0x20 && data->selected_key < 127 )
		    {
			if( data->send_event_to )
			{
			    send_event( data->send_event_to, EVT_USERDEFINED1, 0, 0, 0, data->selected_key, 0, 1024, 0, wm );
			}
		    }
		    else
		    {
			if( data->send_event_to )
			{
			    send_event( data->send_event_to, EVT_USERDEFINED1, 0, 0, 0, 0, 0, 1024, 0, wm );
			}
		    }
		}
		
		retval = 1;
	    }
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	    data->mx = rx;
	    data->my = ry;
	    draw_window( win, wm );
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONUP:
	    if( data->selected_key )
	    {
		if( data->selected_key == 2 || data->selected_key == 3 )
		{
		    //Hide keyboard:
		    data->result = data->selected_key;
		    hide_keyboard_for_text_window( wm );
		}
		else if( data->selected_key == 4 )
		{
		    //Shift:
		    data->shift ^= 1;
		    data->mx = -1;
		    data->my = -1;
		    draw_window( win, wm );
		}
		else
		{
		    if( data->send_event_to )
		    {
			set_focus_win( data->send_event_to, wm );
			int k = data->selected_key;
			uint flags = 0;
			if( k >= 0x41 &&
			    k <= 0x5A &&
			    data->shift )
			{
			    k += 0x20;
			    flags |= EVT_FLAG_SHIFT;
			}
			send_event( data->send_event_to, EVT_BUTTONDOWN, flags, 0, 0, data->selected_key, 0, 1024, 0, wm );
			send_event( data->send_event_to, EVT_BUTTONUP, flags, 0, 0, data->selected_key, 0, 1024, 0, wm );
		    }
		    data->mx = -1;
		    data->my = -1;
		    draw_window( win, wm );
		}
	    }
	    data->mx = -1;
	    data->my = -1;
	    data->selected_key = 0;
	    retval = 1;
	    break;
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
	case EVT_BEFORESHOW:
	case EVT_SCREENRESIZE:
	    {
		WINDOWPTR p = win->parent;
		
		int xsize;
		int ysize;
		int x;
		int y;
		
		xsize = XKEYS * (int)( (float)wm->scrollbar_size * KEYSCALE );
		ysize = ( YKEYS + 1 ) * (int)( (float)wm->scrollbar_size * KEYSCALE );
		if( xsize > wm->screen_xsize ) xsize = wm->screen_xsize;
		if( ysize > wm->screen_ysize ) ysize = wm->screen_ysize;
		x = ( wm->screen_xsize - xsize ) / 2;
		y = ( wm->screen_ysize - ysize ) / 2;
		
		p->x = x;
		p->y = y;
		p->xsize = xsize;
		p->ysize = ysize;
		
		//Text edit:
		int text_ysize = ysize / ( YKEYS + 1 );
		p->childs[ 0 ]->xsize = xsize - 2;
		p->childs[ 0 ]->ysize = text_ysize - 1;
		wbd_lock( win );
		int ychar = char_y_size( wm );
		int ssize = string_x_size( "######", wm );
		wbd_unlock( wm );
                if( ( xsize / XKEYS ) * 3 >= ssize * 2 && 
                    ysize / ( YKEYS + 1 ) >= ychar * 2 )
                    text_set_zoom( p->childs[ 0 ], 512, wm );
                else
                    text_set_zoom( p->childs[ 0 ], 256, wm );

		//Keyboard:
		p->childs[ 1 ]->y = text_ysize;
		p->childs[ 1 ]->xsize = xsize;
		p->childs[ 1 ]->ysize = ysize - text_ysize;
	    }
	    retval = 1;
	    break;
    }
    return retval;
}

int keyboard_bg_handler( sundog_event* evt, window_manager* wm )
{
    //if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    switch( evt->type )
    {
	case EVT_AFTERCREATE:
	    retval = 1;
	    break;
	case EVT_BEFORESHOW:
	case EVT_SCREENRESIZE:
	    {
		WINDOWPTR p = win->parent;
		win->x = 0;
		win->y = 0;
		win->xsize = p->xsize;
		win->ysize = p->ysize;
	    }
	    retval = 1;
	    break;
    }
    return retval;
}

void hide_keyboard_for_text_window( window_manager* wm )
{
    if( wm->vk_win )
    {
	hide_window( wm->vk_win, wm );
    }
}

void show_keyboard_for_text_window( WINDOWPTR text, window_manager* wm )
{
    if( wm->show_virtual_keyboard == 0 ) return;
    
    if( wm->vk_win ) return;
    
    WINDOWPTR bg = new_window( "KBD BG", 0, 0, 10, 10, wm->color1, wm->root_win, keyboard_bg_handler, wm );
    show_window( bg, wm );
    
    wm->vk_win = new_window( "KBD", 0, 0, 10, 10, BORDER_COLOR( wm->color1 ), wm->root_win, null_handler, wm );
    wm->opt_text_no_virtual_keyboard = true;
    WINDOWPTR edit_win = new_window( "kbd text", 1, 1, 10, 10, wm->text_background, wm->vk_win, text_handler, wm );
    utf8_char* text_source = text_get_text( text, wm );
    if( text_source )
    {
	text_set_text( edit_win, text_source, wm );
	text_set_cursor_position( edit_win, text_get_cursor_position( text, wm ), wm );
	set_focus_win( edit_win, wm );
    }
    WINDOWPTR keys = new_window( 
	"kbd", 
	0, 0, 10, 10,
	wm->dialog_color, 
	wm->vk_win, 
	keyboard_handler, 
	wm );
    keys->flags |= WIN_FLAG_ALWAYS_UNFOCUSED;
    kbd_data* kdata = (kbd_data*)keys->data;
    kdata->send_event_to = edit_win;
    show_window( wm->vk_win, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    while( 1 )
    {
	sundog_event evt;
	EVENT_LOOP_BEGIN( &evt, wm );
	if( EVENT_LOOP_END( wm ) ) break;
	if( wm->vk_win->visible == 0 ) break;
	if( keys->visible == 0 ) break;
	if( text->visible == 0 ) break;
    }
    if( kdata->result == 3 )
    {
	//ENTER pressed:
	text_set_text( text, text_get_text( edit_win, wm ), wm );
	text_changed( text, wm );
    }
    remove_window( wm->vk_win, wm ); wm->vk_win = 0;
    remove_window( bg, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
}
