#include "core/core.h"
#include "pixilang.h"

enum 
{
    pix_status_none = 0,
    pix_status_loading,
    pix_status_working,
    pix_status_disabled
};

pix_data* g_pd = 0; //Global Pixilang structure
utf8_char* g_pix_prog_name = 0;
bool g_pix_save_code_request = 0;
#ifdef IPHONE
    bool g_pix_save_code_and_exit = 0;
#else
    bool g_pix_save_code_and_exit = 1;
#endif
bool g_pix_no_program_selection_dialog = 0;
int g_pix_open_count = 0;
WINDOWPTR g_pix_win = 0;

#ifdef IPHONE
    #include "various/iphone/webserver.h"
    extern char* g_osx_resources_path;
    utf8_char* g_pix_prog_name2 = 0;
#endif

#ifdef ANDROID
    #ifdef FREE_VERSION
	#include "android_resources.h"
    #else
	#include "../hidden/android_resources.h"
    #endif
    #include "various/android/sundog_bridge.h"
#endif

struct pixilang_window_data
{
    WINDOWPTR this_window;
    pix_vm* vm; //Pixilang virtual machine
    int status;
    sundog_image* screen_image;
    int timer;
};

enum
{
    STR_PIX_UI_PREFS,
    STR_PIX_UI_COMPILE,
    STR_PIX_UI_SELECT,
    STR_PIX_UI_MENU,
    STR_PIX_UI_MENU_NAME,
    STR_PIX_UI_LOADING,
};

const utf8_char* pix_ui_get_string( int str_id )
{
    const utf8_char* str = 0;
    const utf8_char* lang = blocale_get_lang();
    while( 1 )
    {
        if( bmem_strstr( lang, "ru_" ) )
        {
            switch( str_id )
            {
                case STR_PIX_UI_PREFS: str = "Настр."; break;
    		case STR_PIX_UI_COMPILE: str = "Компил."; break;
    		case STR_PIX_UI_SELECT: str = "Укажите pixi-программу для запуска"; break;
    		case STR_PIX_UI_MENU: str = "Выход"; break;
    		case STR_PIX_UI_MENU_NAME: str = "Меню Pixilang"; break;
    		case STR_PIX_UI_LOADING: str = "Загрузка..."; break;
            }
            if( str ) break;
        }
        //Default:
        switch( str_id )
        {
    	    case STR_PIX_UI_PREFS: str = "Prefs"; break;
    	    case STR_PIX_UI_COMPILE: str = "Compile"; break;
    	    case STR_PIX_UI_SELECT: str = "Select a program"; break;
    	    case STR_PIX_UI_MENU: str = "Exit"; break;
    	    case STR_PIX_UI_MENU_NAME: str = "Pixilang Menu"; break;
    	    case STR_PIX_UI_LOADING: str = "Loading..."; break;
        }
        break;
    }
    return str;
}

