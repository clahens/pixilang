/*
    handlers_preferences.cpp
    This file is part of the SunDog engine.
    Copyright (C) 2010 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%
            
#include "core/core.h"

uchar g_color_themes[ 3 * 4 * COLOR_THEMES ] = 
{
    //1
    0, 0, 0,
    0, 16, 0,
    120, 200, 120,
    255, 255, 255,
    
    //2
    255, 255, 255,
    150, 145, 140,
    60, 50, 40,
    0, 0, 0,

    //3
    0, 0, 0,
    14, 24, 14,
    130, 220, 130,
    255, 255, 255,

    //4
    0, 0, 0,
    20, 20, 20,
    180, 180, 180,
    255, 255, 255,

    //5
    0, 0, 0,
    20, 0, 0,
    240, 130, 130,
    255, 255, 255,

    //6
    0, 0, 0,
    0, 0, 20,
    140, 140, 240,
    255, 255, 255,

    //7
    255, 255, 255,
    140, 140, 140,
    50, 50, 50,
    0, 0, 0,

    //8
    255, 255, 255,
    130, 140, 150,
    40, 50, 60,
    0, 0, 0,

    //9
    255, 255, 255,
    130, 110, 100,
    80, 20, 10,
    0, 0, 0,

    //10
    0, 0, 0,
    10, 10, 40,
    120, 120, 220,
    255, 255, 255,

    //11
    0, 0, 0,
    20, 0, 20,
    200, 120, 200,
    255, 255, 255,

    //12
    0, 0, 0,
    20, 20, 0,
    0, 210, 100,
    255, 255, 255,

    //13
    0, 0, 0,
    40, 40, 40,
    190, 190, 190,
    255, 255, 255,

    //14
    255, 255, 255,
    80, 80, 90,
    16, 16, 32,
    0, 0, 0,

    //15
    255, 255, 255,
    80, 90, 80,
    16, 32, 16,
    0, 0, 0,

    //16
    255, 255, 255,
    70, 100, 100,
    16, 40, 40,
    0, 0, 0,

    //17
    255, 255, 255,
    70, 100, 180,
    16, 40, 40,
    0, 0, 0,

    //18
    0, 0, 0,
    10, 20, 20,
    180, 190, 200,
    255, 255, 255,

    //19
    0, 0, 0,
    10, 30, 30,
    200, 120, 100,
    255, 255, 255,

    //20
    0, 0, 0,
    28, 28, 28,
    200, 200, 50,
    255, 255, 255,
    
    //21
    255, 255, 255,
    80, 140, 210,
    16, 40, 80,
    0, 0, 0,

    //22
    255, 255, 255,
    80, 210, 210,
    32, 80, 80,
    0, 0, 0,

    //23
    255, 255, 255,
    220, 220, 150,
    80, 80, 32,
    0, 0, 0,

    //24
    255, 255, 255,
    220, 220, 220,
    80, 80, 80,
    0, 0, 0,

    //25
    255, 255, 255,
    220, 210, 200,
    80, 40, 32,
    0, 0, 0,
};

bool g_color_theme_changed = false;

struct colortheme_data
{
    int grid_xcells;
    int grid_ycells;
    int prev_color_theme;
    
    WINDOWPTR ok;
    WINDOWPTR cancel;
};

int colortheme_ok_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    int prev_color_theme = 0;
    {
	colortheme_data* data = (colortheme_data*)user_data;
	prev_color_theme = data->prev_color_theme;
    }
    remove_window( wm->colortheme_win, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    profile_set_int_value( KEY_COLOR_THEME, wm->color_theme, 0 );
    profile_save( 0 );
    if( wm->color_theme == prev_color_theme ) return 0;
    if( dialog( wm_get_string( STR_WM_COLOR_THEME_MSG_RESTART ), wm_get_string( STR_WM_YESNO ), wm ) == 0 )
    {
	wm->exit_request = 1;
	wm->restart_request = 1;
	g_color_theme_changed = true;
    }
    else 
    {
    }
    return 0;
}

int colortheme_cancel_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    int prev_color_theme = 0;
    {
	colortheme_data* data = (colortheme_data*)user_data;
	prev_color_theme = data->prev_color_theme;
    }
    remove_window( wm->colortheme_win, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    wm->color_theme = prev_color_theme;
    return 0;
}

int colortheme_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    colortheme_data* data = (colortheme_data*)win->data;
    int rx = evt->x - win->screen_x;
    int ry = evt->y - win->screen_y;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( colortheme_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		g_color_theme_changed = false;
		data->prev_color_theme = wm->color_theme;
		data->grid_xcells = 1;
		data->grid_ycells = 1;
		int i = 0;
		while( data->grid_xcells * data->grid_ycells < COLOR_THEMES )
		{
		    if( i & 1 ) 
		    {
			data->grid_ycells++;
		    }
		    else 
		    {
			
			data->grid_xcells++;
		    }
		    i++;
		}
		
		data->ok = new_window( wm_get_string( STR_WM_OK ), 0, 0, 1, 1, wm->button_color, win, button_handler, wm );
		data->cancel = new_window( wm_get_string( STR_WM_CANCEL ), 0, 0, 1, 1, wm->button_color, win, button_handler, wm );
		set_handler( data->ok, colortheme_ok_handler, data, wm );
		set_handler( data->cancel, colortheme_cancel_handler, data, wm );
		int x = wm->interelement_space;
		set_window_controller( data->ok, 0, wm, (WCMD)x, CEND );
		set_window_controller( data->ok, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		x += wm->button_xsize;
		set_window_controller( data->ok, 2, wm, (WCMD)x, CEND );
		set_window_controller( data->ok, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->button_ysize + wm->interelement_space, CEND );
		x += 1;
		set_window_controller( data->cancel, 0, wm, (WCMD)x, CEND );
		set_window_controller( data->cancel, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		x += wm->button_xsize;
		set_window_controller( data->cancel, 2, wm, (WCMD)x, CEND );
		set_window_controller( data->cancel, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->button_ysize + wm->interelement_space, CEND );
	    }
	    retval = 1;
	    break;
	case EVT_DRAW:
	    {
		wbd_lock( win );
		
		draw_frect( 0, 0, win->xsize, win->ysize, win->color, wm );
		
		int t = 0;
		float grid_xsize = (float)( win->xsize - wm->interelement_space * 2 ) / data->grid_xcells;
		float grid_ysize = (float)( win->ysize - wm->interelement_space * 3 - wm->button_ysize ) / data->grid_ycells;
		for( int grid_y = 0; grid_y < data->grid_ycells; grid_y++ )
		{
		    for( int grid_x = 0; grid_x < data->grid_xcells; grid_x++ )
		    {
			if( t >= COLOR_THEMES ) break;
			float fx = (float)wm->interelement_space + (float)grid_x * grid_xsize;
			float fy = (float)wm->interelement_space + (float)grid_y * grid_ysize;
			int x = fx;
			int y = fy;
			int xsize = (int)( ( fx + grid_xsize ) - (float)x );
			int ysize = (int)( ( fy + grid_ysize ) - (float)y );
			xsize--;
			ysize--;
			if( grid_x == data->grid_xcells - 1 ) xsize = win->xsize - wm->interelement_space - x;
			if( grid_y == data->grid_ycells - 1 ) ysize = win->ysize - wm->interelement_space * 2 - wm->button_ysize - y;
			
			COLOR cc[ 4 ];
			for( int c = 0; c < 4; c++ )
			{
			    uchar rr = g_color_themes[ t * 3 * 4 + c * 3 + 0 ];
			    uchar gg = g_color_themes[ t * 3 * 4 + c * 3 + 1 ];
			    uchar bb = g_color_themes[ t * 3 * 4 + c * 3 + 2 ];
			    cc[ c ] = get_color( rr, gg, bb );
			}
			
			int xsize2 = xsize / 4;
			draw_frect( x, y, xsize2, ysize, cc[ 0 ], wm );
			draw_frect( x + xsize2, y, xsize2, ysize, cc[ 1 ], wm );
			draw_frect( x + xsize2 * 2, y, xsize2, ysize, cc[ 2 ], wm );
			draw_frect( x + xsize2 * 3, y, xsize - xsize2 * 3, ysize, cc[ 3 ], wm );
						
			draw_vgradient( x, y, xsize, ysize / 2, wm->color3, 180, 0, wm );
			draw_vgradient( x, y + ysize / 2, xsize, ysize - ysize / 2, win->color, 0, 180, wm );
			
			wm->cur_opacity = 128;
			draw_rect( x, y, xsize - 1, ysize - 1, win->color, wm );
			wm->cur_opacity = 255;
			
			if( t == wm->color_theme )
			{
			    wm->cur_opacity = 128;
			    draw_corners( x + 1 + wm->corners_size, y + 1 + wm->corners_size, xsize - 2 - wm->corners_size * 2, ysize - 2 - wm->corners_size * 2, wm->corners_size, wm->corners_len, wm->color0, wm );
			    wm->cur_opacity = 255;
			    draw_corners( x + wm->corners_size, y + wm->corners_size, xsize - wm->corners_size * 2, ysize - wm->corners_size * 2, wm->corners_size, wm->corners_len, wm->color3, wm );
			}
			
			t++;
		    }
		    if( t >= COLOR_THEMES ) break;
		}
		
		wbd_draw( wm );
		wbd_unlock( wm );
	    }
	    retval = 1;
	    break;
	case EVT_BUTTONDOWN:
	    if( evt->key == KEY_ENTER )
	    {
		colortheme_ok_handler( data, data->ok, wm );
		retval = 1;
	    }
	    if( evt->key == KEY_LEFT || evt->key == KEY_UP )
	    {
		int t = wm->color_theme;
		if( evt->key == KEY_LEFT ) t--;
		if( evt->key == KEY_UP ) t -= data->grid_xcells;
		if( t >= 0 )
		{
		    wm->color_theme = t;
		    draw_window( win, wm );
		}
		retval = 1;
	    }
	    if( evt->key == KEY_RIGHT || evt->key == KEY_DOWN )
	    {
		int t = wm->color_theme;
		if( evt->key == KEY_RIGHT ) t++;
		if( evt->key == KEY_DOWN ) t += data->grid_xcells;
		if( t < COLOR_THEMES )
		{
		    wm->color_theme = t;
		    draw_window( win, wm );
		}
		retval = 1;
	    }
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	    if( evt->key & MOUSE_BUTTON_LEFT )
	    {
		int t = 0;
		float grid_xsize = (float)( win->xsize - wm->interelement_space * 2 ) / data->grid_xcells;
		float grid_ysize = (float)( win->ysize - wm->interelement_space * 3 - wm->button_ysize ) / data->grid_ycells;
		for( int grid_y = 0; grid_y < data->grid_ycells; grid_y++ )
		{
		    for( int grid_x = 0; grid_x < data->grid_xcells; grid_x++ )
		    {
			if( t >= COLOR_THEMES ) break;
			
			float fx = (float)wm->interelement_space + (float)grid_x * grid_xsize;
			float fy = (float)wm->interelement_space + (float)grid_y * grid_ysize;
			int x = fx;
			int y = fy;
			int xsize = (int)grid_xsize;
			int ysize = (int)grid_ysize;
			if( rx >= x && ry >= y && rx < x + xsize && ry < y + ysize )
			{
			    wm->color_theme = t;
			    draw_window( win, wm );
			    break;
			}
			
			t++;
		    }
		    if( t >= COLOR_THEMES ) break;
		}
	    }
	    retval = 1;
	    break;
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    retval = 1;
	    wm->colortheme_win = 0;
	    break;
    }
    return retval;
}
