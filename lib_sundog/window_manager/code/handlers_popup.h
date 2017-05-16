/*
    handlers_popup.h.
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

#define POPUP_MAX_LINES			128
#define POPUP_ITEM_FLAG_SHADED		( 1 << 0 ) // ...@
#define POPUP_ITEM_FLAG_BOLD		( 1 << 1 ) // ...@@
#define POPUP_ITEM_FLAG_TWO_SECTIONS	( 1 << 2 ) // ...|...

struct popup_data
{
    WINDOWPTR win;
    utf8_char* text;
    int16 lines[ POPUP_MAX_LINES ];
    uchar line_flags[ POPUP_MAX_LINES ];
    int lines_num;
    int first_section_size;
    int cols; //Columns
    int16 col_xsize[ 8 ];
    int16 col_ysize[ 8 ];
    int current_selected;
    WINDOWPTR prev_focus;
    int* exit_flag;
    int* result;
};

int popup_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    popup_data* data = (popup_data*)win->data;
    int popup_border = 2;
    int rx = evt->x - win->screen_x;
    int ry = evt->y - win->screen_y;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( popup_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		//Set focus:
		data->prev_focus = wm->focus_win;
		set_focus_win( win, wm );

		//Data init:
		data->win = win;
	    	data->text = 0;
		data->current_selected = -1;
		data->first_section_size = 0;
		win->action_result = -1;
		bmem_set( data->line_flags, sizeof( data->line_flags ), 0 );

		//Set window size:
		int xsize = 0;
		int ysize = 0;
		int x = win->x;
		int y = win->y;
		int name_xsize = 0;
		int name_ysize = 0;
		if( win->name && win->name[ 0 ] != 0 )
		{
		    name_xsize = font_string_x_size( win->name, win->font, wm ) + 1;
		    name_ysize = wm->list_item_ysize;
		}
		data->cols = 1;
		data->col_xsize[ 0 ] = 0;
		data->col_ysize[ 0 ] = name_ysize;
		if( wm->opt_popup_text )
		{
		    //Text popup:
		    data->text = (utf8_char*)bmem_strdup( wm->opt_popup_text );
		    data->lines_num = 0;
		    data->lines[ 0 ] = 0;
		    int nl_code = 0;
		    int text_size = bmem_strlen( data->text );
		    int line_start = 0;
		    for( int i = 0; i <= text_size; i++ )
		    {
			utf8_char c = data->text[ i ];
			if( c == '|' )
			{
			    int ss = font_string_x_size_limited( data->text + line_start, i - line_start + 1, win->font, wm );
			    if( ss > data->first_section_size )
				data->first_section_size = ss;
			    data->line_flags[ data->lines_num ] |= POPUP_ITEM_FLAG_TWO_SECTIONS; 
			}
			if( c == 0xA || c == 0 )
			{
			    data->text[ i ] = 0;
			    if( i - 1 >= 0 && data->text[ i - 1 ] == '@' )
			    {
				data->text[ i - 1 ] = 0; 
				data->line_flags[ data->lines_num ] |= POPUP_ITEM_FLAG_SHADED;
			    }
			    if( i - 2 >= 0 && data->text[ i - 2 ] == '@' )
			    {
				data->text[ i - 2 ] = 0; 
				data->line_flags[ data->lines_num ] |= POPUP_ITEM_FLAG_BOLD;
			    }
			    data->lines[ data->lines_num ] = line_start; //Save the start of this line
			    //Set X size:
			    int cur_x_size = font_string_x_size( data->text + line_start, win->font, wm );
                            if( cur_x_size > data->col_xsize[ data->cols - 1 ] ) data->col_xsize[ data->cols - 1 ] = cur_x_size + 1;
                            //Set Y size:
			    data->col_ysize[ data->cols - 1 ] += wm->list_item_ysize;
			    if( data->col_ysize[ data->cols - 1 ] > wm->screen_ysize - popup_border * 2 - wm->list_item_ysize )
                            {
                                data->col_xsize[ data->cols ] = 0;
                                data->col_ysize[ data->cols ] = name_ysize;
                                data->cols++;
                            }
			    //Go to the next string:
			    while( 1 )
			    {
				i++;
				if( i >= text_size + 1 ) break;
				line_start = i;
				if( data->text[ i ] != 0xA && data->text[ i ] != 0xD && data->text[ i ] != 0 && data->text[ i ] != '@' )
				    break;
			    }
			    data->lines_num++;
			    if( data->lines_num >= POPUP_MAX_LINES )
				break;
			}
		    }
		    for( int i = 0; i < data->cols; i++ ) 
		    {
			xsize += data->col_xsize[ i ];
		    }
		    if( xsize < name_xsize ) 
		    {
			data->col_xsize[ data->cols - 1 ] += name_xsize - xsize;
			xsize = name_xsize;
		    }
		    xsize += ( data->cols - 1 );
		    ysize += data->col_ysize[ 0 ];
		}
		xsize += popup_border * 2;
		ysize += popup_border * 2;
		if( xsize < wm->scrollbar_size * 4 && data->cols == 1 )
		{
		    xsize = wm->scrollbar_size * 4;
		    data->col_xsize[ 0 ] = xsize;
		}

		//Control window position:
		if( x + xsize > win->parent->xsize && x > 0 ) x -= ( x + xsize ) - win->parent->xsize;
		if( y + ysize > win->parent->ysize && y > 0 ) y -= ( y + ysize ) - win->parent->ysize;
		if( x < 0 ) x = 0;
		if( y < 0 ) y = 0;

		win->x = x;
		win->y = y;
		win->xsize = xsize;
		win->ysize = ysize;

		data->exit_flag = wm->opt_popup_exit_ptr;
		data->result = wm->opt_popup_result_ptr;

		wm->opt_popup_text = 0;
		wm->opt_popup_exit_ptr = 0;
		wm->opt_popup_result_ptr = 0;
    	    
		retval = 1;
	    }
	    break;
	case EVT_BEFORECLOSE:
	    bmem_free( data->text );
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	    {
		int prev_sel = data->current_selected;
		data->current_selected = -1;
		int cur_y = popup_border;
		int name_ysize = 0;
		if( win->name && win->name[ 0 ] != 0 )
		{
		    name_ysize = wm->list_item_ysize;
		}
		cur_y += name_ysize;
		if( data->text && data->lines_num > 0 )
		{
		    int col_x = popup_border;
		    int line = 0;
		    for( int i = 0; i < data->cols; i++ )
		    {
			int xsize = data->col_xsize[ i ];
			int lines = ( win->ysize - name_ysize - popup_border * 2 ) / wm->list_item_ysize;
			if( line + lines > data->lines_num ) lines = data->lines_num - line;
			if( rx >= col_x && rx < col_x + data->col_xsize[ i ] )
			{
			    if( ry >= cur_y && ry < cur_y + lines * wm->list_item_ysize )
			    {
				data->current_selected = line + ( ry - cur_y ) / wm->list_item_ysize;
				break;
			    }
			}
			col_x += xsize + 1;
			line += lines;
		    }
		}
		if( prev_sel != data->current_selected )
		    draw_window( win, wm );
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONUP:
	    if( data->current_selected >= 0 && evt->key == MOUSE_BUTTON_LEFT )
	    {
		//Successful selection:
		//win->action_result = data->current_selected;
		if( data->result )
		    *data->result = data->current_selected;
		set_focus_win( data->prev_focus, wm ); // -> EVT_UNFOCUS -> remove_window()
	    }
	    retval = 1;
	    break;
	case EVT_UNFOCUS:
	    if( data->exit_flag ) 
		*data->exit_flag = 1;
	    remove_window( win, wm );
	    recalc_regions( wm );
	    draw_window( wm->root_win, wm );
	    retval = 1;
	    break;
	case EVT_DRAW:
	    wbd_lock( win );
	    wm->cur_opacity = 255;
	    {
		int cur_y = popup_border;
		int cur_x = popup_border;
		draw_frect( 0, 0, win->xsize, win->ysize, win->color, wm );

		//Draw name:
		int name_ysize = 0;
		if( win->name && win->name[ 0 ] != 0 )
		{
		    name_ysize = wm->list_item_ysize;
		    wm->cur_font_color = wm->header_text_color;
		    draw_string( 
			win->name, 
			cur_x, 
			cur_y + ( wm->list_item_ysize - char_y_size( wm ) ) / 2, 
			wm );
		    cur_y += name_ysize;
		}

		//Draw text:
		if( data->text && data->lines_num > 0 )
		{
		    int col = 0;
		    int xsize = data->col_xsize[ col ];
		    int ysize = data->col_ysize[ col ];
		    int y = 0;
		    for( int i = 0; i < data->lines_num; i++ )
		    {
			utf8_char* text = data->text + data->lines[ i ];
			int text_y = cur_y + ( wm->list_item_ysize - char_y_size( wm ) ) / 2;
			COLOR bg_color = win->color;
			if( i == data->current_selected )
			{
			    bg_color = blend( wm->color2, wm->color1, 32 );
			    wm->cur_font_color = wm->color0;
			}
			else
			{
			    if( y & 1 )
				bg_color = blend( bg_color, wm->color3, 16 );
			    if( data->line_flags[ i ] & POPUP_ITEM_FLAG_SHADED )
				bg_color = blend( bg_color, wm->color2, 90 );
			    wm->cur_font_color = wm->color3;
			}
			if( bg_color != win->color )
			{
			    draw_frect( 
				cur_x, 
				cur_y, 
				xsize, 
				wm->list_item_ysize, 
				bg_color,
				wm );
			}
			if( data->line_flags[ i ] & POPUP_ITEM_FLAG_TWO_SECTIONS )
			{
			    int p = 0;
			    while( 1 )
			    {
				utf8_char c = text[ p ];
				if( c == '|' )
				{
				    draw_string_limited( text, cur_x, text_y, p, wm );
				    draw_string( text + p + 1, cur_x + data->first_section_size, text_y, wm );
				    break;
				}
				p++;
			    }
			    wm->cur_opacity = 32;
			    draw_frect( 
				cur_x, 
				cur_y, 
				data->first_section_size, 
				wm->list_item_ysize, 
				wm->color3,
				wm );
			    wm->cur_opacity = 255;
			}
			else
			{
			    draw_string( text, cur_x, text_y, wm );
			}
			cur_y += wm->list_item_ysize;
			y++;
			if( cur_y - popup_border >= ysize )
			{
			    //Go to the next column:
			    y = 0;
			    cur_y = popup_border + name_ysize;
			    cur_x += xsize + 1;
			    col++;
			    xsize = data->col_xsize[ col ];
			}
		    }
		}

		draw_rect( 0, 0, win->xsize - 1, win->ysize - 1, BORDER_COLOR( win->color ), wm );

		retval = 1;
	    }
	    wbd_draw( wm );
	    wbd_unlock( wm );
	    break;
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
    }
    return retval;
}