void pix_vm_draw_screen( WINDOWPTR win, bool draw_changes )
{
    pixilang_window_data* data = (pixilang_window_data*)win->data;
    window_manager* wm = win->wm;
    pix_vm* vm = data->vm;
    if( !vm ) return;

    win_draw_lock( win, wm );

    vm->screen_redraw_counter++;

    while( 1 )
    {
	if( vm->webserver_request || vm->file_dialog_request )
	{
	    win_draw_frect( win, 0, 0, win->xsize, win->ysize, win->color, wm );
	    break;
	}
    
        vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i = win->xsize / vm->pixel_size;
	vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i = win->ysize / vm->pixel_size;
	vm->vars[ PIX_GVAR_PPI ].i = wm->screen_ppi / vm->pixel_size;
    
        int x, y;
        int sxsize, sysize;
	int screen_change_x;
        int screen_change_y;
        int screen_change_xsize;
        int screen_change_ysize;
        pix_vm_container* c = 0;
        bool ready_for_answer = false;
	bool new_data_available = false;
        bool screen_image_can_be_redrawn = false;
#ifdef OPENGL    
	screen_image_can_be_redrawn = true;
#endif  
    
#ifdef OPENGL    
        if( vm->gl_callback != -1 )
	{
	    //OpenGL graphics:
	
	    bool gl_draw = 0;
    	    if( vm->gl_callback != -1 ) //Double check
	    {
		ready_for_answer = 1;
		gl_draw = 1;

		gl_program_reset( wm );
		bmem_copy( vm->gl_wm_transform, wm->gl_projection_matrix, sizeof( vm->gl_wm_transform ) );
    		matrix_4x4_translate( (int)( ( vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i * vm->pixel_size ) / 2 ), (int)( ( vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i * vm->pixel_size ) / 2 ), 0, vm->gl_wm_transform );
		if( vm->pixel_size != 1 )
    		    matrix_4x4_scale( vm->pixel_size, vm->pixel_size, 1, vm->gl_wm_transform );
    		pix_vm_gl_program_reset( vm );
	
    		pix_vm_function fun;
    		PIX_VAL pp[ 1 ];
    		char pp_types[ 1 ];
		fun.p = pp;
    		fun.p_types = pp_types;
    		fun.addr = vm->gl_callback;
    		fun.p[ 0 ] = vm->gl_userdata;
    		fun.p_types[ 0 ] = vm->gl_userdata_type;
    		fun.p_num = 1;
    		pix_vm_run( PIX_VM_THREADS - 2, 0, &fun, PIX_VM_CALL_FUNCTION, vm );

    		pix_vm_gl_program_reset( vm );
	    }
	
	    if( gl_draw )
	    {
		screen_changed( wm );
		goto skip_frame_drawing;
    	    }
	}
#endif

        //Check for changes:
        if( vm->screen_redraw_request != vm->screen_redraw_answer )
        {
    	    ready_for_answer = 1;
	    if( (unsigned)vm->screen < (unsigned)vm->c_num && vm->c[ vm->screen ] )
    	    {
	        c = vm->c[ vm->screen ];
		screen_change_x = vm->screen_change_x;
		screen_change_y = vm->screen_change_y;
		screen_change_xsize = vm->screen_change_xsize;
		screen_change_ysize = vm->screen_change_ysize;
	    }
	}
	if( c && c->data )
	{
	    new_data_available = true;
	    if( data->screen_image ) remove_image( data->screen_image );		
	    data->screen_image = new_image( c->xsize, c->ysize, c->data, 0, 0, c->xsize, c->ysize, IMAGE_NATIVE_RGB | IMAGE_STATIC_SOURCE, wm );
	    //You can redraw this image later only if:
	    // 1) OpenGL available (GPU texture created);
	    //   or
	    // 2) c && c->data, so the new image is created at the moment - image data pointer is valid.
	    screen_image_can_be_redrawn = true;
	}

	//Empty data control:
	if( draw_changes )
	{
	    if( !new_data_available )
	    {
		//Nothing to draw:
		goto skip_frame_drawing;		
	    }
	}
	else
	{
	    if( data->screen_image == 0 )
	    {
		//Nothing to draw:
		win_draw_frect( win, 0, 0, win->xsize, win->ysize, win->color, wm );
		goto skip_frame_drawing;		
	    }
	}

	sxsize = data->screen_image->xsize * vm->pixel_size;
	sysize = data->screen_image->ysize * vm->pixel_size;
        x = ( win->xsize - sxsize ) / 2;
        y = ( win->ysize - sysize ) / 2;
	
	if( draw_changes )
	{
	    //Draw changed screen region only:
	    
	    if( vm->pixel_size == 1 )
	    {
	        win_draw_image_ext( 
	    	    win,
    		    x + screen_change_x, y + screen_change_y, //dest XY
		    screen_change_xsize, screen_change_ysize, //dest size
		    screen_change_x, screen_change_y, //src XY
		    data->screen_image, 
		    wm );
	    }
	    else
	    {
	        wbd_lock( win );
	        sundog_image_scaled img;
	        img.img = data->screen_image;
	        img.src_x = screen_change_x << IMG_PREC;
	        img.src_y = screen_change_y << IMG_PREC;
	        img.src_xsize = screen_change_xsize << IMG_PREC;
	        img.src_ysize = screen_change_ysize << IMG_PREC;
		img.dest_xsize = screen_change_xsize * vm->pixel_size;
		img.dest_ysize = screen_change_ysize * vm->pixel_size;
	        draw_image_scaled( x + screen_change_x * vm->pixel_size, y + screen_change_y * vm->pixel_size, &img, wm );
		wbd_draw( wm );
		wbd_unlock( wm );
	    }
	}
	else
	{
	    //Force last available screen redraw + border redraw:

    	    if( vm->pixel_size == 1 )
	    {
	        win_draw_frect( win, 0, 0, x, win->ysize, win->color, wm );
	        win_draw_frect( win, x + sxsize, 0, win->xsize - ( x + sxsize ), win->ysize, win->color, wm );
	        win_draw_frect( win, x, 0, sxsize, y, win->color, wm );
    	        win_draw_frect( win, x, y + sysize, sxsize, win->ysize - ( y + sysize ), win->color, wm );
    	        if( screen_image_can_be_redrawn )
    	        {
    		    win_draw_image( win, x, y, data->screen_image, wm );
            	}
	    }
	    else
	    {
	        wbd_lock( win );
	        draw_frect( 0, 0, x, win->ysize, win->color, wm );
	        draw_frect( x + sxsize, 0, win->xsize - ( x + sxsize ), win->ysize, win->color, wm );
	        draw_frect( x, 0, sxsize, y, win->color, wm );
    	        draw_frect( x, y + sysize, sxsize, win->ysize - ( y + sysize ), win->color, wm );
	        sundog_image_scaled img;
	        img.img = data->screen_image;
	    	img.src_x = 0;
	    	img.src_y = 0;
	    	img.src_xsize = data->screen_image->xsize << IMG_PREC;
	    	img.src_ysize = data->screen_image->ysize << IMG_PREC;
	    	img.dest_xsize = sxsize;
	    	img.dest_ysize = sysize;
	        draw_image_scaled( x, y, &img, wm );
		wbd_draw( wm );
		wbd_unlock( wm );
	    }
	}
    
skip_frame_drawing:

	if( ready_for_answer )
	{
	    if( vm->screen_redraw_request != vm->screen_redraw_answer )
		vm->screen_redraw_answer = vm->screen_redraw_request;
	}
	    
	break;
    }

    win_draw_unlock( win, wm );
}

