/*
    wmanager.cpp. SunDog window manager
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

#include "core/core.h"

//Fonts (WIN1251)
#include "fonts.h"

const utf8_char g_text_up[ 2 ] = { 0x11, 0 };
const utf8_char g_text_down[ 2 ] = { 0x12, 0 };
const utf8_char g_text_left[ 2 ] = { 0x13, 0 };
const utf8_char g_text_right[ 2 ] = { 0x14, 0 };

//
// Device-dependent modules
//

#ifdef WIN
    #include "wm_win.h"
#endif

#ifdef WINCE
    #include "wm_wince.h"
#endif

#ifdef UNIX
    #ifdef X11
	#include "wm_unix_x11.h"
    #else
	#ifdef DIRECTDRAW
	    #include "wm_unix_sdl.h"
	#endif
	#ifdef IPHONE
	    #include "wm_iphone.h"
	#endif
	#ifdef OSX
	    #include "wm_osx.h"
	#endif
	#ifdef ANDROID
	    #include "wm_android.h"
	#endif
    #endif
#endif

void empty_device_event_handler( window_manager* wm )
{
    int pause = 1000 / wm->max_fps;
    if( wm->flags & WIN_INIT_FLAG_FULL_CPU_USAGE )
        pause = 1;
    if( wm->events_count )
        pause = 0;
    if( pause )
        time_sleep( pause );
}
int empty_device_start( const utf8_char* windowname, int xsize, int ysize, window_manager* wm )
{
    wm->screen_xsize = 128;
    wm->screen_ysize = 128;
    wm->real_window_width = wm->screen_xsize;
    wm->real_window_height = wm->screen_ysize;
    //if( wm->screen_ppi == 0 ) wm->screen_ppi = 110;
    //win_zoom_init( wm );
    return 0;
}
void empty_device_end( window_manager* wm )
{
}
void empty_device_draw_line( int x1, int y1, int x2, int y2, COLOR color, window_manager* wm )
{
}
void empty_device_draw_frect( int x, int y, int xsize, int ysize, COLOR color, window_manager* wm )
{
}
void empty_device_draw_image(
    int dest_x, int dest_y,
    int dest_xs, int dest_ys,
    int src_x, int src_y,
    sundog_image* img,
    window_manager* wm )
{
}
void empty_device_screen_lock( WINDOWPTR win, window_manager* wm )
{
}
void empty_device_screen_unlock( WINDOWPTR win, window_manager* wm )
{
}
void empty_device_vsync( bool vsync, window_manager* wm )
{
}
void empty_device_redraw_framebuffer( window_manager* wm )
{
}

//
// Main functions
// 

int win_init(
    const utf8_char* windowname,
    int xsize,
    int ysize,
    uint flags,
    sundog_engine* sd )
{
    int retval = 0;

    //FIRST STEP: set defaults
    
    window_manager* wm = sd->wm;
    bmem_set( wm, sizeof( window_manager ), 0 );
    wm->sd = sd;
    
    wm->vcap = 0;
    wm->vcap_in_fps = 30;
    wm->vcap_in_bitrate_kb = 1000;
    wm->vcap_in_name = 0;

    wm->events_count = 0;
    wm->current_event_num = 0;
    bmem_set( &wm->frame_evt, sizeof( wm->frame_evt ), 0 );
    bmem_set( &wm->empty_evt, sizeof( wm->empty_evt ), 0 );
    wm->frame_evt.type = EVT_FRAME;
    bmutex_init( &wm->events_mutex, 0 );
    
#ifdef OPENGL
    bmutex_init( &wm->gl_mutex, 0 );
#endif    

    wm->restart_request = 0;
    wm->exit_request = 0;
    wm->root_win = 0;
    wm->trash = 0;
    wm->window_counter = 0;
    wm->pen_x = -1;
    wm->pen_y = -1;
    wm->focus_win = 0;
    wm->handler_of_unhandled_events = 0;

    bmem_set( wm->timers, sizeof( wm->timers ), 0 );
    wm->timers_num = 0;
    wm->timers_id_counter = 0;
    wm->status_timer = -1;

    wm->km = keymap_new( wm );
    
    wm->fdialog_copy_file_name = 0;
    wm->fdialog_copy_file_name2 = 0;
    wm->fdialog_cut_file_flag = false;
    wm->prefs_win = 0;
    wm->prefs_sections = 0;
    wm->prefs_section_ysize = 200;
    wm->colortheme_win = 0;
    wm->keymap_win = 0;
    wm->vk_win = 0;

    //Load settings from configuration file:
    
    windowname = (const utf8_char*)profile_get_str_value( KEY_WINDOWNAME, windowname, 0 );
    if( profile_get_int_value( KEY_NOBORDER, -1, 0 ) != -1 ) flags |= WIN_INIT_FLAG_NOBORDER;
    int angle = profile_get_int_value( KEY_ROTATE, 0, 0 );
    if( angle > 0 ) flags |= ( ( angle / 90 ) & 3 ) << WIN_INIT_FLAGOFF_ANGLE;
    int zoom = profile_get_int_value( KEY_ZOOM, 0, 0 );
    if( zoom > 0 ) flags |= ( zoom & 7 ) << WIN_INIT_FLAGOFF_ZOOM;
    if( profile_get_int_value( KEY_NOCURSOR, -1, 0 ) != -1 ) flags |= WIN_INIT_FLAG_NOCURSOR;
    if( profile_get_int_value( KEY_FULLSCREEN, -1, 0 ) != -1 ) flags |= WIN_INIT_FLAG_FULLSCREEN;
    if( profile_get_int_value( KEY_VSYNC, -1, 0 ) != -1 ) flags |= WIN_INIT_FLAG_VSYNC;
#ifdef FRAMEBUFFER
    flags |= WIN_INIT_FLAG_FRAMEBUFFER;
#endif
    int fb = profile_get_int_value( KEY_FRAMEBUFFER, -1, 0 );
    if( fb == 1 ) flags |= WIN_INIT_FLAG_FRAMEBUFFER;
    if( fb == 0 ) flags &= ~WIN_INIT_FLAG_FRAMEBUFFER;
#if defined(UNIX)
    flags |= WIN_INIT_FLAG_OPTIMIZE_MOVE_EVENTS;
#endif
    if( profile_get_int_value( KEY_NOWINDOW, -1, 0 ) != -1 ) flags |= WIN_INIT_FLAG_NOWINDOW;

    //Save flags:
    wm->flags = flags;

    wm->screen_buffer_preserved = 1;
    wm->screen_angle = ( flags >> WIN_INIT_FLAGOFF_ANGLE ) & 3;
    wm->screen_zoom = ( flags >> WIN_INIT_FLAGOFF_ZOOM ) & 7;
    if( wm->screen_zoom == 0 ) wm->screen_zoom = 1;
#ifndef SCREEN_ROTATE_SUPPORTED
    wm->screen_angle = 0;
#endif
#ifndef SCREEN_ZOOM_SUPPORTED
    wm->screen_zoom = 1;
#endif
    wm->screen_ppi = profile_get_int_value( KEY_PPI, 0, 0 ); //Default - 0 (unknown)
    wm->screen_scale = (float)profile_get_int_value( KEY_SCALE, 0, 0 ) / 256.0F; //Default - 0 (unknown)
    wm->screen_font_scale = (float)profile_get_int_value( KEY_FONT_SCALE, 0, 0 ) / 256.0F; //Default - 0 (unknown)
    wm->screen_lock_counter = 0;
    wm->screen_is_active = false;
    wm->screen_changed = 0;
    
    wm->show_virtual_keyboard = false;
#ifdef VIRTUALKEYBOARD
    wm->show_virtual_keyboard = true;
#endif
    if( profile_get_int_value( KEY_SHOW_VIRT_KBD, -1, 0 ) != -1 )
	wm->show_virtual_keyboard = true;
    if( profile_get_int_value( KEY_HIDE_VIRT_KBD, -1, 0 ) != -1 )
	wm->show_virtual_keyboard = false;

    wm->double_click_time = profile_get_int_value( KEY_DOUBLECLICK, DEFAULT_DOUBLE_CLICK_TIME, 0 );
    
    //SECOND STEP: SCREEN SIZE SETTING and device specific init
    blog( "WM: device start\n" );

    wm->device_event_handler = device_event_handler;
    wm->device_start = device_start;
    wm->device_end = device_end;
#ifdef OPENGL
    wm->device_draw_line = gl_draw_line;
    wm->device_draw_frect = gl_draw_frect;
    wm->device_draw_image = gl_draw_image;
#else
    wm->device_draw_line = device_draw_line;
    wm->device_draw_frect = device_draw_frect;
    wm->device_draw_image = device_draw_image;
#endif
    wm->device_screen_lock = device_screen_lock;
    wm->device_screen_unlock = device_screen_unlock;
    wm->device_vsync = device_vsync;
    wm->device_redraw_framebuffer = device_redraw_framebuffer;
    if( flags & WIN_INIT_FLAG_FRAMEBUFFER )
    {
	wm->device_draw_line = fb_draw_line;
	wm->device_draw_frect = fb_draw_frect;
	wm->device_draw_image = fb_draw_image;
    }
    if( flags & WIN_INIT_FLAG_NOWINDOW )
    {
	wm->device_event_handler = empty_device_event_handler;
	wm->device_start = empty_device_start;
	wm->device_end = empty_device_end;
	wm->device_draw_line = empty_device_draw_line;
	wm->device_draw_frect = empty_device_draw_frect;
	wm->device_draw_image = empty_device_draw_image;
	wm->device_screen_lock = empty_device_screen_lock;
	wm->device_screen_unlock = empty_device_screen_unlock;
	wm->device_vsync = empty_device_vsync;
	wm->device_redraw_framebuffer = empty_device_redraw_framebuffer;
    }

#if CPUMARK >= 10
    wm->max_fps = 60;
#else
    wm->max_fps = 30;
#endif
#if defined(MAX_FPS)
    wm->max_fps = MAX_FPS;
#endif
    wm->max_fps = profile_get_int_value( KEY_MAXFPS, wm->max_fps, 0 );

    wm->fb_offset = 0;
    wm->fb_xpitch = 1;
    wm->fb_ypitch = 0;
    
    int err = wm->device_start( windowname, xsize, ysize, wm ); //DEVICE DEPENDENT PART
    if( err )
    {
	blog( "Error in device_start() %d\n", err );
	return 1; //Error
    }
    
    if( wm->screen_ppi == 0 ) wm->screen_ppi = 110;
    if( wm->screen_zoom > 1 ) wm->screen_ppi /= wm->screen_zoom;
    if( wm->screen_scale == 0 ) wm->screen_scale = 1;
    if( wm->screen_font_scale == 0 ) wm->screen_font_scale = 1;
    if( wm->font_zoom == 0 ) wm->font_zoom = 1;

    if( profile_get_int_value( KEY_TOUCHCONTROL, -1, 0 ) != -1 )
	wm->flags |= WIN_INIT_FLAG_TOUCHCONTROL;
    if( profile_get_int_value( KEY_PENCONTROL, -1, 0 ) != -1 )
	wm->flags &= ~WIN_INIT_FLAG_TOUCHCONTROL;
    if( wm->flags & WIN_INIT_FLAG_TOUCHCONTROL )
	wm->control_type = TOUCHCONTROL;
    else
	wm->control_type = PENCONTROL;
    if( wm->flags & WIN_INIT_FLAG_VSYNC )
	win_vsync( 1, wm );
    else
	win_vsync( 0, wm );
	
    if( wm->screen_angle ) blog( "WM: screen_angle = %d\n", wm->screen_angle * 90 );
    if( wm->screen_zoom ) blog( "WM: screen_zoom = %d\n", wm->screen_zoom );
    blog( "WM: screen_ppi = %d\n", wm->screen_ppi );
    if( wm->screen_scale != 1 ) blog( "WM: screen_scale = %d / 256\n", (int)( wm->screen_scale * 256 ) );
    if( wm->screen_font_scale != 1 ) blog( "WM: screen_font_scale = %d / 256\n", (int)( wm->screen_font_scale * 256 ) );
    blog( "WM: screen size = %d x %d\n", wm->screen_xsize, wm->screen_ysize );
    if( wm->font_zoom != 1 ) blog( "WM: font_zoom = %d\n", wm->font_zoom );
    if( wm->flags & ~( ( 3 << WIN_INIT_FLAGOFF_ANGLE ) | ( 7 << WIN_INIT_FLAGOFF_ZOOM ) ) )
    {
	blog( "WM: flags " );
	if( wm->flags & WIN_INIT_FLAG_SCALABLE ) blog( "SCALABLE " );
	if( wm->flags & WIN_INIT_FLAG_NOBORDER ) blog( "NOBORDER " );
	if( wm->flags & WIN_INIT_FLAG_FULLSCREEN ) blog( "FULLSCREEN " );
	if( wm->flags & WIN_INIT_FLAG_FULL_CPU_USAGE ) blog( "FULL_CPU_USAGE " );
	if( wm->flags & WIN_INIT_FLAG_TOUCHCONTROL ) blog( "TOUCHCONTROL " );
	if( wm->flags & WIN_INIT_FLAG_OPTIMIZE_MOVE_EVENTS ) blog( "OPTIMIZE_MOVE_EVENTS " );
	if( wm->flags & WIN_INIT_FLAG_NOSCREENROTATE ) blog( "NOSCREENROTATE " );
	if( wm->flags & WIN_INIT_FLAG_LANDSCAPE_ONLY ) blog( "LANDSCAPE_ONLY " );
	if( wm->flags & WIN_INIT_FLAG_PORTRAIT_ONLY ) blog( "PORTRAIT_ONLY " );
	if( wm->flags & WIN_INIT_FLAG_NOCURSOR ) blog( "NOCURSOR " );
	if( wm->flags & WIN_INIT_FLAG_VSYNC ) blog( "VSYNC " );
	if( wm->flags & WIN_INIT_FLAG_FRAMEBUFFER ) blog( "FRAMEBUFFER " );
	if( wm->flags & WIN_INIT_FLAG_NOWINDOW ) blog( "NOWINDOW " );
	blog( "\n" );
    }
    
    {
	int size, min;
	
	int smallest_font = 0;
	int biggest_font = 1;
	
	for( int i = 0; i < 2; i++ )
	{
	    min = 18; //smallest icon size = 16
	    if( min < font_char_y_size( biggest_font, wm ) + 2 )
		min = font_char_y_size( biggest_font, wm ) + 2;
	    float scrollbar_size = (float)wm->screen_ppi * SCROLLBAR_SIZE_COEFF;
	    size = (int)( scrollbar_size * wm->screen_scale );
	    if( size < min ) size = min;
	    wm->scrollbar_size = size;
	
	    int scrsize = wm->screen_xsize;
	    if( wm->screen_ysize < scrsize ) scrsize = wm->screen_ysize;
	    const int max_xbuttons = 6;
	    if( wm->scrollbar_size * max_xbuttons > scrsize )
	    {
		//HACK: ====
		blog( "WM: PPI or Scale is too large! They will be reduced temporary.\n" );
		wm->screen_scale = (float)scrsize / ( scrollbar_size * max_xbuttons );
		if( wm->font_zoom > 1 ) wm->font_zoom /= 2;
		//==========
	    }
	    else
	    {
		break;
	    }
	}

	min = 2;
	size = ( wm->screen_ppi * 2 ) / 160;
	if( size < min ) size = min;
	wm->interelement_space = size;

	min = 1;
	size = wm->interelement_space / 2;
	if( size < min ) size = min;
	wm->interelement_space2 = size;

	min = 4;
    	size = (int)( (float)wm->screen_ppi * 0.07 * wm->screen_scale );
    	if( wm->control_type == TOUCHCONTROL ) size /= 2;
	if( size < min ) size = min;
	wm->decor_border_size = size;

	min = font_char_y_size( biggest_font, wm ) + wm->interelement_space2 * 2;
	size = (int)( (float)wm->screen_ppi * 0.175 * wm->screen_scale );
	if( size < min ) size = min;
	wm->decor_header_size = size;

	min = font_string_x_size( "#", 1, wm ) + wm->interelement_space2 * 2;
        size = (int)( (float)wm->scrollbar_size * 0.8 * wm->screen_scale );
        if( size < min ) size = min;
        wm->small_button_xsize = size;

	min = wm->scrollbar_size * 2;
	size = (int)( (float)font_string_x_size( "Button ||23", 1, wm ) );
	if( size < min ) size = min;
	wm->button_xsize = size;

	min = font_char_y_size( biggest_font, wm ) * 2;
	size = (int)( (float)wm->screen_ppi * BIG_BUTTON_YSIZE_COEFF * wm->screen_scale );
	if( size < min ) size = min;
	wm->button_ysize = size;

	wm->normal_window_xsize = (int)( (float)wm->button_xsize * 4.5 );
	wm->normal_window_ysize = ( wm->normal_window_xsize * 11 ) / 16;
	wm->large_window_xsize = (int)( (float)wm->button_xsize * 7 );
	wm->large_window_ysize = ( wm->large_window_xsize * 11 ) / 16;

	min = font_char_y_size( biggest_font, wm );
	size = (int)( (float)wm->screen_ppi * 0.14 * wm->screen_scale );
	if( size < min ) size = min;
	wm->list_item_ysize = size;
	
	min = font_char_y_size( biggest_font, wm ) + wm->interelement_space2 * 2;
	size = (int)( (float)wm->screen_ppi * TEXT_YSIZE_COEFF * wm->screen_scale );
	if( size < min ) size = min;
	wm->text_ysize = size;

	min = font_char_y_size( smallest_font, wm ) * 2 + wm->interelement_space2 * 2;
	size = (int)( (float)wm->screen_ppi * 0.2 * wm->screen_scale );
	if( size < min ) size = min;
	wm->popup_button_ysize = size;

	min = font_char_y_size( smallest_font, wm ) * 2 + wm->interelement_space2 * 2;
	size = (int)( (float)wm->screen_ppi * 0.2 * wm->screen_scale );
	if( size < min ) size = min;
	wm->controller_ysize = size;
	
	wm->corners_size = (int)( (float)( 2.0F * (float)wm->screen_ppi * wm->screen_scale ) / 160.0 + 0.5 );
        wm->corners_len = (int)( (float)( 4.0F * (float)wm->screen_ppi * wm->screen_scale ) / 160.0 + 0.5 );
        if( wm->corners_size < 2 ) wm->corners_size = 2;
        if( wm->corners_len < 4 ) wm->corners_len = 4;
    }

    wm->default_font = 1;
#if defined(OPENGL)
    //Create font textures:
    for( int f = 0; f < WM_FONTS; f++ )
    {
	uchar* font;
	switch( f )
	{
	    case 0: font = g_font0; break;
	    case 1: font = g_font1; break;
	}
	int cxsize = 8;
	int char_ysize = font[ 0 ];
	int cysize = font[ 0 ];
	if( cysize > 8 ) cysize = 16;
	wm->font_cxsize[ f ] = cxsize;
	wm->font_cysize[ f ] = cysize;
	uchar* tmp = (uchar*)bmem_new( 16 * cxsize * 16 * cysize );
	int img_ptr = 0;
	int font_ptr = 257;
	for( int b = 257 + char_ysize; b < 257 + char_ysize * 2; b++ ) font[ b ] = 0xFF;
	for( int yy = 0; yy < cysize * 16; yy += cysize )
	{
	    for( int xx = 0; xx < cxsize * 16; xx += cxsize )
	    {
		img_ptr = yy * cxsize * 16 + xx;
		for( int fc = 0; fc < char_ysize; fc++ )
		{
		    uchar v = font[ font_ptr ];
		    for( int x = 0; x < cxsize; x++ )
		    {
			if( v & 128 ) 
			    tmp[ img_ptr ] = 255;
			else
			    tmp[ img_ptr ] = 0;
			v <<= 1;
			img_ptr++;
		    }
		    img_ptr += cxsize * 15;
		    font_ptr++;
		}
	    }
	}
	wm->font_img[ f ] = new_image( 16 * cxsize, 16 * cysize, tmp, 0, 0, 16 * cxsize, 16 * cysize, IMAGE_ALPHA8, wm );
	bmem_free( tmp );
    }
#else
    for( int f = 0; f < WM_FONTS; f++ )
    {
	wm->font_img[ f ] = 0;
    }
#endif

    wm->color_theme = profile_get_int_value( KEY_COLOR_THEME, 3, 0 );
    win_colors_init( wm );

    blog( "WM: initialized\n" );

    COMPILER_MEMORY_BARRIER();
    wm->wm_initialized = 1;
    
    return retval;
}

void win_colors_init( window_manager* wm )
{
    blog( "WM: system palette init...\n" );
    
    bool theme = 0;
    if( profile_get_str_value( "theme", 0, 0 ) ) theme = 1;
    
    COLOR cc[ 4 ];
    for( int c = 0; c < 4; c++ )
    {
	uchar rr = g_color_themes[ wm->color_theme * 3 * 4 + c * 3 + 0 ];
	uchar gg = g_color_themes[ wm->color_theme * 3 * 4 + c * 3 + 1 ];
	uchar bb = g_color_themes[ wm->color_theme * 3 * 4 + c * 3 + 2 ];
	cc[ c ] = get_color( rr, gg, bb );
    }
    wm->color0 = cc[ 0 ];
    wm->color1 = cc[ 1 ];
    wm->color2 = cc[ 2 ];
    wm->color3 = cc[ 3 ];
    wm->green = get_color( 0, 255, 0 );
    wm->yellow = get_color( 255, 255, 0 );
    wm->red = get_color( 255, 0, 0 );
    wm->blue = get_color( 0, 0, 255 );
    
    if( theme )
    {
	blog( "WM: color theme loading (base colors)\n" );
	utf8_char* v;
	v = profile_get_str_value( "c_0", 0, 0 ); if( v ) wm->color0 = get_color_from_string( v );
	v = profile_get_str_value( "c_1", 0, 0 ); if( v ) wm->color1 = get_color_from_string( v );
	v = profile_get_str_value( "c_2", 0, 0 ); if( v ) wm->color2 = get_color_from_string( v );
	v = profile_get_str_value( "c_3", 0, 0 ); if( v ) wm->color3 = get_color_from_string( v );
	v = profile_get_str_value( "c_yellow", 0, 0 ); if( v ) wm->yellow = get_color_from_string( v );
	v = profile_get_str_value( "c_green", 0, 0 ); if( v ) wm->green = get_color_from_string( v );
	v = profile_get_str_value( "c_red", 0, 0 ); if( v ) wm->red = get_color_from_string( v );
	v = profile_get_str_value( "c_blue", 0, 0 ); if( v ) wm->blue = get_color_from_string( v );
    }
    
    wm->header_text_color = wm->color2;
    wm->alternative_text_color = wm->yellow;
    wm->dialog_color = wm->color1;
    wm->decorator_color = wm->color1;
    wm->decorator_border = wm->color2;
    wm->button_color = blend( wm->color1, wm->color3, 12 );
    wm->menu_color = wm->color1;
    wm->selection_color = blend( get_color( 255, 255, 255 ), wm->green, 145 );
    wm->text_background = wm->color1;
    wm->list_background = wm->color1;
    wm->scroll_color = wm->button_color;
    wm->scroll_background_color = wm->color1;
    
    if( theme )
    {
	blog( "WM: color theme loading (other colors)\n" );
	utf8_char* v;
	v = profile_get_str_value( "c_header", 0, 0 ); if( v ) wm->header_text_color = get_color_from_string( v );
	v = profile_get_str_value( "c_alt", 0, 0 ); if( v ) wm->alternative_text_color = get_color_from_string( v );
	v = profile_get_str_value( "c_dlg", 0, 0 ); if( v ) wm->dialog_color = get_color_from_string( v );
	v = profile_get_str_value( "c_dec", 0, 0 ); if( v ) wm->decorator_color = get_color_from_string( v );
	v = profile_get_str_value( "c_decbord", 0, 0 ); if( v ) wm->decorator_border = get_color_from_string( v );
	v = profile_get_str_value( "c_btn", 0, 0 ); if( v ) wm->button_color = get_color_from_string( v );
	v = profile_get_str_value( "c_menu", 0, 0 ); if( v ) wm->menu_color = get_color_from_string( v );
	v = profile_get_str_value( "c_sel", 0, 0 ); if( v ) wm->selection_color = get_color_from_string( v );
	v = profile_get_str_value( "c_txtback", 0, 0 ); if( v ) wm->text_background = get_color_from_string( v );
	v = profile_get_str_value( "c_lstback", 0, 0 ); if( v ) wm->list_background = get_color_from_string( v );
	v = profile_get_str_value( "c_scroll", 0, 0 ); if( v ) wm->scroll_color = get_color_from_string( v );
	v = profile_get_str_value( "c_scrollback", 0, 0 ); if( v ) wm->scroll_background_color = get_color_from_string( v );
    }
    
    if( red( wm->color3 ) + green( wm->color3 ) + blue( wm->color3 ) >= red( wm->color0 ) + green( wm->color0 ) + blue( wm->color0 ) )
    {
	wm->color3_is_brighter = 1;
    }
    else 
    {
	wm->color3_is_brighter = 0;
    }
    
    int v1 = 255 - red( wm->color1 );
    if( v1 < 0 ) v1 = -v1;
    int v2 = 255 - green( wm->color1 );
    if( v2 < 0 ) v2 = -v2;
    int v3 = 255 - blue( wm->color1 );
    if( v3 < 0 ) v3 = -v3;
    wm->color1_darkness = ( v1 + v2 + v3 ) / 3;
}

void win_reinit( window_manager* wm )
{
    win_colors_init( wm );
    
    bmem_set( wm->timers, sizeof( wm->timers ), 0 );
    wm->timers_num = 0;
    wm->timers_id_counter = 0;
    
    wm->handler_of_unhandled_events = 0;
    
    wm->restart_request = 0;
    wm->exit_request = 0;
}

int win_calc_font_zoom( int screen_ppi, int screen_zoom, float screen_scale, float screen_font_scale )
{
    int font_zoom = 1;
    int fixed_ppi = profile_get_int_value( "fixedppi", 0, 0 );
#ifdef FIXEDPPI
    fixed_ppi = FIXEDPPI;
#endif
    if( fixed_ppi )
    {
#ifdef SCREEN_ZOOM_SUPPORTED
	screen_zoom = screen_ppi / fixed_ppi;
#endif
    }
    else
    {
	int base_font_ppi = 160;
	int base_font_ppi_limit = 200;
	int dec = base_font_ppi * 2 - base_font_ppi_limit;
	if( screen_ppi / screen_zoom >= base_font_ppi * 2 - dec )
	{
    	    font_zoom = ( screen_ppi / screen_zoom + dec ) / base_font_ppi;
	}
    }
    if( screen_scale <= 0 ) screen_scale = 1;
    if( screen_font_scale <= 0 ) screen_font_scale = 1;
    font_zoom = (int)( (float)font_zoom * screen_scale * screen_font_scale );
    if( font_zoom < 1 ) font_zoom = 1;
    return font_zoom;
}

void win_zoom_init( window_manager* wm )
{
    wm->font_zoom = 1;
    int fixed_ppi = profile_get_int_value( "fixedppi", 0, 0 );
#ifdef FIXEDPPI
    fixed_ppi = FIXEDPPI;
#endif
    if( fixed_ppi )
    {
#ifdef SCREEN_ZOOM_SUPPORTED
	wm->screen_zoom = wm->screen_ppi / fixed_ppi;
#endif
    }
    wm->font_zoom = win_calc_font_zoom( wm->screen_ppi, wm->screen_zoom, wm->screen_scale, wm->screen_font_scale );
}

void win_vsync( bool vsync, window_manager* wm )
{
    if( vsync )
	wm->flags |= WIN_INIT_FLAG_VSYNC;
    else
	wm->flags &= ~WIN_INIT_FLAG_VSYNC;
    wm->device_vsync( vsync, wm );
}

void win_exit_request( window_manager* wm )
{
    if( wm == 0 ) return;
    wm->exit_request = true;
}

void win_suspend( bool suspend, window_manager* wm )
{
    if( wm == 0 ) return;
    wm->suspended = suspend;
}

void win_close( window_manager* wm )
{
    //Clear trash with a deleted windows:
    if( wm->trash )
    {
	for( unsigned int a = 0; a < bmem_get_size( wm->trash ) / sizeof( WINDOWPTR ); a++ )
	{
	    if( wm->trash[ a ] ) 
	    {
		bmem_free( wm->trash[ a ]->x1com );
		bmem_free( wm->trash[ a ]->y1com );
		bmem_free( wm->trash[ a ]->x2com );
		bmem_free( wm->trash[ a ]->y2com );
		bmem_free( wm->trash[ a ] );
	    }
	}
	bmem_free( wm->trash );
    }

    for( int f = 0; f < WM_FONTS; f++ )
    {
	if( wm->font_img[ f ] )
	    remove_image( wm->font_img[ f ] );
    }
    
    if( wm->screen_lock_counter > 0 )
    {
	blog( "WM: WARNING. Screen is still locked (%d)\n", wm->screen_lock_counter );
	while( wm->screen_lock_counter > 0 ) wm->device_screen_unlock( 0, wm );
    }

    bmem_free( wm->screen_pixels ); wm->screen_pixels = 0;
    bmem_free( wm->fdialog_filename ); wm->fdialog_filename = 0;
    bmem_free( wm->fdialog_copy_file_name ); wm->fdialog_copy_file_name = 0;
    bmem_free( wm->fdialog_copy_file_name2 ); wm->fdialog_copy_file_name2 = 0;
    keymap_remove( wm->km, wm ); wm->km = 0;
    
    video_capture_stop( wm );
    bmem_free( wm->vcap_in_name );
    wm->vcap_in_name = 0;

    wm->device_end( wm ); //DEVICE DEPENDENT PART (defined in eventloop.h)

    bmutex_destroy( &wm->events_mutex );
#ifdef OPENGL
    bmutex_destroy( &wm->gl_mutex );
#endif
}

//
// Working with windows
//

WINDOWPTR get_from_trash( window_manager* wm );

WINDOWPTR new_window( 
    const utf8_char* name, 
    int x, 
    int y, 
    int xsize, 
    int ysize, 
    COLOR color, 
    WINDOWPTR parent,
    void* host,
    win_handler_t win_handler,
    window_manager* wm )
{
    //Create window structure:
    WINDOWPTR win = get_from_trash( wm );
    if( win == 0 ) 
    {
	win = (WINDOWPTR)bmem_new( sizeof( WINDOW ) );
	bmem_set( win, sizeof( WINDOW ), 0 );
	win->id = (int16)wm->window_counter;
	wm->window_counter++;
    }

    //Setup properties:
    win->wm = wm;
    win->flags = 0;
    win->name = name;
    win->x = x;
    win->y = y;
    win->xsize = xsize;
    win->ysize = ysize;
    win->color = color;
    win->parent = parent;
    win->host = host;
    win->win_handler = win_handler;
    win->click_time = time_ticks_hires() - time_ticks_per_second_hires() * 10;

    win->controllers_calculated = 0;
    win->controllers_defined = 0;

    win->action_handler = 0;
    win->handler_data = 0;
    win->action_result = 0;
    
    win->font = wm->default_font;

    //Start init:
    if( win_handler )
    {
	sundog_event* evt = &wm->empty_evt;
	evt->win = win;
	evt->type = EVT_GETDATASIZE;
	int datasize = win_handler( evt, wm );
	if( datasize > 0 )
	{
	    win->data = bmem_new( datasize );
	    bmem_zero( win->data );
	}
	evt->type = EVT_AFTERCREATE;
	win_handler( evt, wm );
    }

    //Save it to window manager:
    if( wm->root_win == 0 )
        wm->root_win = win;
    add_child( parent, win, wm );
    return win;
}

WINDOWPTR new_window( 
    const utf8_char* name, 
    int x, 
    int y, 
    int xsize, 
    int ysize, 
    COLOR color, 
    WINDOWPTR parent,
    win_handler_t win_handler,
    window_manager* wm )
{
    void* host = 0;
    if( parent )
    {
	host = parent->host;
    }
    return new_window( name, x, y, xsize, ysize, color, parent, host, win_handler, wm );
}

void set_window_controller( WINDOWPTR win, int ctrl_num, window_manager* wm, ... )
{
    va_list p;
    va_start( p, wm );
    uint ptr = 0;
    win->controllers_defined = 1;
    WCMD* cmds = 0;
    switch( ctrl_num )
    {
	case 0: cmds = win->x1com; break;
	case 1: cmds = win->y1com; break;
	case 2: cmds = win->x2com; break;
	case 3: cmds = win->y2com; break;
    }
    if( cmds == 0 )
	cmds = (WCMD*)bmem_new( sizeof( WCMD ) * 4 );
    while( 1 )
    {
	WCMD command = va_arg( p, WCMD );
	if( bmem_get_size( cmds ) / sizeof( WCMD ) <= ptr )
	    cmds = (WCMD*)bmem_resize( cmds, sizeof( WCMD ) * ( ptr + 4 ) );
	cmds[ ptr ] = command; 
	if( command == CEND ) break;
	ptr++;
    }
    switch( ctrl_num )
    {
	case 0: win->x1com = cmds; break;
	case 1: win->y1com = cmds; break;
	case 2: win->x2com = cmds; break;
	case 3: win->y2com = cmds; break;
    }
    va_end( p );
}

void move_to_trash( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
	win->visible = 0;
	win->flags |= WIN_FLAG_TRASH;
	if( wm->trash == 0 )
	{
	    wm->trash = (WINDOWPTR*)bmem_new( sizeof( WINDOWPTR ) * 16 );
	    bmem_set( wm->trash, sizeof( WINDOWPTR ) * 16, 0 );
	    wm->trash[ 0 ] = win;
	}
	else
	{
	    unsigned int w = 0;
	    for( w = 0; w < bmem_get_size( wm->trash ) / sizeof( WINDOWPTR ); w++ )
	    {
		if( wm->trash[ w ] == 0 ) break;
	    }
	    if( w < bmem_get_size( wm->trash ) / sizeof( WINDOWPTR ) ) 
		wm->trash[ w ] = win;
	    else
	    {
		wm->trash = (WINDOWPTR*)bmem_resize( wm->trash, bmem_get_size( wm->trash ) + sizeof( WINDOWPTR ) * 16 );
		wm->trash[ w ] = win;
	    }
	}
    }
}

WINDOWPTR get_from_trash( window_manager* wm )
{
    if( wm->trash == 0 ) return 0;
    unsigned int w = 0;
    for( w = 0; w < bmem_get_size( wm->trash ) / sizeof( WINDOWPTR ); w++ )
    {
	if( wm->trash[ w ] != 0 ) 
	{
	    WINDOWPTR win = wm->trash[ w ];
	    wm->trash[ w ] = 0;
	    win->id = (int16)wm->window_counter;
	    wm->window_counter++;
	    win->flags = 0;
	    return win;
	}
    }
    return 0;
}

void remove_window( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
	if( win->flags & WIN_FLAG_TRASH )
	{
	    blog( "ERROR: can't remove already removed window (%s)\n", win->name );
	    return;
	}
	if( win->win_handler )
	{
	    //Sent EVT_BEFORECLOSE to window handler:
	    sundog_event* evt = &wm->empty_evt;
	    evt->win = win;
	    evt->type = EVT_BEFORECLOSE;
	    win->win_handler( evt, wm );
	}
	if( win->childs )
	{
	    //Remove childs:
	    while( win->childs_num )
		remove_window( win->childs[ 0 ], wm );
	    bmem_free( win->childs );
	    win->childs = 0;
	}
	//Remove data:
	if( win->data ) 
	    bmem_free( win->data );
	win->data = 0;
	//Remove region:
	if( win->reg ) 
	    GdDestroyRegion( win->reg );
	win->reg = 0;
	//Remove commands:
	bmem_free( win->x1com ); win->x1com = 0;
	bmem_free( win->y1com ); win->y1com = 0;
	bmem_free( win->x2com ); win->x2com = 0;
	bmem_free( win->y2com ); win->y2com = 0;
	//Remove window:
	if( win == wm->focus_win )
	    wm->focus_win = 0;
	if( win == wm->root_win )
	{
	    //It's root win:
	    move_to_trash( win, wm );
	    wm->root_win = 0;
	}
	else
	{
	    remove_child( win->parent, win, wm );
	    //Full remove:
	    move_to_trash( win, wm );
	}
    }
}

void add_child( WINDOWPTR win, WINDOWPTR child, window_manager* wm )
{
    if( win == 0 || child == 0 ) return;

    if( win->childs == 0 )
    {
	win->childs = (WINDOWPTR*)bmem_new( sizeof( WINDOWPTR ) * 4 );
    }
    else
    {
	size_t old_size = bmem_get_size( win->childs ) / sizeof( WINDOWPTR );
	if( (unsigned)win->childs_num >= old_size )
	    win->childs = (WINDOWPTR*)bmem_resize( win->childs, ( old_size + 4 ) * sizeof( WINDOWPTR ) );
    }
    win->childs[ win->childs_num ] = child;
    win->childs_num++;
    child->parent = win;
}

void remove_child( WINDOWPTR win, WINDOWPTR child, window_manager* wm )
{
    if( win == 0 || child == 0 ) return;

    //Remove link from parent window:
    int c;
    for( c = 0; c < win->childs_num; c++ )
    {
	if( win->childs[ c ] == child ) break;
    }
    if( c < win->childs_num )
    {
	for( ; c < win->childs_num - 1; c++ )
	{
	    win->childs[ c ] = win->childs[ c + 1 ];
	}
	win->childs_num--;
	child->parent = 0;
    }
}

WINDOWPTR get_parent_by_win_handler( WINDOWPTR win, win_handler_t win_handler )
{
    WINDOWPTR rv = 0;
    while( 1 )
    {
	if( win->win_handler == win_handler )
	{
	    rv = win;
	    break;
	}
	win = win->parent;
	if( win == 0 ) break;
    }
    if( rv == 0 )
    {
	blog( "Can't find described parent for window %s\n", win->name );
    }
    return rv;
}

static WINDOWPTR get_child_by_win_handler2( WINDOWPTR win, win_handler_t win_handler, bool silent )
{
    WINDOWPTR rv = 0;
    while( 1 )
    {
	if( win->win_handler == win_handler )
	{
	    rv = win;
	    break;
	}
	for( int i = 0; i < win->childs_num; i++ )
	{
	    WINDOWPTR w = win->childs[ i ];
	    if( w )
	    {
		if( w->win_handler == win_handler )
		{
		    rv = w;
		    break;
		}
	    }
	}
	if( rv ) break;
	for( int i = 0; i < win->childs_num; i++ )
	{
	    WINDOWPTR w = get_child_by_win_handler2( win->childs[ i ], win_handler, true );
	    if( w )
	    {
		rv = w;
		break;
	    }
	}
	break;
    }
    if( rv == 0 && !silent )
    {
	blog( "Can't find described child for window %s\n", win->name );
    }
    return rv;
}

WINDOWPTR get_child_by_win_handler( WINDOWPTR win, win_handler_t win_handler )
{	
    return get_child_by_win_handler2( win, win_handler, false );
}

void set_handler( WINDOWPTR win, win_action_handler_t handler, void* handler_data, window_manager* wm )
{
    if( win )
    {
	win->action_handler = handler;;
	win->handler_data = handler_data;
    }
}
 
bool is_window_visible( WINDOWPTR win, window_manager* wm )
{
    if( win && win->visible && win->reg )
    {
	if( win->reg->numRects )
	{
	    return 1;
	}
    }
    return 0;
}

void draw_window( WINDOWPTR win, window_manager* wm )
{
    if( wm->screen_buffer_preserved == 0 ) 
    {
	screen_changed( wm );
	return;
    }
    if( win && win->visible && win->reg )
    {
	win_draw_lock( win, wm );
	if( win->reg->numRects || ( win->flags & WIN_FLAG_ALWAYS_HANDLE_DRAW_EVT ) )
	{
	    sundog_event* evt = &wm->empty_evt;
    	    evt->win = win;
	    evt->type = EVT_DRAW;
	    if( win->win_handler && win->win_handler( evt, wm ) )
	    {
		//Draw event was handled
	    }
	    else
	    {
	        win_draw_frect( win, 0, 0, win->xsize, win->ysize, win->color, wm );
	    }
	}
	if( win->childs_num )
	{
	    for( int c = 0; c < win->childs_num; c++ )
	    {
		draw_window( win->childs[ c ], wm );
	    }
	}
	win_draw_unlock( win, wm );
    }
}

void show_window( WINDOWPTR win, window_manager* wm )
{
    if( win && ( win->flags & WIN_FLAG_ALWAYS_INVISIBLE ) == 0 )
    {
	if( win->visible == 0 )
	{
	    sundog_event* evt = &wm->empty_evt;
	    evt->win = win;
	    evt->type = EVT_BEFORESHOW;
	    win->win_handler( evt, wm );
	}
	win->visible = 1;
	for( int c = 0; c < win->childs_num; c++ )
	{
	    show_window( win->childs[ c ], wm );
	}
    }
}

void hide_window( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
	if( wm->focus_win == win )
	{
	    //We are focused on hidden window. It is wrong
	    wm->focus_win = 0;
	}
	win->visible = 0;
	for( int c = 0; c < win->childs_num; c++ )
	{
	    hide_window( win->childs[ c ], wm );
	}
    }
}

void recalc_controllers( WINDOWPTR win, window_manager* wm );

//Controller Virtual Machine:
static inline void cvm_math_op( int* val1, int val2, int mode )
{
    switch( mode )
    {
        case 0: // =
    	    *val1 = val2; 
    	    break;
        case 1: // SUB
    	    *val1 -= val2; 
    	    break;
        case 2: // ADD
    	    *val1 += val2; 
    	    break;
        case 3: // MAX
    	    if( *val1 > val2 ) *val1 = val2; 
    	    break;
        case 4: // MIN
    	    if( *val1 < val2 ) *val1 = val2; 
    	    break;
    	case 5: // MUL, DIV256
    	    *val1 = ( *val1 * val2 ) >> 8;
    	    break;
    }
}
static void cvm_exec( WINDOWPTR win, WCMD* c, int* val, int size, window_manager* wm )
{
    if( c == 0 ) return;
    int p = 0;
    WINDOWPTR other_win = 0;
    int mode = 0;
    int perc = 0;
    int backval = 0;
    bool brk = false;
    while( !brk )
    {
	switch( c[ p ] )
	{
	    case CWIN: 
		p++;
		other_win = (WINDOWPTR)c[ p ];
		if( other_win->controllers_calculated == 0 ) recalc_controllers( other_win, wm );
		break;
	    case CX1:
		cvm_math_op( val, other_win->x, mode );
		mode = 0;
		perc = 0;
		break;
	    case CY1: 
		cvm_math_op( val, other_win->y, mode );
		mode = 0;
		perc = 0;
		break;
	    case CX2:
		cvm_math_op( val, other_win->x + other_win->xsize, mode );
		mode = 0;
		perc = 0;
		break;
	    case CY2: 
		cvm_math_op( val, other_win->y + other_win->ysize, mode );
		mode = 0;
		perc = 0;
		break;
	    case CXSIZE:
		cvm_math_op( val, other_win->xsize, mode );
		mode = 0;
		perc = 0;
		break;
	    case CYSIZE: 
		cvm_math_op( val, other_win->ysize, mode );
		mode = 0;
		perc = 0;
		break;
	    case CSUB: mode = 1; break;
	    case CADD: mode = 2; break;
	    case CPERC: perc = 1; break;
	    case CBACKVAL0: backval = 0; break;
	    case CBACKVAL1: backval = 1; break;
	    case CMAXVAL: mode = 3; break;
	    case CMINVAL: mode = 4; break;
	    case CMULDIV256: mode = 5; break;
	    case CPUTR0: wm->creg0 = *val; break;
	    case CGETR0: *val = wm->creg0; break;
	    case CR0:	
		cvm_math_op( val, wm->creg0, mode );
		mode = 0;
		perc = 0;
		break;
	    case CEND: brk = true; break;
	    default:
		if( perc )
		{
		    cvm_math_op( val, ( (int)c[ p ] * size ) / 100, mode );
		}
		else
		{
		    if( backval )
		    {
			cvm_math_op( val, size - (int)c[ p ], mode );
		    }
		    else
		    {
			cvm_math_op( val, (int)c[ p ], mode ); //Just a number
		    }
		}
		mode = 0;
		perc = 0;
		break;
	}
	p++;
    }
}

void bring_to_front( WINDOWPTR win, window_manager* wm )
{
    if( win == 0 ) return;
    if( win->parent == 0 ) return;
    
    int i;
    for( i = 0; i < win->parent->childs_num; i++ )
    {
	if( win->parent->childs[ i ] == win ) break;
    }
    if( i < win->parent->childs_num - 1 )
    {
	for( int i2 = i; i2 < win->parent->childs_num - 1; i2++ )
	    win->parent->childs[ i2 ] = win->parent->childs[ i2 + 1 ];
	win->parent->childs[ win->parent->childs_num - 1 ] = win;
    }
}

void recalc_controllers( WINDOWPTR win, window_manager* wm )
{
    if( win && win->controllers_calculated == 0 && win->parent )
    {
	if( win->controllers_defined == 0 || ( win->flags & WIN_FLAG_DONT_USE_CONTROLLERS ) ) 
	    win->controllers_calculated = 1;
	else
	{
	    int x1 = win->x;
	    int y1 = win->y;
	    int x2 = win->x + win->xsize;
	    int y2 = win->y + win->ysize;
	    cvm_exec( win, win->x1com, &x1, win->parent->xsize, wm );
	    cvm_exec( win, win->x2com, &x2, win->parent->xsize, wm );
	    cvm_exec( win, win->y1com, &y1, win->parent->ysize, wm );
	    cvm_exec( win, win->y2com, &y2, win->parent->ysize, wm );
	    int temp;
	    if( x1 > x2 ) { temp = x1; x1 = x2; x2 = temp; }
	    if( y1 > y2 ) { temp = y1; y1 = y2; y2 = temp; }
	    win->x = x1;
	    win->y = y1;
	    win->xsize = x2 - x1;
	    win->ysize = y2 - y1;
	    win->controllers_calculated = 1;
	}
    }
}

//reg - global mask (busy area);
//      initially empty;
//      will be the sum of all app regions.
void recalc_region( WINDOWPTR win, MWCLIPREGION* reg, int cut_x, int cut_y, int cut_x2, int cut_y2, int px, int py, window_manager* wm )
{
    if( !win->visible )
    {
	if( win->reg ) GdDestroyRegion( win->reg );
	win->reg = 0;
	return;
    }
    if( win->controllers_defined && win->controllers_calculated == 0 )
    {
	recalc_controllers( win, wm );
    }
    win->screen_x = win->x + px;
    win->screen_y = win->y + py;
    int x1 = win->x + px;
    int y1 = win->y + py;
    int x2 = win->x + px + win->xsize;
    int y2 = win->y + py + win->ysize;
    if( cut_x > x1 ) x1 = cut_x;
    if( cut_y > y1 ) y1 = cut_y;
    if( cut_x2 < x2 ) x2 = cut_x2;
    if( cut_y2 < y2 ) y2 = cut_y2;
    if( win->childs_num && !( x1 > x2 || y1 > y2 ) )
    {
	for( int c = win->childs_num - 1; c >= 0; c-- )
	{
	    recalc_region( 
		win->childs[ c ], 
		reg, 
		x1, y1, 
		x2, y2, 
		win->x + px, 
		win->y + py, 
		wm );
	}
    }
    if( win->reg == 0 )
    {
	if( x1 > x2 || y1 > y2 )
	    win->reg = GdAllocRegion();
	else
	    win->reg = GdAllocRectRegion( x1, y1, x2, y2 );
    }
    else
    {
	if( x1 > x2 || y1 > y2 )
	{
	    GdSetRectRegion( win->reg, 0, 0, 0, 0 );
	}
	else
	{
	    GdSetRectRegion( win->reg, x1, y1, x2, y2 );
	}
    }
    //Calc corrected win region:
    GdSubtractRegion( win->reg, win->reg, reg ); //win->reg = win->reg - reg
    //Calc corrected invisible region:
    GdUnionRegion( reg, reg, win->reg ); //reg = reg + win->reg
}

void clean_regions( WINDOWPTR win, window_manager* wm )
{
    if( win )
    {
	for( int c = 0; c < win->childs_num; c++ )
	    clean_regions( win->childs[ c ], wm );
	if( win->reg ) 
	{
	    GdSetRectRegion( win->reg, 0, 0, 0, 0 );
	}
	win->controllers_calculated = 0;
    }
}

void recalc_regions( window_manager* wm )
{
    MWCLIPREGION* reg = GdAllocRegion();
    if( wm->root_win )
    {
	//Control ALWAYS_ON_TOP flag: ===
	int size = wm->root_win->childs_num;
	for( int i = 0; i < size; i++ )
	{
	    WINDOWPTR win = wm->root_win->childs[ i ];
	    if( win && ( win->flags & WIN_FLAG_ALWAYS_ON_TOP ) && i < size - 1 )
	    {
		//Bring this window to front:
		for( int i2 = i; i2 < size - 1; i2++ )
		{
		    wm->root_win->childs[ i2 ] = wm->root_win->childs[ i2 + 1 ];
		}
		wm->root_win->childs[ size - 1 ] = win;
	    }
	}
	//===============================
	clean_regions( wm->root_win, wm );
	recalc_region( wm->root_win, reg, 0, 0, wm->screen_xsize, wm->screen_ysize, 0, 0, wm );
    }
    if( reg ) GdDestroyRegion( reg );
    //Don't put any immediate event handlers here!
    //recalc_regions() should not draw!
}

void set_focus_win( WINDOWPTR win, window_manager* wm )
{
    if( win && ( win->flags & WIN_FLAG_TRASH ) )
    {
	//This window removed by someone. But we can't focus on removed window.
	//So.. Just focus on NULL window:
	win = 0;
    }

    sundog_event evt;
    bmem_set( &evt, sizeof( sundog_event ), 0 );
    evt.time = time_ticks_hires();

    //Unfocus:
    
    WINDOWPTR prev_focus_win = wm->focus_win;
    uint16 prev_focus_win_id = wm->focus_win_id;

    WINDOWPTR new_focus_win = win;
    uint16 new_focus_win_id = 0;
    if( win ) new_focus_win_id = win->id;
    
    if( prev_focus_win != new_focus_win || 
	( prev_focus_win == new_focus_win && prev_focus_win_id != new_focus_win_id ) )
    {
	//Focus changed:
	
	if( prev_focus_win && !( prev_focus_win->flags & WIN_FLAG_TRASH ) )
	{
	    //Send UNFOCUS event:
	    evt.type = EVT_UNFOCUS;
	    evt.win = prev_focus_win;
	    if( !( evt.win->flags & WIN_FLAG_UNFOCUS_HANDLING ) )
	    {
		evt.win->flags |= WIN_FLAG_UNFOCUS_HANDLING;
		handle_event( &evt, wm );
		evt.win->flags &= ~WIN_FLAG_UNFOCUS_HANDLING;
	    }
	}
    }
    
    //Focus on the new window:

    prev_focus_win = wm->focus_win;
    prev_focus_win_id = wm->focus_win_id;

    new_focus_win = win;
    new_focus_win_id = 0;
    if( win ) new_focus_win_id = win->id;

    wm->focus_win = new_focus_win;
    wm->focus_win_id = new_focus_win_id;

    if( prev_focus_win != new_focus_win || 
	( prev_focus_win == new_focus_win && prev_focus_win_id != new_focus_win_id ) )
    {
	//Focus changed:

        wm->prev_focus_win = prev_focus_win;
        wm->prev_focus_win_id = prev_focus_win_id;

	if( new_focus_win )
	{
	    //Send FOCUS event:
	    //In this event's handling user can remember previous focused window (wm->prev_focus_win)
	    evt.type = EVT_FOCUS;
	    evt.win = new_focus_win;
	    if( !( evt.win->flags & WIN_FLAG_FOCUS_HANDLING ) )
	    {
		evt.win->flags |= WIN_FLAG_FOCUS_HANDLING;
		handle_event( &evt, wm );
		evt.win->flags &= ~WIN_FLAG_FOCUS_HANDLING;
	    }
	}
    }
}

int find_focus_window( WINDOWPTR win, WINDOW** focus_win, window_manager* wm )
{
    if( win == 0 ) return 0;
    if( win->reg && GdPtInRegion( win->reg, wm->pen_x, wm->pen_y ) )
    {
	*focus_win = win;
	return 1;
    }
    for( int c = 0; c < win->childs_num; c++ )
    {
	if( find_focus_window( win->childs[ c ], focus_win, wm ) )
	    return 1;
    }
    return 0;
}

void convert_real_window_xy( int& x, int& y, window_manager* wm )
{
    int temp;
    switch( wm->screen_angle )
    {
	case 1:
    	    temp = x;
    	    x = ( wm->real_window_height - 1 ) - y;
    	    y = temp;
    	    break;
    	case 2:
    	    x = ( wm->real_window_width - 1 ) - x;
    	    y = ( wm->real_window_height - 1 ) - y;
    	    break;
    	case 3:
    	    temp = x;
    	    x = y;
    	    y = ( wm->real_window_width - 1 ) - temp;
    	    break;
    }
	
    if( wm->screen_zoom )
    {
        x /= wm->screen_zoom;
        y /= wm->screen_zoom;
    }

#ifdef OPENGL
    if( wm->gl_xscale != 1 && wm->gl_yscale != 1 )
    {
	x = (int)( (float)x / wm->gl_xscale );
    	y = (int)( (float)y / wm->gl_xscale );
    }
#endif
}

#ifdef MULTITOUCH

//mode:
//  0 - touch down;
//  1 - touch move;
//  2 - touch up.
int collect_touch_events( int mode, uint touch_flags, uint evt_flags, int x, int y, int pressure, WM_TOUCH_ID touch_id, window_manager* wm )
{
    if( wm == 0 ) return -1;
    if( wm->wm_initialized == 0 ) return -1;
    
    if( wm->touch_evts_cnt >= WM_TOUCH_EVENTS )
	send_touch_events( wm );
    
    sundog_event* evt = &wm->touch_evts[ wm->touch_evts_cnt ];
    wm->touch_evts_cnt++;

    if( touch_flags & TOUCH_FLAG_REALWINDOW_XY )
    {
	convert_real_window_xy( x, y, wm );
    }
    
    if( ( touch_flags & TOUCH_FLAG_LIMIT ) || ( wm->flags & WIN_INIT_FLAG_FULLSCREEN ) )
    {
	if( x < -16 ) x = -16;
	if( y < -16 ) y = -16;
	if( x > wm->screen_xsize + 16 ) x = wm->screen_xsize + 16;
	if( y > wm->screen_ysize + 16 ) y = wm->screen_ysize + 16;
    }

    evt->time = time_ticks_hires();
    evt->win = 0;
    evt->flags = evt_flags;
    evt->x = x;
    evt->y = y;
    evt->key = MOUSE_BUTTON_LEFT;
    evt->pressure = pressure;
    evt->unicode = 0;
    
    switch( mode )
    {
        case 0:
    	    //Touch down:
	    for( uint t = 0; t < WM_TOUCHES; t++ )
	    {
	        if( wm->touch_busy[ t ] == false )
	        {
	    	    wm->touch_busy[ t ] = true;
		    wm->touch_id[ t ] = touch_id;
		    evt->scancode = t;
		    if( t == 0 )
		        evt->type = EVT_MOUSEBUTTONDOWN;
		    else
		        evt->type = EVT_TOUCHBEGIN;
		    break;
		}
	    }
	    break;
	case 1:
	    //Touch move:
	    for( uint t = 0; t < WM_TOUCHES; t++ )
	    {
	        if( wm->touch_busy[ t ] && wm->touch_id[ t ] == touch_id )
	        {
	    	    evt->scancode = t;
		    if( t == 0 )
		        evt->type = EVT_MOUSEMOVE;
		    else
		        evt->type = EVT_TOUCHMOVE;
		    break;
		}
	    }
	    break;
	case 2:
	    //Touch up:
	    for( uint t = 0; t < WM_TOUCHES; t++ )
	    {
	        if( wm->touch_busy[ t ] && wm->touch_id[ t ] == touch_id )
	        {
		    wm->touch_busy[ t ] = false;
		    evt->scancode = t;
		    if( t == 0 )
		        evt->type = EVT_MOUSEBUTTONUP;
		    else
		        evt->type = EVT_TOUCHEND;
		    break;
		}
	    }
	    break;
    }
    
    return 0;
}

int send_touch_events( window_manager* wm )
{
    if( wm == 0 ) return -1;
    if( wm->wm_initialized == 0 ) return -1;
    if( wm->touch_evts_cnt == 0 ) return 0;
    send_events( wm->touch_evts, wm->touch_evts_cnt, wm );
    wm->touch_evts_cnt = 0;
    return 0;
}

#endif

int send_events(
    sundog_event* events,
    int events_num,
    window_manager* wm )
{
    if( wm == 0 ) return -1;
    if( wm->wm_initialized == 0 ) return -1;
    int retval = 0;
    
    bmutex_lock( &wm->events_mutex );

    for( int e = 0; e < events_num; e++ )
    {
	sundog_event* evt = &events[ e ];
	if( wm->events_count + 1 <= WM_EVENTS )
	{
	    //Get pointer to new event:
	    int new_ptr = ( wm->current_event_num + wm->events_count ) & ( WM_EVENTS - 1 );
	
	    if( evt->type == EVT_BUTTONDOWN || evt->type == EVT_BUTTONUP )
	    {
		if( evt->key >= 0x41 && evt->key <= 0x5A ) //Capital
		    evt->key += 0x20; //Make it small
	    }
	
	    //Save new event to FIFO buffer:
	    bmem_copy( &wm->events[ new_ptr ], evt, sizeof( sundog_event ) );
	
	    //Increment number of unhandled events:
	    volatile int new_event_count = wm->events_count + 1;
	    wm->events_count = new_event_count;
	}
	else
	{
	    retval = 1;
	    break;
	}
    }
    
    bmutex_unlock( &wm->events_mutex );
    
    return retval;
}

int send_event(
    WINDOWPTR win,
    int type,
    uint flags,
    int x,
    int y,
    int key,
    int scancode,
    int pressure,
    utf32_char unicode,
    window_manager* wm )
{
    if( wm == 0 ) return -1;
    if( wm->wm_initialized == 0 ) return -1;
    int retval = 0;
    
    bmutex_lock( &wm->events_mutex );
    
    if( wm->events_count + 1 <= WM_EVENTS )
    {
	//Get pointer to new event:
	int new_ptr = ( wm->current_event_num + wm->events_count ) & ( WM_EVENTS - 1 );

	if( type == EVT_BUTTONDOWN || type == EVT_BUTTONUP )
	{
	    if( key >= 0x41 && key <= 0x5A ) //Capital
		key += 0x20; //Make it small
	}

	//Save new event to FIFO buffer:
	wm->events[ new_ptr ].type = (uint16)type;
	wm->events[ new_ptr ].time = time_ticks_hires();
	wm->events[ new_ptr ].win = win;
	wm->events[ new_ptr ].flags = (uint16)flags;
	wm->events[ new_ptr ].x = (int16)x;
	wm->events[ new_ptr ].y = (int16)y;
	wm->events[ new_ptr ].key = (uint16)key;
	wm->events[ new_ptr ].scancode = (uint16)scancode;
	wm->events[ new_ptr ].pressure = (uint16)pressure;
	wm->events[ new_ptr ].unicode = unicode;

	//Increment number of unhandled events:
	wm->events_count = wm->events_count + 1;
    }
    else
    {
	retval = 1;
    }

    bmutex_unlock( &wm->events_mutex );
    
    return retval;
}

int check_event( sundog_event* evt, window_manager* wm )
{
    if( evt == 0 ) return 1;

    if( evt->win == 0 )
    {
	if( !wm->wm_initialized ) return 1;
	
	if( evt->type == EVT_MOUSEBUTTONDOWN ||
	    evt->type == EVT_MOUSEBUTTONUP ||
	    evt->type == EVT_MOUSEMOVE )
	{
	    wm->pen_x = evt->x;
	    wm->pen_y = evt->y;
	    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	    if( wm->last_unfocused_window )
	    {
		evt->win = wm->last_unfocused_window;
		if( evt->type == EVT_MOUSEBUTTONUP )
		{
		    wm->last_unfocused_window = 0;
		}
		return 0;
	    }
	    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	    if( evt->type == EVT_MOUSEBUTTONDOWN )
	    { //If mouse click:
		if( evt->key & MOUSE_BUTTON_SCROLLUP ||
		    evt->key & MOUSE_BUTTON_SCROLLDOWN )
		{
		    //Mouse scroll up/down...
		    WINDOWPTR scroll_win = 0;
		    if( find_focus_window( wm->root_win, &scroll_win, wm ) )
		    {
			evt->win = scroll_win;
			return 0;
		    }
		    else
		    {
			//Window not found under the pointer:
			return 1;
		    }
		}
		else
		{
		    //Mouse click on some window...
		    WINDOWPTR focus_win = 0;
		    if( find_focus_window( wm->root_win, &focus_win, wm ) )
		    {
			//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			if( focus_win->flags & WIN_FLAG_ALWAYS_UNFOCUSED )
			{
			    evt->win = focus_win;
			    wm->last_unfocused_window = focus_win;
			    return 0;
			}
			//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			set_focus_win( focus_win, wm );
		    }
		    else
		    {
			//Window not found under the pointer:
			return 1;
		    }
		}
	    }
	}
	if( wm->focus_win )
	{
	    //Set pointer to window:
	    evt->win = wm->focus_win;

	    if( evt->type == EVT_MOUSEBUTTONDOWN )
	    {
		if( ( evt->time - wm->focus_win->click_time ) < ( wm->double_click_time * time_ticks_per_second_hires() ) / 1000 )
		{
		    //Double click detected:
		    wm->focus_win->flags |= WIN_FLAG_DOUBLECLICK;
		    wm->focus_win->click_time = evt->time - time_ticks_per_second_hires() * 10; //Reset click time
		}
		else
		{
		    //One click. Clear double-click flags:
		    wm->focus_win->flags &= ~WIN_FLAG_DOUBLECLICK;
		    wm->focus_win->click_time = evt->time;
		}
	    }
	    if( wm->focus_win->flags & WIN_FLAG_DOUBLECLICK )
	    {
		evt->flags |= EVT_FLAG_DOUBLECLICK;
	    }
	}
    }

    return 0;
}

void handle_event( sundog_event* evt, window_manager* wm )
{
    sundog_event evt2 = *evt;
    if( !user_event_handler( &evt2, wm ) )
    {
	//Event hot handled by first event handler.
	//Send it to window:
	if( handle_event_by_window( &evt2, wm ) == 0 )
	{
	    evt2.win = wm->handler_of_unhandled_events;
	    handle_event_by_window( &evt2, wm );
	}
    }
}

int handle_event_by_window( sundog_event* evt, window_manager* wm )
{
    int retval = 0;
    WINDOWPTR win = evt->win;
    if( win )
    {
	if( win->flags & WIN_FLAG_TRASH )
	{
	    blog( "ERROR: can't handle event %d by removed window (%s)\n", evt->type, win->name );
	    retval = 1;
	}
	else
	if( evt->type == EVT_DRAW ) 
	{
	    draw_window( win, wm );
	    retval = 1;
	}
	else
	{
	    if( win->win_handler )
	    {
		if( !win->win_handler( evt, wm ) )
		{
		    //Send event to children:
		    for( int c = 0; c < win->childs_num; c++ )
		    {
			evt->win = win->childs[ c ];
			if( handle_event_by_window( evt, wm ) )
			{
			    evt->win = win;
			    retval = 1;
			    goto end_of_handle;
			}
		    }
		    evt->win = win;
		}
		else
		{
		    retval = 1;
		    goto end_of_handle;
		}
	    }
end_of_handle:
	    if( win == wm->root_win )
	    {
		if( evt->type == EVT_SCREENRESIZE )
		{
		    //On screen resize:
		    recalc_regions( wm );
		    draw_window( wm->root_win, wm );
		}
	    }
	}
    }
    return retval;
}

int EVENT_LOOP_BEGIN( sundog_event* evt, window_manager* wm )
{
    int rv = 0;
    wm->device_event_handler( wm );
    if( wm->exit_request ) return 0;
    if( wm->events_count )
    {
	bmutex_lock( &wm->events_mutex );

get_next_event:
	//There are unhandled events:
	//Copy current event to "evt" buffer (prepare it for handling):
	bmem_copy( evt, &wm->events[ wm->current_event_num ], sizeof( sundog_event ) );
	//This event will be handled. So decrement count of events:
	wm->events_count--;
	//And increment FIFO pointer:
	wm->current_event_num = ( wm->current_event_num + 1 ) & ( WM_EVENTS - 1 );
	if( evt->type == EVT_NULL && wm->events_count ) goto get_next_event;
#ifdef MULTITOUCH
	if( evt->flags & EVT_FLAG_DONTDRAW )
	{
	    for( int e = 0; e < wm->events_count; e++ )
            {
                sundog_event* next_evt = &wm->events[ ( wm->current_event_num + e ) & ( WM_EVENTS - 1 ) ];
                next_evt->flags |= EVT_FLAG_HANDLING;
                if( !( next_evt->flags & EVT_FLAG_DONTDRAW ) ) break;
            }
	}
#endif

	bmutex_unlock( &wm->events_mutex );
	
	//Check the event and handle it:
	if( check_event( evt, wm ) == 0 )
	{
	    handle_event( evt, wm );
	    rv = 1;
	}
    }
    return rv;
}

int EVENT_LOOP_END( window_manager* wm )
{
    if( wm->exit_request ) return 1;
    if( wm->frame_event_request )
    {
	if( g_mem_error )
	{
	    int size = (int)g_mem_error;
	    g_mem_error = 0;
	    char ts[ 512 ];
	    sprintf( ts, "Memory allocation error\nCan't allocate %d bytes", (int)size );
	    dialog( ts, "Close", wm );
	}
	if( wm->timers_num )
	{
	    ticks_t cur_time = time_ticks();
	    for( int t = 0; t < wm->timers_num; t++ )
	    {
		sundog_timer* timer = &wm->timers[ t ];
		int timer_id = timer->id;
		if( timer->handler )
		{
		    if( timer->deadline <= cur_time )
		    {
			timer->handler( timer->data, timer, wm ); 
			if( timer->handler && timer->id == timer_id )
			{
			    timer->deadline = time_ticks() + timer->delay;
			}
		    }
		}
	    }
	}
	user_event_handler( &wm->frame_evt, wm );
	wm->frame_event_request = 0;
    }
    if( wm->screen_buffer_preserved == 0 && wm->screen_changed )
    {
	//If buffer is not preserved,
	//gfx can be redrawn in this place only
	//Ways to this place:
	// 1) draw_window();
	// 2) some gfx function called;
	// 3) screen_change().
	wm->screen_buffer_preserved = 1;
#if !defined(NOVCAP)
	if( wm->vcap ) video_capture_frame_begin( wm );
#endif
	draw_window( wm->root_win, wm );
#if !defined(NOVCAP)
	if( wm->vcap ) video_capture_frame_end( wm );
#endif
	wm->screen_buffer_preserved = 0;
    }
    wm->device_redraw_framebuffer( wm ); //Show screen changes, only if they exists.
    if( wm->flags & WIN_INIT_FLAG_OPTIMIZE_MOVE_EVENTS )
    {
	//Optimize mouse and touch events,
	//but only if we can receive the batch of the events
	//(separate threads for SunDog loop and system event loop)
	if( wm->events_count )
	{
	    bmutex_lock( &wm->events_mutex );
	    bool move_flag = true;
#ifdef MULTITOUCH
	    uint mt_move_flags = 0xFFFFFFFF;
#endif
	    for( int i = wm->events_count - 1; i >= 0; i-- )
	    {
		int event_num = ( wm->current_event_num + i ) & ( WM_EVENTS - 1 );
		sundog_event* evt = &wm->events[ event_num ];
		if( evt->type == EVT_MOUSEBUTTONDOWN ||
		    evt->type == EVT_MOUSEBUTTONUP )
		{
		    move_flag = true;
		}
		else if( evt->type == EVT_MOUSEMOVE )
		{
		    if( move_flag == false && !( evt->flags & EVT_FLAG_HANDLING ) ) evt->type = EVT_NULL;
		    if( move_flag ) move_flag = false;
		}
#ifdef MULTITOUCH
		if( evt->type == EVT_TOUCHBEGIN ||
		    evt->type == EVT_TOUCHEND )
		{
		    mt_move_flags |= 1 << evt->scancode;
		}
		else if( evt->type == EVT_TOUCHMOVE )
		{
		    if( ( mt_move_flags & ( 1 << evt->scancode ) ) == 0 && !( evt->flags & EVT_FLAG_HANDLING ) ) evt->type = EVT_NULL;
		    if( mt_move_flags & ( 1 << evt->scancode ) ) mt_move_flags &= ~( 1 << evt->scancode );
		}
#endif
	    }
	    bmutex_unlock( &wm->events_mutex );
	}
    }
    return 0;
}

//
// Status messages
//

void status_timer( void* user_data, sundog_timer* t, window_manager* wm )
{
    hide_status_message( wm );
}

int status_message_handler( sundog_event* evt, window_manager* wm )
{	
    int retval = 0;
    WINDOWPTR win = evt->win;
    switch( evt->type )
    {
	case EVT_DRAW:
	    wbd_lock( win );
	    draw_frect( 0, 1, win->xsize, win->ysize, wm->color0, wm );
	    if( wm->status_message )
	    {
		wm->cur_font_color = wm->color3;
		draw_string( wm->status_message, 0, 1, wm );
	    }
	    draw_frect( 0, 0, win->xsize, 1, blend( win->color, BORDER_COLOR_WITHOUT_OPACITY, BORDER_OPACITY ), wm );
	    wbd_draw( wm );
	    wbd_unlock( wm );
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONUP:
	    hide_status_message( wm );
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    bmem_free( wm->status_message );
	    wm->status_message = 0;
	    retval = 1;
	    break;
    }
    return retval;
}

void show_status_message( const utf8_char* msg, int t, window_manager* wm )
{
    if( t == 0 ) t = 2;
    int lines = 1;
    for( int i = 0; ; i++ )
    {
	int c = msg[ i ];
	if( c == 0 ) break;
	if( c == 0xA ) lines++;
    }
    bmem_free( wm->status_message );
    wm->status_message = (utf8_char*)bmem_new( bmem_strlen( msg ) + 1 );
    wm->status_message[ 0 ] = 0;
    bmem_strcat_resize( wm->status_message, msg );
    if( wm->status_window == 0 )
    {
	wm->status_window = new_window( "status", 0, 0, 8, 8, wm->color0, wm->root_win, status_message_handler, wm );
	wm->status_timer = add_timer( status_timer, 0, time_ticks_per_second() * t, wm );
    }
    else 
    {
	reset_timer( wm->status_timer, time_ticks_per_second() * t, wm );
    }

    set_window_controller( wm->status_window, 0, wm, (WCMD)0, CEND );
    set_window_controller( wm->status_window, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)font_char_y_size( wm->status_window->font, wm ) * lines + 1, CEND );
    set_window_controller( wm->status_window, 2, wm, CPERC, (WCMD)100, CEND );
    set_window_controller( wm->status_window, 3, wm, CPERC, (WCMD)100, CEND );
    
    show_window( wm->status_window, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    wm->device_redraw_framebuffer( wm );
}

void hide_status_message( window_manager* wm )
{
    remove_window( wm->status_window, wm );
    remove_timer( wm->status_timer, wm );
    wm->status_window = 0;
    wm->status_timer = -1;
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
}

//
// Timers
//

void pack_timers( window_manager* wm )
{
    int r = 0;
    int w = 0;
    for( ; r < wm->timers_num; r++ )
    {
	if( wm->timers[ r ].handler )
	{
	    bmem_copy( &wm->timers[ w ], &wm->timers[ r ], sizeof( sundog_timer ) );
	    w++;
	}
    }
    wm->timers_num = w;
}

int add_timer( void (*handler)( void*, sundog_timer*, window_manager* ), void* data, ticks_t delay, window_manager* wm )
{
    if( wm->timers_num >= WM_TIMERS )
	return -1;
    int t = wm->timers_num;
    wm->timers[ t ].handler = handler;
    wm->timers[ t ].data = data;
    wm->timers[ t ].deadline = time_ticks() + delay;
    wm->timers[ t ].delay = delay;
    wm->timers_id_counter++;
    wm->timers_id_counter &= 0xFFFFFFF;
    wm->timers[ t ].id = wm->timers_id_counter;
    wm->timers_num++;
    return wm->timers[ t ].id;
}

void reset_timer( int timer, window_manager* wm )
{
    for( int i = 0; i < wm->timers_num; i++ )
    {
	if( wm->timers[ i ].id == timer )
	{
	    if( wm->timers[ i ].handler )
	    {
		wm->timers[ i ].deadline = time_ticks() + wm->timers[ i ].delay;
		pack_timers( wm );
	    }
	    break;
	}
    }
}

void reset_timer( int timer, ticks_t new_delay, window_manager* wm )
{
    for( int i = 0; i < wm->timers_num; i++ )
    {
	if( wm->timers[ i ].id == timer )
	{
	    if( wm->timers[ i ].handler )
	    {
		wm->timers[ i ].deadline = time_ticks() + new_delay;
		wm->timers[ i ].delay = new_delay;
		pack_timers( wm );
	    }
	    break;
	}
    }
}

void remove_timer( int timer, window_manager* wm )
{
    if( timer == -1 ) return;
    for( int i = 0; i < wm->timers_num; i++ )
    {
	if( wm->timers[ i ].id == timer )
	{
	    if( wm->timers[ i ].handler )
	    {
		wm->timers[ i ].handler = 0;
		pack_timers( wm );
	    }
	    break;
	}
    }
}

//
// Window decorations
//

#define DRAG_LEFT	1
#define DRAG_RIGHT	2
#define DRAG_TOP	4
#define DRAG_BOTTOM	8
#define DRAG_MOVE	16
struct decorator_data
{
    WINDOWPTR win;
    
    int start_win_x;
    int start_win_y;
    int start_win_xs;
    int start_win_ys;
    int start_pen_x;
    int start_pen_y;
    int drag_mode;
    bool fullscreen;
    bool minimized;
    int prev_x;
    int prev_y;
    int prev_xsize;
    int prev_ysize;
    uint flags;
    
    int border;
    int header;
    
    WINDOWPTR close;
    WINDOWPTR minimize;
    
    int initial_x;
    int initial_y;
    int initial_xsize;
    int initial_ysize;
};

int decorator_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    decorator_data* data = (decorator_data*)win->data;
    int dx, dy;
    int rx = evt->x - win->screen_x;
    int ry = evt->y - win->screen_y;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( decorator_data );
	    break;
	case EVT_AFTERCREATE:
	    data->win = win;
	    data->fullscreen = 0;
	    data->minimized = 0;
	    data->drag_mode = 0;
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	    if( data->flags & DECOR_FLAG_STATIC )
	    {
		retval = 1;
		break;
	    }
	    if( evt->key == MOUSE_BUTTON_LEFT )
	    {
		bring_to_front( win, wm );
		recalc_regions( wm );
		data->start_pen_x = evt->x;
		data->start_pen_y = evt->y;
		data->start_win_x = win->x;
		data->start_win_y = win->y;
		data->start_win_xs = win->xsize;
		data->start_win_ys = win->ysize;
		data->drag_mode = DRAG_MOVE;
		if( ry >= data->border && !( data->flags & DECOR_FLAG_NOBORDER ) && !( data->flags & DECOR_FLAG_NORESIZE ) )
		{
		    if( rx < data->border + 8 ) data->drag_mode |= DRAG_LEFT;
		    if( rx >= win->xsize - data->border - 8 ) data->drag_mode |= DRAG_RIGHT;
		    if( ry >= win->ysize - data->border - 8 ) data->drag_mode |= DRAG_BOTTOM;
		}
		draw_window( win, wm );
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONUP:
	    retval = 1;
	    if( data->drag_mode )
	    {
		data->drag_mode = 0;
		if( ( data->flags & DECOR_FLAG_STATIC ) ||
		    ( data->flags & DECOR_FLAG_NOBORDER ) )
		{
		    break;
		}
		if( ( evt->key == MOUSE_BUTTON_LEFT ) &&
		    ( evt->flags & EVT_FLAG_DOUBLECLICK ) )
		{
		    //Make fullscreen (maximize):
		    if( data->fullscreen == 1 )
		    {
			win->x = data->prev_x;
			win->y = data->prev_y;
			win->xsize = data->prev_xsize;
			win->ysize = data->prev_ysize;
		    }
		    else
		    {
			data->prev_x = win->x;
			data->prev_y = win->y;
			data->prev_xsize = win->xsize;
			data->prev_ysize = win->ysize;
			win->x = 0; 
			win->y = 0;
			win->xsize = win->parent->xsize;
			win->ysize = win->parent->ysize;
		    }
		    win->childs[ 0 ]->xsize = win->xsize - data->border * 2;
		    win->childs[ 0 ]->ysize = win->ysize - data->border * 2 - data->header;
		    data->fullscreen ^= 1;
		    recalc_regions( wm );
		    draw_window( wm->root_win, wm );
		}
	    }
	    break;
	case EVT_UNFOCUS:
	    data->drag_mode = 0;
	    retval = 1;
	    break;
	case EVT_MOUSEMOVE:
	    if( data->flags & DECOR_FLAG_STATIC )
	    {
		retval = 1;
		break;
	    }
	    if( evt->key == MOUSE_BUTTON_LEFT )
	    {
		dx = evt->x - data->start_pen_x;
		dy = evt->y - data->start_pen_y;
		if( data->drag_mode == DRAG_MOVE )
		{
		    //Move:
		    int prev_x = win->x;
		    int prev_y = win->y;
		    win->x = data->start_win_x + dx;
		    win->y = data->start_win_y + dy;
		    if( prev_x != win->x || prev_y != win->y )
		    {
			if( data->fullscreen == 1 )
			{
			    win->xsize = data->prev_xsize;
			    win->ysize = data->prev_ysize;
			    data->fullscreen = 0;
			}
		    }
		}
		
		if( !( data->flags & DECOR_FLAG_NOBORDER ) )
		{
		    if( data->drag_mode & DRAG_LEFT )
		    {
			int prev_x = win->x;
			int prev_xsize = win->xsize;
			win->x = data->start_win_x + dx;
			win->xsize = data->start_win_xs - dx;
			if( prev_x != win->x || prev_xsize != win->xsize )
			{
			    if( win->xsize < 16 ) win->xsize = 16;
			    data->fullscreen = 0;
			}
		    }
		    if( data->drag_mode & DRAG_RIGHT )
		    {
			int prev_xsize = win->xsize;
			win->xsize = data->start_win_xs + dx;
			if( prev_xsize != win->xsize )
			{
			    if( win->xsize < 16 ) win->xsize = 16;
			    data->fullscreen = 0;
			}
		    }
		    if( data->drag_mode & DRAG_BOTTOM )
		    {
			int prev_ysize = win->ysize;
			win->ysize = data->start_win_ys + dy;
			if( prev_ysize != win->ysize )
			{
			    if( win->ysize < 16 ) win->ysize = 16;
			    data->fullscreen = 0;
			}
		    }
		    if( win->childs_num )
		    {
			win->childs[ 0 ]->xsize = win->xsize - data->border * 2;
			win->childs[ 0 ]->ysize = win->ysize - data->border * 2 - data->header;
		    }
		} //!NOBORDER
		
		recalc_regions( wm );
		draw_window( wm->root_win, wm );
	    }
	    retval = 1;
	    break;
	case EVT_DRAW:
	    if( win->childs_num )
	    {
		wbd_lock( win );
		
		COLOR c;
		c = win->childs[ 0 ]->color;
		wm->cur_font_color = wm->header_text_color;
		if( data->flags & DECOR_FLAG_NOBORDER )
		{
		    draw_frect( 0, 0, win->xsize, win->ysize, c, wm );
		    if( win->childs[ 0 ]->name )
		    {
			draw_string( win->childs[ 0 ]->name, wm->interelement_space, ( data->header - char_y_size( wm ) ) / 2, wm );
		    }
		}
		else
		{
		    draw_frect( data->border, 0, win->xsize - data->border * 2, win->ysize, c, wm );
		    if( win->childs[ 0 ]->name )
		    {
			draw_string( win->childs[ 0 ]->name, data->border + wm->interelement_space, ( data->header + data->border - char_y_size( wm ) ) / 2, wm );
		    }
		    draw_frect( 0, 0, data->border, win->ysize, wm->decorator_border, wm );
		    draw_frect( win->xsize - data->border, 0, data->border, win->ysize, wm->decorator_border, wm );
		}

		wbd_draw( wm );
		wbd_unlock( wm );
	    }
	    else
	    {
		//No childs more :( Empty decorator. Lets remove it:
		remove_window( win, wm );
		recalc_regions( wm );
		draw_window( wm->root_win, wm );
	    }
	    retval = 1;
	    break;
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
	case EVT_SCREENRESIZE:
	    if( wm->flags & WIN_INIT_FLAG_FULLSCREEN )
		change_decorator( win, data->initial_x, data->initial_y, data->initial_xsize, data->initial_ysize, data->flags, wm );
	    break;
    }
    return retval;
}

void change_decorator(
    WINDOWPTR dec,
    int x, 
    int y, 
    int xsize, 
    int ysize, 
    uint flags,
    window_manager* wm )
{
    if( dec == 0 ) return;
    if( dec->childs == 0 ) return;
    WINDOWPTR win = dec->childs[ 0 ];
    
    decorator_data* ddata = (decorator_data*)dec->data;
    
    ddata->fullscreen = 0;
    ddata->flags = flags;
    ddata->initial_x = x;
    ddata->initial_y = y;
    ddata->initial_xsize = xsize;
    ddata->initial_ysize = ysize;
    
    int border = wm->decor_border_size;
    int header = wm->decor_header_size;
    
    if( flags & DECOR_FLAG_NOBORDER )
	border = 0;

    if( flags & DECOR_FLAG_NOHEADER )
	header = 0;
    
    if( flags & DECOR_FLAG_FULLSCREEN )
    {
	x = 0;
	y = 0;
	xsize = dec->parent->xsize;
	ysize = dec->parent->ysize;
    }
    
    x -= border;
    y -= border + header;
    int dec_xsize = xsize + border * 2;
    int dec_ysize = ysize + border * 2 + header;
    if( flags & DECOR_FLAG_CENTERED )
    {
	x = ( dec->parent->xsize - dec_xsize ) / 2;
	y = ( dec->parent->ysize - dec_ysize ) / 2;
    }
    if( flags & DECOR_FLAG_CHECK_SIZE )
    {
	if( x < 0 )
	{
	    dec_xsize -= ( -x );
	    x = 0;
	}
	if( y < 0 )
	{
	    dec_ysize -= ( -y );
	    y = 0;
	}
	if( x + dec_xsize > dec->parent->xsize )
	{
	    dec_xsize -= ( ( x + dec_xsize ) - dec->parent->xsize );
	}
	if( y + dec_ysize > dec->parent->ysize )
	{
	    dec_ysize -= ( ( y + dec_ysize ) - dec->parent->ysize );
	}
    }
    
    dec->x = x;
    dec->y = y;
    dec->xsize = dec_xsize;
    dec->ysize = dec_ysize;
    
    ddata->border = border;
    ddata->header = header;
        
    if( win )
    {
	win->xsize = dec->xsize - border * 2;
	win->ysize = dec->ysize - ( header + border * 2 );
	win->x = border;
	win->y = header + border;
    }
}

void resize_window_with_decorator( WINDOWPTR win, int xsize, int ysize, window_manager* wm )
{
    WINDOWPTR dec = win->parent;
    decorator_data* ddata = (decorator_data*)dec->data;
    if( xsize > 0 )
    {
	int prev_size = win->xsize;
	win->xsize = xsize;
	int delta = prev_size - win->xsize;
	dec->xsize -= delta;
	if( ddata->flags & DECOR_FLAG_CHECK_SIZE )
	{
	    if( dec->xsize > dec->parent->xsize )
	    {
		int s = dec->xsize - dec->parent->xsize;
		win->xsize -= s;
		dec->xsize -= s;
		delta += s;
	    }
	}
	if( ddata->flags & DECOR_FLAG_CENTERED )
	{
	    dec->x += delta / 2;
	}
    }
    if( ysize > 0 )
    {
	if( ddata->flags & DECOR_FLAG_CHECK_SIZE )
	{
	    if( ysize > dec->parent->ysize )
		ysize = dec->parent->ysize;
	}
	int prev_size = win->ysize;
	win->ysize = ysize;
	int delta = prev_size - win->ysize;
	dec->ysize -= delta;
	if( ddata->flags & DECOR_FLAG_CHECK_SIZE )
	{
	    if( dec->ysize > dec->parent->ysize )
	    {
		int s = dec->ysize - dec->parent->ysize;
		win->ysize -= s;
		dec->ysize -= s;
		delta += s;
	    }
	}
	if( ddata->flags & DECOR_FLAG_CENTERED )
	{
	    dec->y += delta / 2;
	}
    }
}

int decor_close_button_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    decorator_data* data = (decorator_data*)user_data;
    WINDOWPTR cwin = data->win->childs[ 0 ];
    
    if( cwin )
    {
	send_event( cwin, EVT_CLOSEREQUEST, 0, 0, 0, 0, 0, 0, 0, wm );
    }
    
    return 0;
}

int decor_minimize_button_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    decorator_data* data = (decorator_data*)user_data;
    WINDOWPTR dwin = data->win;
    WINDOWPTR cwin = dwin->childs[ 0 ];
    if( cwin == 0 ) return 0;
    
    //Minimize / maximize:
    if( data->minimized )
    {
	dwin->xsize = data->prev_xsize;
	dwin->ysize = data->prev_ysize;
	data->minimized = 0;
	win->name = "-";
    }
    else
    {
	data->prev_xsize = dwin->xsize;
	data->prev_ysize = dwin->ysize;
	dwin->xsize = data->border * 2 + wm->scrollbar_size;
	if( data->close ) dwin->xsize += wm->scrollbar_size;
	if( data->minimize ) dwin->xsize += wm->scrollbar_size;
	dwin->ysize = data->border + data->header;
	data->minimized = 1;
	win->name = "+";
    }
    cwin->xsize = dwin->xsize - data->border * 2;
    cwin->ysize = dwin->ysize - data->border * 2 - data->header;
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    
    return 0;
}

WINDOWPTR new_window_with_decorator( 
    const utf8_char* name, 
    int x, 
    int y, 
    int xsize, 
    int ysize, 
    COLOR color,
    WINDOWPTR parent,
    void* host,
    win_handler_t win_handler,
    uint flags,
    window_manager* wm )
{
    WINDOWPTR dec = new_window( 
	"decorator", 
	x, y, xsize, ysize,
	wm->decorator_color,
	parent,
	host,
	decorator_handler,
	wm );
    new_window( name, x, y, xsize, ysize, color, dec, host, win_handler, wm );
    change_decorator( dec, x, y, xsize, ysize, flags, wm );
    decorator_data* data = (decorator_data*)dec->data;
    int xx = data->border + wm->interelement_space;    
    if( flags & DECOR_FLAG_WITH_CLOSE )
    {
	wm->opt_button_flat = 1;
	data->close = new_window( "x", 0, 0, wm->small_button_xsize, data->header + data->border, dec->color, dec, host, button_handler, wm );
	set_handler( data->close, decor_close_button_handler, data, wm );
	set_window_controller( data->close, 0, wm, CPERC, (WCMD)100, CSUB, (WCMD)xx + wm->small_button_xsize, CEND );
	set_window_controller( data->close, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)xx, CEND );
	xx += wm->small_button_xsize;
    }
    if( flags & DECOR_FLAG_WITH_MINIMIZE )
    {
	wm->opt_button_flat = 1;
	data->minimize = new_window( "-", 0, 0, wm->small_button_xsize, data->header + data->border, dec->color, dec, host, button_handler, wm );
	set_handler( data->minimize, decor_minimize_button_handler, data, wm );
	set_window_controller( data->minimize, 0, wm, CPERC, (WCMD)100, CSUB, (WCMD)xx + wm->small_button_xsize, CEND );
	set_window_controller( data->minimize, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)xx, CEND );
	xx += wm->small_button_xsize;
    }
    return dec;
}

WINDOWPTR new_window_with_decorator( 
    const utf8_char* name, 
    int x, 
    int y, 
    int xsize, 
    int ysize, 
    COLOR color,
    WINDOWPTR parent,
    win_handler_t win_handler,
    uint flags,
    window_manager* wm )
{
    void* host = 0;
    if( parent )
    {
	host = parent->host;
    }
    return new_window_with_decorator( name, x, y, xsize, ysize, color, parent, host, win_handler, flags, wm );
}

//
// Drawing functions
//

void win_draw_lock( WINDOWPTR win, window_manager* wm )
{
    wm->cur_opacity = 255;
    wm->device_screen_lock( win, wm );
}

void win_draw_unlock( WINDOWPTR win, window_manager* wm )
{
    wm->device_screen_unlock( win, wm );
}

void win_draw_rect( WINDOWPTR win, int x, int y, int xsize, int ysize, COLOR color, window_manager* wm )
{
    win_draw_line( win, x, y, x + xsize - 1, y, color, wm );
    win_draw_line( win, x + xsize - 1, y, x + xsize - 1, y + ysize - 1, color, wm );
    win_draw_line( win, x + xsize - 1, y + ysize - 1, x, y + ysize - 1, color, wm );
    win_draw_line( win, x, y + ysize - 1, x, y, color, wm );
}

void win_draw_frect( WINDOWPTR win, int x, int y, int xsize, int ysize, COLOR color, window_manager* wm )
{
    if( win && win->visible && win->reg && win->reg->numRects )
    {
	x += win->screen_x;
	y += win->screen_y;
	if( win->reg->numRects )
	{
	    for( int r = 0; r < win->reg->numRects; r++ )
	    {
		int rx1 = win->reg->rects[ r ].left;
		int rx2 = win->reg->rects[ r ].right;
		int ry1 = win->reg->rects[ r ].top;
		int ry2 = win->reg->rects[ r ].bottom;

		//Control box size:
		int nx = x;
		int ny = y;
		int nxsize = xsize;
		int nysize = ysize;
		if( nx < rx1 ) { nxsize -= ( rx1 - nx ); nx = rx1; }
		if( ny < ry1 ) { nysize -= ( ry1 - ny ); ny = ry1; }
		if( nx + nxsize <= rx1 ) continue;
		if( ny + nysize <= ry1 ) continue;
		if( nx + nxsize > rx2 ) nxsize -= nx + nxsize - rx2;
		if( ny + nysize > ry2 ) nysize -= ny + nysize - ry2;
		if( nx >= rx2 ) continue;
		if( ny >= ry2 ) continue;
		if( nxsize < 0 ) continue;
		if( nysize < 0 ) continue;
        	
		//Draw it:
		wm->device_draw_frect( nx, ny, nxsize, nysize, color, wm );
		wm->screen_changed++;
	    }
	}
    }
}

void win_draw_image_ext( 
    WINDOWPTR win, 
    int x, 
    int y, 
    int dest_xsize, 
    int dest_ysize,
    int source_x,
    int source_y,
    sundog_image* img, 
    window_manager* wm )
{
    if( source_x < 0 ) { dest_xsize += source_x; x -= source_x; source_x = 0; }
    if( source_y < 0 ) { dest_ysize += source_y; y -= source_y; source_y = 0; }
    if( source_x >= img->xsize ) return;
    if( source_y >= img->ysize ) return;
    if( source_x + dest_xsize > img->xsize ) dest_xsize -= ( source_x + dest_xsize ) - img->xsize;
    if( source_y + dest_ysize > img->ysize ) dest_ysize -= ( source_y + dest_ysize ) - img->ysize;
    if( dest_xsize <= 0 ) return;
    if( dest_ysize <= 0 ) return;
    if( win && win->visible && win->reg && win->reg->numRects )
    {
	x += win->screen_x;
	y += win->screen_y;
	int xsize = dest_xsize;
	int ysize = dest_ysize;
	if( win->reg->numRects )
	{
	    for( int r = 0; r < win->reg->numRects; r++ )
	    {
		int rx1 = win->reg->rects[ r ].left;
		int rx2 = win->reg->rects[ r ].right;
		int ry1 = win->reg->rects[ r ].top;
		int ry2 = win->reg->rects[ r ].bottom;

		//Control box size:
		int src_x = source_x;
		int src_y = source_y;
		int nx = x;
		int ny = y;
		int nxsize = xsize;
		int nysize = ysize;
		if( nx < rx1 ) { nxsize -= ( rx1 - nx ); src_x += ( rx1 - nx ); nx = rx1; }
		if( ny < ry1 ) { nysize -= ( ry1 - ny ); src_y += ( ry1 - ny ); ny = ry1; }
		if( nx + nxsize <= rx1 ) continue;
		if( ny + nysize <= ry1 ) continue;
		if( nx + nxsize > rx2 ) nxsize -= nx + nxsize - rx2;
		if( ny + nysize > ry2 ) nysize -= ny + nysize - ry2;
		if( nx >= rx2 ) continue;
		if( ny >= ry2 ) continue;
		if( nxsize < 0 ) continue;
		if( nysize < 0 ) continue;
        	
		//Draw it:
		wm->device_draw_image( nx, ny, nxsize, nysize, src_x, src_y, img, wm );
		wm->screen_changed++;
	    }
	}
    }
}

void win_draw_image( 
    WINDOWPTR win, 
    int x, 
    int y, 
    sundog_image* img, 
    window_manager* wm )
{
    if( win && win->visible && win->reg && win->reg->numRects )
    {
	x += win->screen_x;
	y += win->screen_y;
	if( win->reg->numRects )
	{
	    for( int r = 0; r < win->reg->numRects; r++ )
	    {
		int rx1 = win->reg->rects[ r ].left;
		int rx2 = win->reg->rects[ r ].right;
		int ry1 = win->reg->rects[ r ].top;
		int ry2 = win->reg->rects[ r ].bottom;

		//Control box size:
		int src_x = 0;
		int src_y = 0;
		int nx = x;
		int ny = y;
		int nxsize = img->xsize;
		int nysize = img->ysize;
		if( nx < rx1 ) { nxsize -= ( rx1 - nx ); src_x += ( rx1 - nx ); nx = rx1; }
		if( ny < ry1 ) { nysize -= ( ry1 - ny ); src_y += ( ry1 - ny ); ny = ry1; }
		if( nx + nxsize <= rx1 ) continue;
		if( ny + nysize <= ry1 ) continue;
		if( nx + nxsize > rx2 ) nxsize -= nx + nxsize - rx2;
		if( ny + nysize > ry2 ) nysize -= ny + nysize - ry2;
		if( nx >= rx2 ) continue;
		if( ny >= ry2 ) continue;
		if( nxsize < 0 ) continue;
		if( nysize < 0 ) continue;
        	
		//Draw it:
		wm->device_draw_image( nx, ny, nxsize, nysize, src_x, src_y, img, wm );
		wm->screen_changed++;
	    }
	}
    }
}

#define cbottom 1
#define ctop 2
#define cleft 4
#define cright 8
inline int line_clip_make_code( int x, int y, int clip_x1, int clip_y1, int clip_x2, int clip_y2 )
{
    int code = 0;
    if( y >= clip_y2 ) code = cbottom;
    else if( y < clip_y1 ) code = ctop;
    if( x >= clip_x2 ) code += cright;
    else if( x < clip_x1 ) code += cleft;
    return code;
}

bool line_clip( int* x1, int* y1, int* x2, int* y2, int clip_x1, int clip_y1, int clip_x2, int clip_y2 )
{
    //Cohen-Sutherland line clipping algorithm:
    int code0;
    int code1;
    int out_code;
    int x, y;
    code0 = line_clip_make_code( *x1, *y1, clip_x1, clip_y1, clip_x2, clip_y2 );
    code1 = line_clip_make_code( *x2, *y2, clip_x1, clip_y1, clip_x2, clip_y2 );
    while( code0 || code1 )
    {
	if( code0 & code1 ) 
	{
	    //Trivial reject
	    return 0;
	}
	else
	{
	    //Failed both tests, so calculate the line segment to clip
	    if( code0 )
		out_code = code0; //Clip the first point
	    else
		out_code = code1; //Clip the last point
	    
	    if( out_code & cbottom )
	    {
		//Clip the line to the bottom of the viewport
		y = clip_y2 - 1;
		x = *x1 + ( *x2 - *x1 ) * ( y - *y1 ) / ( *y2 - *y1 );
	    }
	    else 
	    {
		if( out_code & ctop )
		{
		    y = clip_y1;
		    x = *x1 + ( *x2 - *x1 ) * ( y - *y1 ) / ( *y2 - *y1 );
		}
		else
		{
		    if( out_code & cright )
		    {
			x = clip_x2 - 1;
			y = *y1 + ( *y2 - *y1 ) * ( x - *x1 ) / ( *x2 - *x1 );
		    }
		    else
		    {
			if( out_code & cleft )
			{
			    x = clip_x1;
			    y = *y1 + ( *y2 - *y1 ) * ( x - *x1 ) / ( *x2 - *x1 );
			}
		    }
		}
	    }
	    
	    if( out_code == code0 )
	    { 
		//Modify the first coordinate 
		*x1 = x; *y1 = y;
		code0 = line_clip_make_code( *x1, *y1, clip_x1, clip_y1, clip_x2, clip_y2 );
	    }
	    else
	    { 
		//Modify the second coordinate
		*x2 = x; *y2 = y;
		code1 = line_clip_make_code( *x2, *y2, clip_x1, clip_y1, clip_x2, clip_y2 );
	    }
	}
    }
    
    return 1;
}

void win_draw_line( WINDOWPTR win, int x1, int y1, int x2, int y2, COLOR color, window_manager* wm )
{
    if( win && win->visible && win->reg && win->reg->numRects )
    {
	x1 += win->screen_x;
	y1 += win->screen_y;
	x2 += win->screen_x;
	y2 += win->screen_y;
	for( int r = 0; r < win->reg->numRects; r++ )
	{
	    int rx1 = win->reg->rects[ r ].left;
	    int rx2 = win->reg->rects[ r ].right;
	    int ry1 = win->reg->rects[ r ].top;
	    int ry2 = win->reg->rects[ r ].bottom;

	    int lx1 = x1;
	    int ly1 = y1;
	    int lx2 = x2;
	    int ly2 = y2;
	    
	    if( line_clip( &lx1, &ly1, &lx2, &ly2, rx1, ry1, rx2, ry2 ) )
	    {	
		wm->device_draw_line( lx1, ly1, lx2, ly2, color, wm );
		wm->screen_changed++;
	    }
	}
    }
}

void screen_changed( window_manager* wm )
{
    wm->screen_changed++;
}

//
// Video capture
//

bool video_capture_supported( window_manager* wm )
{
#ifndef NOVCAP
    return device_video_capture_supported( wm );
#else
    return false;
#endif
}

void video_capture_set_in_name( const utf8_char* name, window_manager* wm )
{
#ifndef NOVCAP
    bmem_free( wm->vcap_in_name );
    wm->vcap_in_name = bmem_strdup( name );
#endif
}

const utf8_char* video_capture_get_file_ext( window_manager* wm )
{
#ifndef NOVCAP
    return device_video_capture_get_file_ext( wm );
#else
    return 0;
#endif
}

int video_capture_start( window_manager* wm )
{
    int rv = -1;
#ifndef NOVCAP
    if( wm->vcap == 0 )
    {
	rv = device_video_capture_start( wm );
	if( rv == 0 )
	{
	    blog( "video_capture_start( %d, %d, %d, %d ) ok\n", wm->screen_xsize, wm->screen_ysize, wm->vcap_in_fps, wm->vcap_in_bitrate_kb );
	    wm->vcap = 1;
	}
	else
	{
	    blog( "video_capture_start( %d, %d, %d, %d ) error %d\n", wm->screen_xsize, wm->screen_ysize, wm->vcap_in_fps, wm->vcap_in_bitrate_kb, rv );
	}
    }
#endif
    return rv;
}

int video_capture_frame_begin( window_manager* wm )
{
#ifndef NOVCAP
    if( wm->vcap )
	return device_video_capture_frame_begin( wm );
#endif
    return -1;
}

int video_capture_frame_end( window_manager* wm )
{
#ifndef NOVCAP
    if( wm->vcap )
	return device_video_capture_frame_end( wm );
#endif
    return -1;
}

int video_capture_stop( window_manager* wm )
{
    int rv = -1;
#ifndef NOVCAP
    if( wm->vcap )
    {
	rv = device_video_capture_stop( wm );
	if( rv == 0 )
	{
	    blog( "video_capture_stop() ok\n" );
    	    wm->vcap = 0;
    	}
    	else
	    blog( "video_capture_stop() error %d\n", rv );
    }
#endif
    return rv;
}

int video_capture_encode( window_manager* wm )
{
    int rv = -1;
#ifndef NOVCAP
    blog( "Video capture encoding to %s ...\n", wm->vcap_in_name );
    ticks_t t = time_ticks();
    rv = device_video_capture_encode( wm );
    blog( "Video capture encoding finished (%d). Time: %f sec\n", rv, (float)( time_ticks() - t ) / (float)time_ticks_per_second() );
#endif
    return rv;
}

//
// Strings
//

const utf8_char* wm_get_string( wm_string str_id )
{
    const utf8_char* str = 0;
    const utf8_char* lang = blocale_get_lang();
    while( 1 )
    {
	if( bmem_strstr( lang, "ru_" ) )
	{
	    switch( str_id )
	    {
		case STR_WM_OKCANCEL: str = "OK;Отмена"; break;
		case STR_WM_CANCEL: str = "Отмена"; break;
		case STR_WM_YESNO: str = "Да;Нет"; break;
		case STR_WM_YES_CAP: str = "ДА"; break;
    		case STR_WM_NO_CAP: str = "НЕТ"; break;
		case STR_WM_ENABLED_CAP: str = "ВКЛ"; break;
		case STR_WM_DISABLED_CAP: str = "ВЫКЛ"; break;
		case STR_WM_CLOSE: str = "Закрыть"; break;
		case STR_WM_RESET: str = "Сброс"; break;
    		case STR_WM_RESET_ALL: str = "Сбросить все"; break;
		case STR_WM_LOAD: str = "Загрузить"; break;
    		case STR_WM_SAVE: str = "Сохранить"; break;
    	        case STR_WM_INFO: str = "Инфо"; break;
		case STR_WM_AUTO: str = "Авто"; break;
		case STR_WM_FIND: str = "Найти"; break;
		case STR_WM_EDIT: str = "Редакт."; break;
		case STR_WM_NEW: str = "Новый"; break;
		case STR_WM_DELETE: str = "Удалить"; break;
		case STR_WM_RENAME: str = "Переименовать"; break;
		case STR_WM_RENAME_FILE: str = "Переименовать файл"; break;
		case STR_WM_CUT: str = "Вырезать"; break;
		case STR_WM_CUT2: str = "Вырез."; break;
		case STR_WM_COPY: str = "Скопировать"; break;
		case STR_WM_COPY2: str = "Скопир."; break;
		case STR_WM_PASTE: str = "Вставить"; break;
		case STR_WM_PASTE2: str = "Встав."; break;
		case STR_WM_CREATE_DIR: str = "Создать директорию"; break;
    		case STR_WM_DELETE_DIR: str = "Удалить текущ. директорию"; break;
    	        case STR_WM_RECURS: str = "рекурсивно"; break;
		case STR_WM_ERROR: str = "Ошибка"; break;
		case STR_WM_NOT_FOUND: str = "не найден"; break;

		case STR_WM_MS: str = "мс"; break;
		case STR_WM_SEC: str = "с"; break;
		case STR_WM_HZ: str = "Гц"; break;
		case STR_WM_INCH: str = "Дюйм"; break;
		case STR_WM_DECIBEL: str = "дБ"; break;
		case STR_WM_BIT: str = "бит"; break;
		case STR_WM_BYTES: str = "байт"; break;
		
		case STR_WM_DEMOVERSION: str = "!К сожалению, эта функция\nне доступна в демо-версии"; break;
		
		case STR_WM_PREFERENCES: str = "Настройки"; break;
		case STR_WM_PREFS_CHANGED: str = "!Настройки были изменены.\nНужно перезапустить программу для\nактивации новых настроек.\nПерезапустить сейчас?"; break;
		case STR_WM_INTERFACE: str = "Интерфейс"; break;
    		case STR_WM_AUDIO: str = "Звук"; break;
    		case STR_WM_VIDEO: str = "Видео"; break;
    		case STR_WM_CAMERA: str = "Камера"; break;
    		case STR_WM_BACK_CAM: str = "Задняя"; break;
    		case STR_WM_FRONT_CAM: str = "Передняя"; break;
		case STR_WM_MAXFPS: str = "Макс. FPS"; break;
		case STR_WM_ANGLE: str = "Поворот"; break;
		case STR_WM_DOUBLE_CLICK_TIME: str = "Двойной клик"; break;
		case STR_WM_CTL_TYPE: str = "Тип управления"; break;
		case STR_WM_CTL_FINGERS: str = "пальцами"; break;
		case STR_WM_CTL_PEN: str = "пером или мышкой"; break;
    		case STR_WM_SHOW_KBD: str = "Текст.клавиатура"; break;
    		case STR_WM_WINDOW_PARS: str = "Параметры окна"; break;
		case STR_WM_WINDOW_WIDTH: str = "Ширина по умолчанию:"; break;
    	        case STR_WM_WINDOW_HEIGHT: str = "Высота по умолчанию:"; break;
    		case STR_WM_WINDOW_FULLSCREEN: str = "Полноэкранный режим"; break;
		case STR_WM_SET_COLOR_THEME: str = "Установка цветовой схемы"; break;
		case STR_WM_SET_UI_SCALE: str = "Масштабирование интерфейса"; break;
		case STR_WM_SHORTCUTS_SHORT: str = "Сочетания клавиш"; break;
		case STR_WM_SHORTCUTS: str = "Сочетания клавиш (shortcuts)"; break;
		case STR_WM_UI_SCALE: str = "Масштаб"; break;
		case STR_WM_COLOR_THEME: str = "Цветовая схема"; break;
		case STR_WM_COLOR_THEME_MSG_RESTART: str = "!Чтобы увидеть новые цвета, нужно перезапустить программу.\nПерезапустить сейчас?"; break;
		case STR_WM_LANG: str = "Язык"; break;
		case STR_WM_DRIVER: str = "Драйвер"; break;
		case STR_WM_OUTPUT: str = "Выход"; break;
		case STR_WM_INPUT: str = "Вход"; break;
		case STR_WM_BUFFER: str = "Буфер"; break;
		case STR_WM_FREQ: str = "Частота дискр."; break;
		case STR_WM_DEVICE: str = "Устройство"; break;
		case STR_WM_INPUT_DEVICE: str = "Входное устройство"; break;
		case STR_WM_BUFFER_SIZE: str = "Размер аудиобуфера"; break;
		case STR_WM_ASIO_OPTIONS: str = "Опции ASIO"; break;
		case STR_WM_FIRST_OUT_CH: str = "Номер первого вых.канала:"; break;
		case STR_WM_FIRST_IN_CH: str = "Номер первого вх.канала:"; break;
		case STR_WM_OPTIONS: str = "Опции"; break;
		case STR_WM_MORE_OPTIONS: str = "Доп. опции"; break;
		case STR_WM_CUR_DRIVER: str = "Текущий драйвер"; break;
		case STR_WM_CUR_FREQ: str = "Текущая частота"; break;
		case STR_WM_CUR_LATENCY: str = "Текущая задержка"; break;
		case STR_WM_ACTION: str = "Действие:"; break;
		case STR_WM_SHORTCUTS2: str = "Сочетания клавиш и MIDI:"; break;
		case STR_WM_SHORTCUT: str = "Сочетание клавиш"; break;
		case STR_WM_RESET_TO_DEF: str = "Сбросить"; break;
		case STR_WM_ENTER_SHORTCUT: str = "Нажмите что-нибудь"; break;
		case STR_WM_AUTO_SCALE: str = "Автомасштаб"; break;
		case STR_WM_PPI: str = "Кол-во пикселей на дюйм"; break;
		case STR_WM_BUTTON_SCALE: str = "Масштаб кнопок (норм.=256)"; break;
		case STR_WM_FONT_SCALE: str = "Масштаб шрифта (норм.=256)"; break;
		case STR_WM_BUTTON: str = "Кнопка"; break;

		case STR_WM_DISABLED_ENABLED_MENU: str = "Выкл\nВкл"; break;
		case STR_WM_AUTO_YES_NO_MENU: str = "Авто\nДа\nНет"; break;
		case STR_WM_CTL_TYPE_MENU: str = "Авто\nПальцами\nПером или мышкой"; break;

		case STR_WM_FILE_NAME: str = "Имя:"; break;
		case STR_WM_FILE_PATH: str = "Путь:"; break;
		case STR_WM_FILE_MSG_NONAME: str = "!Введите имя файла"; break;
	        case STR_WM_FILE_OVERWRITE: str = "Записать поверх"; break;
	        
	        default: break;
	    }
	    if( str ) break;
	}
	//Default:
	switch( str_id )
	{
	    case STR_WM_OK: str = "OK"; break;
	    case STR_WM_OKCANCEL: str = "OK;Cancel"; break;
	    case STR_WM_CANCEL: str = "Cancel"; break;
	    case STR_WM_YESNO: str = "Yes;No"; break;
	    case STR_WM_YES_CAP: str = "YES"; break;
	    case STR_WM_NO_CAP: str = "NO"; break;
	    case STR_WM_ENABLED_CAP: str = "ENABLED"; break;
	    case STR_WM_DISABLED_CAP: str = "DISABLED"; break;
	    case STR_WM_CLOSE: str = "Close"; break;
	    case STR_WM_RESET: str = "Reset"; break;
	    case STR_WM_RESET_ALL: str = "Reset all"; break;
	    case STR_WM_LOAD: str = "Load"; break;
    	    case STR_WM_SAVE: str = "Save"; break;
    	    case STR_WM_INFO: str = "Info"; break;
	    case STR_WM_AUTO: str = "Auto"; break;
	    case STR_WM_FIND: str = "Find"; break;
	    case STR_WM_EDIT: str = "Edit"; break;
	    case STR_WM_NEW: str = "New"; break;
	    case STR_WM_DELETE: str = "Delete"; break;
	    case STR_WM_RENAME: str = "Rename"; break;
	    case STR_WM_RENAME_FILE: str = "Rename file"; break;
	    case STR_WM_CUT: str = "Cut"; break;
	    case STR_WM_CUT2: str = "Cut"; break;
	    case STR_WM_COPY: str = "Copy"; break;
	    case STR_WM_COPY2: str = "Copy"; break;
	    case STR_WM_PASTE: str = "Paste"; break;
	    case STR_WM_PASTE2: str = "Paste"; break;
	    case STR_WM_CREATE_DIR: str = "Create directory"; break;
    	    case STR_WM_DELETE_DIR: str = "Delete current directory"; break;
    	    case STR_WM_RECURS: str = "recursively"; break;
	    case STR_WM_ERROR: str = "Error"; break;
	    case STR_WM_NOT_FOUND: str = "not found"; break;

	    case STR_WM_MS: str = "ms"; break;
	    case STR_WM_SEC: str = "s"; break;
	    case STR_WM_HZ: str = "Hz"; break;
	    case STR_WM_INCH: str = "Inch"; break;
	    case STR_WM_DECIBEL: str = "dB"; break;
	    case STR_WM_BIT: str = "bit"; break;
	    case STR_WM_BYTES: str = "bytes"; break;
	    
	    case STR_WM_DEMOVERSION: str = "!Sorry, but this feature\nis not available\nin demo version"; break;
	    
	    case STR_WM_PREFERENCES: str = "Preferences"; break;
	    case STR_WM_PREFS_CHANGED: str = "!Some preferences changed.\nYou must restart the program\nto apply these changes.\nRestart now?"; break;
	    case STR_WM_INTERFACE: str = "Interface"; break;
	    case STR_WM_AUDIO: str = "Audio"; break;
	    case STR_WM_VIDEO: str = "Video"; break;
    	    case STR_WM_CAMERA: str = "Camera"; break;
    	    case STR_WM_BACK_CAM: str = "Back"; break;
    	    case STR_WM_FRONT_CAM: str = "Front"; break;
	    case STR_WM_MAXFPS: str = "Max FPS"; break;
	    case STR_WM_ANGLE: str = "UI rotate"; break;
	    case STR_WM_DOUBLE_CLICK_TIME: str = "Double click"; break;
	    case STR_WM_CTL_TYPE: str = "Control type"; break;
	    case STR_WM_CTL_FINGERS: str = "Fingers"; break;
	    case STR_WM_CTL_PEN: str = "Pen or Mouse"; break;
    	    case STR_WM_SHOW_KBD: str = "Show text keyboard"; break;
    	    case STR_WM_WINDOW_PARS: str = "Window parameters"; break;
    	    case STR_WM_WINDOW_WIDTH: str = "Default window width:"; break;
    	    case STR_WM_WINDOW_HEIGHT: str = "Default window height:"; break;
    	    case STR_WM_WINDOW_FULLSCREEN: str = "Fullscreen"; break;
	    case STR_WM_SET_COLOR_THEME: str = "Select a color theme"; break;
	    case STR_WM_SET_UI_SCALE: str = "Set UI scale"; break;
	    case STR_WM_SHORTCUTS_SHORT:
	    case STR_WM_SHORTCUTS: str = "Shortcuts"; break;
	    case STR_WM_UI_SCALE: str = "Scale"; break;
	    case STR_WM_COLOR_THEME: str = "Color theme"; break;
	    case STR_WM_COLOR_THEME_MSG_RESTART: str = "!You must restart the program to see the new color theme.\nRestart now?"; break;
	    case STR_WM_LANG: str = "Language"; break;
	    case STR_WM_DRIVER: str = "Driver"; break;
	    case STR_WM_OUTPUT: str = "Output"; break;
	    case STR_WM_INPUT: str = "Input"; break;
	    case STR_WM_BUFFER: str = "Buffer"; break;
	    case STR_WM_FREQ: str = "Frequency"; break;
	    case STR_WM_DEVICE: str = "Device"; break;
	    case STR_WM_INPUT_DEVICE: str = "Input device"; break;
	    case STR_WM_BUFFER_SIZE: str = "Audio buffer size"; break;
	    case STR_WM_ASIO_OPTIONS: str = "ASIO options"; break;
	    case STR_WM_FIRST_OUT_CH: str = "First output channel:"; break;
	    case STR_WM_FIRST_IN_CH: str = "First input channel:"; break;
	    case STR_WM_OPTIONS: str = "Options"; break;
	    case STR_WM_MORE_OPTIONS: str = "More options"; break;
	    case STR_WM_CUR_DRIVER: str = "Current driver"; break;
	    case STR_WM_CUR_FREQ: str = "Current frequency"; break;
	    case STR_WM_CUR_LATENCY: str = "Current latency"; break;
	    case STR_WM_ACTION: str = "Action:"; break;
	    case STR_WM_SHORTCUTS2: str = "Shortcuts (kbd+MIDI):"; break;
	    case STR_WM_SHORTCUT: str = "Shortcut"; break;
	    case STR_WM_RESET_TO_DEF: str = "Reset to defaults"; break;
	    case STR_WM_ENTER_SHORTCUT: str = "Enter the shortcut"; break;
	    case STR_WM_AUTO_SCALE: str = "Auto scale"; break;
	    case STR_WM_PPI: str = "PPI (Pixels Per Inch)"; break;
	    case STR_WM_BUTTON_SCALE: str = "Button scale (norm.=256)"; break;
	    case STR_WM_FONT_SCALE: str = "Font scale (norm.=256)"; break;
	    case STR_WM_BUTTON: str = "Button"; break;

	    case STR_WM_DISABLED_ENABLED_MENU: str = "Disabled\nEnabled"; break;
	    case STR_WM_AUTO_YES_NO_MENU: str = "Auto\nYES\nNO"; break;
	    case STR_WM_CTL_TYPE_MENU: str = "Auto\nFingers\nPen or Mouse"; break;
	    
	    case STR_WM_FILE_NAME: str = "Name:"; break;
	    case STR_WM_FILE_PATH: str = "Path:"; break;
	    case STR_WM_FILE_MSG_NONAME: str = "!Please enter the file name"; break;
	    case STR_WM_FILE_OVERWRITE: str = "Overwrite"; break;

	    default: break;
	}
	break;
    }
    return str;
}

//
// Dialogs & built-in windows
//

// Universal dialog

int dialog( const utf8_char* name, const utf8_char* buttons, window_manager* wm )
{
    WINDOWPTR prev_focus = wm->focus_win;
    int result = 0;
    
    wm->dialog_open++;

    dialog_item* ditems = wm->opt_dialog_items;
    
    wm->opt_dialog_buttons_text = buttons;
    wm->opt_dialog_text = name;
    wm->opt_dialog_result_ptr = &result;
    uint flags = DECOR_FLAG_CENTERED | DECOR_FLAG_CHECK_SIZE;
    const utf8_char* dialog_name = "";
    if( ditems == 0 || name == 0 || name[ 0 ] == 0 )
    {
	flags |= DECOR_FLAG_NOHEADER;
    }
    else
    {
	dialog_name = name;
    }
    int xsize = wm->normal_window_xsize;
    int ysize = 0;
    WINDOWPTR win = new_window_with_decorator( 
	dialog_name,
	0, 0, 
	xsize, 64,
	wm->dialog_color, 
	wm->root_win, 
	dialog_handler,
	flags,
	wm );
    if( flags & DECOR_FLAG_NOHEADER )
    {
	wbd_lock( win );
	int text_xsize;
	int text_ysize;
	draw_string_wordwrap( name, 0, 0, xsize - wm->interelement_space * 2, &text_xsize, &text_ysize, 1, wm );
	if( text_ysize >= wm->screen_ysize - wm->scrollbar_size * 2 )
	{
	    draw_string_wordwrap( name, 0, 0, wm->screen_xsize - wm->interelement_space * 2 - wm->decor_border_size * 2, &text_xsize, &text_ysize, 1, wm );
	}
	if( text_xsize > xsize ) 
	{
	    xsize = text_xsize;
	}
	ysize = text_ysize;
	if( ysize > wm->screen_ysize - wm->scrollbar_size )
	{
	    ysize = wm->screen_ysize - wm->scrollbar_size;
	}
	wbd_unlock( wm );
    }
    if( ditems )
    {
	int i = 0;
	while( 1 )
	{
	    int type = ditems[ i ].type;
	    if( type == DIALOG_ITEM_NONE )
	    {
		ysize -= wm->interelement_space;
		break;
	    }
	    switch( type )
	    {
		case DIALOG_ITEM_NUMBER:
		case DIALOG_ITEM_NUMBER_HEX:
		    ysize += DIALOG_ITEM_NUMBER_SIZE + wm->interelement_space;
		    break;
		case DIALOG_ITEM_SLIDER:
		    ysize += DIALOG_ITEM_SLIDER_SIZE + wm->interelement_space;
		    break;
		case DIALOG_ITEM_TEXT:
		    ysize += DIALOG_ITEM_TEXT_SIZE + wm->interelement_space;
		    break;
		case DIALOG_ITEM_LABEL:
		    ysize += DIALOG_ITEM_LABEL_SIZE + wm->interelement_space;
		    break;
		case DIALOG_ITEM_POPUP:
		    ysize += DIALOG_ITEM_POPUP_SIZE + wm->interelement_space;
		    break;
	    }	    
	    i++;
	}
    }
    change_decorator( win, 0, 0, xsize, ysize + wm->button_ysize + wm->interelement_space * 3, flags, wm );
    show_window( win, wm );
    bring_to_front( win, wm );
    recalc_regions( wm );
    draw_window( win, wm );

    while( 1 )
    {
	sundog_event evt;
	EVENT_LOOP_BEGIN( &evt, wm );
	if( EVENT_LOOP_END( wm ) ) break;
	if( win->visible == 0 ) break;
    }

    set_focus_win( prev_focus, wm );
    
    wm->dialog_open--;

    return result;
}

// File dialog

utf8_char* dialog_open_file( const utf8_char* name, const utf8_char* mask, const utf8_char* id, const utf8_char* def_filename, window_manager* wm )
{
#ifdef DEMO_MODE
    if( strstr( name, "Save" ) ||
        strstr( name, "save" ) ||
        strstr( name, "Export" ) ||
        strstr( name, "export" ) ||
        strstr( name, "храни" ) ||
        strstr( name, "хране" ) ||
        strstr( name, "кспорт" ) )
    {
	dialog( wm_get_string( STR_WM_DEMOVERSION ), wm_get_string( STR_WM_OK ), wm );
	return 0;
    }
#endif

    if( wm->fdialog_open ) return 0;
    wm->fdialog_open = true;
  
    WINDOWPTR prev_focus = wm->focus_win;

    const utf8_char* path = bfs_get_conf_path();
    int path_len = bmem_strlen( (const utf8_char*)path );
    int id_len = bmem_strlen( id );
    utf8_char* id2 = (utf8_char*)bmem_new( path_len + id_len + 1 );
    id2[ 0 ] = 0;
    bmem_strcat_resize( id2, (const utf8_char*)path );
    bmem_strcat_resize( id2, id );

    int xsize = wm->large_window_xsize;
    int ysize = wm->button_ysize * 2 + wm->list_item_ysize * 32 + wm->text_ysize * 2;
    uint flags = DECOR_FLAG_CENTERED | DECOR_FLAG_CHECK_SIZE;
    
    if( wm->control_type == TOUCHCONTROL )
    {
        flags |= DECOR_FLAG_STATIC | DECOR_FLAG_NOBORDER | DECOR_FLAG_FULLSCREEN;
    }
    
    if( wm->fdialog_filename == 0 )
	wm->fdialog_filename = (utf8_char*)bmem_new( MAX_DIR_LEN );
    wm->fdialog_filename[ 0 ] = 0;

    wm->opt_files_props = (const utf8_char*)id2;
    wm->opt_files_mask = mask;
    wm->opt_files_def_filename = def_filename;
    WINDOWPTR win = new_window_with_decorator( 
	name, 
	0, 0, 
	xsize, ysize, 
	wm->dialog_color,
	wm->root_win, 
	files_handler,
	flags,
	wm );
    //win - decorator
    show_window( win, wm );
    bring_to_front( win, wm );
    recalc_regions( wm );
    draw_window( win, wm );

    while( 1 )
    {
	sundog_event evt;
	EVENT_LOOP_BEGIN( &evt, wm );
	if( EVENT_LOOP_END( wm ) ) break;
	if( win->visible == 0 ) break;
    }

    set_focus_win( prev_focus, wm );
    
    bmem_free( id2 );
    
    wm->fdialog_open = false;

    if( wm->fdialog_filename[ 0 ] == 0 ) return 0;
    return wm->fdialog_filename;
}

int dialog_overwrite( utf8_char* filename, window_manager* wm ) //0 - YES; 1 - NO;
{
    utf8_char* ts = (utf8_char*)bmem_new( bmem_strlen( filename ) + 1024 );
    sprintf( ts, "!%s?\n%s", wm_get_string( STR_WM_FILE_OVERWRITE ), bfs_get_filename_without_dir( filename ) );
    int rv = dialog( ts, wm_get_string( STR_WM_YESNO ), wm );
    bmem_free( ts );
    return rv;
}

// Pupup menu

int popup_menu( const utf8_char* name, const utf8_char* items, int x, int y, COLOR color, window_manager* wm )
{
    int exit_flag = 0;
    int result = -1;
    wm->opt_popup_text = items;
    wm->opt_popup_exit_ptr = &exit_flag;
    wm->opt_popup_result_ptr = &result;
    WINDOWPTR win = new_window( name, x, y, 1, 1, color, wm->root_win, popup_handler, wm );
    show_window( win, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    while( 1 )
    {
	sundog_event evt;
	EVENT_LOOP_BEGIN( &evt, wm );
	if( EVENT_LOOP_END( wm ) ) break;
	if( exit_flag || win->visible == 0 ) break;
    }
    return result; //win->action_result;
}

// Preferences

void prefs_clear( window_manager* wm )
{
    wm->prefs_section_names[ 0 ] = 0;
    wm->prefs_sections = 0;
    wm->prefs_flags = 0;
}

void prefs_add_sections( const utf8_char** names, void** handlers, int num, window_manager* wm )
{
    for( int s = 0; s < num; s++ )
    {
	wm->prefs_section_names[ wm->prefs_sections ] = names[ s ];
	wm->prefs_section_handlers[ wm->prefs_sections ] = handlers[ s ];
	wm->prefs_sections++;
    }
    wm->prefs_section_names[ wm->prefs_sections ] = 0;
}

void prefs_add_default_sections( window_manager* wm )
{
    const utf8_char* def_names[ 3 ];
    void* def_handlers[ 3 ];
    int s = 0;
    def_handlers[ s ] = (void*)prefs_ui_handler;
    def_names[ s ] = wm_get_string( STR_WM_INTERFACE ); s++;
#ifndef NOVIDEO
    def_handlers[ s ] = (void*)prefs_video_handler;
    def_names[ s ] = wm_get_string( STR_WM_VIDEO ); s++;
#endif
    def_handlers[ s ] = (void*)prefs_audio_handler;
    def_names[ s ] = wm_get_string( STR_WM_AUDIO ); s++;
    prefs_add_sections( def_names, def_handlers, s, wm );
}

void prefs_open( void* host, window_manager* wm )
{
    if( wm->prefs_win == 0 )
    {
	wm->prefs_restart_request = false;
        
        uint flags = DECOR_FLAG_CENTERED | DECOR_FLAG_CHECK_SIZE;
        wm->prefs_win = new_window_with_decorator(
            wm_get_string( STR_WM_PREFERENCES ),
            0, 0,
            wm->large_window_xsize, wm->large_window_ysize,
            wm->dialog_color,
            wm->root_win,
            host,
            prefs_handler,
            flags,
            wm );
	show_window( wm->prefs_win, wm );
        bring_to_front( wm->prefs_win, wm );
	recalc_regions( wm );
        draw_window( wm->root_win, wm );
    }
}

// Color theme

void colortheme_open( window_manager* wm )
{
    if( wm->colortheme_win == 0 )
    {
        uint flags = DECOR_FLAG_CENTERED | DECOR_FLAG_CHECK_SIZE;
        wm->colortheme_win = new_window_with_decorator(
            wm_get_string( STR_WM_SET_COLOR_THEME ),
            0, 0,
            wm->large_window_xsize, wm->large_window_ysize,
            wm->dialog_color,
            wm->root_win,
            colortheme_handler,
            flags,
            wm );
        show_window( wm->colortheme_win, wm );
        bring_to_front( wm->colortheme_win, wm );
        recalc_regions( wm );
        draw_window( wm->root_win, wm );
    }
}

// UI Scale

void ui_scale_open( window_manager* wm )
{
    if( wm->ui_scale_win == 0 )
    {
        uint flags = DECOR_FLAG_CENTERED | DECOR_FLAG_CHECK_SIZE;
        wm->ui_scale_win = new_window_with_decorator(
            wm_get_string( STR_WM_SET_UI_SCALE ),
            0, 0,
            wm->normal_window_xsize, wm->normal_window_ysize,
            wm->dialog_color,
            wm->root_win,
            ui_scale_handler,
            flags,
            wm );
        show_window( wm->ui_scale_win, wm );
        bring_to_front( wm->ui_scale_win, wm );
        recalc_regions( wm );
        draw_window( wm->root_win, wm );
    }
}

// Keymap

void keymap_open( window_manager* wm )
{
    if( wm->keymap_win == 0 )
    {
        uint flags = DECOR_FLAG_CENTERED | DECOR_FLAG_CHECK_SIZE;
        wm->keymap_win = new_window_with_decorator(
            wm_get_string( STR_WM_SHORTCUTS ),
            0, 0,
            wm->large_window_xsize, wm->large_window_ysize,
            wm->dialog_color,
            wm->root_win,
            keymap_handler,
            flags,
            wm );
        show_window( wm->keymap_win, wm );
        bring_to_front( wm->keymap_win, wm );
        recalc_regions( wm );
        draw_window( wm->root_win, wm );
    }
}
