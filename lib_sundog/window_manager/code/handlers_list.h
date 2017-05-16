/*
    handlers_list.h.
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

struct wlist_data
{
    WINDOWPTR win;
    list_data* list;
    int prev_selected;
    WINDOWPTR scrollbar;
    int ychar_size;
    int ychars;
    int work_area;
    int last_action;	//0 - none
    int numbered_size;
    bool size_not_defined;
    bool double_click_item_selected;
    bool show_selection;
    int pressed; //1 - left key pressed; 2 - right key pressed;
    
    int timer;
    int scroll_speed;
    bool scroll_direction;
    int scroll_start;
    ticks_t scroll_start_time;
    
    bool numbered;
    bool no_extension;
};

int list_scrollbar_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    wlist_data* data = (wlist_data*)user_data;
    data->list->start_item = scrollbar_get_value( win, wm );
    if( data->list->start_item < 0 ) data->list->start_item = 0;
    draw_window( data->win, wm );
    return 0;
}

void list_timer( void* user_data, sundog_timer* t, window_manager* wm )
{
    wlist_data* data = (wlist_data*)user_data;
    ticks_t tdelta = time_ticks() - data->scroll_start_time;
    if( data->scroll_direction )
    {
	//Down:
	int cur = data->scroll_start + ( data->scroll_speed * tdelta ) / time_ticks_per_second();
	data->list->start_item = cur;
    }
    else 
    {
	//Up:
	int cur = data->scroll_start - ( data->scroll_speed * tdelta ) / time_ticks_per_second();
	data->list->start_item = cur;
    }
    bool bound = 0;
    if( data->list->start_item < 0 ) 
    {
	data->list->start_item = 0;
	bound = 1;
    }
    if( data->list->start_item > 0 )
    {
	if( data->list->items_num * data->ychar_size < data->work_area )
	{
	    data->list->start_item = 0;
	    bound = 1;
	}
	else 
	{
	    if( -data->list->start_item + data->list->items_num * data->ychar_size < data->work_area )
	    {
		data->list->start_item = data->list->items_num * data->ychar_size - data->work_area;
		bound = 1;
	    }
	}
    }
    if( bound )
    {
	data->scroll_start = data->list->start_item;
	data->scroll_start_time = time_ticks();
    }
    draw_window( data->win, wm );
    t->deadline = time_ticks() + t->delay;
}

int list_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    wlist_data* data = (wlist_data*)win->data;
    int i;
    int ry = evt->y - win->screen_y;
    int xborder = 0;
    int yborder = 0;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( wlist_data );
	    break;
	case EVT_AFTERCREATE:
	    data->win = win;
	    data->list = (list_data*)bmem_new( sizeof( list_data ) );
	    list_init( data->list );
	    data->prev_selected = -999;
	    data->ychar_size = 0;
	    data->ychars = 0;
	    data->work_area = 0;
	    data->size_not_defined = 1;
	    data->double_click_item_selected = 0;
	    data->show_selection = 0;
	    data->pressed = 0;
	    data->timer = -1;
	    data->scroll_speed = 60;
	    //SCROLLBAR:
	    if( wm->opt_list_without_scrollbar )
	    {
		data->scrollbar = 0;
	    }
	    else 
	    {
		wm->opt_scrollbar_vertical = true;
		wm->opt_scrollbar_flat = true;
		data->scrollbar = new_window( "scroll", 1, 1, 1, 1, win->color, win, scrollbar_handler, wm );
		set_window_controller( data->scrollbar, 0, wm, CPERC, (WCMD)100, CEND );
		set_window_controller( data->scrollbar, 1, wm, (WCMD)0, CEND );
		set_window_controller( data->scrollbar, 2, wm, CPERC, (WCMD)100, CSUB, wm->scrollbar_size, CEND );
		set_window_controller( data->scrollbar, 3, wm, CPERC, (WCMD)100, CEND );
		set_handler( data->scrollbar, list_scrollbar_handler, data, wm );
	    }
	    data->numbered = wm->opt_list_numbered;
	    data->no_extension = wm->opt_list_without_extensions;
	    if( data->numbered )
	    {
		data->numbered_size = font_char_x_size( '9', win->font, wm ) * 2 + 2;
	    }
    	    wm->opt_list_numbered = false;
	    wm->opt_list_without_scrollbar = false;
	    wm->opt_list_without_extensions = false;
	    retval = 1;
	    break;
	case EVT_BUTTONDOWN:
	    {
		data->last_action = 0;
		int updown = 0;
		int step = 0;
		if( evt->key == KEY_ESCAPE ) { data->last_action = LIST_ACTION_ESCAPE; updown = 0; step = 0; retval = 1; }
		if( evt->key == KEY_ENTER ||
		    evt->key == ' ' ) { data->last_action = LIST_ACTION_ENTER; updown = 0; step = 0; retval = 1; }
		if( evt->key == KEY_UP ) { data->last_action = LIST_ACTION_UPDOWN; updown = 1; step = 1; retval = 1; }
		if( evt->key == KEY_PAGEUP ) { data->last_action = LIST_ACTION_UPDOWN; updown = 1; step = data->ychars; retval = 1; }
		if( evt->key == KEY_DOWN ) { data->last_action = LIST_ACTION_UPDOWN; updown = 2; step = 1; retval = 1; }
		if( evt->key == KEY_PAGEDOWN ) { data->last_action = LIST_ACTION_UPDOWN; updown = 2; step = data->ychars; retval = 1; }
		if( updown == 1 )
		{
		    data->list->selected_item -= step;
		    if( data->list->selected_item < 0 ) data->list->selected_item = 0;
		    if( data->list->selected_item * data->ychar_size < data->list->start_item )
		        data->list->start_item = data->list->selected_item * data->ychar_size;
		    draw_window( win, wm );
		}
		if( updown == 2 )
		{
		    data->list->selected_item += step;
		    if( data->list->selected_item >= data->list->items_num ) 
			data->list->selected_item = data->list->items_num - 1;
		    if( data->ychar_size )
		    {
		        if( -data->list->start_item + ( data->list->selected_item + 1 ) * data->ychar_size > data->work_area )
		            data->list->start_item = ( data->list->selected_item * data->ychar_size ) - ( data->work_area - data->ychar_size );
		    }
		    if( data->list->start_item < 0 ) data->list->start_item = 0;
		    draw_window( win, wm );
		}
		if( retval )
		{
		    win->action_result = data->list->selected_item;
		    if( win->action_handler )
			win->action_handler( win->handler_data, win, wm );
		}
	    }
	    break;
	case EVT_MOUSEBUTTONDOWN:
	    if( data->pressed == 0 )
	    {
		if( evt->key == MOUSE_BUTTON_LEFT )
		{
		    data->double_click_item_selected = 0;
		    data->pressed = 1;
		}
		if( evt->key == MOUSE_BUTTON_RIGHT )
		{
		    data->double_click_item_selected = 0;
		    data->pressed = 2;
		}
	    }
	case EVT_MOUSEMOVE:
	    if( data->pressed )
	    {
		if( data->pressed == 1 && evt->key != MOUSE_BUTTON_LEFT ) break;
	        if( data->pressed == 2 && evt->key != MOUSE_BUTTON_RIGHT ) break;
		if( data->ychar_size )
		{
		    int new_item = ( ( ry - yborder ) + data->list->start_item ) / data->ychar_size;
		    while( 1 )
		    {
			if( evt->type == EVT_MOUSEBUTTONDOWN )
			{
			    if( new_item >= data->list->items_num || new_item < 0 )
			    {
				break;
			    }
			}
			if( evt->type == EVT_MOUSEMOVE )
			{
			    if( data->pressed == 2 )
			    {
				break;
			    }
			}
		        data->list->selected_item = new_item;
			break;
		    }
		}
		if( data->list->selected_item < 0 ) 
		    data->list->selected_item = 0;
		if( data->list->selected_item >= data->list->items_num ) 
		    data->list->selected_item = data->list->items_num - 1;
		if( data->prev_selected != data->list->selected_item || evt->type == EVT_MOUSEBUTTONDOWN )
		{
		    //If lower win bound:
		    bool bound = false;
		    if( ry < yborder + data->ychar_size )
		    {
		        data->scroll_direction = 0;
		        if( data->timer == -1 )
		        {
			    data->scroll_start = data->list->start_item;
			    data->scroll_start_time = time_ticks();
			    data->timer = add_timer( list_timer, (void*)data, 0, wm );
			}
			bound = true;
		    }
		    //If higher win bound:
		    if( ry >= win->ysize - ( yborder + data->ychar_size ) )
		    {
		        data->scroll_direction = 1;
		        if( data->timer == -1 )
		        {
		    	    data->scroll_start = data->list->start_item;
			    data->scroll_start_time = time_ticks();
			    data->timer = add_timer( list_timer, (void*)data, 0, wm );
			}
			bound = true;
		    }
		    if( bound == false )
		    {
			if( data->timer >= 0 )
			{
			    remove_timer( data->timer, wm );
			    data->timer = -1;
			}
		    }
		    draw_window( win, wm );
		}
		if( ( evt->type == EVT_MOUSEBUTTONDOWN ) &&
		    ( data->prev_selected == data->list->selected_item ) && 
		    ( evt->flags & EVT_FLAG_DOUBLECLICK ) )
		{
		    //Double click:
		    data->double_click_item_selected = 1;
		}
		data->prev_selected = data->list->selected_item;
	    }
	    else if( evt->key == MOUSE_BUTTON_SCROLLUP )
	    {
		data->list->start_item -= data->ychar_size;
		if( data->list->start_item < 0 ) data->list->start_item = 0;
		draw_window( win, wm );
	    }
	    else if( evt->key == MOUSE_BUTTON_SCROLLDOWN )
	    {
		if( data->ychar_size )
		{
		    data->list->start_item += data->ychar_size;
		
		    if( data->list->start_item + data->work_area > data->list->items_num * data->ychar_size )
		        data->list->start_item = data->list->items_num * data->ychar_size - data->work_area;
		}
		if( data->list->start_item < 0 ) data->list->start_item = 0;
		draw_window( win, wm );
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONUP:
	    if( data->pressed )
	    {
		if( data->pressed == 1 && evt->key != MOUSE_BUTTON_LEFT ) break;
	        if( data->pressed == 2 && evt->key != MOUSE_BUTTON_RIGHT ) break;
		data->pressed = 0;
		if( data->timer >= 0 )
		{
		    remove_timer( data->timer, wm );
		}
		if( data->double_click_item_selected && 
		    ( evt->flags & EVT_FLAG_DOUBLECLICK ) )
		{
		    //Double click finished:
		    data->last_action = LIST_ACTION_DOUBLECLICK;
		}
		else
		{
		    //Single click finished
		    if( evt->key == MOUSE_BUTTON_LEFT )
			data->last_action = LIST_ACTION_CLICK;
		    else
			data->last_action = LIST_ACTION_RCLICK;
		}
		win->action_result = data->list->selected_item;
		data->double_click_item_selected = 0;
		if( data->list->selected_item >= 0 && data->list->selected_item < data->list->items_num )
		    if( win->action_handler )
		        win->action_handler( win->handler_data, win, wm );
	    }
	    retval = 1;
	    break;
	case EVT_UNFOCUS:
	    data->pressed = 0;
	    retval = 1;
	    break;
	case EVT_DRAW:
	    {
		wbd_lock( win );
		
		data->size_not_defined = 0;

		//Get item properties:
		data->ychar_size = wm->list_item_ysize;
		data->work_area = ( win->ysize - yborder * 2 );
		data->ychars = data->work_area / data->ychar_size;
		data->scroll_speed = data->ychar_size * 8;

		//If need to show selected item:
		if( data->show_selection )
		{
		    list_select_item( win, data->list->selected_item, true );
		    data->show_selection = 0;
		}
	    
		//Set scrollbar parameters:
		int page_size;
		int max_val;
		int step = data->ychar_size;
		page_size = data->work_area;
		max_val = data->list->items_num * data->ychar_size - page_size;
		if( max_val < 0 ) max_val = 0;
		if( data->scrollbar )
		{
		    scrollbar_set_parameters( data->scrollbar, data->list->start_item, max_val, page_size, step, wm );
		}
		
		//Fill window:
		draw_frect( 0, 0, win->xsize, win->ysize, win->color, wm );

		//Draw items:
		i = data->list->start_item / data->ychar_size;
		int y = -data->list->start_item + i * data->ychar_size;
		y += yborder;
		for( ; y < win->ysize; y += data->ychar_size, i++ )
		{
		    if( (unsigned)i < (unsigned)data->list->items_num )
		    {
		        wm->cur_font_color = wm->color3;
		        if( list_get_attr( i, data->list ) == 1 ) 
		        {
			    wm->cur_font_color = wm->alternative_text_color;
			    if( wm->color1_darkness < 64 )
			    {
			        wm->cur_font_color = blend( wm->alternative_text_color, wm->color3, 128 );
			    }
			}
			if( i & 1 ) 
			{
			    draw_frect( xborder, y, win->xsize - xborder * 2, data->ychar_size, blend( win->color, wm->color3, 16 ), wm );
			}
			if( data->list->selected_item == i )
			{
			    draw_frect( xborder, y, win->xsize - xborder * 2, data->ychar_size, blend( wm->color2, wm->color1, 32 ), wm );
			    wm->cur_font_color = wm->color0;
			}
    
			utf8_char* item_str = list_get_item( i, data->list );
			int tab = -1;
			int len = (int)bmem_strlen( item_str );
			for( int l = len - 1; l >= 0; l-- )
			{
			    if( item_str[ l ] == '\t' )
			    {
			        len = l;
			        tab = l + 1;
			        break;
			    }
			}
			if( len > 0 )
			{
			    if( data->no_extension )
			    {
			        for( int l = len - 1; l >= 0; l-- )
			        {
			    	    if( item_str[ l ] == '.' ) 
				    {
				        len = l;
				        break;
				    }
    				}
    			    }
    			    if( len > 0 )
    			    {
			        draw_string_limited( item_str, xborder, y + ( data->ychar_size - char_y_size( wm ) ) / 2, len, wm );
				if( tab >= 0 )
				{					
				    int column2_size = string_x_size( item_str + tab, wm );
				    int xx = win->xsize - xborder - column2_size;
				    if( data->scrollbar ) xx -= wm->scrollbar_size;
				    if( xx > xborder + string_x_size_limited( item_str, len, wm ) )
				    {
				        wm->cur_opacity = 128;
				        draw_string( item_str + tab, xx, y + ( data->ychar_size - char_y_size( wm ) ) / 2, wm );
				        wm->cur_opacity = 255;
				    }
				}
			    }
			}
		    }
		}
	    
		wbd_draw( wm );
		wbd_unlock( wm );

		retval = 1;
	    }
	    break;
	case EVT_BEFORECLOSE:
	    if( data->list ) 
	    { 
		list_close( data->list ); 
		bmem_free( data->list ); 
	    }
	    if( data->timer >= 0 )
	    {
		remove_timer( data->timer, wm );
		data->timer = -1;
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

list_data* list_get_data( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
	wlist_data* data = (wlist_data*)win->data;
	return data->list;
    }
    return 0;
}

int list_get_last_action( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
	wlist_data* data = (wlist_data*)win->data;
	return data->last_action;
    }
    return 0;
}

void list_select_item( WINDOWPTR win, int item_num, bool make_it_visible )
{
    if( win )
    {
	wlist_data* data = (wlist_data*)win->data;
	list_data* ldata = (list_data*)data->list;

	if( item_num >= 0 && item_num < ldata->items_num )
	{
	    ldata->selected_item = item_num;
	    if( data->size_not_defined )
	    {
		data->show_selection = 1;
		return;
	    }
	    if( make_it_visible )
	    {
		ldata->start_item = item_num * data->ychar_size + data->ychar_size / 2 - ( data->ychars * data->ychar_size ) / 2;
		if( ( ldata->start_item + data->work_area ) / data->ychar_size >= ldata->items_num ) 
	    	    ldata->start_item = ldata->items_num * data->ychar_size - data->work_area;
		if( ldata->start_item < 0 ) ldata->start_item = 0;
	    }
	}
    }
}

int list_get_selected_item( WINDOWPTR win )
{
    if( win )
    {
	wlist_data* data = (wlist_data*)win->data;
	list_data* ldata = (list_data*)data->list;
	return ldata->selected_item;
    }
    return -1;
}

int list_get_pressed( WINDOWPTR win ) //1 - left mouse button; 2 - right; 0 - nothing
{
    if( win )
    {
	wlist_data* data = (wlist_data*)win->data;
	return data->pressed;
    }
    return 0;
}