int pixilang_menu_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    switch( win->action_result )
    {
	case 0:
	    //EXIT:
	    send_event( 0, EVT_BUTTONDOWN, 0, 0, 0, KEY_ESCAPE, 0, 1024, 0, wm );
	    break;
    }
    return 0;
}

int pixilang_prefs_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_clear( wm );
    wm->prefs_flags |= PREFS_FLAG_NO_COLOR_THEME | PREFS_FLAG_NO_CONTROL_TYPE | PREFS_FLAG_NO_KEYMAP;
    prefs_add_default_sections( wm );
    prefs_open( 0, wm );
    return 0;
}

int pixilang_compile_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    g_pix_save_code_request ^= 1;
    g_pix_save_code_and_exit = 0;
    if( g_pix_save_code_request )
	win->color = SELECTED_BUTTON_COLOR;
    else
	win->color = wm->button_color;
    draw_window( win, wm );
    return 0;
}

void pixilang_timer( void* user_data, sundog_timer* t, window_manager* wm )
{
    pixilang_window_data* data = (pixilang_window_data*)user_data;
    WINDOWPTR win = data->this_window;
    
    switch( data->status )
    {
	case pix_status_none:
	    {
		data->status = pix_status_loading;

		if( g_pix_no_program_selection_dialog && g_pix_open_count > 0 )
		{
		    wm->exit_request = 1;
		    return;
		}

		if( g_pix_prog_name == 0 )
		{
		    wm->opt_files_user_button[ 0 ] = pix_ui_get_string( STR_PIX_UI_PREFS );
		    wm->opt_files_user_button_handler[ 0 ] = pixilang_prefs_handler;
		    wm->opt_files_user_button[ 1 ] = pix_ui_get_string( STR_PIX_UI_COMPILE );
		    wm->opt_files_user_button_handler[ 1 ] = pixilang_compile_handler;
		    g_pix_prog_name = dialog_open_file( pix_ui_get_string( STR_PIX_UI_SELECT ), 0, ".pixilang_f", 0, wm );
		}
			
		draw_window( win, wm );
                wm->device_redraw_framebuffer( wm );
			
		if( g_pix_prog_name == 0 )
		{
		    wm->exit_request = 1;
		}
		else 
		{
		    g_pix_open_count++;
		    if( data->vm )
		    {
			pix_vm_deinit( data->vm );
			bmem_free( data->vm );
		    }
		    data->vm = (pix_vm*)bmem_new( sizeof( pix_vm ) );
		    bmem_zero( data->vm );
		    pix_vm_init( data->vm, wm );
		    int cres = pix_compile( g_pix_prog_name, data->vm, g_pd );
		    if( cres == 0 )
		    {
			if( g_pix_save_code_request )
			{
			    utf8_char* code_name = (utf8_char*)bmem_new( bmem_strlen( g_pix_prog_name ) + 10 );
			    code_name[ 0 ] = 0; bmem_strcat_resize( code_name, g_pix_prog_name );
			    int dot_ptr = bmem_strlen( code_name );
			    for( int i = bmem_strlen( code_name ); i >= 0; i-- )
			    {
				if( code_name[ i ] == '.' )
				{
				    dot_ptr = i;
				    break;
				}
			    }
			    code_name[ dot_ptr ] = 0;
			    bmem_strcat_resize( code_name, ".pixicode" );
#ifdef IPHONE
			    pix_vm_save_code( "1:/boot.pixicode", data->vm );
#else
			    pix_vm_save_code( code_name, data->vm );
			    cres = -1;
#endif
			    if( g_pix_save_code_and_exit ) wm->exit_request = 1;
			    g_pix_save_code_request = 0;
			    bmem_free( code_name );
			}
		    }
		    data->status = pix_status_none;
		    draw_window( win, wm );
            	    wm->device_redraw_framebuffer( wm );
		    if( cres == 0 )
		    {
			data->vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i = win->xsize;
			data->vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i = win->ysize;
			data->vm->vars[ PIX_GVAR_PPI ].i = wm->screen_ppi;
			data->vm->vars[ PIX_GVAR_SCALE ].f = wm->screen_scale;
			data->vm->vars[ PIX_GVAR_FONT_SCALE ].f = wm->screen_font_scale;
			pix_vm_resize_container( data->vm->screen, win->xsize, win->ysize, -1, 0, data->vm );
			pix_vm_gfx_set_screen( data->vm->screen, data->vm );
			PIX_VAL v;
			v.i = 0;
			set_focus_win( win, wm );
			pix_vm_clean_container( data->vm->screen, 0, v, 0, -1, data->vm );
			pix_vm_run( 0, 1, 0, PIX_VM_CALL_MAIN, data->vm );
			data->status = pix_status_working;
		    }
		    else
		    {
			if( g_pix_no_program_selection_dialog ) wm->exit_request = 1;
		    }
		}
			
		g_pix_prog_name = 0;
	    }
	    break;
	case pix_status_loading:
	    break;
	case pix_status_working:
	    if( data->vm && data->vm->th[ 0 ] && data->vm->th[ 0 ]->active == 0 )
	    {
		data->status = pix_status_none;
	    }
	    else 
	    {
		if( data->vm->screen_redraw_request != data->vm->screen_redraw_answer )
		{
		    if( wm->screen_buffer_preserved )
			pix_vm_draw_screen( win, 1 );
		    else
			screen_changed( wm ); //... and all windows will be redrawn by SunDog engine before device_redraw_framebuffer
		}
	    }
	    if( data->vm->file_dialog_request == 1 )
	    {
		data->vm->file_dialog_request = 2;
		data->vm->file_dialog_result = dialog_open_file( data->vm->file_dialog_name, data->vm->file_dialog_mask, data->vm->file_dialog_id, data->vm->file_dialog_def_name, wm );
		data->vm->file_dialog_request = 0;
	    }
	    if( data->vm->prefs_dialog_request == 1 )
	    {
		data->vm->prefs_dialog_request = 2;
		//Show global preferences:
		pixilang_prefs_handler( 0, 0, wm );
		data->vm->prefs_dialog_request = 0;
	    }
	    if( data->vm->vsync_request > 0 )
	    {
		win_vsync( data->vm->vsync_request - 1, wm );
		data->vm->vsync_request = 0;
	    }
#ifdef WEBSERVER
            if( data->vm->webserver_request == 1 )
            {
                data->vm->webserver_request = 2;
                webserver_open( wm );
                webserver_wait_for_close( wm );
                data->vm->webserver_request = 0;
            }
#endif
	    if( data->vm->vcap_request )
	    {
		if( data->vm->vcap_request == 1 )
		{
		    wm->vcap_in_fps = data->vm->vcap_in_fps;
		    wm->vcap_in_bitrate_kb = data->vm->vcap_in_bitrate_kb;
		    data->vm->vcap_out_err = video_capture_start( wm );
		    data->vm->vcap_request = 0;
		}
		if( data->vm->vcap_request == 2 )
		{
		    data->vm->vcap_out_err = video_capture_stop( wm );
		    data->vm->vcap_request = 0;
		}
	    }
	    break;
    }
}

