/*
    handlers_dialog.h.
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

struct dialog_data
{
    WINDOWPTR win;
    const utf8_char* text;
    int* result;
    utf8_char** buttons_text;
    WINDOWPTR* buttons;
    int buttons_num;
    dialog_item* items;
    int timer;
    ticks_t timer_start;
    COLOR base_color;
};

int dialog_item_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    dialog_data* data = (dialog_data*)user_data;
    if( data->items )
    {
	int i = 0;
	while( 1 )
	{
	    if( data->items[ i ].type == DIALOG_ITEM_NONE )
		break;
		
	    if( data->items[ i ].win == win )
	    {
		switch( data->items[ i ].type )
		{
		    case DIALOG_ITEM_NUMBER:
		    case DIALOG_ITEM_NUMBER_HEX:
			data->items[ i ].int_val = text_get_value( win, wm );
			break;
		    case DIALOG_ITEM_SLIDER:
			data->items[ i ].int_val = scrollbar_get_value( win, wm ) + data->items[ i ].min;
			break;
		    case DIALOG_ITEM_TEXT:
			{
			    if( data->items[ i ].str_val )
				bmem_free( data->items[ i ].str_val );
			    utf8_char* str = text_get_text( win, wm );
			    if( str )
			    {
				int len = bmem_get_size( str );
				data->items[ i ].str_val = (utf8_char*)bmem_new( len );
				bmem_copy( data->items[ i ].str_val, str, len );
			    }
			    else
			    {
				data->items[ i ].str_val = 0;
			    }
			}
			break;
		    case DIALOG_ITEM_POPUP:
			data->items[ i ].int_val = button_get_menu_val( win, wm );
			break;
		}
		break;
	    }
	    
	    i++;
	}
    }
    return 0;
}

int dialog_button_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    dialog_data* data = (dialog_data*)user_data;
    if( data->result ) 
    {
	int i;
	for( i = 0; i < data->buttons_num; i++ )
	{
	    if( data->buttons[ i ] == win )
	    {
		*data->result = i;
		break;
	    }
	}
	if( i == data->buttons_num )
	{
	    *data->result = -1;
	}
    }
    remove_window( data->win, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    return 0;
}

void dialog_timer( void* user_data, sundog_timer* timer, window_manager* wm )
{
    dialog_data* data = (dialog_data*)user_data;
    ticks_t t = time_ticks() - data->timer_start;
    t = ( t * 256 ) / time_ticks_per_second() + 256 + 128;
    int v = g_hsin_tab[ t & 255 ];
    if( t & 256 ) v = -v;
    v = v / 2 + 128;
    v /= 2;
    data->win->color = blend( data->base_color, wm->color2, v );
    for( int i = 0; i < data->buttons_num; i++ )
    {
	data->buttons[ i ]->color = blend( wm->button_color, wm->color2, v );
    }
    draw_window( data->win->parent, wm );
}

int dialog_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    dialog_data* data = (dialog_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( dialog_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		WINDOWPTR focus_on_item = 0;
		
		data->win = win;
		data->text = wm->opt_dialog_text;
		data->result = wm->opt_dialog_result_ptr;
		data->items = wm->opt_dialog_items;

		int but_ysize = wm->button_ysize;
	        int but_xsize = wm->button_xsize;
		
		data->timer = -1;
		if( data->text && data->text[ 0 ] == '!' )
		{
		    data->text++;
		    data->timer = add_timer( dialog_timer, data, 0, wm );
		    data->timer_start = time_ticks();
		    data->base_color = win->color;
		}

		if( data->items )
		{
		    int i = 0;
		    int y = wm->interelement_space;
		    while( 1 )
		    {
			dialog_item* item = &data->items[ i ];
			
			if( item->type == DIALOG_ITEM_NONE )
			    break;
			
			WINDOWPTR w = 0;
			switch( item->type )
			{
			    case DIALOG_ITEM_NUMBER:
			    case DIALOG_ITEM_NUMBER_HEX:
				if( item->type == DIALOG_ITEM_NUMBER )
				    wm->opt_text_numeric = 1;
				else
				    wm->opt_text_numeric = 2;
				wm->opt_text_num_min = item->min;
				wm->opt_text_num_max = item->max;
				wm->opt_text_call_handler_on_any_changes = true;
				w = new_window( "d.item.num", 0, y, 8, DIALOG_ITEM_NUMBER_SIZE, wm->text_background, win, text_handler, wm );
				text_set_value( w, item->int_val, wm );
				y += DIALOG_ITEM_NUMBER_SIZE + wm->interelement_space;
				break;
			    case DIALOG_ITEM_SLIDER:
				wm->opt_scrollbar_compact = true;
				w = new_window( "d.item.slider", 0, y, 8, DIALOG_ITEM_SLIDER_SIZE, wm->scroll_color, win, scrollbar_handler, wm );
				w->font = 0;
				scrollbar_set_name( w, item->str_val, wm );
            			scrollbar_set_parameters( w, item->int_val - item->min, item->max - item->min, 1, 1, wm );
            			scrollbar_set_showing_offset( w, item->min, wm );
            			scrollbar_set_normal_value( w, item->normal_val - item->min, wm );
				y += DIALOG_ITEM_SLIDER_SIZE + wm->interelement_space;
				break;
			    case DIALOG_ITEM_TEXT:
				wm->opt_text_call_handler_on_any_changes = true;
				w = new_window( "d.item.txt", 0, y, 8, DIALOG_ITEM_TEXT_SIZE, wm->text_background, win, text_handler, wm );
				if( item->str_val )
				{
				    text_set_text( w, (const utf8_char*)item->str_val, wm );
				    item->str_val = bmem_strdup( item->str_val );
				}
				y += DIALOG_ITEM_TEXT_SIZE + wm->interelement_space;
				break;
			    case DIALOG_ITEM_LABEL:
				w = new_window( (const utf8_char*)item->str_val, 0, y, 8, DIALOG_ITEM_LABEL_SIZE, win->color, win, label_handler, wm );
				y += DIALOG_ITEM_LABEL_SIZE + wm->interelement_space;
				break;
			    case DIALOG_ITEM_POPUP:
				wm->opt_button_flat = 1;
				wm->opt_button_flags = BUTTON_FLAG_SHOW_VALUE;
				w = new_window( (const utf8_char*)item->str_val, 0, y, 8, DIALOG_ITEM_POPUP_SIZE, wm->button_color, win, button_handler, wm );
				button_set_menu( w, item->menu, wm );
				button_set_menu_val( w, item->int_val, wm );
				y += DIALOG_ITEM_POPUP_SIZE + wm->interelement_space;
				break;
			}
			set_window_controller( w, 0, wm, (WCMD)wm->interelement_space, CEND );
			set_window_controller( w, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
			set_handler( w, dialog_item_handler, data, wm );
			item->win = w;
			
			if( item->flags & DIALOG_ITEM_FLAG_FOCUS )
			    focus_on_item = w;
			    
			i++;
		    }
		}
		
		data->buttons_text = 0;
		if( wm->opt_dialog_buttons_text )
		{
		    int buttons_num = 1;
		    int i, p, len;
		    int slen = bmem_strlen( wm->opt_dialog_buttons_text );
		    for( i = 0; i < slen; i++ )
		    {
			if( wm->opt_dialog_buttons_text[ i ] == ';' )
			    buttons_num++;
		    }
		    data->buttons_text = (utf8_char**)bmem_new( sizeof( utf8_char* ) * buttons_num );
		    data->buttons = (WINDOWPTR*)bmem_new( sizeof( WINDOWPTR ) * buttons_num );
		    data->buttons_num = buttons_num;
		    bmem_set( data->buttons_text, sizeof( utf8_char* ) * buttons_num, 0 );
		    int word_start = 0;
		    for( i = 0; i < buttons_num; i++ )
		    {
			len = 0;
			while( 1 )
			{
			    if( wm->opt_dialog_buttons_text[ word_start + len ] == 0 || wm->opt_dialog_buttons_text[ word_start + len ] == ';' )
			    {
				break;
			    }
			    len++;
			}
			if( len > 0 )
			{
			    data->buttons_text[ i ] = (utf8_char*)bmem_new( len + 1 );
			    for( p = word_start; p < word_start + len; p++ )
			    {
				data->buttons_text[ i ][ p - word_start ] = wm->opt_dialog_buttons_text[ p ];
			    }
			    data->buttons_text[ i ][ p - word_start ] = 0;
			}
			word_start += len + 1;
			if( word_start >= slen ) break;
		    }
		    
		    //Create buttons:
		    int xoff = wm->interelement_space;
		    for( i = 0; i < buttons_num; i++ )
		    {
		        but_xsize = button_get_optimal_xsize( data->buttons_text[ i ], win->font, false, wm );
			xoff += but_xsize + wm->interelement_space2;
		    }
		    bool smallest = false;
		    int xsize = win->xsize;
		    if( xsize > wm->screen_xsize )
			xsize = wm->screen_xsize;
		    if( xoff >= xsize ) 
			smallest = true;
		    xoff = wm->interelement_space;
		    for( i = 0; i < buttons_num; i++ )
		    {
		        but_xsize = button_get_optimal_xsize( data->buttons_text[ i ], win->font, smallest, wm );
			data->buttons[ i ] = new_window( data->buttons_text[ i ], xoff, 0, but_xsize, 1, wm->button_color, win, button_handler, wm );
			set_handler( data->buttons[ i ], dialog_button_handler, data, wm );
			set_window_controller( data->buttons[ i ], 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
			set_window_controller( data->buttons[ i ], 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)but_ysize + wm->interelement_space, CEND );
			xoff += but_xsize + wm->interelement_space2;
		    }
		}

		wm->opt_dialog_items = 0;
		wm->opt_dialog_buttons_text = 0;
		wm->opt_dialog_text = 0;
		wm->opt_dialog_result_ptr = 0;

		//SET FOCUS:
		if( focus_on_item )
		    set_focus_win( focus_on_item, wm );
		else		
		    set_focus_win( win, wm );
	    }
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    if( data->buttons_text )
	    {
		for( int i = 0; i < data->buttons_num; i++ )
		{
		    if( data->buttons_text[ i ] )
		    {
			bmem_free( data->buttons_text[ i ] );
		    }
		}
		bmem_free( data->buttons_text );
	    }
	    if( data->buttons )
	    {
		bmem_free( data->buttons );
	    }
	    remove_timer( data->timer, wm );
	    retval = 1;
	    break;
	case EVT_DRAW:
	    wbd_lock( win );
	    wm->cur_font_color = wm->color3;
    	    draw_frect( 0, 0, win->xsize, win->ysize, win->color, wm );
	    if( data->text )
	    {
	        draw_string_wordwrap( data->text, wm->interelement_space, wm->interelement_space, win->xsize - wm->interelement_space * 2, 0, 0, 0, wm );
	    }
	    wbd_draw( wm );
	    wbd_unlock( wm );
	    retval = 1;
	    break;
	case EVT_BUTTONDOWN:
	    if( evt->key == KEY_ESCAPE )
	    {
		dialog_button_handler( data, 0, wm );
		retval = 1;
	    }
	    if( evt->key == 'y' || evt->key == KEY_ENTER )
	    {
		if( data->buttons_num < 3 )
		{
		    dialog_button_handler( data, data->buttons[ 0 ], wm );
		    retval = 1;
		}
	    }
	    if( evt->key == 'n' )
	    {
		if( data->buttons_num < 3 )
		{
		    dialog_button_handler( data, data->buttons[ 1 ], wm );
		    retval = 1;
		}
	    }
	    break;
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