int pixilang_window_handler( sundog_event* evt, window_manager* wm )
{
    int retval = 0;
    WINDOWPTR win = evt->win;
    pixilang_window_data* data = (pixilang_window_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( pixilang_window_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		data->this_window = win;
		data->vm = 0;
		data->status = pix_status_none;
		data->screen_image = 0;
#ifdef SHOW_PIXILANG_MENU
		int but_size = wm->scrollbar_size;
		wm->opt_button_flat = 1;
		WINDOWPTR menu = new_window( pix_ui_get_string( STR_PIX_UI_MENU_NAME ), 0, 0, but_size, but_size, wm->color0, win, button_handler, wm );
		button_set_menu( menu, pix_ui_get_string( STR_PIX_UI_MENU ), wm );
		button_set_text( menu, (utf8_char*)g_text_down, wm );
		set_window_controller( menu, 0, wm, CPERC, 100, CSUB, but_size, CEND );
		set_window_controller( menu, 2, wm, CPERC, 100, CEND );
		set_handler( menu, pixilang_menu_handler, 0, wm );
#endif
		if( g_pd == 0 )
		{
		    g_pd = (pix_data*)bmem_new( sizeof( pix_data ) );
		    bmem_zero( g_pd );
		    pix_init( g_pd );
		}
		data->timer = add_timer( pixilang_timer, (void*)data, 0, wm );
	    }
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    {
		if( data->vm )
		{
		    if( data->status == pix_status_working )
			pix_vm_send_event( PIX_EVT_QUIT, 0, 0, 0, 0, 0, 0, 0, data->vm );
		    pix_vm_deinit( data->vm );
		    bmem_free( data->vm );
		    data->vm = 0;
		}
		remove_image( data->screen_image );
		data->screen_image = 0;	
		remove_timer( data->timer, wm );
		data->timer = -1;
		video_capture_stop( wm );
	    }
	    retval = 1;
	    break;
	case EVT_DRAW:
	    switch( data->status )
	    {
		case pix_status_none:
		    win_draw_frect( win, 0, 0, win->xsize, win->ysize, win->color, wm );
		    break;
		case pix_status_loading:
		    wbd_lock( win );
		    wm->cur_font_color = wm->color3;
		    draw_frect( 0, 0, win->xsize, win->ysize, win->color, wm );
		    draw_string( pix_ui_get_string( STR_PIX_UI_LOADING ), 0, 0, wm );
		    wbd_draw( wm );
		    wbd_unlock( wm );
		    break;
		case pix_status_working:
		    pix_vm_draw_screen( win, 0 );
		    break;
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	    if( data->vm )
	    {
		int x = evt->x - win->xsize / 2;
		int y = evt->y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_MOUSEBUTTONDOWN, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, evt->unicode, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONUP:
	    if( data->vm )
	    {
		int x = evt->x - win->xsize / 2;
		int y = evt->y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_MOUSEBUTTONUP, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, evt->unicode, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEMOVE:
	    if( data->vm )
	    {
		int x = evt->x - win->xsize / 2;
		int y = evt->y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_MOUSEMOVE, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, evt->unicode, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_TOUCHBEGIN:
	    if( data->vm )
	    {
		int x = evt->x - win->xsize / 2;
		int y = evt->y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_TOUCHBEGIN, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, evt->unicode, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_TOUCHEND:
	    if( data->vm )
	    {
		int x = evt->x - win->xsize / 2;
		int y = evt->y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_TOUCHEND, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, evt->unicode, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_TOUCHMOVE:
	    if( data->vm )
	    {
		int x = evt->x - win->xsize / 2;
		int y = evt->y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_TOUCHMOVE, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, evt->unicode, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_BUTTONDOWN:
	    if( data->vm )
	    {
		pix_vm_send_event( PIX_EVT_BUTTONDOWN, evt->flags, 0, 0, evt->key, evt->scancode, evt->pressure, evt->unicode, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_BUTTONUP:
	    if( data->vm )
	    {
		pix_vm_send_event( PIX_EVT_BUTTONUP, evt->flags, 0, 0, evt->key, evt->scancode, evt->pressure, evt->unicode, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_SCREENRESIZE:
	    win->x = 0;
	    win->y = 0;
	    win->xsize = wm->screen_xsize;
	    win->ysize = wm->screen_ysize;
            if( data->vm )
	    {
		pix_vm_send_event( PIX_EVT_SCREENRESIZE, 0, 0, 0, 0, 0, 0, 0, data->vm );
	    }
	    break;
    }
    return retval;
}

int user_init( window_manager* wm )
{
    g_pd = 0;
    g_pix_prog_name = 0;
    g_pix_no_program_selection_dialog = 0;
    g_pix_open_count = 0;
    
    wm->root_win = new_window( 
	"Desktop", 
	0, 0, 
	wm->screen_xsize, wm->screen_ysize, 
	wm->color0, 
	0, 
	desktop_handler,
	wm );
    show_window( wm->root_win, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );

#ifdef ANDROID
    copy_resources( wm );
    check_resources();
#endif
        
    bfs_file f;
    
    if( wm->sd->argc > 1 )
    {
	if( bmem_strcmp( wm->sd->argv[ 1 ], "-c" ) == 0 )
	{
	    if( wm->sd->argc > 2 )
	    {
		g_pix_prog_name = wm->sd->argv[ 2 ];
		g_pix_save_code_request = 1;
	    }
	    else
		blog( "Error: no source file.\nUsage: pixilang -c source_file_name\n" );
	}
	else
	    g_pix_prog_name = wm->sd->argv[ 1 ];
	f = bfs_open( g_pix_prog_name, "rb" );
	if( f )
	    bfs_close( f );
	else 
	{
	    g_pix_prog_name = 0;
	}
    }
    
#ifdef IPHONE
    if( g_pix_prog_name == 0 )
    {
	g_pix_prog_name2 = (char*)bmem_new( bmem_strlen( g_osx_resources_path ) + 256 );

	sprintf( g_pix_prog_name2, "%s/boot.pixicode", g_osx_resources_path );
	g_pix_prog_name = g_pix_prog_name2;
	f = bfs_open( g_pix_prog_name, "rb" );
	if( f )
	{
	    blog( "boot.pixicode found\n" );
	    bfs_close( f );
	}
	else
	    g_pix_prog_name = 0;

	if( g_pix_prog_name == 0 )
	{
	    sprintf( g_pix_prog_name2, "%s/boot.pixi", g_osx_resources_path );
	    g_pix_prog_name = g_pix_prog_name2;
	    f = bfs_open( g_pix_prog_name, "rb" );
	    if( f )
		bfs_close( f );
	    else 
		g_pix_prog_name = 0;
	}
    }
#else    
    if( g_pix_prog_name == 0 )
    {
	g_pix_prog_name = (utf8_char*)"1:/boot.pixicode";
	f = bfs_open( g_pix_prog_name, "rb" );
	if( f )
	    bfs_close( f );
	else 
	{
	    g_pix_prog_name = 0;
	}
    }
    
    if( g_pix_prog_name == 0 )
    {
	g_pix_prog_name = (utf8_char*)"1:/boot.pixi";
	f = bfs_open( g_pix_prog_name, "rb" );
	if( f )
	    bfs_close( f );
	else 
	{
	    g_pix_prog_name = 0;
	}
    }

    if( g_pix_prog_name == 0 )
    {
	g_pix_prog_name = (utf8_char*)"1:/boot.txt";
	f = bfs_open( g_pix_prog_name, "rb" );
	if( f )
	    bfs_close( f );
	else 
	{
	    g_pix_prog_name = 0;
	}
    }
#endif

    if( g_pix_prog_name )
    {
        g_pix_no_program_selection_dialog = 1;
#ifdef PIX_SAVE_BYTECODE
        g_pix_save_code_request = 1;
#endif
    }
    
    g_pix_win = new_window(
	"Pixilang window", 
	0, 0, 
	wm->screen_xsize, wm->screen_ysize, 
	wm->color0, 
	wm->root_win, 
	pixilang_window_handler,
	wm );
    set_window_controller( g_pix_win, 0, wm, (WCMD)0, CEND );
    set_window_controller( g_pix_win, 1, wm, (WCMD)0, CEND );
    set_window_controller( g_pix_win, 2, wm, CPERC, (WCMD)100, CEND );
    set_window_controller( g_pix_win, 3, wm, CPERC, (WCMD)100, CEND );
    g_pix_win->font = 1;
    show_window( g_pix_win, wm );
    recalc_regions( wm );
    draw_window( g_pix_win, wm );

    return 0;
}

int user_event_handler( sundog_event* evt, window_manager* wm )
{
    int handled = 0;

    switch( evt->type )
    {
	case EVT_BUTTONDOWN:
	    if( evt->key != KEY_ESCAPE ) break;
	case EVT_QUIT:
	    {
		pixilang_window_data* pdata = 0;
		if( g_pix_win )
		    pdata = (pixilang_window_data*)g_pix_win->data;
		if( g_pix_win && pdata->status != pix_status_working )
		{
		    wm->exit_request = 1;
		}
		else 
		{
		    pix_vm_send_event( PIX_EVT_QUIT, evt->flags, 0, 0, 0, 1, 0, 0, pdata->vm );
		    if( pdata->vm->quit_action == 1 )
		    {
			pix_vm_deinit( pdata->vm );
			bmem_free( pdata->vm );
			pdata->vm = 0;
			pdata->status = pix_status_none;
			remove_image( pdata->screen_image );
            		pdata->screen_image = 0;
			draw_window( wm->root_win, wm );
            	    }
		}
		handled = 1;
	    }
	    break;
    }
    
    return handled;
}

void user_close( window_manager* wm )
{
    remove_window( wm->root_win, wm );

    if( g_pd )
    {
	pix_deinit( g_pd );
	bmem_free( g_pd );
	g_pd = 0;
    }
#ifdef IPHONE
    bmem_free( g_pix_prog_name2 );
    g_pix_prog_name2 = 0;
#endif
}
