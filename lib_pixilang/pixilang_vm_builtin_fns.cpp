/*
    pixilang_vm_builtin_fns.cpp
    This file is part of the Pixilang programming language.
    
    [ MIT license ]

    Copyright (c) 2006 - 2016, Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to 
    deal in the Software without restriction, including without limitation the 
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is 
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in 
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

//Modularity: 100%

#include "pixilang_vm_builtin_fns.h"

#include "dsp.h"

#include <errno.h>

const utf8_char* g_pix_fn_names[ FN_NUM ] = 
{
    "new",
    "remove",
    "remove_with_alpha",
    "resize",
    "rotate",
    "clean",
    "clone",
    "copy",
    "get_size",
    "get_xsize",
    "get_ysize",
    "get_esize",
    "get_type",
    "get_flags",
    "set_flags",
    "reset_flags",
    "get_prop",
    "set_prop",
    "remove_props",
    "convert_type",
    "show_mem_debug_messages",
    "zlib_pack",
    "zlib_unpack",
    
    "num_to_str",
    "str_to_num",

    "strcat",
    "strcmp"
    "strlen",
    "strstr",
    "sprintf",
    "printf",
    "fprintf",

    "logf",
    "get_log",
    "get_system_log",
    
    "load",
    "fload",
    "save",
    "fsave",
    "get_real_path",
    "new_flist",
    "remove_flist",
    "get_flist_name",
    "get_flist_type",
    "flist_next",
    "get_file_size",
    "remove_file",
    "rename_file",
    "copy_file",
    "create_directory",
    "set_disk0",
    "get_disk0",

    "fopen",
    "fopen_mem",
    "fclose",
    "fputc",
    "fputs",
    "fwrite",
    "fgetc",
    "fgets",
    "fread",
    "feof",
    "fflush",
    "fseek",
    "ftell",
    "setxattr",
    
    "frame",
    "vsync",
    "set_pixel_size",
    "get_pixel_size",
    "set_screen",
    "get_screen",
    "set_zbuf",
    "get_zbuf",
    "get_color",
    "get_red",
    "get_green",
    "get_blue",
    "get_blend",
    "transp",
    "get_transp",
    "clear",
    "clone",
    "dot",
    "dot3d",
    "get_dot",
    "get_dot3d",
    "line",
    "line3d",
    "box",
    "fbox",
    "pixi",
    "triangles3d",
    "sort_triangles3d",
    "set_key_color",
    "get_key_color",
    "set_alpha",
    "get_alpha",
    "print",
    "get_text_xsize",
    "get_text_ysize",
    "set_font",
    "get_font",
    "effector",
    "color_gradient",
    "split_rgb",
    "split_ycbcr",

    "set_gl_callback",
    "remove_gl_data",
    "gl_draw_arrays",
    "gl_blend_func",
    "gl_bind_framebuffer",
    "gl_new_prog",
    "gl_use_prog",
    "gl_uniform",
    "gl_uniform_matrix",
    
    "unpack_frame",
    "pack_frame",
    "create_anim",
    "remove_anim",
    "clone_frame",
    "remove_frame",
    "play",
    "stop",
    
    "video_open",
    "video_close",
    "video_start",
    "video_stop",
    "video_set_props",
    "video_get_props",
    "video_capture_frame",
    
    "t_reset",
    "t_rotate",
    "t_translate",
    "t_scale",
    "t_push_matrix",
    "t_pop_matrix",
    "t_get_matrix",
    "t_set_matrix",
    "t_mul_matrix",
    "t_point",
    
    "set_audio_callback",
    "get_note_freq",
    
    "midi_open_client",
    "midi_close_client",
    "midi_get_device",
    "midi_open_port",
    "midi_reopen_port",
    "midi_close_port",
    "midi_get_event",
    "midi_get_event_time",
    "midi_next_event",
    "midi_send_event",
    
    "start_timer",
    "get_timer",
    "get_year",
    "get_month",
    "get_day",
    "get_hours",
    "get_minutes",
    "get_seconds",
    "get_ticks",
    "get_tps",
    "sleep",
    
    "get_event",
    "set_quit_action",
    
    "thread_create",
    "thread_destroy",
    "mutex_create",
    "mutex_destroy",
    "mutex_lock",
    "mutex_trylock",
    "mutex_unlock",
    
    "acos",
    "acosh",
    "asin",
    "asinh",
    "atan",
    "atanh",
    "ceil",
    "cos",
    "cosh",
    "exp",
    "exp2",
    "expm1",
    "abs",
    "floor",
    "mod",
    "log",
    "log2",
    "log10",
    "pow",
    "sin",
    "sinh",
    "sqrt",
    "tan",
    "tanh",
    "rand",
    "rand_seed",
    
    "op_cn",
    "op_cc",
    "op_ccn",
    "generator",
    "wavetable_generator",
    "sampler",
    "envelope2p",
    "gradient",
    "fft",
    "new_filter",
    "remove_filter",
    "init_filter",
    "reset_filter",
    "apply_filter",
    "replace_values",
    "copy_and_resize",
    
    "file_dialog",
    "prefs_dialog",

    "open_url",
    
    "dlopen",
    "dlclose",
    "dlsym",
    "dlcall",

    "system",
    "argc",
    "argv",
    "exit",
    
    "system_copy",
    "system_paste",
    "send_file_to_email",
    "send_file_to_gallery",
    "open_webserver",
    "set_audio_play_status",
    "get_audio_event",
    "wm_video_capture_supported",
    "wm_video_capture_start",
    "wm_video_capture_stop",
    "wm_video_capture_get_ext",
    "wm_video_capture_encode",
};

pix_builtin_fn g_pix_fns[ FN_NUM ] = 
{
    fn_new_pixi,
    fn_remove_pixi,
    fn_remove_pixi_with_alpha,
    fn_resize_pixi,
    fn_rotate_pixi,
    fn_clean_pixi,
    fn_clone_pixi,
    fn_copy_pixi,
    fn_get_pixi_info, //size
    fn_get_pixi_info, //xsize
    fn_get_pixi_info, //ysize
    fn_get_pixi_info, //esize
    fn_get_pixi_info, //type
    fn_get_pixi_flags,
    fn_set_pixi_flags,
    fn_reset_pixi_flags,
    fn_get_pixi_prop_OR_set_pixi_prop, //get_prop
    fn_get_pixi_prop_OR_set_pixi_prop, //set_prop
    fn_remove_pixi_props,
    fn_convert_pixi_type,
    fn_show_mem_debug_messages,
    fn_zlib_pack,
    fn_zlib_unpack,
    
    fn_num_to_string,
    fn_string_to_num,

    fn_strcat,
    fn_strcmp_OR_strstr, //strcmp
    fn_strlen,
    fn_strcmp_OR_strstr, //strstr
    fn_sprintf,
    fn_sprintf,
    fn_sprintf,

    fn_sprintf,
    fn_get_log,
    fn_get_system_log,
    
    fn_load,
    fn_fload,
    fn_save,
    fn_fsave,
    fn_get_real_path,
    fn_new_flist,
    fn_remove_flist,
    fn_get_flist_name,
    fn_get_flist_type,
    fn_flist_next,
    fn_get_file_size,
    fn_remove_file,
    fn_rename_file,
    fn_copy_file,
    fn_create_directory,
    fn_set_disk0,
    fn_get_disk0,

    fn_fopen,
    fn_fopen_mem,
    fn_fclose,
    fn_fputc,
    fn_fputs,
    fn_fgets_OR_fwrite_OR_fread, //fwrite
    fn_fgetc,
    fn_fgets_OR_fwrite_OR_fread, //fgets
    fn_fgets_OR_fwrite_OR_fread, //fread
    fn_feof_OF_fflush_OR_ftell, //feof
    fn_feof_OF_fflush_OR_ftell, //fflush
    fn_fseek,
    fn_feof_OF_fflush_OR_ftell, //ftell
    fn_setxattr,
    
    fn_frame,
    fn_vsync,
    fn_set_pixel_size,
    fn_get_pixel_size,
    fn_set_screen,
    fn_get_screen,
    fn_set_zbuf,
    fn_get_zbuf,
    fn_clear_zbuf,
    fn_get_color,
    fn_get_red,
    fn_get_green,
    fn_get_blue,
    fn_get_blend,
    fn_transp,
    fn_get_transp,
    fn_clear,
    fn_dot,
    fn_dot, //3d
    fn_get_dot,
    fn_get_dot, //3d
    fn_line,
    fn_line, //3d
    fn_box,
    fn_box, //fbox
    fn_pixi,
    fn_triangles3d,
    fn_sort_triangles3d,
    fn_set_key_color,
    fn_get_key_color,
    fn_set_alpha,
    fn_get_alpha,
    fn_print,
    fn_get_text_xsize,
    fn_get_text_ysize,
    fn_set_font,
    fn_get_font,
    fn_effector,
    fn_color_gradient,
    fn_split_rgb,
    fn_split_rgb,
    
    fn_set_gl_callback,
    fn_remove_gl_data,
    fn_gl_draw_arrays,
    fn_gl_blend_func,
    fn_gl_bind_framebuffer,
    fn_gl_new_prog,
    fn_gl_use_prog,
    fn_gl_uniform,
    fn_gl_uniform_matrix,
    
    fn_pixi_unpack_frame,
    fn_pixi_pack_frame,
    fn_pixi_create_anim,
    fn_pixi_remove_anim,
    fn_pixi_clone_frame,
    fn_pixi_remove_frame,
    fn_pixi_play,
    fn_pixi_stop,

    fn_video_open,
    fn_video_close,
    fn_video_start,
    fn_video_stop,
    fn_video_props,
    fn_video_props,
    fn_video_capture_frame,
    
    fn_t_reset,
    fn_t_rotate,
    fn_t_translate,
    fn_t_scale,
    fn_t_push_matrix,
    fn_t_pop_matrix,
    fn_t_get_matrix,
    fn_t_set_matrix,
    fn_t_mul_matrix,
    fn_t_point,
    
    fn_set_audio_callback,
    fn_enable_audio_input,
    fn_get_note_freq,
    
    fn_midi_open_client,
    fn_midi_close_client,
    fn_midi_get_device,
    fn_midi_open_port,
    fn_midi_reopen_port,
    fn_midi_close_port,
    fn_midi_get_event,
    fn_midi_get_event_time,
    fn_midi_next_event,
    fn_midi_send_event,
    
    fn_start_timer,
    fn_get_timer,
    fn_get_year,
    fn_get_month,
    fn_get_day,
    fn_get_hours,
    fn_get_minutes,
    fn_get_seconds,
    fn_get_ticks,
    fn_get_tps,
    fn_sleep,
    
    fn_get_event,
    fn_set_quit_action,
    
    fn_thread_create,
    fn_thread_destroy,
    fn_mutex_create,
    fn_mutex_destroy,
    fn_mutex_lock,
    fn_mutex_trylock,
    fn_mutex_unlock,
    
    fn_acos,
    fn_acosh,
    fn_asin,
    fn_asinh,
    fn_atan,
    fn_atanh,
    fn_ceil,
    fn_cos,
    fn_cosh,
    fn_exp,
    fn_exp2,
    fn_expm1,
    fn_abs,
    fn_floor,
    fn_mod,
    fn_log,
    fn_log2,
    fn_log10,
    fn_pow,
    fn_sin,
    fn_sinh,
    fn_sqrt,
    fn_tan,
    fn_tanh,
    fn_rand,
    fn_rand_seed,
    
    fn_op_cn,
    fn_op_cc,
    fn_op_ccn,
    fn_generator,
    fn_wavetable_generator,
    fn_sampler,
    fn_envelope2p,
    fn_gradient,
    fn_fft,
    fn_new_filter,
    fn_remove_filter,
    fn_init_filter,
    fn_reset_filter,
    fn_apply_filter,
    fn_replace_values,
    fn_copy_and_resize,
    
    fn_file_dialog,
    fn_prefs_dialog,

    fn_system_copy_OR_open_url, //open_url
    
    fn_dlopen,
    fn_dlclose,
    fn_dlsym,
    fn_dlcall,

    fn_system,
    fn_argc,
    fn_argv,
    fn_exit,
    
    fn_system_copy_OR_open_url, //copy
    fn_system_paste,
    fn_send_file_to,
    fn_send_file_to,
    fn_webserver,
    fn_set_audio_play_status,
    fn_get_audio_event,
    fn_wm_video_capture_supported,
    fn_wm_video_capture_start,
    fn_wm_video_capture_stop,
    fn_wm_video_capture_get_ext,
    fn_wm_video_capture_encode,
};

//
// Containers (memory management)
//

void fn_new_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_INT xsize = 1;
    PIX_INT ysize = 1;
    int type = 32;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( xsize, 0, PIX_INT );
    if( pars_num >= 2 ) GET_VAL_FROM_STACK( ysize, 1, PIX_INT );
    if( pars_num >= 3 ) GET_VAL_FROM_STACK( type, 2, int );
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = pix_vm_new_container( -1, xsize, ysize, type, 0, vm );
}

void fn_remove_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_remove_container( cnum, vm );
    }
}

void fn_remove_pixi_with_alpha( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	PIX_CID a = pix_vm_get_container_alpha( cnum, vm );
	pix_vm_remove_container( a, vm );
	pix_vm_remove_container( cnum, vm );
    }
}

void fn_resize_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 2 ) return;
    PIX_CID cnum;
    PIX_INT xsize = -1;
    PIX_INT ysize = -1;
    int type = -1;
    uint flags = 0;
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    GET_VAL_FROM_STACK( xsize, 1, PIX_INT );
    if( pars_num >= 3 ) GET_VAL_FROM_STACK( ysize, 2, PIX_INT );
    if( pars_num >= 4 ) GET_VAL_FROM_STACK( type, 3, int );
    if( pars_num >= 5 ) GET_VAL_FROM_STACK( flags, 4, int );
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = pix_vm_resize_container( cnum, xsize, ysize, type, flags, vm );
}

void fn_rotate_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 2 ) return;
    PIX_CID cnum;
    int angle = 0;
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    GET_VAL_FROM_STACK( angle, 1, int );
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = pix_vm_rotate_container( cnum, angle, vm );
}

void fn_clean_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    if( pars_num > 0 ) 
    {
	PIX_INT offset = 0;
	PIX_INT size = -1;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	if( pars_num > 1 )
	{
	    if( pars_num > 2 ) { GET_VAL_FROM_STACK( offset, 2, PIX_INT ); }
	    if( pars_num > 3 ) { GET_VAL_FROM_STACK( size, 3, PIX_INT ); }
	    pix_vm_clean_container( cnum, stack_types[ sp + 1 ], stack[ sp + 1 ], offset, size, vm );
	}
	else 
	{
	    PIX_VAL v;
	    v.i = 0;
	    pix_vm_clean_container( cnum, 0, v, 0, -1, vm );
	}
    }
}

void fn_clone_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	stack[ sp + ( pars_num - 1 ) ].i = pix_vm_clone_container( cnum, vm );
    }
    else
    {
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}

void fn_copy_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_INT rv = -1;
    
    if( pars_num >= 2 )
    {
	PIX_CID cnum1; //destination
	PIX_CID cnum2; //source
	uint flags = 0;
	GET_VAL_FROM_STACK( cnum1, 0, PIX_CID );
	GET_VAL_FROM_STACK( cnum2, 1, PIX_CID );
	if( pars_num >= 8 ) GET_VAL_FROM_STACK( flags, 7, size_t );
	if( (unsigned)cnum1 >= (unsigned)vm->c_num ) goto copy_end;
	pix_vm_container* cont1 = vm->c[ cnum1 ];
#ifdef OPENGL
	if( cnum2 == PIX_GL_SCREEN )
	{
	    if( g_pix_container_type_sizes[ cont1->type ] == COLORLEN )
	    {
		int format = -1;
		int type = -1;
		if( COLORLEN == 4 ) { format = GL_RGBA; type = GL_UNSIGNED_BYTE; }
		if( COLORLEN == 2 ) { format = GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; }
		if( format >= 0 && type >= 0 )
		{
		    glReadPixels( 
			0, 
			0, 
			cont1->xsize, 
			cont1->ysize,
			format, type, cont1->data );
		    if( ( flags & PIX_COPY_NO_AUTOROTATE ) == 0 )
		    {
			COLORPTR src1 = (COLORPTR)cont1->data;
			COLORPTR src2 = src1 + cont1->xsize * ( cont1->ysize - 1 );
			for( int y = 0; y < cont1->ysize / 2; y++ )
			{
			    for( int x = 0; x < cont1->xsize; x++ )
			    {
				COLOR temp = *src2;
				*src2 = *src1;
				*src1 = temp;
				src1++;
				src2++;
			    }
			    src2 -= cont1->xsize * 2;
			}
		    }
		    rv = 0;
		}
		goto copy_end;
	    }
	}
#endif
	if( (unsigned)cnum2 >= (unsigned)vm->c_num ) goto copy_end;
	pix_vm_container* cont2 = vm->c[ cnum2 ];
	if( cont1 == 0 || cont2 == 0 ) goto copy_end;
	
	size_t dest_offset;
	size_t src_offset;
	size_t count;
	size_t dest_step;
	size_t src_step;
	if( pars_num >= 3 ) { GET_VAL_FROM_STACK( dest_offset, 2, size_t ); } else { dest_offset = 0; }
	if( pars_num >= 4 ) { GET_VAL_FROM_STACK( src_offset, 3, size_t ); } else { src_offset = 0; }
	if( pars_num >= 5 ) { GET_VAL_FROM_STACK( count, 4, size_t ); } else { count = cont1->size; }
	if( pars_num >= 6 ) { GET_VAL_FROM_STACK( dest_step, 5, size_t ); } else { dest_step = 1; }
	if( pars_num >= 7 ) { GET_VAL_FROM_STACK( src_step, 6, size_t ); } else { src_step = 1; }
	if( dest_offset >= cont1->size ) goto copy_end;
	if( src_offset >= cont2->size ) goto copy_end;
	if( count == 0 ) goto copy_end;
	size_t end_bound = dest_offset + ( count - 1 ) * dest_step;
	if( end_bound >= cont1->size ) 
	    count = ( cont1->size - dest_offset ) / dest_step;
	end_bound = src_offset + ( count - 1 ) * src_step;
	if( end_bound >= cont2->size )
	    count = ( cont2->size - src_offset ) / src_step;
	
	rv = (PIX_INT)count;
	
	if( cont1->type == cont2->type && dest_step == 1 && src_step == 1 )
	{
	    bmem_copy( 
		     (char*)cont1->data + dest_offset * g_pix_container_type_sizes[ cont1->type ], 
		     (char*)cont2->data + src_offset * g_pix_container_type_sizes[ cont1->type ], 
		     count * g_pix_container_type_sizes[ cont1->type ] 
		    );
	    rv = 0;
	}
	else 
	{	    
	    if( ( flags & PIX_COPY_CLIPPING ) && cont1->type < PIX_CONTAINER_TYPE_INT32 )
	    {
		for( PIX_INT i = dest_offset, i2 = src_offset; i < dest_offset + count * dest_step; i += dest_step, i2 += src_step )
		{
		    PIX_INT val;
		    switch( cont2->type )
		    {
		        case PIX_CONTAINER_TYPE_INT8: val = ( (signed char*)cont2->data )[ i2 ]; break;
		        case PIX_CONTAINER_TYPE_INT16: val = ( (signed short*)cont2->data )[ i2 ]; break;
		        case PIX_CONTAINER_TYPE_INT32: val = ( (signed int*)cont2->data )[ i2 ]; break;
#ifdef PIX_INT64_ENABLED
		        case PIX_CONTAINER_TYPE_INT64: val = ( (signed long long*)cont2->data )[ i2 ]; break;
#endif
		        case PIX_CONTAINER_TYPE_FLOAT32: val = ( (float*)cont2->data )[ i2 ]; break;
#ifdef PIX_FLOAT64_ENABLED
		        case PIX_CONTAINER_TYPE_FLOAT64: val = ( (double*)cont2->data )[ i2 ]; break;
#endif
		        default: val = 0; break;
		    }
		    if( cont1->type == PIX_CONTAINER_TYPE_INT8 )
		    {
		        if( val < -128 ) 
		    	    val = -128;
			else if( val > 127 ) 
			    val = 127;
			( (signed char*)cont1->data )[ i ] = val;
		    }
		    else
		    {
		        if( val < -32768 )
			    val = -32768;
			else if( val > 32767 ) 
			    val = 32767;
			( (signed short*)cont1->data )[ i ] = val;
		    }
		}
		rv = 0;
	    }
	    else
	    {
		if( cont2->type < PIX_CONTAINER_TYPE_FLOAT32 )
		{
		    //INT source:
		    for( PIX_INT i = dest_offset, i2 = src_offset; i < dest_offset + count * dest_step; i += dest_step, i2 += src_step )
		    {
			PIX_INT val;
			switch( cont2->type )
			{
			    case PIX_CONTAINER_TYPE_INT8: val = ( (signed char*)cont2->data )[ i2 ]; break;
			    case PIX_CONTAINER_TYPE_INT16: val = ( (signed short*)cont2->data )[ i2 ]; break;
			    case PIX_CONTAINER_TYPE_INT32: val = ( (signed int*)cont2->data )[ i2 ]; break;
#ifdef PIX_INT64_ENABLED
			    case PIX_CONTAINER_TYPE_INT64: val = ( (signed long long*)cont2->data )[ i2 ]; break;
#endif
			    default: val = 0; break;
			}
			switch( cont1->type )
			{
			    case PIX_CONTAINER_TYPE_INT8: ( (signed char*)cont1->data )[ i ] = val; break;
			    case PIX_CONTAINER_TYPE_INT16: ( (signed short*)cont1->data )[ i ] = val; break;
			    case PIX_CONTAINER_TYPE_INT32: ( (signed int*)cont1->data )[ i ] = val; break;
#ifdef PIX_INT64_ENABLED
			    case PIX_CONTAINER_TYPE_INT64: ( (signed long long*)cont1->data )[ i ] = val; break;
#endif
			    case PIX_CONTAINER_TYPE_FLOAT32: ( (float*)cont1->data )[ i ] = val; break;
#ifdef PIX_FLOAT64_ENABLED
			    case PIX_CONTAINER_TYPE_FLOAT64: ( (double*)cont1->data )[ i ] = val; break;
#endif
			    default: break;		
			}
		    }
		}
		else
		{
		    //FLOAT source:
		    for( PIX_INT i = dest_offset, i2 = src_offset; i < dest_offset + count * dest_step; i += dest_step, i2 += src_step )
		    {
			PIX_FLOAT val;
			switch( cont2->type )
			{
			    case PIX_CONTAINER_TYPE_FLOAT32: val = ( (float*)cont2->data )[ i2 ]; break;
#ifdef PIX_FLOAT64_ENABLED
			    case PIX_CONTAINER_TYPE_FLOAT64: val = ( (double*)cont2->data )[ i2 ]; break;
#endif
			    default: val = 0; break;
			}
			switch( cont1->type )
			{
			    case PIX_CONTAINER_TYPE_INT8: ( (signed char*)cont1->data )[ i ] = val; break;
			    case PIX_CONTAINER_TYPE_INT16: ( (signed short*)cont1->data )[ i ] = val; break;
			    case PIX_CONTAINER_TYPE_INT32: ( (signed int*)cont1->data )[ i ] = val; break;
#ifdef PIX_INT64_ENABLED
			    case PIX_CONTAINER_TYPE_INT64: ( (signed long long*)cont1->data )[ i ] = val; break;
#endif
			    case PIX_CONTAINER_TYPE_FLOAT32: ( (float*)cont1->data )[ i ] = val; break;
#ifdef PIX_FLOAT64_ENABLED
			    case PIX_CONTAINER_TYPE_FLOAT64: ( (double*)cont1->data )[ i ] = val; break;
#endif
			    default: break;		
			}
		    }
		}
		rv = 0;
	    }
	}
    }
    
copy_end:

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_get_pixi_info( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    PIX_INT rv = 1;
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
#ifdef OPENGL
	if( cnum == PIX_GL_SCREEN )
    	{
    	    switch( fn_num )
    	    {
	        case FN_GET_PIXI_SIZE: rv = vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i * vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i; break;
	        case FN_GET_PIXI_XSIZE: rv = vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i; break;
	        case FN_GET_PIXI_YSIZE: rv = vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i; break;
		case FN_GET_PIXI_ESIZE: rv = COLORLEN; break;
		case FN_GET_PIXI_TYPE: 
		    if( COLORLEN == 2 ) rv = PIX_CONTAINER_TYPE_INT16; 
		    if( COLORLEN == 4 ) rv = PIX_CONTAINER_TYPE_INT32; 
		    break;
    	    }
    	    goto get_info_end;
    	}
#endif
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    switch( fn_num )
	    {
	        case FN_GET_PIXI_SIZE: rv = c->xsize * c->ysize; break;
	        case FN_GET_PIXI_XSIZE: rv = c->xsize; break;
	        case FN_GET_PIXI_YSIZE: rv = c->ysize; break;
	        case FN_GET_PIXI_ESIZE: rv = g_pix_container_type_sizes[ c->type ]; break;
	        case FN_GET_PIXI_TYPE: rv = c->type; break;
	    }
	}
    }
get_info_end:
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_get_pixi_flags( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    uint flags = 0;
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    flags = c->flags;
	}
    }
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = flags;
}

void fn_set_pixi_flags( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    uint flags;
    if( pars_num >= 2 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( flags, 1, int );
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    volatile uint new_flags = c->flags | flags;
	    c->flags = new_flags;
	}
    }
}

void fn_reset_pixi_flags( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    uint flags;
    if( pars_num >= 2 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( flags, 1, int );
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    volatile uint new_flags = c->flags & ~flags;
	    c->flags = new_flags;
	}
    }
}

void fn_get_pixi_prop_OR_set_pixi_prop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 2 ) 
    {
	PIX_CID cnum;
	PIX_CID prop_name;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( prop_name, 1, PIX_CID );
	bool need_to_free = 0;
	utf8_char* prop_name_str = pix_vm_make_cstring_from_container( prop_name, &need_to_free, vm );
	switch( fn_num )
	{
	    case FN_GET_PIXI_PROP:
		{	    
		    if( pars_num > 2 )
		    {
			stack[ sp + ( pars_num - 1 ) ] = stack[ sp + 2 ];
			stack_types[ sp + ( pars_num - 1 ) ] = stack_types[ sp + 2 ];
		    }
		    else
		    {
			stack_types[ sp + ( pars_num - 1 ) ] = 0;
			stack[ sp + ( pars_num - 1 ) ].i = 0;
		    }
    
		    pix_sym* sym = pix_vm_get_container_property( cnum, prop_name_str, -1, vm );
		    
		    if( sym )
		    {
			if( sym->type == SYMTYPE_NUM_F )
			    stack_types[ sp + ( pars_num - 1 ) ] = 1;
			stack[ sp + ( pars_num - 1 ) ] = sym->val;
		    }

		}
		break;
	    case FN_SET_PIXI_PROP:
		{
		    pix_vm_set_container_property( cnum, prop_name_str, -1, stack_types[ sp + 2 ], stack[ sp + 2 ], vm );
		}
		break;
	}
	if( need_to_free ) bmem_free( prop_name_str );
    }
}

void fn_remove_pixi_props( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER; 
   
    if( pars_num >= 1 ) 
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c && c->opt_data )
	{
	    pix_symtab_deinit( &c->opt_data->props );
	}
    }
}

void fn_convert_pixi_type( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = 1;
    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int type;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( type, 1, int );
	
	rv = pix_vm_convert_container_type( cnum, type, vm );
    }
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_show_mem_debug_messages( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER; 
   
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( vm->c_show_debug_messages, 0, bool );
    }
}

void fn_zlib_pack( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;
    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	int level = -1;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	if( pars_num >= 2 ) GET_VAL_FROM_STACK( level, 1, int );
	
	rv = pix_vm_zlib_pack_container( cnum, level, vm );
    }
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_zlib_unpack( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;
    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	
	rv = pix_vm_zlib_unpack_container( cnum, vm );
    }
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

//
// Working with strings
//
	
void fn_num_to_string( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
		
    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    utf8_char ts[ 128 ];
	    if( stack_types[ sp + 1 ] == 0 )
	    {
		//int:
		sprintf( ts, "%d", (int)stack[ sp + 1 ].i );
	    }
	    else 
	    {
		//float:
		sprintf( ts, "%f", (float)stack[ sp + 1 ].f );
	    }
	    size_t size = c->size * g_pix_container_type_sizes[ c->type ];
	    PIX_INT ts_len = (PIX_INT)bmem_strlen( ts );
	    if( ts_len > size )
	    {
		if( pix_vm_resize_container( cnum, ts_len, 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) return;
		size = c->size * g_pix_container_type_sizes[ c->type ];
	    }
	    bmem_copy( c->data, ts, ts_len );
	    if( ts_len < size ) ((utf8_char*)c->data)[ ts_len ] = 0;
	}
    }
}

void fn_string_to_num( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    bool ok = 0;
    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    if( c && c->data )
	    {
		size_t size = c->size * g_pix_container_type_sizes[ c->type ];
		utf8_char* str = (utf8_char*)c->data;
		utf8_char ts[ 128 + 1 ];
		ts[ 0 ] = 0;
		bool float_num = 0;
		int i = 0;
		while( 1 )
		{
		    utf8_char cc = str[ i ];
		    if( cc == '.' ) float_num = 1;
		    if( cc == 0 ) break;
		    ts[ i ] = cc;
		    i++;
		    if( i == 128 ) break;
		    if( i == size ) break;
		}
		ts[ i ] = 0;
		if( float_num )
		{
		    stack_types[ sp + ( pars_num - 1 ) ] = 1;
		    stack[ sp + ( pars_num - 1 ) ].f = atof( ts );
		    ok = 1;
		}
		else 
		{
		    PIX_INT val = 0;
		    bool minus = 0;
		    for( int i2 = 0; i2 < i; i2++ )
		    {
			utf8_char cc = ts[ i2 ];
			if( cc == '-' ) 
			{
			    minus = 1;
			}
			else
			{
			    val *= 10;
			    val += cc - '0';
			}
		    }
		    if( minus ) val = -val;
		    stack_types[ sp + ( pars_num - 1 ) ] = 0;
		    stack[ sp + ( pars_num - 1 ) ].i = val;
		    ok = 1;
		}
	    }
	}
    }
    if( ok == 0 )
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = 0;
    }
}

//
// Working with strings (posix)
//

//Appends a copy of the source string to the destination string:
void fn_strcat( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID s1;
    PIX_CID s2;
    pix_vm_container* s1_cont;
    pix_vm_container* s2_cont;
    PIX_INT off1 = 0;
    PIX_INT off2 = 0;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 2 ) { err = 1; break; }
	GET_VAL_FROM_STACK( s1, 0, PIX_CID );
	if( pars_num >= 3 )
	{
	    GET_VAL_FROM_STACK( off1, 1, PIX_INT );
	    GET_VAL_FROM_STACK( s2, 2, PIX_CID );
	    if( pars_num >= 4 )
	    {
		GET_VAL_FROM_STACK( off2, 3, PIX_INT );
	    }
	}
	else
	{
	    GET_VAL_FROM_STACK( s2, 1, PIX_CID );
	}
	if( (unsigned)s1 >= (unsigned)vm->c_num ) { err = 1; break; }
	s1_cont = vm->c[ s1 ];
	if( (unsigned)s2 >= (unsigned)vm->c_num ) { err = 1; break; }
	s2_cont = vm->c[ s2 ];
	if( s1_cont == 0 ) { err = 1; break; }
	if( s2_cont == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    while( err == 0 )
    {
	size_t s1_size = s1_cont->size * g_pix_container_type_sizes[ s1_cont->type ];
	size_t s2_size = s2_cont->size * g_pix_container_type_sizes[ s2_cont->type ];
	if( off2 >= s2_size ) break;
	s2_size -= off2;
	size_t s1_len;
	size_t s2_len;
	utf8_char* s1_ptr = (utf8_char*)s1_cont->data;
	utf8_char* s2_ptr = (utf8_char*)s2_cont->data;
	s2_ptr += off2;
	for( s2_len = 0; s2_len < s2_size; s2_len++ )
	    if( s2_ptr[ s2_len ] == 0 ) break;
	if( s2_len == 0 ) break;
	if( off1 >= s1_size )
	{
	    s1_size = off1 + s2_len;
	    if( pix_vm_resize_container( s1, (PIX_INT)( s1_size ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) break;
	    s1_size -= off1;
	    s1_ptr = (utf8_char*)s1_cont->data;
	    s1_ptr += off1;
	    bmem_copy( s1_ptr, s2_ptr, s2_len );
	}
	else
	{
	    s1_size -= off1;
	    s1_ptr += off1;
	    for( s1_len = 0; s1_len < s1_size; s1_len++ )
		if( s1_ptr[ s1_len ] == 0 ) break;
	    if( s1_len + s2_len > s1_size )
	    {
		if( pix_vm_resize_container( s1, (PIX_INT)( off1 + s1_len + s2_len ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) break;
		s1_ptr = (utf8_char*)s1_cont->data;
		s1_ptr += off1;
		s1_size = s1_len + s2_len;
	    }
	    else 
	    {
		if( s1_len + s2_len < s1_size )
		    s1_ptr[ s1_len + s2_len ] = 0;
	    }
	    bmem_copy( s1_ptr + s1_len, s2_ptr, s2_len );
	}
	break;
    }
}

//Compare two strings /
//Returns a offset of the first occurrence of str2 in str1, or -1 if str2 is not part of str1:
void fn_strcmp_OR_strstr( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID s1;
    PIX_CID s2;
    pix_vm_container* s1_cont;
    pix_vm_container* s2_cont;
    PIX_INT off1 = 0;
    PIX_INT off2 = 0;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 2 ) { err = 1; break; }
	GET_VAL_FROM_STACK( s1, 0, PIX_CID );
	if( pars_num >= 3 )
	{
	    GET_VAL_FROM_STACK( off1, 1, PIX_INT );
	    GET_VAL_FROM_STACK( s2, 2, PIX_CID );
	    if( pars_num >= 4 )
	    {
		GET_VAL_FROM_STACK( off2, 3, PIX_INT );
	    }
	}
	else
	{
	    GET_VAL_FROM_STACK( s2, 1, PIX_CID );
	}
	if( (unsigned)s1 >= (unsigned)vm->c_num ) { err = 1; break; }
	s1_cont = vm->c[ s1 ];
	if( (unsigned)s2 >= (unsigned)vm->c_num ) { err = 1; break; }
	s2_cont = vm->c[ s2 ];
	if( s1_cont == 0 ) { err = 1; break; }
	if( s2_cont == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    if( err == 0 )
    {
	size_t s1_size = s1_cont->size * g_pix_container_type_sizes[ s1_cont->type ];
	size_t s2_size = s2_cont->size * g_pix_container_type_sizes[ s2_cont->type ];
	if( off1 > s1_size ) { s1_size = 0; off1 = 0; }
	if( off2 > s2_size ) { s2_size = 0; off2 = 0; }
	s1_size -= off1;
	s2_size -= off2;
	size_t s1_len;
	size_t s2_len;
	utf8_char* s1_ptr = (char*)s1_cont->data;
	utf8_char* s2_ptr = (char*)s2_cont->data;
	s1_ptr += off1;
	s2_ptr += off2;
	for( s1_len = 0; s1_len < s1_size; s1_len++ )
	    if( s1_ptr[ s1_len ] == 0 ) break;
	for( s2_len = 0; s2_len < s2_size; s2_len++ )
	    if( s2_ptr[ s2_len ] == 0 ) break;
	utf8_char* s1_str = 0;
	utf8_char* s2_str = 0;
	if( s1_len == s1_size )
	{
	    s1_str = (utf8_char*)bmem_new( s1_len + 1 );
	    bmem_copy( s1_str, s1_ptr, s1_len );
	    s1_str[ s1_len ] = 0;
	}
	else 
	{
	    s1_str = s1_ptr;
	}
	if( s2_len == s2_size )
	{
	    s2_str = (utf8_char*)bmem_new( s2_len + 1 );
	    bmem_copy( s2_str, s2_ptr, s2_len );
	    s2_str[ s2_len ] = 0;
	}
	else 
	{
	    s2_str = s2_ptr;
	}
	if( fn_num == FN_STRCMP )
	{
	    stack[ sp + ( pars_num - 1 ) ].i = bmem_strcmp( s1_str, s2_str );
	}
	else 
	{
	    utf8_char* substr = strstr( s1_str, s2_str );
	    if( substr == 0 )
		stack[ sp + ( pars_num - 1 ) ].i = -1;
	    else
		stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)( substr - s1_str ) + off1;
	}
	if( s1_len == s1_size ) bmem_free( s1_str );
	if( s2_len == s2_size ) bmem_free( s2_str );
    }
    else 
    {
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}

//Returns the length of the string:
void fn_strlen( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID str;
    PIX_INT off = 0;

    //Get parameters:
    if( pars_num < 1 ) 
    { 
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = 0;
	return;
    }
    
    GET_VAL_FROM_STACK( str, 0, PIX_CID );
    if( pars_num >= 2 )
    {
	GET_VAL_FROM_STACK( off, 1, PIX_INT );
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)pix_vm_get_container_strlen( str, (size_t)off, vm );
}

//Writes into the array pointed by str a C string consisting on a sequence of data formatted as the format argument specifies:
void fn_sprintf( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    bool err = 0;
    int i2;
    
    bfs_file dest_stream = 0;
    PIX_CID str = -1;
    PIX_CID format = -1;
    pix_vm_container* str_cont = 0;
    pix_vm_container* format_cont = 0;
    int args_off;
    
    //Get parameters:
    if( fn_num == FN_SPRINTF )
    {
	//sprintf:
	while( 1 )
	{
	    if( pars_num < 2 ) { err = 1; break; }
	    GET_VAL_FROM_STACK( str, 0, PIX_CID );
	    GET_VAL_FROM_STACK( format, 1, PIX_CID );
	    if( (unsigned)str >= (unsigned)vm->c_num ) { err = 1; break; }
	    if( (unsigned)format >= (unsigned)vm->c_num ) { err = 1; break; }
	    str_cont = vm->c[ str ];
	    format_cont = vm->c[ format ];
	    if( str_cont == 0 ) { err = 1; break; }
	    if( format_cont == 0 ) { err = 1; break; }
	    args_off = 2;
	    break;
	}
    }
    else
    {
	//printf or fprintf
	while( 1 )
	{
	    if( fn_num == FN_PRINTF || fn_num == FN_LOGF )
	    {
		if( pars_num < 1 ) { err = 1; break; }
		GET_VAL_FROM_STACK( format, 0, PIX_CID );
		args_off = 1;
	    }
	    else
	    {
		//fprintf
		if( pars_num < 2 ) { err = 1; break; }
		GET_VAL_FROM_STACK( dest_stream, 0, bfs_file );
		GET_VAL_FROM_STACK( format, 1, PIX_CID );
		args_off = 2;
	    }
	    if( (unsigned)format >= (unsigned)vm->c_num ) { err = 1; break; }
	    format_cont = vm->c[ format ];
	    if( format_cont == 0 ) { err = 1; break; }
	    break;
	}
    }
    
    //Execute:
    if( err == 0 ) 
    {
	size_t format_size = format_cont->size * g_pix_container_type_sizes[ format_cont->type ];
	size_t str_size;
	size_t str_ptr;
	utf8_char* cstr = 0;
	if( fn_num == FN_SPRINTF )
	{
	    //sprintf:
	    str_size = str_cont->size * g_pix_container_type_sizes[ format_cont->type ];
	    str_ptr = 0;
	}
	else
	{
	    //printf:
	    str_size = 256;
	    str_ptr = 0;
	    cstr = (utf8_char*)bmem_new( str_size );
	    if( cstr == 0 ) goto sprintf_error;
	}
	int arg_num = 0;
	utf8_char* format_str = (utf8_char*)format_cont->data;
	for( size_t i = 0; i < format_size; i++ )
	{
	    utf8_char c = format_str[ i ];
	    bool c_to_output = 0;
	    if( c == '%' )
	    {
		i++;
		if( i >= format_size ) break;
		c = format_str[ i ];
		
		//Parse format:
		    
		utf8_char flags[ 2 ];
		flags[ 0 ] = 0;
		flags[ 1 ] = 0;
		switch( c )
		{
		    case '-':
		    case '+':
		    case ' ':
		    case '#':
		    case '0':
			flags[ 0 ] = c;
			break;
		}
		if( flags[ 0 ] )
		{
		    i++;
		    if( i >= format_size ) break;
		    c = format_str[ i ];
		}
		
		utf8_char width[ 4 ];
		i2 = 0;
		while( 1 )
		{
		    if( ( c >= '0' && c <= '9' ) || c == '*' )
		    {
			if( i2 < 3 )
			    width[ i2 ] = c;
			i2++;
			i++;
			if( i >= format_size ) break;
			c = format_str[ i ];
		    }
		    else break;
		}
		if( i >= format_size ) break;
		if( i2 < 4 ) width[ i2 ] = 0;
		
		utf8_char prec[ 5 ];
		i2 = 0;
		while( 1 )
		{
		    if( ( c >= '0' && c <= '9' ) || c == '*' || c == '.' )
		    {
			if( i2 < 4 )
			    prec[ i2 ] = c;
			i2++;
			i++;
			if( i >= format_size ) break;
			c = format_str[ i ];
		    }
		    else break;
		}
		if( i >= format_size ) break;
		if( i2 < 5 ) prec[ i2 ] = 0;
		
		utf8_char len[ 2 ];
		len[ 0 ] = 0;
		len[ 1 ] = 0;
		switch( c )
		{
		    case 'h':
		    case 'l':
		    case 'L':
			len[ 0 ] = c;
			break;
		}
		if( len[ 0 ] )
		{
		    i++;
		    if( i >= format_size ) break;
		    c = format_str[ i ];
		}
		
		utf8_char specifier[ 2 ];
		specifier[ 0 ] = c;
		specifier[ 1 ] = 0;
		
		if( specifier[ 0 ] == '%' )
		{
		    c_to_output = 1;
		}
		else 
		{
		    utf8_char arg_format[ 16 ];
		    arg_format[ 0 ] = 0;
		    strncat( arg_format, "%", 16 );
		    strncat( arg_format, flags, 16 );
		    strncat( arg_format, width, 16 );
		    strncat( arg_format, prec, 16 );
		    strncat( arg_format, len, 16 );
		    strncat( arg_format, specifier, 16 );
		    switch( specifier[ 0 ] )
		    {
			case 's': //String of characters:
			    {
				PIX_CID arg_str;
				GET_VAL_FROM_STACK( arg_str, args_off + arg_num, PIX_CID );
				arg_num++;
				if( (unsigned)arg_str >= (unsigned)vm->c_num ) break;
				pix_vm_container* arg_str_cont = vm->c[ arg_str ];
				if( arg_str_cont == 0 ) break;
				size_t arg_str_size = pix_vm_get_container_strlen( arg_str, 0, vm );
				if( str_ptr + arg_str_size > str_size )
				{
				    if( fn_num == FN_SPRINTF )
				    {
					//sprintf:
					if( pix_vm_resize_container( str, (PIX_INT)( str_ptr + arg_str_size + 8 ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) 
					    break;
				    }
				    else
				    {
					//printf:
					cstr = (utf8_char*)bmem_resize( cstr, str_ptr + arg_str_size + 8 );
					if( cstr == 0 )
					    break;
				    }
				    str_size = str_ptr + arg_str_size + 8;
				}
				for( i2 = 0; i2 < arg_str_size; i2++ )
				{
				    if( fn_num == FN_SPRINTF )
				    {
					//sprintf:
					((utf8_char*)str_cont->data)[ str_ptr ] = ((utf8_char*)arg_str_cont->data)[ i2 ];
				    }
				    else
				    {
					//printf:
					cstr[ str_ptr ] = ((utf8_char*)arg_str_cont->data)[ i2 ];
				    }
				    str_ptr++;
				}
			    }
			    break;
			case 'c': //Character:
			case 'd': //Signed decimal integer:
			case 'i': //Signed decimal integer:
			case 'o': //Signed octal:
			case 'u': //Unsigned decimal integer:
			case 'x': //Unsigned hexadecimal integer:
			case 'X': //Unsigned hexadecimal integer (capital letters):
			    {
				PIX_INT arg_int;
				GET_VAL_FROM_STACK( arg_int, args_off + arg_num, PIX_INT );
				arg_num++;
				utf8_char ts[ 32 ];
				snprintf( ts, 32, arg_format, (int)arg_int );
				for( i2 = 0; i2 < 32; i2++ )
				{
				    if( ts[ i2 ] == 0 ) break;
				    if( str_ptr >= str_size )
				    {
					if( fn_num == FN_SPRINTF )
					{
					    //sprintf:
					    if( pix_vm_resize_container( str, (PIX_INT)str_ptr + 8, 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) 
						break;
					}
					else
					{
					    //printf:
					    cstr = (utf8_char*)bmem_resize( cstr, str_ptr + 8 );
					    if( cstr == 0 ) break;
					}
					str_size = str_ptr + 8;
				    }
				    if( fn_num == FN_SPRINTF )
				    {
					//sprintf:
					((utf8_char*)str_cont->data)[ str_ptr ] = ts[ i2 ];
				    }
				    else
				    {
					//printf:
					cstr[ str_ptr ] = ts[ i2 ];
				    }
				    str_ptr++;
				}
			    }
			    break;
			case 'e': //Scientific notation (mantise/exponent) using e character:
			case 'E': //Scientific notation (mantise/exponent) using E character:
			case 'f': //Decimal floating point:
			case 'g': //Use the shorter of %e or %f:
			case 'G': //Use the shorter of %E or %f:
			    {
				PIX_FLOAT arg_float;
				GET_VAL_FROM_STACK( arg_float, args_off + arg_num, PIX_FLOAT );
				arg_num++;
				utf8_char ts[ 32 ];
				snprintf( ts, 32, arg_format, (float)arg_float );
				for( i2 = 0; i2 < 32; i2++ )
				{
				    if( ts[ i2 ] == 0 ) break;
				    if( str_ptr >= str_size )
				    {
					if( fn_num == FN_SPRINTF )
					{
					    //sprintf:
					    if( pix_vm_resize_container( str, (PIX_INT)str_ptr + 8, 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) 
						break;
					}
					else
					{
					    //printf:
					    cstr = (utf8_char*)bmem_resize( cstr, str_ptr + 8 );
					    if( cstr == 0 ) break;
					}
					str_size = str_ptr + 8;
				    }
				    if( fn_num == FN_SPRINTF )
				    {
					//sprintf:
					((utf8_char*)str_cont->data)[ str_ptr ] = ts[ i2 ];
				    }
				    else
				    {
					//printf:
					cstr[ str_ptr ] = ts[ i2 ];
				    }
				    str_ptr++;
				}
			    }
			    break;
		    }
		}
	    }
	    else
	    {
		c_to_output = 1;
	    }
	    if( c_to_output )
	    {
		if( str_ptr >= str_size )
		{
		    if( fn_num == FN_SPRINTF )
		    {
			//sprintf:
			if( pix_vm_resize_container( str, (PIX_INT)str_ptr + 8, 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) 
			    break;
		    }
		    else
		    {
			//printf:
			cstr = (utf8_char*)bmem_resize( cstr, str_ptr + 8 );
			if( cstr == 0 ) break;
		    }
		    str_size = str_ptr + 8;
		}
		if( fn_num == FN_SPRINTF )
		{
		    //sprintf:
		    ((utf8_char*)str_cont->data)[ str_ptr ] = c;
		}
		else
		{
		    //printf:
		    cstr[ str_ptr ] = c;
		}
		str_ptr++;
	    }
	}
	if( fn_num == FN_SPRINTF )
	{
	    //sprintf:
	    if( str_ptr < str_size )
	    {
		((utf8_char*)str_cont->data)[ str_ptr ] = 0;
	    }
	}
	else
	{
	    //printf / fprintf / logf:
	    if( cstr )
	    {
		if( str_ptr + 1 >= str_size )
		{
		    cstr = (utf8_char*)bmem_resize( cstr, str_ptr + 1 );
		}
		if( cstr )
		{
		    if( fn_num == FN_PRINTF || fn_num == FN_LOGF )
		    {
			cstr[ str_ptr ] = 0;
			if( fn_num == FN_LOGF )
			{
			    //Add message to the log buffer:
			    pix_vm_log( cstr, vm );
			}
			else
			{
			    //Printf:
			    printf( "%s", cstr );
			}
		    }
		    else
		    {
			//fprintf:
			str_ptr = bfs_write( cstr, 1, str_ptr, dest_stream );
		    }
		}
	    }
	}
	bmem_free( cstr );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)str_ptr;
    }
    else 
    {
	//Some error occured:
sprintf_error:
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}

//
// Log mamagement
//

void fn_get_log( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;
    
    if( vm->log_filled > 0 )
    {
	bmutex_lock( &vm->log_mutex );
	size_t log_size = bmem_get_size( vm->log_buffer );
	rv = pix_vm_new_container( -1, vm->log_filled + 1, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
	utf8_char* buf = (utf8_char*)pix_vm_get_container_data( rv, vm );
	if( buf )
	{
	    buf[ vm->log_filled ] = 0;
	    for( int i = (int)vm->log_filled - 1, i2 = (int)vm->log_ptr; i >= 0; i-- )
	    {
		i2--; if( i2 < 0 ) i2 = (int)log_size - 1;
	        buf[ i ] = vm->log_buffer[ i2 ];
	    }
	}
	bmutex_unlock( &vm->log_mutex );
    }

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_get_system_log( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;
    
    const utf8_char* fname = blog_get_file();
    if( fname )
    {
	size_t log_size = bfs_get_file_size( fname );
	if( log_size > 0 )
	{
	    PIX_INT cnum = pix_vm_new_container( -1, log_size, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
	    utf8_char* buf = (utf8_char*)pix_vm_get_container_data( cnum, vm );
	    if( buf )
	    {
		bfs_file f = bfs_open( fname, "rb" );
		if( f )
		{
		    bfs_read( buf, 1, log_size, f );
		    bfs_close( f );
		    rv = cnum;
		}
	    }
	}
    }

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

//
// Working with files
//

void fn_load( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num == 0 ) return;
		
    PIX_CID name;
    int par1 = 0;
    GET_VAL_FROM_STACK( name, 0, PIX_CID );
    if( pars_num >= 2 )
    {
	GET_VAL_FROM_STACK( par1, 1, int );
    }

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = -1;

    bool need_to_free = 0;
    utf8_char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
    if( ts == 0 ) return;
	
    utf8_char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
    if( full_path )
    {
	stack[ sp + ( pars_num - 1 ) ].i = pix_vm_load( (const utf8_char*)full_path, 0, par1, vm );
	bmem_free( full_path );
    }
    
    if( need_to_free ) bmem_free( ts );
}

void fn_fload( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num == 0 ) return;

    bfs_file stream;
    int par1 = 0;
    GET_VAL_FROM_STACK( stream, 0, bfs_file );
    if( pars_num >= 2 )
    {
	GET_VAL_FROM_STACK( par1, 1, int );
    }

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = pix_vm_load( 0, stream, par1, vm );
}    

void fn_save( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 3 ) return;
    
    PIX_CID cnum;
    PIX_CID name;
    int format;
    int par1 = 0;
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    GET_VAL_FROM_STACK( name, 1, PIX_CID );
    GET_VAL_FROM_STACK( format, 2, int );
    if( pars_num > 3 ) GET_VAL_FROM_STACK( par1, 3, int );

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = -1;

    bool need_to_free = 0;
    utf8_char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
    if( ts == 0 ) return;
    
    utf8_char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
    if( full_path )
    {
	stack[ sp + ( pars_num - 1 ) ].i = pix_vm_save( cnum, (const utf8_char*)full_path, 0, format, par1, vm );
	bmem_free( full_path );
    }
    
    if( need_to_free ) bmem_free( ts );
}

void fn_fsave( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 3 ) return;
    
    PIX_CID cnum;
    bfs_file stream;
    int format;
    int par1 = 0;
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    GET_VAL_FROM_STACK( stream, 1, bfs_file );
    GET_VAL_FROM_STACK( format, 2, int );
    if( pars_num > 3 ) GET_VAL_FROM_STACK( par1, 3, int );

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = pix_vm_save( cnum, 0, stream, format, par1, vm );
}

void fn_get_real_path( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name;
    pix_vm_container* name_cont;
            
    bool err = 0;
            
    //Get parameters:
    while( 1 )
    {
        if( pars_num < 1 ) { err = 1; break; }
        GET_VAL_FROM_STACK( name, 0, PIX_CID );
        if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
        name_cont = vm->c[ name ];
        break;
    }
        
    //Execute:
    if( err == 0 )
    {
        bool need_to_free = 0;
        utf8_char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
        if( ts )
        {
	    utf8_char* path = bfs_make_filename( (const utf8_char*)ts );
	    if( path == 0 )
	    {
		err = 1;
	    }
	    else
	    {
		int path_len = (int)bmem_strlen( path );
        	PIX_CID path_cnum = pix_vm_new_container( -1, path_len, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
        	if( path_cnum >= 0 )
        	{
            	    pix_vm_container* path_cont = vm->c[ path_cnum ];
            	    bmem_copy( path_cont->data, path, path_len );
        	}
        	bmem_free( path );
    		stack_types[ sp + ( pars_num - 1 ) ] = 0;
    		stack[ sp + ( pars_num - 1 ) ].i = path_cnum;
    	    }
    	    if( need_to_free ) bmem_free( ts );
    	}
    }
    
    if( err != 0 )
    {
        stack_types[ sp + ( pars_num - 1 ) ] = 0;
        stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}

struct flist_data
{
    utf8_char* path;
    utf8_char* mask;
    bfs_find_struct fs;
    utf8_char* cur_file;
    int cur_type;
};

void fn_new_flist( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    flist_data* f = 0;    
    utf8_char* path = 0;
    utf8_char* mask = 0;
    PIX_CID path_cnum = -1;
    PIX_CID mask_cnum = -1;
    
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( path_cnum, 0, PIX_CID );
    if( pars_num >= 2 ) GET_VAL_FROM_STACK( mask_cnum, 1, PIX_CID );
    
    bool need_to_free1 = 0;
    path = pix_vm_make_cstring_from_container( path_cnum, &need_to_free1, vm );
    bool need_to_free2 = 0;
    mask = pix_vm_make_cstring_from_container( mask_cnum, &need_to_free2, vm );
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = -1;
    
    if( path )
    {
	f = (flist_data*)bmem_new( sizeof( flist_data ) );
	bmem_zero( f );
	f->path = (utf8_char*)bmem_new( bmem_strlen( path ) + 1 ); f->path[ 0 ] = 0; bmem_strcat_resize( f->path, path );
	if( mask )
	{    
	    f->mask = (utf8_char*)bmem_new( bmem_strlen( mask ) + 1 ); f->mask[ 0 ] = 0; bmem_strcat_resize( f->mask, mask );
	}
	f->fs.start_dir = (const utf8_char*)f->path;
	f->fs.mask = (const utf8_char*)f->mask;
	if( bfs_find_first( &f->fs ) )
	{
	    f->cur_file = f->fs.name;
	    f->cur_type = f->fs.type;
	}
	stack[ sp + ( pars_num - 1 ) ].i = pix_vm_new_container( -1, bmem_get_size( f ), 1, PIX_CONTAINER_TYPE_INT8, f, vm );
    }
    
    if( need_to_free1 ) bmem_free( path );
    if( need_to_free2 ) bmem_free( mask );
}

void fn_remove_flist( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;    
    PIX_CID flist_cnum = -1;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( flist_cnum, 0, PIX_CID );
    flist_data* f = (flist_data*)pix_vm_get_container_data( flist_cnum, vm );
    if( f )
    {
	bfs_find_close( &f->fs );
	bmem_free( f->path );
	bmem_free( f->mask );
	pix_vm_remove_container( flist_cnum, vm );
    }
}

void fn_get_flist_name( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_CID flist_cnum = -1;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( flist_cnum, 0, PIX_CID );
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = -1;
    flist_data* f = (flist_data*)pix_vm_get_container_data( flist_cnum, vm );
    if( f && f->cur_file )
    {
	int name_len = (int)bmem_strlen( f->cur_file );
        PIX_CID name_cnum = pix_vm_new_container( -1, name_len, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
        if( name_cnum >= 0 )
        {
    	    pix_vm_container* name_cont = vm->c[ name_cnum ];
            bmem_copy( name_cont->data, f->cur_file, name_len );
	    stack[ sp + ( pars_num - 1 ) ].i = name_cnum;
        }
    }
}

void fn_get_flist_type( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_CID flist_cnum = -1;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( flist_cnum, 0, PIX_CID );
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = -1;
    flist_data* f = (flist_data*)pix_vm_get_container_data( flist_cnum, vm );
    if( f )
    {
	stack[ sp + ( pars_num - 1 ) ].i = f->cur_type;
    }
}

void fn_flist_next( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_CID flist_cnum = -1;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( flist_cnum, 0, PIX_CID );
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = 0;
    flist_data* f = (flist_data*)pix_vm_get_container_data( flist_cnum, vm );
    if( f )
    {
	if( bfs_find_next( &f->fs ) )
	{
	    f->cur_file = f->fs.name;
	    f->cur_type = f->fs.type;
	    stack[ sp + ( pars_num - 1 ) ].i = 1;
	}
    }
}

void fn_get_file_size( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name;
    pix_vm_container* name_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	name_cont = vm->c[ name ];
	break;
    }
    
    //Execute:
    if( err == 0 )
    {
	bool need_to_free = 0;
	utf8_char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
	
	utf8_char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)bfs_get_file_size( full_path );
	bmem_free( full_path );
	
	if( need_to_free ) bmem_free( ts );
    }
    else 
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = 0;
    }
}

//Remove a file:
void fn_remove_file( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name;
    pix_vm_container* name_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	name_cont = vm->c[ name ];
	break;
    }
    
    //Execute:
    if( err == 0 )
    {
	bool need_to_free = 0;
	utf8_char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
	
	utf8_char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)bfs_remove( full_path );
	bmem_free( full_path );
	
	if( need_to_free ) bmem_free( ts );
    }
    else 
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}

//Rename a file:
void fn_rename_file( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name1;
    PIX_CID name2;
    pix_vm_container* name1_cont;
    pix_vm_container* name2_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 2 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name1, 0, PIX_CID );
	GET_VAL_FROM_STACK( name2, 1, PIX_CID );
	if( (unsigned)name1 >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name1 ] == 0 ) { err = 1; break; }
	name1_cont = vm->c[ name1 ];
	if( (unsigned)name2 >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name2 ] == 0 ) { err = 1; break; }
	name2_cont = vm->c[ name2 ];
	if( name1_cont->size == 0 ) { err = 1; break; }
	if( name2_cont->size == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    if( err == 0 )
    {
	bool need_to_free1 = 0;
	bool need_to_free2 = 0;
	utf8_char* ts1 = pix_vm_make_cstring_from_container( name1, &need_to_free1, vm );
	utf8_char* ts2 = pix_vm_make_cstring_from_container( name2, &need_to_free2, vm );
	
	utf8_char* full_path1 = pix_compose_full_path( vm->base_path, ts1, vm );
	utf8_char* full_path2 = pix_compose_full_path( vm->base_path, ts2, vm );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)bfs_rename( full_path1, full_path2 );
	bmem_free( full_path1 );
	bmem_free( full_path2 );
	
	if( need_to_free1 ) bmem_free( ts1 );
	if( need_to_free2 ) bmem_free( ts2 );
    }
    else 
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}

//Copy a file:
void fn_copy_file( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name1;
    PIX_CID name2;
    pix_vm_container* name1_cont;
    pix_vm_container* name2_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 2 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name1, 0, PIX_CID );
	GET_VAL_FROM_STACK( name2, 1, PIX_CID );
	if( (unsigned)name1 >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name1 ] == 0 ) { err = 1; break; }
	name1_cont = vm->c[ name1 ];
	if( (unsigned)name2 >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name2 ] == 0 ) { err = 1; break; }
	name2_cont = vm->c[ name2 ];
	if( name1_cont->size == 0 ) { err = 1; break; }
	if( name2_cont->size == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    if( err == 0 )
    {
	bool need_to_free1 = 0;
	bool need_to_free2 = 0;
	utf8_char* ts1 = pix_vm_make_cstring_from_container( name1, &need_to_free1, vm );
	utf8_char* ts2 = pix_vm_make_cstring_from_container( name2, &need_to_free2, vm );
	
	utf8_char* full_path1 = pix_compose_full_path( vm->base_path, ts1, vm );
	utf8_char* full_path2 = pix_compose_full_path( vm->base_path, ts2, vm );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = bfs_copy_file( (const utf8_char*)full_path2, (const utf8_char*)full_path1 );
	bmem_free( full_path1 );
	bmem_free( full_path2 );
	
	if( need_to_free1 ) bmem_free( ts1 );
	if( need_to_free2 ) bmem_free( ts2 );
    }
    else 
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}

void fn_create_directory( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name;
    uint mode = 0;
#ifdef UNIX
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
#endif
    pix_vm_container* name_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
        if( pars_num > 1 ) GET_VAL_FROM_STACK( mode, 1, uint );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	name_cont = vm->c[ name ];
	if( name_cont->size == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    if( err == 0 )
    {
	bool need_to_free = 0;
	utf8_char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
	
	utf8_char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = bfs_mkdir( (const utf8_char*)full_path, mode );
	bmem_free( full_path );
	
	if( need_to_free ) bmem_free( ts );
    }
    else 
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}

//Set virtual disk 0:
void fn_set_disk0( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    bfs_file stream;    
    
    if( pars_num < 1 ) return;
    GET_VAL_FROM_STACK( stream, 0, bfs_file );
    vm->virt_disk0 = stream;
}

//Get virtual disk 0:
void fn_get_disk0( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = vm->virt_disk0;
}

//
// Working with files (posix)
//

//Open a stream:
void fn_fopen( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name;
    PIX_CID mode;
    pix_vm_container* name_cont;
    pix_vm_container* mode_cont;
    
    bool err = 0;

    //Get parameters:
    while( 1 )
    {
	if( pars_num < 2 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
	GET_VAL_FROM_STACK( mode, 1, PIX_CID );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	name_cont = vm->c[ name ];
	if( (unsigned)mode >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ mode ] == 0 ) { err = 1; break; }
	mode_cont = vm->c[ mode ];
	if( name_cont->size == 0 ) { err = 1; break; }
	if( mode_cont->size == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    if( err == 0 )
    {
	bool need_to_free1 = 0;
	bool need_to_free2 = 0;
	utf8_char* ts1 = pix_vm_make_cstring_from_container( name, &need_to_free1, vm );
	utf8_char* ts2 = pix_vm_make_cstring_from_container( mode, &need_to_free2, vm );
	
	utf8_char* full_path = pix_compose_full_path( vm->base_path, ts1, vm );
	bfs_file f = bfs_open( full_path, ts2 );
	bmem_free( full_path );
	
	if( need_to_free1 ) bmem_free( ts1 );
	if( need_to_free2 ) bmem_free( ts2 );
	
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)f;
    }
    else 
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = 0;
    }
}

//Open a stream in memory:
void fn_fopen_mem( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID data;
    pix_vm_container* data_cont = 0;
    
    bool err = 0;

    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( data, 0, PIX_CID );
	data_cont = pix_vm_get_container( data, vm );
	if( data_cont == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    if( err == 0 )
    {
	bfs_file f = bfs_open_in_memory( data_cont->data, data_cont->size * g_pix_container_type_sizes[ data_cont->type ] );
	bfs_set_user_data( f, data );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)f;
    }
    else 
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = 0;
    }
}

//Close a stream:
void fn_fclose( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	bfs_file f;
	GET_VAL_FROM_STACK( f, 0, bfs_file );
	if( bfs_get_type( f ) == BFS_FILE_IN_MEMORY )
	{
	    PIX_CID data_cnum = bfs_get_user_data( f );
	    if( data_cnum )
	    {
		void* data_ptr = bfs_get_data( f );
		if( data_ptr )
		{
		    pix_vm_container* data_cont = pix_vm_get_container( data_cnum, vm );
		    if( data_cont )
		    {
			size_t data_size = bfs_get_data_size( f );
			if( data_cont->data == data_ptr && data_cont->size * g_pix_container_type_sizes[ data_cont->type ] == data_size )
			{
			    //No changes
			}
			else
			{
			    data_cont->data = data_ptr;
			    data_cont->size = data_size / g_pix_container_type_sizes[ data_cont->type ];
			    data_cont->xsize = data_cont->size;
			    data_cont->ysize = 1;
			}
		    }
		}
	    }
	}
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = bfs_close( f );
    }
    else 
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}

//Put a byte on a stream:
void fn_fputc( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int c;
    bfs_file f;
    if( pars_num >= 2 ) 
    {
	GET_VAL_FROM_STACK( c, 0, int );
	GET_VAL_FROM_STACK( f, 1, bfs_file );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = bfs_putc( c, f );
    }
    else if( pars_num == 1 )
    {
	GET_VAL_FROM_STACK( c, 0, int );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = bfs_putc( c, BFS_STDOUT );
    }
}

//Put a string on a stream:
void fn_fputs( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_INT rv = -1;
    PIX_CID s = -1;
    bfs_file f;
    if( pars_num >= 2 ) 
    {
	GET_VAL_FROM_STACK( s, 0, PIX_CID );
	GET_VAL_FROM_STACK( f, 1, bfs_file );
    }
    else if( pars_num == 1 )
    {
	GET_VAL_FROM_STACK( s, 0, PIX_CID );
	f = BFS_STDOUT;
    }
    if( (unsigned)s < (unsigned)vm->c_num )
    {
	pix_vm_container* cont = vm->c[ s ];
	if( cont )
	{
	    size_t str_len;
	    for( str_len = 0; str_len < cont->size; str_len++ )
		if( ((utf8_char*)cont->data)[ str_len ] == 0 )
		    break;
	    rv = (PIX_INT)bfs_write( cont->data, 1, str_len, f );
	}
    }
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_fgets_OR_fwrite_OR_fread( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = 0;
    PIX_CID s = -1;
    PIX_INT offset = 0;
    PIX_INT size;
    bfs_file f;
    if( pars_num >= 3 ) 
    {
	GET_VAL_FROM_STACK( s, 0, PIX_CID );
	GET_VAL_FROM_STACK( size, 1, PIX_INT );
	GET_VAL_FROM_STACK( f, 2, bfs_file );
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( offset, 3, PIX_INT );
    }
    if( (unsigned)s < (unsigned)vm->c_num )
    {
	pix_vm_container* cont = vm->c[ s ];
	if( cont )
	{
	    size_t real_size = cont->size * g_pix_container_type_sizes[ cont->type ];
	    if( offset + size > real_size )
		size = (PIX_INT)real_size - offset;
	    char* data = (char*)cont->data + offset;
	    switch( fn_num )
	    {
		case FN_FGETS: 
		    {
			//Get a string from a stream:
			utf8_char* str = (utf8_char*)data;
			size_t p;
			for( p = 0; p < real_size - 1; p++ )
			{
			    int c = bfs_getc( f );
			    if( c == -1 ) break;
			    if( c == 0xD ) break;
			    if( c == 0xA ) break;
			    str[ p ] = (utf8_char)c;
			}
			str[ p ] = 0;
			rv = (PIX_INT)p;
		    }
		    break;
		case FN_FWRITE: 
		    rv = (PIX_INT)bfs_write( data, 1, size, f ); 
		    break;
		case FN_FREAD: 
		    rv = (PIX_INT)bfs_read( data, 1, size, f ); 
		    break;
	    }
	}
    }
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

//Get a byte from a stream:
void fn_fgetc( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    bfs_file f;
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( f, 0, bfs_file );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = bfs_getc( f );
    }
    else if( pars_num == 0 )
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = bfs_getc( BFS_STDIN );
    }
}

void fn_feof_OF_fflush_OR_ftell( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	bfs_file f;
	GET_VAL_FROM_STACK( f, 0, bfs_file );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	switch( fn_num )
	{
	    case FN_FEOF: stack[ sp + ( pars_num - 1 ) ].i = bfs_eof( f ); break;
	    case FN_FFLUSH: stack[ sp + ( pars_num - 1 ) ].i = bfs_flush( f ); break;
	    case FN_FTELL: stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)bfs_tell( f ); break;
	}
    }
}

void fn_fseek( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 3 )
    {
	bfs_file f;
	PIX_INT offset;
	PIX_INT mode;
	GET_VAL_FROM_STACK( f, 0, bfs_file );
	GET_VAL_FROM_STACK( offset, 1, PIX_INT );
	GET_VAL_FROM_STACK( mode, 2, PIX_INT );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = bfs_seek( f, offset, mode );
    }
}

void fn_setxattr( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

#if defined(UNIX) && !defined(NO_SETXATTR)
    
    PIX_CID path;
    PIX_CID name;
    PIX_CID value;
    size_t size;
    uint flags;
    pix_vm_container* path_cont;
    pix_vm_container* name_cont;
    void* value_data;
    
    bool err = 0;

    //Get parameters:
    while( 1 )
    {
	if( pars_num < 5 ) { err = 1; break; }
	GET_VAL_FROM_STACK( path, 0, PIX_CID );
	GET_VAL_FROM_STACK( name, 1, PIX_CID );
	GET_VAL_FROM_STACK( value, 2, PIX_CID );
	GET_VAL_FROM_STACK( size, 3, size_t );
	GET_VAL_FROM_STACK( flags, 4, int );
	path_cont = pix_vm_get_container( path, vm ); if( path_cont == 0 ) { err = 1; break; }
	name_cont = pix_vm_get_container( name, vm ); if( name_cont == 0 ) { err = 1; break; }
	value_data = pix_vm_get_container_data( value, vm ); if( value_data == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    if( err == 0 )
    {
	bool need_to_free1 = 0;
	bool need_to_free2 = 0;
	utf8_char* ts1 = pix_vm_make_cstring_from_container( path, &need_to_free1, vm );
	utf8_char* ts2 = pix_vm_make_cstring_from_container( name, &need_to_free2, vm );
	
	utf8_char* full_path = pix_compose_full_path( vm->base_path, ts1, vm );
	utf8_char* full_path2 = bfs_make_filename( full_path );
	bmem_free( full_path );
#if defined(FREEBSD) || defined(OSX) || defined(IPHONE)
	int res = setxattr( full_path2, ts2, value_data, size, 0, flags );
#else
	int res = setxattr( full_path2, ts2, value_data, size, flags );
#endif
	bmem_free( full_path2 );
	
	if( need_to_free1 ) bmem_free( ts1 );
	if( need_to_free2 ) bmem_free( ts2 );
	
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)res;
    }
    else 
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }

#else

    //Not *NIX compatible system:
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = -1;
    
#endif

}

//
// Graphics
//
	
void fn_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = 0;
    while( 1 )
    {
	if( vm->ready == 0 ) { time_sleep( 50 ); break; }
	int delay = 0;
	if( pars_num >= 1 ) GET_VAL_FROM_STACK( delay, 0, int );
	PIX_CID scr = vm->screen;
	if( (unsigned)scr < (unsigned)vm->c_num && vm->c[ scr ] && vm->screen_ptr != 0 )
	{
	    pix_vm_container* c = vm->c[ scr ];
	    vm->screen_change_x = 0;
	    vm->screen_change_y = 0;
	    vm->screen_change_xsize = c->xsize;
	    vm->screen_change_ysize = c->ysize;
	    if( pars_num >= 2 ) GET_VAL_FROM_STACK( vm->screen_change_x, 1, int );
	    if( pars_num >= 3 ) GET_VAL_FROM_STACK( vm->screen_change_y, 2, int );
	    if( pars_num >= 4 ) GET_VAL_FROM_STACK( vm->screen_change_xsize, 3, int );
	    if( pars_num >= 5 ) GET_VAL_FROM_STACK( vm->screen_change_ysize, 4, int );
	    vm->last_displayed_screen = vm->screen;
	    volatile int prev_counter = vm->screen_redraw_counter;
	    vm->screen_redraw_request++;
	    if( time_ticks() - vm->fps_time > time_ticks_per_second() )
	    {
		vm->fps = vm->fps_counter;
		vm->vars[ PIX_GVAR_FPS ].i = vm->fps;
		vm->var_types[ PIX_GVAR_FPS ] = 0;
		vm->fps_time = time_ticks();
		vm->fps_counter = 0;
	    }
	    vm->fps_counter++;
	    if( delay ) time_sleep( delay );
	    ticks_t t = time_ticks();
	    ticks_t t_timeout = time_ticks_per_second() / 2;
	    bool no_timeout = 0;
	    while( vm->screen_redraw_request != vm->screen_redraw_answer )
	    {
		if( vm->wm->suspended )
		    time_sleep( 1000 );
		else
		    time_sleep( 8 );
		if( no_timeout == 0 && time_ticks() - t >= t_timeout )
		{
		    if( prev_counter != vm->screen_redraw_counter )
		    {
			//Screen drawing callback is still active. So just waiting...
			no_timeout = 1;
		    }
		    else		
		    {
			//UI thread is not active for some reason
			rv = -2;
			break;
		    }
		}
	    }
	}
	else 
	{
	    //No screen.
	    if( delay == 0 ) delay = 50; 
	    time_sleep( delay );
	    rv = -1;
	}
	break;
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_vsync( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num < 1 ) return;
    bool vsync;
    GET_VAL_FROM_STACK( vsync, 0, bool );
    vm->vsync_request = vsync + 1;
    while( vm->vsync_request != 0 ) { time_sleep( 50 ); }
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = 0;
}

void fn_set_pixel_size( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	int size;
	GET_VAL_FROM_STACK( size, 0, int );
	if( size < 1 ) size = 1;
	int prev_size = vm->pixel_size;
	if( prev_size != size )
	{
	    float scale = (float)prev_size / (float)size;
	    vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i *= scale;
	    vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i *= scale;
	    vm->vars[ PIX_GVAR_PPI ].i *= scale;
	    vm->pixel_size = (uint16)size;
	}
    }
}

void fn_get_pixel_size( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = vm->pixel_size;
}

void fn_set_screen( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_CID scr;
	GET_VAL_FROM_STACK( scr, 0, PIX_CID );
	pix_vm_gfx_set_screen( scr, vm );
    }
}

void fn_get_screen( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = vm->screen;
}

void fn_set_zbuf( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
		
    if( pars_num >= 1 )
    {
	PIX_CID z;
	GET_VAL_FROM_STACK( z, 0, PIX_CID );
#ifdef OPENGL
	if( vm->screen == PIX_GL_SCREEN )
	{
	    if( vm->zbuf != PIX_GL_ZBUF && z == PIX_GL_ZBUF )
	    {
		glDepthFunc( GL_LESS );
		glEnable( GL_DEPTH_TEST );
	    }
	    if( vm->zbuf == PIX_GL_ZBUF && z == -1 )
	    {
		glDisable( GL_DEPTH_TEST );
	    }
	}
#endif
	vm->zbuf = z;
    }
}

void fn_get_zbuf( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = vm->zbuf;
}

void fn_clear_zbuf( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_VAL v;
    v.i = 0x80000000;
    pix_vm_clean_container( vm->zbuf, 0, v, 0, -1, vm );

#ifdef OPENGL
    if( vm->screen == PIX_GL_SCREEN )
    {
	if( vm->zbuf == PIX_GL_ZBUF )
	{
	    glClear( GL_DEPTH_BUFFER_BIT );
	}
    }
#endif
}

void fn_get_color( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
	    
    if( pars_num >= 3 )
    {
	int r, g, b;
	GET_VAL_FROM_STACK( r, 0, int );
	GET_VAL_FROM_STACK( g, 1, int );
	GET_VAL_FROM_STACK( b, 2, int );
	if( r < 0 ) r = 0;
	if( g < 0 ) g = 0;
	if( b < 0 ) b = 0;
	if( r > 255 ) r = 255;
	if( g > 255 ) g = 255;
	if( b > 255 ) b = 255;
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (COLORSIGNED)get_color( r, g, b );
    }
}

void fn_get_red( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_INT c;
	GET_VAL_FROM_STACK( c, 0, PIX_INT );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)red( c );
    }
}

void fn_get_green( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_INT c;
	GET_VAL_FROM_STACK( c, 0, PIX_INT );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)green( c );
    }
}

void fn_get_blue( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_INT c;
	GET_VAL_FROM_STACK( c, 0, PIX_INT );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)blue( c );
    }
}

void fn_get_blend( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 3 )
    {
	PIX_INT c1, c2, v;
	GET_VAL_FROM_STACK( c1, 0, PIX_INT );
	GET_VAL_FROM_STACK( c2, 1, PIX_INT );
	GET_VAL_FROM_STACK( v, 2, PIX_INT );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (COLORSIGNED)blend( c1, c2, (int)v );
    }
}

void fn_transp( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_INT t;
	GET_VAL_FROM_STACK( t, 0, PIX_INT );
	if( t < 0 ) t = 0;
	if( t > 255 ) t = 255;
	vm->transp = (uchar)t;
    }
}

void fn_get_transp( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = vm->transp;
}

void fn_clear( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int t = vm->transp;
    if( t == 0 ) return;

#ifdef OPENGL
    if( vm->screen == PIX_GL_SCREEN )
    {
	COLOR c = CLEARCOLOR;
	if( pars_num >= 1 )
	{
	    GET_VAL_FROM_STACK( c, 0, COLOR );
	}
	if( t >= 255 ) 
	{
	    glClearColor( (float)red( c ) / 255, (float)green( c ) / 255, (float)blue( c ) / 255, 1 );
	    glClear( GL_COLOR_BUFFER_BIT );
	}
	else
	{
	    bool prev_t_enabled = vm->t_enabled;
	    if( prev_t_enabled )
	    {
		vm->t_enabled = 0;
		pix_vm_gl_program_reset( vm );
	    }
	    gl_program_struct* p = vm->gl_prog_solid;
    	    if( vm->gl_current_prog != p )
	    {
		pix_vm_gl_use_prog( p, vm );
		gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) );
	    }
	    float v[ 8 ];
	    float xsize = (float)vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i / 2.0F;
	    float ysize = (float)vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i / 2.0F;
	    v[ 0 ] = -xsize; v[ 1 ] = -ysize;
	    v[ 2 ] = xsize; v[ 3 ] = -ysize;
	    v[ 4 ] = -xsize; v[ 5 ] = ysize;
	    v[ 6 ] = xsize; v[ 7 ] = ysize;
	    glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( c ) / 255, (float)green( c ) / 255, (float)blue( c ) / 255, (float)t / 255 );
	    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 2, GL_FLOAT, false, 0, v );
	    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	    if( prev_t_enabled )
	    {
		vm->t_enabled = 1;
		pix_vm_gl_program_reset( vm );
	    }
	}
	return;
    }
#endif
    
    if( vm->screen_ptr == 0 ) return;
    COLOR c = CLEARCOLOR;
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( c, 0, COLOR );
    }
    COLORPTR p = vm->screen_ptr;
    COLORPTR p_end = vm->screen_ptr + vm->screen_xsize * vm->screen_ysize;
    if( t == 255 )
    {
	for( ; p < p_end; p++ )
	{
	    *p = c;
	}
    }
    else 
    {
	for( ; p < p_end; p++ )
	{
	    *p = fast_blend( *p, c, t );
	}
    }
}

void fn_dot( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( vm->screen_ptr == 0 ) return;
    if( vm->transp == 0 ) return;
		
    COLOR c = COLORMASK;
    PIX_INT x, y, z;

    if( fn_num == FN_DOT )
    {
	if( pars_num < 2 ) return;
	if( pars_num >= 3 )
	{
	    GET_VAL_FROM_STACK( c, 2, COLOR );
	}
    }
    else 
    {
	if( pars_num < 3 ) return;
	if( pars_num >= 4 )
	{
	    GET_VAL_FROM_STACK( c, 3, COLOR );
	}
    }

#ifdef OPENGL
    if( vm->screen == PIX_GL_SCREEN )
    {
	float v[ 3 ];
	if( fn_num == FN_DOT )
	{
	    GET_VAL_FROM_STACK( v[ 0 ], 0, float );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, float );
	    v[ 2 ] = 0;
	}
	else
	{
	    GET_VAL_FROM_STACK( v[ 0 ], 0, float );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, float );
	    GET_VAL_FROM_STACK( v[ 2 ], 2, float );
	}
	gl_program_struct* p = vm->gl_prog_solid;
	if( vm->gl_user_defined_prog ) p = vm->gl_user_defined_prog;
	if( vm->gl_current_prog != p )
        {
            pix_vm_gl_use_prog( p, vm );
            gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) );
        }
	glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( c ) / 255, (float)green( c ) / 255, (float)blue( c ) / 255, (float)vm->transp / 255 );
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, v );
	glDrawArrays( GL_POINTS, 0, 1 );
	return;
    }
#endif
		    
    if( vm->t_enabled )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT v[ 3 ];
	GET_VAL_FROM_STACK( v[ 0 ], 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( v[ 1 ], 1, PIX_FLOAT );
	if( fn_num == FN_DOT )
	    v[ 2 ] = 0;
	else 
	    GET_VAL_FROM_STACK( v[ 2 ], 2, PIX_FLOAT );
	pix_vm_gfx_vertex_transform( v, m );
	x = v[ 0 ];
	y = v[ 1 ];
	z = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
    }
    else 
    {
	GET_VAL_FROM_STACK( x, 0, PIX_INT );
	GET_VAL_FROM_STACK( y, 1, PIX_INT );
	if( fn_num == FN_GET_DOT )
	    z = 0;
	else 
	{
	    PIX_FLOAT zf;
	    GET_VAL_FROM_STACK( zf, 1, PIX_FLOAT );
	    z = zf * ( 1 << PIX_FIXED_MATH_PREC );
	}
    }
    x += vm->screen_xsize / 2;
    y += vm->screen_ysize / 2;
    if( (unsigned)x < (unsigned)vm->screen_xsize &&
	(unsigned)y < (unsigned)vm->screen_ysize )
    {
	int* zbuf = pix_vm_gfx_get_zbuf( vm );
	if( zbuf )
	{
	    int p = y * vm->screen_xsize + x;
	    if( z > zbuf[ p ] ) 
	    {
		if( vm->transp == 255 )
		{
		    vm->screen_ptr[ p ] = c;
		}
		else
		{
		    vm->screen_ptr[ p ] = fast_blend( vm->screen_ptr[ p ], c, vm->transp );
		}
		zbuf[ p ] = z;
	    }
	}
	else 
	{
	    COLORPTR p = vm->screen_ptr + y * vm->screen_xsize + x;
	    if( vm->transp == 255 )
	    {
		*p = c;
	    }
	    else
	    {
		*p = fast_blend( *p, c, vm->transp );
	    }
	}
    }
}

void fn_get_dot( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( vm->screen_ptr == 0 ) return;
    
    PIX_INT x, y;
    
    if( fn_num == FN_GET_DOT )
    {
	if( pars_num < 2 ) return;
    }
    else 
    {
	if( pars_num < 3 ) return;
    }
    
    if( vm->t_enabled )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT v[ 3 ];
	GET_VAL_FROM_STACK( v[ 0 ], 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( v[ 1 ], 1, PIX_FLOAT );
	if( fn_num == FN_GET_DOT )
	    v[ 2 ] = 0;
	else 
	    GET_VAL_FROM_STACK( v[ 2 ], 2, PIX_FLOAT );
	pix_vm_gfx_vertex_transform( v, m );
	x = v[ 0 ];
	y = v[ 1 ];
    }
    else 
    {
	GET_VAL_FROM_STACK( x, 0, PIX_INT );
	GET_VAL_FROM_STACK( y, 1, PIX_INT );
    }
    x += vm->screen_xsize / 2;
    y += vm->screen_ysize / 2;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    if( (unsigned)x < (unsigned)vm->screen_xsize &&
	(unsigned)y < (unsigned)vm->screen_ysize )
    {
	COLORPTR p = vm->screen_ptr + y * vm->screen_xsize + x;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)*p;
    }
    else 
    {
	stack[ sp + ( pars_num - 1 ) ].i = 0;
    }
}

void fn_line( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( vm->screen_ptr == 0 ) return;
    if( vm->transp == 0 ) return;

    COLOR c = COLORMASK;
    PIX_INT x1, y1, z1, x2, y2, z2;
    
    if( fn_num == FN_LINE )
    {
	if( pars_num < 4 ) return;
	if( pars_num >= 5 )
	{
	    GET_VAL_FROM_STACK( c, 4, COLOR );
	}	
    }
    else
    {
	if( pars_num < 6 ) return;
	if( pars_num >= 7 )
	{
	    GET_VAL_FROM_STACK( c, 6, COLOR );
	}
    }

#ifdef OPENGL
    if( vm->screen == PIX_GL_SCREEN )
    {
	float v[ 6 ];
	if( fn_num == FN_LINE )
	{
	    GET_VAL_FROM_STACK( v[ 0 ], 0, float );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, float );
	    v[ 2 ] = 0;
	    GET_VAL_FROM_STACK( v[ 3 ], 2, float );
	    GET_VAL_FROM_STACK( v[ 4 ], 3, float );
	    v[ 5 ] = 0;
	}
	else
	{
	    GET_VAL_FROM_STACK( v[ 0 ], 0, float );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, float );
	    GET_VAL_FROM_STACK( v[ 2 ], 2, float );
	    GET_VAL_FROM_STACK( v[ 3 ], 3, float );
	    GET_VAL_FROM_STACK( v[ 4 ], 4, float );
	    GET_VAL_FROM_STACK( v[ 5 ], 5, float );
	}
	gl_program_struct* p = vm->gl_prog_solid;
	if( vm->gl_user_defined_prog ) p = vm->gl_user_defined_prog;
	if( vm->gl_current_prog != p )
        {
            pix_vm_gl_use_prog( p, vm );
            gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) );
        }
	glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( c ) / 255, (float)green( c ) / 255, (float)blue( c ) / 255, (float)vm->transp / 255 );
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, v );
	glDrawArrays( GL_LINES, 0, 2 );
	return;
    }
#endif
    
    int* zbuf = pix_vm_gfx_get_zbuf( vm );

    if( fn_num == FN_LINE )
    {
	if( vm->t_enabled )
	{
	    PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	    PIX_FLOAT v[ 3 ];
	    
	    GET_VAL_FROM_STACK( v[ 0 ], 0, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, PIX_FLOAT );
	    v[ 2 ] = 0;
	    pix_vm_gfx_vertex_transform( v, m );
	    x1 = v[ 0 ];
	    y1 = v[ 1 ];
	    if( zbuf ) z1 = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );

	    GET_VAL_FROM_STACK( v[ 0 ], 2, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 1 ], 3, PIX_FLOAT );
	    v[ 2 ] = 0;
	    pix_vm_gfx_vertex_transform( v, m );
	    x2 = v[ 0 ];
	    y2 = v[ 1 ];
	    if( zbuf ) z2 = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	}
	else 
	{
	    GET_VAL_FROM_STACK( x1, 0, PIX_INT );
	    GET_VAL_FROM_STACK( y1, 1, PIX_INT );
	    GET_VAL_FROM_STACK( x2, 2, PIX_INT );
	    GET_VAL_FROM_STACK( y2, 3, PIX_INT );
	    if( zbuf )
	    {
		z1 = 0;
		z2 = 0;
	    }
	}
    }
    else 
    {
	if( vm->t_enabled )
	{
	    PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	    PIX_FLOAT v[ 3 ];
	    
	    GET_VAL_FROM_STACK( v[ 0 ], 0, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 2 ], 2, PIX_FLOAT );
	    pix_vm_gfx_vertex_transform( v, m );
	    x1 = v[ 0 ];
	    y1 = v[ 1 ];
	    if( zbuf ) z1 = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    
	    GET_VAL_FROM_STACK( v[ 0 ], 3, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 1 ], 4, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 2 ], 5, PIX_FLOAT );
	    pix_vm_gfx_vertex_transform( v, m );
	    x2 = v[ 0 ];
	    y2 = v[ 1 ];
	    if( zbuf ) z2 = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	}
	else 
	{
	    PIX_FLOAT zz;
	    GET_VAL_FROM_STACK( x1, 0, PIX_INT );
	    GET_VAL_FROM_STACK( y1, 1, PIX_INT );
	    GET_VAL_FROM_STACK( x2, 3, PIX_INT );
	    GET_VAL_FROM_STACK( y2, 4, PIX_INT );
	    if( zbuf ) 
	    {
		GET_VAL_FROM_STACK( zz, 2, PIX_FLOAT );
		z1 = zz * ( 1 << PIX_FIXED_MATH_PREC );
		GET_VAL_FROM_STACK( zz, 5, PIX_FLOAT );
		z2 = zz * ( 1 << PIX_FIXED_MATH_PREC );
	    }
	}
    }
    
    int screen_hxsize = vm->screen_xsize / 2;
    int screen_hysize = vm->screen_ysize / 2;
    
    x1 += screen_hxsize;
    y1 += screen_hysize;
    x2 += screen_hxsize;
    y2 += screen_hysize;
    if( zbuf )
    {
	pix_vm_gfx_draw_line_zbuf( x1, y1, z1, x2, y2, z2, c, zbuf, vm );
    }
    else 
    {
	pix_vm_gfx_draw_line( x1, y1, x2, y2, c, vm );
    }
}

void fn_box( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( vm->screen_ptr == 0 ) return;
    if( vm->transp == 0 ) return;
		
    COLOR c = COLORMASK;
    if( pars_num < 4 ) return;
    if( pars_num >= 5 )
    {
	GET_VAL_FROM_STACK( c, 4, COLOR );
    }

    int screen_hxsize = vm->screen_xsize / 2;
    int screen_hysize = vm->screen_ysize / 2;
    
    if( vm->t_enabled || vm->zbuf >= 0 || vm->screen == PIX_GL_SCREEN )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT x, y, xsize, ysize;
	
	GET_VAL_FROM_STACK( x, 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( y, 1, PIX_FLOAT );
	GET_VAL_FROM_STACK( xsize, 2, PIX_FLOAT );
	GET_VAL_FROM_STACK( ysize, 3, PIX_FLOAT );
	
	PIX_FLOAT v[ 12 ];
	v[ 0 ] = x;
	v[ 1 ] = y;
	v[ 2 ] = 0;
	v[ 3 ] = x + xsize;
	v[ 4 ] = y;
	v[ 5 ] = 0;
	v[ 6 ] = x;
	v[ 7 ] = y + ysize;
	v[ 8 ] = 0;
	v[ 9 ] = x + xsize;
	v[ 10 ] = y + ysize;
	v[ 11 ] = 0;
#ifdef OPENGL
	if( vm->screen == PIX_GL_SCREEN )
	{
	    gl_program_struct* p = vm->gl_prog_solid;
	    if( vm->gl_user_defined_prog ) p = vm->gl_user_defined_prog;
	    if( vm->gl_current_prog != p )
    	    {
    		pix_vm_gl_use_prog( p, vm );
    		gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) );
    	    }
	    glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( c ) / 255, (float)green( c ) / 255, (float)blue( c ) / 255, (float)vm->transp / 255 );
	    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, v );
	    float vvv[ 15 ];
	    float* vv;
	    if( fn_num == FN_BOX )
	    {
		vvv[ 0 ] = v[ 0 ];
		vvv[ 1 ] = v[ 1 ];
		vvv[ 2 ] = v[ 2 ];
		vvv[ 3 ] = v[ 3 ];
		vvv[ 4 ] = v[ 4 ];
		vvv[ 5 ] = v[ 5 ];
		vvv[ 6 ] = v[ 9 ];
		vvv[ 7 ] = v[ 10 ];
		vvv[ 8 ] = v[ 11 ];
		vvv[ 9 ] = v[ 6 ];
		vvv[ 10 ] = v[ 7 ];
		vvv[ 11 ] = v[ 8 ];
		vvv[ 12 ] = v[ 0 ];
		vvv[ 13 ] = v[ 1 ];
		vvv[ 14 ] = v[ 2 ];
		glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, vvv );
		glDrawArrays( GL_LINE_STRIP, 0, 5 );
	    }
	    else
	    {
		if( sizeof( float ) == sizeof( PIX_FLOAT ) )
	    	    vv = (float*)v;
		else
		{
		    vvv[ 0 ] = v[ 0 ];
		    vvv[ 1 ] = v[ 1 ];
		    vvv[ 2 ] = v[ 2 ];
		    vvv[ 3 ] = v[ 3 ];
		    vvv[ 4 ] = v[ 4 ];
		    vvv[ 5 ] = v[ 5 ];
		    vvv[ 6 ] = v[ 6 ];
		    vvv[ 7 ] = v[ 7 ];
		    vvv[ 8 ] = v[ 8 ];
		    vvv[ 9 ] = v[ 9 ];
		    vvv[ 10 ] = v[ 10 ];
		    vvv[ 11 ] = v[ 11 ];
		    vv = vvv;
		}
		glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, vv );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	    }
	    return;
	}
#endif
	if( vm->t_enabled )
	{
	    pix_vm_gfx_vertex_transform( v, m );
	    pix_vm_gfx_vertex_transform( v + 3, m );
	    pix_vm_gfx_vertex_transform( v + 6, m );
	    pix_vm_gfx_vertex_transform( v + 9, m );
	}
	v[ 0 ] += screen_hxsize;
	v[ 1 ] += screen_hysize;
	v[ 3 ] += screen_hxsize;
	v[ 4 ] += screen_hysize;
	v[ 6 ] += screen_hxsize;
	v[ 7 ] += screen_hysize;
	v[ 9 ] += screen_hxsize;
	v[ 10 ] += screen_hysize;
	if( fn_num == FN_BOX )
	{
	    pix_vm_gfx_draw_line( v[ 0 ], v[ 1 ], v[ 3 ], v[ 4 ], c, vm );
	    pix_vm_gfx_draw_line( v[ 3 ], v[ 4 ], v[ 9 ], v[ 10 ], c, vm );
	    pix_vm_gfx_draw_line( v[ 9 ], v[ 10 ], v[ 6 ], v[ 7 ], c, vm );
	    pix_vm_gfx_draw_line( v[ 6 ], v[ 7 ], v[ 0 ], v[ 1 ], c, vm );
	}
	else 
	{
	    pix_vm_ivertex iv1;
	    pix_vm_ivertex iv2;
	    pix_vm_ivertex iv3;
	    iv1.x = v[ 0 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.y = v[ 1 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.z = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.x = v[ 3 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.y = v[ 4 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.z = v[ 5 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.x = v[ 6 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.y = v[ 7 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.z = v[ 8 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    if( vm->zbuf >= 0 )
		pix_vm_gfx_draw_triangle_zbuf( &iv1, &iv2, &iv3, c, vm );
	    else 
		pix_vm_gfx_draw_triangle( &iv1, &iv2, &iv3, c, vm );
	    iv1.x = v[ 9 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.y = v[ 10 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.z = v[ 11 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    if( vm->zbuf >= 0 )
		pix_vm_gfx_draw_triangle_zbuf( &iv3, &iv2, &iv1, c, vm );
	    else 
		pix_vm_gfx_draw_triangle( &iv3, &iv2, &iv1, c, vm );
	}
    }
    else 
    {
	PIX_INT x, y, xsize, ysize;
	
	GET_VAL_FROM_STACK( x, 0, PIX_INT );
	GET_VAL_FROM_STACK( y, 1, PIX_INT );
	GET_VAL_FROM_STACK( xsize, 2, PIX_INT );
	GET_VAL_FROM_STACK( ysize, 3, PIX_INT );
	
	x += screen_hxsize;
	y += screen_hysize;
	
	if( fn_num == FN_BOX )
	    pix_vm_gfx_draw_box( x, y, xsize, ysize, c, vm );
	else 
	    pix_vm_gfx_draw_fbox( x, y, xsize, ysize, c, vm );
    }
}

void fn_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( vm->screen_ptr == 0 ) return;

    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    PIX_FLOAT x, y, xscale, yscale;
    PIX_INT tx, ty, txsize, tysize;
    COLOR c = get_color( 255, 255, 255 );
    x = 0;
    y = 0;
    xscale = 1;
    yscale = 1;
    tx = 0;
    ty = 0;
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );

    if( (unsigned)cnum >= (unsigned)vm->c_num ) return;
    pix_vm_container* cont = vm->c[ cnum ];
    if( cont == 0 ) return;
    
    if( cont->opt_data && cont->opt_data->hdata )
    {
	//Try to auto-play animation:
	int f = pix_vm_container_hdata_autoplay_control( cnum, vm );
	if( f >= 0 ) pix_vm_container_hdata_unpack_frame( cnum, vm );
    }

    txsize = cont->xsize;
    tysize = cont->ysize;

    while( 1 )
    {
	if( pars_num >= 2 ) GET_VAL_FROM_STACK( x, 1, PIX_FLOAT ) else break;
        if( pars_num >= 3 ) GET_VAL_FROM_STACK( y, 2, PIX_FLOAT ) else break;
        if( pars_num >= 4 ) GET_VAL_FROM_STACK( c, 3, COLOR ) else break;
	if( pars_num >= 5 ) GET_VAL_FROM_STACK( xscale, 4, PIX_FLOAT ) else break;
        if( pars_num >= 6 ) GET_VAL_FROM_STACK( yscale, 5, PIX_FLOAT ) else break;
	if( xscale == 0 ) return;
	if( yscale == 0 ) return;
        if( pars_num >= 7 ) GET_VAL_FROM_STACK( tx, 6, PIX_INT ) else break;
        if( pars_num >= 8 ) GET_VAL_FROM_STACK( ty, 7, PIX_INT ) else break;
        if( pars_num >= 9 ) GET_VAL_FROM_STACK( txsize, 8, PIX_INT ) else break;
        if( pars_num >= 10 ) GET_VAL_FROM_STACK( tysize, 9, PIX_INT ) else break;
        break;
    }
    
    PIX_FLOAT xsize = (PIX_FLOAT)txsize * xscale;
    PIX_FLOAT ysize = (PIX_FLOAT)tysize * yscale;
        
    pix_vm_gfx_draw_container( cnum, x - xsize / 2, y - ysize / 2, 0, xsize, ysize, tx, ty, txsize, tysize, c, vm );
}

void fn_triangles3d( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num < 2 ) return;
    
    PIX_CID vertices;
    PIX_CID triangles;
    PIX_INT tnum2 = -1;
    
    GET_VAL_FROM_STACK( vertices, 0, PIX_CID );
    GET_VAL_FROM_STACK( triangles, 1, PIX_CID );
    while( 1 )
    {
        if( pars_num >= 3 ) GET_VAL_FROM_STACK( tnum2, 2, PIX_INT ) else break;
        break;
    }

    if( tnum2 == 0 ) return;
    
    pix_vm_container* v_cont = pix_vm_get_container( vertices, vm );
    pix_vm_container* t_cont = pix_vm_get_container( triangles, vm );
    if( v_cont == 0 ) return;
    if( t_cont == 0 ) return;
    if( v_cont->data == 0 ) return;
    if( t_cont->data == 0 ) return;
    if( v_cont->size < 8 ) return;
    if( t_cont->size < 8 ) return;

    int screen_hxsize = vm->screen_xsize / 2;
    int screen_hysize = vm->screen_ysize / 2;

    size_t vnum = v_cont->size / 8;
    size_t tnum = t_cont->size / 8;
    if( tnum2 > 0 )
    {
	if( tnum2 < tnum )
	    tnum = tnum2;
    }
    for( size_t t = 0; t < tnum; t++ )
    {
	size_t ts = pix_vm_get_container_int_element( triangles, t * 8 + 7, vm );
	PIX_INT v1num = pix_vm_get_container_int_element( triangles, ts * 8 + 0, vm );
	PIX_INT v2num = pix_vm_get_container_int_element( triangles, ts * 8 + 1, vm );
	PIX_INT v3num = pix_vm_get_container_int_element( triangles, ts * 8 + 2, vm );
	COLOR color = (COLOR)pix_vm_get_container_int_element( triangles, ts * 8 + 3, vm );
	PIX_INT texture = pix_vm_get_container_int_element( triangles, ts * 8 + 4, vm );
	PIX_INT transp = pix_vm_get_container_int_element( triangles, ts * 8 + 5, vm );
	if( transp <= 0 ) continue;
	if( transp >= 255 ) transp = 255;
	uchar prev_transp = vm->transp;
	uchar new_transp = ( (int)transp * (int)vm->transp ) / 256;
	if( new_transp == 0 ) continue;
	vm->transp = new_transp;
	if( (unsigned)v1num >= vnum ) continue;
	if( (unsigned)v2num >= vnum ) continue;
	if( (unsigned)v3num >= vnum ) continue;
	PIX_FLOAT v1[ 5 ];
	PIX_FLOAT v2[ 5 ];
	PIX_FLOAT v3[ 5 ];
	v1[ 0 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 0, vm );
	v1[ 1 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 1, vm );
	v1[ 2 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 2, vm );
	v2[ 0 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 0, vm );
	v2[ 1 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 1, vm );
	v2[ 2 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 2, vm );
	v3[ 0 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 0, vm );
	v3[ 1 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 1, vm );
	v3[ 2 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 2, vm );
	if( texture >= 0 )
	{
	    v1[ 3 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 3, vm );
	    v1[ 4 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 4, vm );
	    v2[ 3 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 3, vm );
	    v2[ 4 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 4, vm );
	    v3[ 3 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 3, vm );
	    v3[ 4 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 4, vm );
	}
#ifdef OPENGL
	if( vm->screen == PIX_GL_SCREEN )
	{
	    pix_vm_container_gl_data* gl = 0;
	    gl_program_struct* p;
	    float v[ 3 * 3 ];
	    v[ 0 ] = v1[ 0 ]; v[ 1 ] = v1[ 1 ]; v[ 2 ] = v1[ 2 ];
	    v[ 3 ] = v2[ 0 ]; v[ 4 ] = v2[ 1 ]; v[ 5 ] = v2[ 2 ];
	    v[ 6 ] = v3[ 0 ]; v[ 7 ] = v3[ 1 ]; v[ 8 ] = v3[ 2 ];
	    if( texture == -1 )
	    {
	        p = vm->gl_prog_solid;
	    }
	    else
	    {
		gl = pix_vm_create_container_gl_data( texture, vm );
    		if( gl == 0 ) goto triangle_draw_end;
    		if( gl->texture_format == GL_ALPHA )
	    	    p = vm->gl_prog_tex_alpha_solid;
        	else
    	    	    p = vm->gl_prog_tex_rgb_solid;
    	    }
    	    if( vm->gl_user_defined_prog ) p = vm->gl_user_defined_prog;
	    if( vm->gl_current_prog != p )
            {
    	        pix_vm_gl_use_prog( p, vm );
    	        uint attr = 1 << GL_PROG_ATT_POSITION;
    		if( texture != -1 )
    		{
	    	    attr |= 1 << GL_PROG_ATT_TEX_COORD;
            	    glActiveTexture( GL_TEXTURE0 );
        	    glUniform1i( p->uniforms[ GL_PROG_UNI_TEXTURE ], 0 );
        	}
    	        gl_enable_attributes( p, attr );
    	    }
    	    glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( color ) / 255, (float)green( color ) / 255, (float)blue( color ) / 255, (float)vm->transp / 255 );
    	    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, v );
	    if( texture != -1 )
	    {
    		float t[ 2 * 3 ];
    		t[ 0 ] = v1[ 3 ] / gl->xsize; t[ 1 ] = v1[ 4 ] / gl->ysize;
    		t[ 2 ] = v2[ 3 ] / gl->xsize; t[ 3 ] = v2[ 4 ] / gl->ysize;
    		t[ 4 ] = v3[ 3 ] / gl->xsize; t[ 5 ] = v3[ 4 ] / gl->ysize;
		glBindTexture( GL_TEXTURE_2D, gl->texture_id );
        	glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( color ) / 255, (float)green( color ) / 255, (float)blue( color ) / 255, (float)vm->transp / 255 );
		glVertexAttribPointer( p->attributes[ GL_PROG_ATT_TEX_COORD ], 2, GL_FLOAT, false, 0, t );
    	    }
    	    glDrawArrays( GL_TRIANGLES, 0, 3 );
	    goto triangle_draw_end;
	}
#endif
	if( vm->t_enabled )
	{
	    pix_vm_gfx_vertex_transform( v1, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	    pix_vm_gfx_vertex_transform( v2, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	    pix_vm_gfx_vertex_transform( v3, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	}
	v1[ 0 ] += screen_hxsize;
	v1[ 1 ] += screen_hysize;
	v2[ 0 ] += screen_hxsize;
	v2[ 1 ] += screen_hysize;
	v3[ 0 ] += screen_hxsize;
	v3[ 1 ] += screen_hysize;
	if( texture == -1 )
	{
	    //No texture:
	    pix_vm_ivertex iv1;
	    pix_vm_ivertex iv2;
	    pix_vm_ivertex iv3;
	    iv1.x = v1[ 0 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.y = v1[ 1 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.z = v1[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.x = v2[ 0 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.y = v2[ 1 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.z = v2[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.x = v3[ 0 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.y = v3[ 1 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.z = v3[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    if( vm->zbuf >= 0 )
		pix_vm_gfx_draw_triangle_zbuf( &iv1, &iv2, &iv3, color, vm );
	    else 
		pix_vm_gfx_draw_triangle( &iv1, &iv2, &iv3, color, vm );
	}
	else
	{
	    pix_vm_gfx_draw_triangle_t( v1, v2, v3, texture, color, vm );
	}
triangle_draw_end:
	vm->transp = prev_transp;
    }
}

void fn_sort_triangles3d( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num < 2 ) return;
    
    PIX_CID vertices;
    PIX_CID triangles;
    PIX_INT tnum2 = -1;
    
    GET_VAL_FROM_STACK( vertices, 0, PIX_CID );
    GET_VAL_FROM_STACK( triangles, 1, PIX_CID );
    while( 1 )
    {
        if( pars_num >= 3 ) GET_VAL_FROM_STACK( tnum2, 2, PIX_INT ) else break;
        break;
    }

    if( tnum2 == 0 ) return;

    pix_vm_container* v_cont = pix_vm_get_container( vertices, vm );
    pix_vm_container* t_cont = pix_vm_get_container( triangles, vm );
    if( v_cont == 0 ) return;
    if( t_cont == 0 ) return;
    if( v_cont->data == 0 ) return;
    if( t_cont->data == 0 ) return;
    if( v_cont->size < 8 ) return;
    if( t_cont->size < 8 ) return;
    
    size_t vnum = v_cont->size / 8;
    size_t tnum = t_cont->size / 8;
    if( tnum2 > 0 )
    {
	if( tnum2 < tnum )
	    tnum = tnum2;
    }
    PIX_FLOAT* zz = (PIX_FLOAT*)bmem_new( tnum * sizeof( PIX_FLOAT ) );
    int* order = (int*)bmem_new( tnum * sizeof( int ) );
    if( zz == 0 ) return;
    if( order == 0 ) return;
    for( size_t t = 0; t < tnum; t++ )
    {
	order[ t ] = (int)t;
	zz[ t ] = 0;
	PIX_INT v1num = pix_vm_get_container_int_element( triangles, t * 8 + 0, vm );
	PIX_INT v2num = pix_vm_get_container_int_element( triangles, t * 8 + 1, vm );
	PIX_INT v3num = pix_vm_get_container_int_element( triangles, t * 8 + 2, vm );
	if( (unsigned)v1num >= vnum ) continue;
	if( (unsigned)v2num >= vnum ) continue;
	if( (unsigned)v3num >= vnum ) continue;
	PIX_FLOAT v1[ 3 ];
	PIX_FLOAT v2[ 3 ];
	PIX_FLOAT v3[ 3 ];
	v1[ 0 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 0, vm );
	v1[ 1 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 1, vm );
	v1[ 2 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 2, vm );
	v2[ 0 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 0, vm );
	v2[ 1 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 1, vm );
	v2[ 2 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 2, vm );
	v3[ 0 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 0, vm );
	v3[ 1 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 1, vm );
	v3[ 2 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 2, vm );
	if( vm->t_enabled )
	{
	    pix_vm_gfx_vertex_transform( v1, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	    pix_vm_gfx_vertex_transform( v2, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	    pix_vm_gfx_vertex_transform( v3, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	}
	zz[ t ] = ( v1[ 2 ] + v2[ 2 ] + v3[ 2 ] ) / 3;
    }
    while( 1 )
    {
	bool sort = 0;
	for( size_t t = 0; t < tnum - 1; t++ )
	{
	    if( zz[ t ] > zz[ t + 1 ] )
	    {
		PIX_FLOAT tz = zz[ t ];
		zz[ t ] = zz[ t + 1 ];
		zz[ t + 1 ] = tz;
		int to = order[ t ];
		order[ t ] = order[ t + 1 ];
		order[ t + 1 ] = to;
		sort = 1;
	    }
	}
	if( sort == 0 ) break;
    }
    for( size_t t = 0; t < tnum; t++ )
    {
	pix_vm_set_container_int_element( triangles, t * 8 + 7, order[ t ], vm );
    }
    bmem_free( zz );
    bmem_free( order );
}

void fn_set_key_color( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    bool key_color = 0;
    PIX_INT key;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    if( pars_num > 1 )
    {
	key_color = 1;
	GET_VAL_FROM_STACK( key, 1, PIX_INT );
    }
    
    if( key_color )
    {
	pix_vm_set_container_key_color( cnum, key, vm );
    }
    else 
    {
	pix_vm_remove_container_key_color( cnum, vm );
    }
}

void fn_get_key_color( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = pix_vm_get_container_key_color( cnum, vm );
}

void fn_set_alpha( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    PIX_CID alpha_cnum = -1;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    if( pars_num > 1 )
    {
	GET_VAL_FROM_STACK( alpha_cnum, 1, PIX_CID );
    }
    
    pix_vm_set_container_alpha( cnum, alpha_cnum, vm );
}

void fn_get_alpha( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = pix_vm_get_container_alpha( cnum, vm );
}

void fn_print( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    PIX_FLOAT x = 0, y = 0;
    COLOR c = get_color( 255, 255, 255 );
    int align = 0;
    int max_xsize = 0;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    while( 1 )
    {
	if( pars_num > 1 ) { GET_VAL_FROM_STACK( x, 1, PIX_FLOAT ); } else break;
	if( pars_num > 2 ) { GET_VAL_FROM_STACK( y, 2, PIX_FLOAT ); } else break;
	if( pars_num > 3 ) { GET_VAL_FROM_STACK( c, 3, COLOR ); } else break;
	if( pars_num > 4 ) { GET_VAL_FROM_STACK( align, 4, int ); } else break;
	if( pars_num > 5 ) { GET_VAL_FROM_STACK( max_xsize, 5, int ); } else break;
	break;
    }
    
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* cont = vm->c[ cnum ];
	if( cont && cont->data && cont->size )
	{
	    size_t real_size = cont->size * g_pix_container_type_sizes[ cont->type ];
	    pix_vm_gfx_draw_text( (utf8_char*)cont->data, real_size, x, y, 0, align, c, max_xsize, 0, 0, 0, vm );
	}
    }
}

void fn_get_text_xsize( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    int align = 0;
    int max_xsize = 0;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    if( pars_num > 1 ) GET_VAL_FROM_STACK( align, 1, int );
    if( pars_num > 2 ) GET_VAL_FROM_STACK( max_xsize, 2, int );
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = 0;
    
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* cont = vm->c[ cnum ];
	if( cont && cont->data && cont->size )
	{
	    size_t real_size = cont->size * g_pix_container_type_sizes[ cont->type ];
	    int xsize;
	    pix_vm_gfx_draw_text( (utf8_char*)cont->data, real_size, 0, 0, 0, align, 1, max_xsize, &xsize, 0, 1, vm );
	    stack[ sp + ( pars_num - 1 ) ].i = xsize;
	}
    }
}

void fn_get_text_ysize( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    int align = 0;
    int max_xsize = 0;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    if( pars_num > 1 ) GET_VAL_FROM_STACK( align, 1, int );
    if( pars_num > 2 ) GET_VAL_FROM_STACK( max_xsize, 2, int );
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = 0;
    
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* cont = vm->c[ cnum ];
	if( cont && cont->data && cont->size )
	{
	    size_t real_size = cont->size * g_pix_container_type_sizes[ cont->type ];
	    int ysize;
	    pix_vm_gfx_draw_text( (utf8_char*)cont->data, real_size, 0, 0, 0, align, 1, max_xsize, 0, &ysize, 1, vm );
	    stack[ sp + ( pars_num - 1 ) ].i = ysize;
	}
    }
}

void fn_set_font( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num < 2 ) return;
    
    utf32_char first_char;
    PIX_CID cnum;
    int xchars = 0;
    int ychars = 0;
    
    GET_VAL_FROM_STACK( first_char, 0, utf32_char );
    GET_VAL_FROM_STACK( cnum, 1, PIX_CID );
    if( pars_num >= 3 )
	GET_VAL_FROM_STACK( xchars, 2, int );
    if( pars_num >= 4 )
	GET_VAL_FROM_STACK( ychars, 3, int );
	
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)pix_vm_set_font( first_char, cnum, xchars, ychars, vm );
}

void fn_get_font( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    utf32_char char_code;
    GET_VAL_FROM_STACK( char_code, 0, utf32_char );
    
    pix_vm_font* font = pix_vm_get_font_for_char( char_code, vm );

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    if( font )
	stack[ sp + ( pars_num - 1 ) ].i = font->font;
    else
	stack[ sp + ( pars_num - 1 ) ].i = -1;
}

void fn_effector( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 2 ) return;
    
    int transp = vm->transp;
    if( transp == 0 ) return;
    
    int type;
    int power;
    COLOR color = get_color( 255, 255, 255 );
    int sx = -vm->screen_xsize / 2;
    int sy = -vm->screen_ysize / 2;
    int xsize = vm->screen_xsize;
    int ysize = vm->screen_ysize;
    int x_step = 1;
    int y_step = 1;
    
    int pnum = 0;
    GET_VAL_FROM_STACK( type, 0, int );
    GET_VAL_FROM_STACK( power, 1, int );
    pnum = 2;
    while( 1 )
    {
	if( pars_num > pnum ) { GET_VAL_FROM_STACK( color, pnum, COLOR ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( sx, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( sy, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( xsize, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( ysize, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( x_step, pnum, int ); } else break; pnum++;
	if( pars_num > pnum ) { GET_VAL_FROM_STACK( y_step, pnum, int ); } else break; pnum++;
	break;
    }

    sx += vm->screen_xsize / 2;
    sy += vm->screen_ysize / 2;
    
    if( sx + xsize < 0 ) return;
    if( sy + ysize < 0 ) return;
    if( sx < 0 ) { xsize -= -sx; sx = 0; }
    if( sy < 0 ) { ysize -= -sy; sy = 0; }
    if( sx + xsize > vm->screen_xsize ) xsize = vm->screen_xsize - sx;
    if( sy + ysize > vm->screen_ysize ) ysize = vm->screen_ysize - sy;
    if( xsize <= 0 ) return;
    if( ysize <= 0 ) return;
    if( x_step <= 0 ) x_step = 1;
    if( y_step <= 0 ) y_step = 1;

    int* cols_r = 0;
    int* cols_g = 0;
    int* cols_b = 0;
    switch( type )
    {
	case PIX_EFFECT_HBLUR:
	case PIX_EFFECT_VBLUR:
	    if( vm->effector_colors_r == 0 ) vm->effector_colors_r = (int*)bmem_new( 512 * sizeof( int ) );
	    if( vm->effector_colors_g == 0 ) vm->effector_colors_g = (int*)bmem_new( 512 * sizeof( int ) );
	    if( vm->effector_colors_b == 0 ) vm->effector_colors_b = (int*)bmem_new( 512 * sizeof( int ) );
	    cols_r = vm->effector_colors_r;
	    cols_g = vm->effector_colors_g;
	    cols_b = vm->effector_colors_b;
	    if( cols_r == 0 ) return;
	    if( cols_g == 0 ) return;
	    if( cols_b == 0 ) return;
	    break;
    }
    
    switch( type )
    {
	case PIX_EFFECT_NOISE:
	    {
		transp *= power;
		transp /= 256;
		if( transp <= 0 ) break;
		for( int cy = 0; cy < ysize; cy += y_step )
		{
		    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize + sx;
		    for( int cx = 0; cx < xsize; cx += x_step )
		    {
			*scr = blend( *scr, color, ( transp * ( pseudo_random() & 255 ) ) / 256 );
			scr += x_step;
		    }
		}
	    }
	    break;
	case PIX_EFFECT_SPREAD_LEFT:
	    {
		power /= 2;
		if( power <= 0 ) break;
		power++;
		for( int cy = 0; cy < ysize; cy += y_step )
		{
		    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize;
		    for( int cx = sx; cx < sx + xsize; cx += x_step )
		    {
		        int cx2;
		        int rnd = ( ( pseudo_random() & 4095 ) * power ) / 4096;
		        cx2 = cx + rnd;
		        if( cx2 >= vm->screen_xsize ) cx2 = vm->screen_xsize - 1;
		        scr[ cx ] = blend( scr[ cx ], scr[ cx2 ], transp );
		    }
		}
	    }
	    break;
	case PIX_EFFECT_SPREAD_RIGHT:
	    {
		power /= 2;
		if( power <= 0 ) break;
		power++;
		for( int cy = 0; cy < ysize; cy += y_step )
		{
		    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize;
		    for( int cx = sx + ( ( xsize - 1 ) / x_step ) * x_step; cx >= sx; cx -= x_step )
		    {
		        int cx2;
		        int rnd = ( ( pseudo_random() & 4095 ) * power ) / 4096;
		        cx2 = cx - rnd;
		        if( cx2 < 0 ) cx2 = 0;
		        scr[ cx ] = blend( scr[ cx ], scr[ cx2 ], transp );
		    }
		}
	    }
	    break;
	case PIX_EFFECT_SPREAD_UP:
	    {
		power /= 2;
		if( power <= 0 ) break;
		power++;
		for( int cx = 0; cx < xsize; cx += x_step )
		{
		    COLORPTR scr = vm->screen_ptr + sx + cx;
		    for( int cy = sy; cy < sy + ysize; cy += y_step )
		    {
		        int cy2;
		        int rnd = ( ( pseudo_random() & 4095 ) * power ) / 4096;
		        cy2 = cy + rnd;
		        if( cy2 >= vm->screen_ysize ) cy2 = vm->screen_ysize - 1;
		        COLORPTR s = &scr[ cy * vm->screen_xsize ];
		        *s = blend( *s, scr[ cy2 * vm->screen_xsize ], transp );
		    }
		}
	    }
	    break;
	case PIX_EFFECT_SPREAD_DOWN:
	    {
		power /= 2;
		if( power <= 0 ) break;
		power++;
		for( int cx = 0; cx < xsize; cx += x_step )
		{
		    COLORPTR scr = vm->screen_ptr + sx + cx;
		    for( int cy = sy + ( ( ysize - 1 ) / y_step ) * y_step; cy >= sy; cy -= y_step )
		    {
		        int cy2;
		        int rnd = ( ( pseudo_random() & 4095 ) * power ) / 4096;
		        cy2 = cy - rnd;
		        if( cy2 < 0 ) cy2 = 0;
		        COLORPTR s = &scr[ cy * vm->screen_xsize ];
		        *s = blend( *s, scr[ cy2 * vm->screen_xsize ], transp );
		    }
		}
	    }
	    break;
	case PIX_EFFECT_HBLUR:
	    {
		power /= 2;
		if( power > 255 ) power = 255;
		if( power <= 0 ) break;
		for( int cy = 0; cy < ysize; cy += y_step )
		{
		    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize + sx;
		    int col_ptr = 0;
		    COLOR pcol = *scr;
		    int start_r = red( pcol );
		    int start_g = green( pcol );
		    int start_b = blue( pcol );
		    cols_r[ 0 ] = 0;
		    cols_g[ 0 ] = 0;
		    cols_b[ 0 ] = 0;
		    for( int i = 1; i < power + 1; i++ ) 
		    { 
			cols_r[ i ] = start_r * i; 
			cols_g[ i ] = start_g * i; 
			cols_b[ i ] = start_b * i; 
		    }
		    col_ptr = power + 1;
		    for( int i = 0; ( i < power ) && ( i < xsize ); i++ ) 
		    {
			pcol = scr[ i ];
			cols_r[ col_ptr ] = cols_r[ col_ptr - 1 ] + red( pcol );
			cols_g[ col_ptr ] = cols_g[ col_ptr - 1 ] + green( pcol );
			cols_b[ col_ptr ] = cols_b[ col_ptr - 1 ] + blue( pcol );
			col_ptr++;
		    }
		    if( power > xsize )
		    {
			pcol = scr[ xsize - 1 ];
			int rr = red( pcol );
			int gg = green( pcol );
			int bb = blue( pcol );
			for( int i = 0; i < power - xsize; i++ ) 
			{
			    cols_r[ col_ptr ] = cols_r[ col_ptr - 1 ] + rr;
			    cols_g[ col_ptr ] = cols_g[ col_ptr - 1 ] + gg;
			    cols_b[ col_ptr ] = cols_b[ col_ptr - 1 ] + bb;
			    col_ptr++;
			}
		    }
		    for( int cx = 0; cx < xsize; cx += x_step )
		    {
			int prev_ptr = ( col_ptr - 1 ) & 511;
			int new_ptr = cx + power; if( new_ptr > xsize - 1 ) new_ptr = xsize - 1;
			pcol = scr[ new_ptr ];
			cols_r[ col_ptr ] = cols_r[ prev_ptr ] + red( pcol );
			cols_g[ col_ptr ] = cols_g[ prev_ptr ] + green( pcol );
			cols_b[ col_ptr ] = cols_b[ prev_ptr ] + blue( pcol );
			COLOR res = get_color(
					      ( cols_r[ col_ptr ] - cols_r[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 ),
					      ( cols_g[ col_ptr ] - cols_g[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 ),
					      ( cols_b[ col_ptr ] - cols_b[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 )
					      );
			scr[ cx ] = fast_blend( scr[ cx ], res, transp );
			col_ptr ++;
			col_ptr &= 511;
		    }
		}
	    }
	    break;
	case PIX_EFFECT_VBLUR:
	    {
		power /= 2;
		if( power > 255 ) power = 255;
		if( power <= 0 ) break;
		for( int cx = 0; cx < xsize; cx += x_step )
		{
		    COLORPTR scr = vm->screen_ptr + sy * vm->screen_xsize + sx + cx;
		    int col_ptr = 0;
		    COLOR pcol = *scr;
		    int start_r = red( pcol );
		    int start_g = green( pcol );
		    int start_b = blue( pcol );
		    cols_r[ 0 ] = 0;
		    cols_g[ 0 ] = 0;
		    cols_b[ 0 ] = 0;
		    int i;
		    for( i = 1; i < power + 1; i++ ) 
		    { 
			cols_r[ i ] = start_r * i; 
			cols_g[ i ] = start_g * i; 
			cols_b[ i ] = start_b * i; 
		    }
		    col_ptr = power + 1;
		    for( int i = 0, i2 = 0; ( i < power ) && ( i < ysize ); i++ ) 
		    {
			pcol = scr[ i2 ];
			cols_r[ col_ptr ] = cols_r[ col_ptr - 1 ] + red( pcol );
			cols_g[ col_ptr ] = cols_g[ col_ptr - 1 ] + green( pcol );
			cols_b[ col_ptr ] = cols_b[ col_ptr - 1 ] + blue( pcol );
			i2 += vm->screen_xsize;
			col_ptr++;
		    }
		    if( power > ysize )
		    {
			pcol = scr[ ( ysize - 1 ) * vm->screen_xsize ];
			int rr = red( pcol );
			int gg = green( pcol );
			int bb = blue( pcol );
			for( int i = 0; i < power - ysize; i++ )
			{ 
			    cols_r[ col_ptr ] = cols_r[ col_ptr - 1 ] + rr;
			    cols_g[ col_ptr ] = cols_g[ col_ptr - 1 ] + gg;
			    cols_b[ col_ptr ] = cols_b[ col_ptr - 1 ] + bb;
			    col_ptr++;
			}
		    }
		    int power2 = power * vm->screen_xsize;
		    int end_ptr = ( ysize - 1 ) * vm->screen_xsize;
		    for( int cy = 0; cy < ysize * vm->screen_xsize; cy += y_step * vm->screen_xsize )
		    {
			int prev_ptr = ( col_ptr - 1 ) & 511;
			int new_ptr = cy + power2; if( new_ptr > end_ptr ) new_ptr = end_ptr;
			pcol = scr[ new_ptr ];
			cols_r[ col_ptr ] = cols_r[ prev_ptr ] + red( pcol );
			cols_g[ col_ptr ] = cols_g[ prev_ptr ] + green( pcol );
			cols_b[ col_ptr ] = cols_b[ prev_ptr ] + blue( pcol );
			COLOR res = get_color(
					      ( cols_r[ col_ptr ] - cols_r[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 ),
					      ( cols_g[ col_ptr ] - cols_g[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 ),
					      ( cols_b[ col_ptr ] - cols_b[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 )
					      );
			scr[ cy ] = fast_blend( scr[ cy ], res, transp );
			col_ptr ++;
			col_ptr &= 511;
		    }
		}
	    }
	    break;
	case PIX_EFFECT_COLOR:
	    {
		transp *= power;
		transp /= 256;
		if( transp <= 0 ) break;
		if( transp > 255 ) transp = 255;
		for( int cy = 0; cy < ysize; cy += y_step )
		{
		    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize + sx;
		    for( int cx = 0; cx < xsize; cx += x_step )
		    {
			*scr = blend( *scr, color, transp );
			scr += x_step;
		    }
		}
	    }
    }
}

void fn_color_gradient( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 8 ) return;
    
    int transp = vm->transp;
    if( transp == 0 ) return;

    COLOR c[ 4 ];
    int t[ 4 ];
    int sx = -vm->screen_xsize / 2;
    int sy = -vm->screen_ysize / 2;
    int xsize = vm->screen_xsize;
    int ysize = vm->screen_ysize;
    int x_step = 1;
    int y_step = 1;

    bool zero_transp = 1;
    bool no_transp = 1;
    for( int i = 0; i < 4; i++ )
    {
	GET_VAL_FROM_STACK( c[ i ], i * 2, COLOR );
	GET_VAL_FROM_STACK( t[ i ], i * 2 + 1, int );
	if( transp < 255 ) t[ i ] = ( t[ i ] * transp ) / 255;
	if( t[ i ] < 0 ) t[ i ] = 0;
	if( t[ i ] > 255 ) t[ i ] = 255;
	if( t[ i ] != 0 ) zero_transp = 0;
	if( t[ i ] != 255 ) no_transp = 0;
    }
    if( zero_transp ) return;
    int pnum = 8;
    while( 1 )
    {
	if( pars_num > pnum ) { GET_VAL_FROM_STACK( sx, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( sy, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( xsize, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( ysize, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( x_step, pnum, int ); } else break; pnum++;
	if( pars_num > pnum ) { GET_VAL_FROM_STACK( y_step, pnum, int ); } else break; pnum++;
	break;
    }

    sx += vm->screen_xsize / 2;
    sy += vm->screen_ysize / 2;

    if( sx + xsize < 0 ) return;
    if( sy + ysize < 0 ) return;
    if( sx >= vm->screen_xsize ) return;
    if( sy >= vm->screen_ysize ) return;
    if( xsize <= 0 ) return;
    if( ysize <= 0 ) return;
    if( x_step <= 0 ) x_step = 1;
    if( y_step <= 0 ) y_step = 1;

    int xd = ( 256 << 8 ) / xsize;
    int xstart = 0;
    int yd = ( 256 << 8 ) / ysize;
    int ystart = 0;

    if( sx < 0 ) { xsize -= -sx; xstart = -sx * xd; sx = 0; }
    if( sy < 0 ) { ysize -= -sy; ystart = -sy * yd; sy = 0; }
    if( sx + xsize > vm->screen_xsize ) xsize = vm->screen_xsize - sx;
    if( sy + ysize > vm->screen_ysize ) ysize = vm->screen_ysize - sy;
    
    xd *= x_step;
    yd *= y_step;

    int yy = ystart;
    if( no_transp )
    {
	for( int cy = 0; cy < ysize; cy += y_step )
	{
	    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize + sx;
	    int xx = xstart;
	    int v = yy >> 8;
	    COLOR c1 = blend( c[ 0 ], c[ 2 ], v );
	    COLOR c2 = blend( c[ 1 ], c[ 3 ], v );
	    for( int cx = 0; cx < xsize; cx += x_step )
	    {
		*scr = blend( c1, c2, xx >> 8 );
		scr += x_step;
		xx += xd;
	    }
	    yy += yd;
	}
    }
    else
    {
	for( int cy = 0; cy < ysize; cy += y_step )
	{
	    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize + sx;
	    int xx = xstart;
	    int v = yy >> 8;
	    int vv = 256 - v;
	    COLOR c1 = blend( c[ 0 ], c[ 2 ], v );
	    COLOR c2 = blend( c[ 1 ], c[ 3 ], v );
	    int t1 = ( t[ 0 ] * vv + t[ 2 ] * v ) >> 8;
	    int t2 = ( t[ 1 ] * vv + t[ 3 ] * v ) >> 8;
	    for( int cx = 0; cx < xsize; cx += x_step )
	    {
		v = xx >> 8;
		vv = 256 - v;
		int t = ( t1 * vv + t2 * v ) >> 8;
		COLOR pixel = blend( c1, c2, v );
		*scr = blend( *scr, pixel, t );
		scr += x_step;
		xx += xd;
	    }
	    yy += yd;
	}
    }
}

const int YR = 19595, YG = 38470, YB = 7471, CB_R = -11059, CB_G = -21709, CB_B = 32768, CR_R = 32768, CR_G = -27439, CR_B = -5329;
inline uchar clamp( int i ) { if( i < 0 ) i = 0; else if( i > 255 ) i = 255; return (uchar)i; }
inline float clamp_f( float i ) { if( i < 0 ) i = 0; else if( i > 1 ) i = 1; return i; }
#define SPLIT_PRECISION 9
#define SPLIT_CONV( num ) (int)( (float)num * (float)( 1 << SPLIT_PRECISION ) )
#define SPLIT_AMUL( num ) ( num >> SPLIT_PRECISION )

void fn_split_rgb( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 3 ) return;
    
    int rv = -1;

    int direction; //0 - from image to RGB; 1 - from RGB to image;   
    PIX_CID image;
    PIX_CID channels[ 3 ] = { -1, -1, -1 };
    size_t image_offset = 0;
    size_t channel_offset = 0;
    size_t size = -1;
    
    const utf8_char* fn_name;
    bool rgb;
    if( fn_num == FN_SPLIT_RGB )
    {
	fn_name = "split_rgb";
	rgb = 1;
    }
    else
    {
	fn_name = "split_ycbcr";
	rgb = 0;
    }
    
    GET_VAL_FROM_STACK( direction, 0, int );
    GET_VAL_FROM_STACK( image, 1, PIX_CID );
    GET_VAL_FROM_STACK( channels[ 0 ], 2, PIX_CID );
    while( 1 )
    {
	if( pars_num > 3 ) { GET_VAL_FROM_STACK( channels[ 1 ], 3, PIX_CID ); } else break;
	if( pars_num > 4 ) { GET_VAL_FROM_STACK( channels[ 2 ], 4, PIX_CID ); } else break;
	if( pars_num > 5 ) { GET_VAL_FROM_STACK( image_offset, 5, size_t ); } else break;
	if( pars_num > 6 ) { GET_VAL_FROM_STACK( channel_offset, 6, size_t ); } else break;
	if( pars_num > 7 ) { GET_VAL_FROM_STACK( size, 7, size_t ); } else break;
	break;
    }
    
    while( 1 )
    {
	pix_vm_container* image_cont = pix_vm_get_container( image, vm );
	if( image_cont == 0 ) break;
	if( g_pix_container_type_sizes[ image_cont->type ] != COLORLEN )
	{
	    PIX_VM_LOG( "%s: image container must be of PIXEL type\n", fn_name );
	    break;
	}
	pix_vm_container* channels_cont[ 3 ];
	int channel_type = -1;
	for( int i = 0; i < 3; i++ )
	{
	    channels_cont[ i ] = pix_vm_get_container( channels[ i ], vm );
	    if( channels_cont[ i ] )
	    {
		if( channel_type == -1 ) 
		    channel_type = channels_cont[ i ]->type;
		else
		{
		    if( channels_cont[ i ]->type != channel_type )
		    {
			PIX_VM_LOG( "%s: all channels must have the same type\n", fn_name );
			channel_type = -1;
			break;
		    }
		}
	    }
	}
	if( channel_type == -1 )
	{
	    PIX_VM_LOG( "%s: can't split the image (no channels selected)\n", fn_name );
	    break;
	}
	
	if( size == -1 ) size = image_cont->size;
	if( image_offset >= image_cont->size ) break;
	if( image_offset + size > image_cont->size )
	{
	    size = image_cont->size - image_offset;
	}
	
	COLORPTR image_ptr = (COLORPTR)image_cont->data + image_offset;
	void* ch[ 3 ];
	for( int i = 0; i < 3; i++ )
	{
	    if( channels_cont[ i ] && channels_cont[ i ]->data ) 
	    {
		if( channel_offset >= channels_cont[ i ]->size ) size = 0;
		if( channel_offset + size > channels_cont[ i ]->size )
		{
		    size = channels_cont[ i ]->size - channel_offset;
		}
		ch[ i ] = (void*)( (uchar*)channels_cont[ i ]->data + channel_offset * g_pix_container_type_sizes[ channel_type ] );
	    }
	    else ch[ i ] = 0;
	}
	if( direction == 0 )
	{
	    //From image to RGB / YCbCr:
	    switch( channel_type )
	    {
		case PIX_CONTAINER_TYPE_INT8:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((uchar*)ch[ 0 ])[ p ] = red( pixel );
				if( ch[ 1 ] ) ((uchar*)ch[ 1 ])[ p ] = green( pixel );
				if( ch[ 2 ] ) ((uchar*)ch[ 2 ])[ p ] = blue( pixel );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				COLOR pixel = image_ptr[ p ];
				int r = red( pixel );
				int g = green( pixel );
				int b = blue( pixel );
				if( ch[ 0 ] ) ((uchar*)ch[ 0 ])[ p ] = (uchar)( ( r * YR + g * YG + b * YB + 32768 ) >> 16 );
				if( ch[ 1 ] ) ((uchar*)ch[ 1 ])[ p ] = clamp( 128 + ( ( r * CB_R + g * CB_G + b * CB_B + 32768 ) >> 16 ) );
				if( ch[ 2 ] ) ((uchar*)ch[ 2 ])[ p ] = clamp( 128 + ( ( r * CR_R + g * CR_G + b * CR_B + 32768 ) >> 16 ) );
			    }
			rv = 0;
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT16:
		    {
			if( rgb )			
			    for( size_t p = 0; p < size; p++ )
			    {
				COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((int16*)ch[ 0 ])[ p ] = red( pixel );
				if( ch[ 1 ] ) ((int16*)ch[ 1 ])[ p ] = green( pixel );
				if( ch[ 2 ] ) ((int16*)ch[ 2 ])[ p ] = blue( pixel );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				COLOR pixel = image_ptr[ p ];
				int r = red( pixel );
				int g = green( pixel );
				int b = blue( pixel );
				if( ch[ 0 ] ) ((uint16*)ch[ 0 ])[ p ] = (uint16)( ( r * YR + g * YG + b * YB + 32768 ) >> 16 );
				if( ch[ 1 ] ) ((uint16*)ch[ 1 ])[ p ] = clamp( 128 + ( ( r * CB_R + g * CB_G + b * CB_B + 32768 ) >> 16 ) );
				if( ch[ 2 ] ) ((uint16*)ch[ 2 ])[ p ] = clamp( 128 + ( ( r * CR_R + g * CR_G + b * CR_B + 32768 ) >> 16 ) );
			    }
			rv = 0;
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT32:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((int*)ch[ 0 ])[ p ] = red( pixel );
				if( ch[ 1 ] ) ((int*)ch[ 1 ])[ p ] = green( pixel );
				if( ch[ 2 ] ) ((int*)ch[ 2 ])[ p ] = blue( pixel );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				int r = red( pixel );
				int g = green( pixel );
				int b = blue( pixel );
				if( ch[ 0 ] ) ((int*)ch[ 0 ])[ p ] = ( r * YR + g * YG + b * YB + 32768 ) >> 16;
				if( ch[ 1 ] ) ((int*)ch[ 1 ])[ p ] = clamp( 128 + ( ( r * CB_R + g * CB_G + b * CB_B + 32768 ) >> 16 ) );
				if( ch[ 2 ] ) ((int*)ch[ 2 ])[ p ] = clamp( 128 + ( ( r * CR_R + g * CR_G + b * CR_B + 32768 ) >> 16 ) );
			    }
			rv = 0;
		    }
		    break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((int64*)ch[ 0 ])[ p ] = red( pixel );
				if( ch[ 1 ] ) ((int64*)ch[ 1 ])[ p ] = green( pixel );
				if( ch[ 2 ] ) ((int64*)ch[ 2 ])[ p ] = blue( pixel );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				int r = red( pixel );
				int g = green( pixel );
				int b = blue( pixel );
				if( ch[ 0 ] ) ((int64*)ch[ 0 ])[ p ] = ( r * YR + g * YG + b * YB + 32768 ) >> 16;
				if( ch[ 1 ] ) ((int64*)ch[ 1 ])[ p ] = clamp( 128 + ( ( r * CB_R + g * CB_G + b * CB_B + 32768 ) >> 16 ) );
				if( ch[ 2 ] ) ((int64*)ch[ 2 ])[ p ] = clamp( 128 + ( ( r * CR_R + g * CR_G + b * CR_B + 32768 ) >> 16 ) );
			    }
			rv = 0;
		    }
		    break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((float*)ch[ 0 ])[ p ] = (float)red( pixel ) / 255.0F;
				if( ch[ 1 ] ) ((float*)ch[ 1 ])[ p ] = (float)green( pixel ) / 255.0F;
				if( ch[ 2 ] ) ((float*)ch[ 2 ])[ p ] = (float)blue( pixel ) / 255.0F;
			    }
			else
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				float r = red( pixel ) / 255.0F;
				float g = green( pixel ) / 255.0F;
				float b = blue( pixel ) / 255.0F;
				if( ch[ 0 ] ) ((float*)ch[ 0 ])[ p ] = 0.299 * r + 0.587 * g + 0.114 * b;
				if( ch[ 1 ] ) ((float*)ch[ 1 ])[ p ] = - 0.168736 * r - 0.331264 * g + 0.5 * b;
				if( ch[ 2 ] ) ((float*)ch[ 2 ])[ p ] = 0.5 * r - 0.418688 * g - 0.081312 * b;
                    	    }			
			rv = 0;
		    }
		    break;
#ifdef PIX_FLOAT32_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((double*)ch[ 0 ])[ p ] = (float)red( pixel ) / 255.0F;
				if( ch[ 1 ] ) ((double*)ch[ 1 ])[ p ] = (float)green( pixel ) / 255.0F;
				if( ch[ 2 ] ) ((double*)ch[ 2 ])[ p ] = (float)blue( pixel ) / 255.0F;
			    }
			else
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				float r = red( pixel ) / 255.0F;
				float g = green( pixel ) / 255.0F;
				float b = blue( pixel ) / 255.0F;
				if( ch[ 0 ] ) ((double*)ch[ 0 ])[ p ] = 0.299 * r + 0.587 * g + 0.114 * b;
				if( ch[ 1 ] ) ((double*)ch[ 1 ])[ p ] = - 0.168736 * r - 0.331264 * g + 0.5 * b;
				if( ch[ 2 ] ) ((double*)ch[ 2 ])[ p ] = 0.5 * r - 0.418688 * g - 0.081312 * b;
                    	    }			
			rv = 0;
		    }
		    break;
#endif
	    }
	}
	else
	{
	    //From RGB / YCbCr to image:
	    switch( channel_type )
	    {
		case PIX_CONTAINER_TYPE_INT8:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				int r, g, b;
				if( ch[ 0 ] ) r = ((uchar*)ch[ 0 ])[ p ]; else r = 0;
				if( ch[ 1 ] ) g = ((uchar*)ch[ 1 ])[ p ]; else g = 0;
				if( ch[ 2 ] ) b = ((uchar*)ch[ 2 ])[ p ]; else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				int Y, Cb, Cr;
				int r, g, b;
				if( ch[ 0 ] ) Y = ((uchar*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((uchar*)ch[ 1 ])[ p ] - 128; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((uchar*)ch[ 2 ])[ p ] - 128; else Cr = 0;
				r = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.40200 ) * Cr ) );
    				g = clamp( Y - SPLIT_AMUL( SPLIT_CONV( 0.34414 ) * Cb ) - SPLIT_AMUL( SPLIT_CONV( 0.71414 ) * Cr ) );
    				b = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.77200 ) * Cb ) );
				image_ptr[ p ] = get_color( r, g, b );
			    }			
			rv = 0;
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT16:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				int r, g, b;
				if( ch[ 0 ] ) r = clamp( ((int16*)ch[ 0 ])[ p ] ); else r = 0;
				if( ch[ 1 ] ) g = clamp( ((int16*)ch[ 1 ])[ p ] ); else g = 0;
				if( ch[ 2 ] ) b = clamp( ((int16*)ch[ 2 ])[ p ] ); else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				int Y, Cb, Cr;
				int r, g, b;
				if( ch[ 0 ] ) Y = ((int16*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((int16*)ch[ 1 ])[ p ] - 128; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((int16*)ch[ 2 ])[ p ] - 128; else Cr = 0;
				r = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.40200 ) * Cr ) );
    				g = clamp( Y - SPLIT_AMUL( SPLIT_CONV( 0.34414 ) * Cb ) - SPLIT_AMUL( SPLIT_CONV( 0.71414 ) * Cr ) );
    				b = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.77200 ) * Cb ) );
				image_ptr[ p ] = get_color( r, g, b );
			    }			
			rv = 0;
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT32:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				int r, g, b;
				if( ch[ 0 ] ) r = clamp( ((int*)ch[ 0 ])[ p ] ); else r = 0;
				if( ch[ 1 ] ) g = clamp( ((int*)ch[ 1 ])[ p ] ); else g = 0;
				if( ch[ 2 ] ) b = clamp( ((int*)ch[ 2 ])[ p ] ); else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				int Y, Cb, Cr;
				int r, g, b;
				if( ch[ 0 ] ) Y = ((int*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((int*)ch[ 1 ])[ p ] - 128; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((int*)ch[ 2 ])[ p ] - 128; else Cr = 0;
				r = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.40200 ) * Cr ) );
    				g = clamp( Y - SPLIT_AMUL( SPLIT_CONV( 0.34414 ) * Cb ) - SPLIT_AMUL( SPLIT_CONV( 0.71414 ) * Cr ) );
    				b = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.77200 ) * Cb ) );
				image_ptr[ p ] = get_color( r, g, b );
			    }			
			rv = 0;
		    }
		    break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				int64 r, g, b;
				if( ch[ 0 ] ) r = clamp( ((int64*)ch[ 0 ])[ p ] ); else r = 0;
				if( ch[ 1 ] ) g = clamp( ((int64*)ch[ 1 ])[ p ] ); else g = 0;
				if( ch[ 2 ] ) b = clamp( ((int64*)ch[ 2 ])[ p ] ); else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				int Y, Cb, Cr;
				int r, g, b;
				if( ch[ 0 ] ) Y = ((int64*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((int64*)ch[ 1 ])[ p ] - 128; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((int64*)ch[ 2 ])[ p ] - 128; else Cr = 0;
				r = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.40200 ) * Cr ) );
    				g = clamp( Y - SPLIT_AMUL( SPLIT_CONV( 0.34414 ) * Cb ) - SPLIT_AMUL( SPLIT_CONV( 0.71414 ) * Cr ) );
    				b = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.77200 ) * Cb ) );
				image_ptr[ p ] = get_color( r, g, b );
			    }			
			rv = 0;
		    }
		    break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32:
		    {
			if( rgb )			
			    for( size_t p = 0; p < size; p++ )
			    {
				int r, g, b;
				if( ch[ 0 ] ) r = clamp( ((float*)ch[ 0 ])[ p ] * 255 ); else r = 0;
				if( ch[ 1 ] ) g = clamp( ((float*)ch[ 1 ])[ p ] * 255 ); else g = 0;
				if( ch[ 2 ] ) b = clamp( ((float*)ch[ 2 ])[ p ] * 255 ); else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				float Y, Cb, Cr;
				float r, g, b;
				if( ch[ 0 ] ) Y = ((float*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((float*)ch[ 1 ])[ p ]; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((float*)ch[ 2 ])[ p ]; else Cr = 0;
				r = clamp_f( Y + 1.40200 * Cr );
    				g = clamp_f( Y - 0.34414 * Cb - 0.71414 * Cr );
    				b = clamp_f( Y + 1.77200 * Cb );
				image_ptr[ p ] = get_color( r * 255, g * 255, b * 255 );
			    }						
			rv = 0;
		    }
		    break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				int r, g, b;
				if( ch[ 0 ] ) r = clamp( ((double*)ch[ 0 ])[ p ] * 255 ); else r = 0;
				if( ch[ 1 ] ) g = clamp( ((double*)ch[ 1 ])[ p ] * 255 ); else g = 0;
				if( ch[ 2 ] ) b = clamp( ((double*)ch[ 2 ])[ p ] * 255 ); else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				float Y, Cb, Cr;
				float r, g, b;
				if( ch[ 0 ] ) Y = ((double*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((double*)ch[ 1 ])[ p ]; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((double*)ch[ 2 ])[ p ]; else Cr = 0;
				r = clamp_f( Y + 1.40200 * Cr );
    				g = clamp_f( Y - 0.34414 * Cb - 0.71414 * Cr );
    				b = clamp_f( Y + 1.77200 * Cb );
				image_ptr[ p ] = get_color( r * 255, g * 255, b * 255 );
			    }						
			rv = 0;
		    }
		    break;
#endif
	    }
	}
	
	break;
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

//
// OpenGL graphics
//

void fn_set_gl_callback( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
#ifdef OPENGL
    if( pars_num > 0 )
    {
	gl_lock( vm->wm );
	
	PIX_ADDR callback;
	PIX_VAL userdata;
	userdata.i = 0;
	char userdata_type = 0;
	GET_VAL_FROM_STACK( callback, 0, PIX_ADDR );
	if( callback == -1 || IS_ADDRESS_CORRECT( callback ) )
	{
	    if( callback != -1 )
		callback &= PIX_INT_ADDRESS_MASK;
	    if( pars_num > 1 ) 
	    {
		userdata = stack[ sp + 1 ];
		userdata_type = stack_types[ sp + 1 ];
	    }
	    vm->gl_userdata = userdata;
	    vm->gl_userdata_type = userdata_type;
	    vm->gl_callback = callback;
	    stack[ sp + ( pars_num - 1 ) ].i = 0;
	    stack_types[ sp + ( pars_num - 1 ) ] = 0;
	}
	else
	{
	    stack[ sp + ( pars_num - 1 ) ].i = -1;
	    stack_types[ sp + ( pars_num - 1 ) ] = 0;
	    PIX_VM_LOG( "set_gl_callback() error: wrong callback address %d\n", (int)callback );
	}
	
	gl_unlock( vm->wm );
    }
#else
    stack[ sp + ( pars_num - 1 ) ].i = -1;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    PIX_VM_LOG( "set_gl_callback() error: no OpenGL. Please use the special Pixilang version with OpenGL support.\n" );
#endif
}

void fn_remove_gl_data( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
#ifdef OPENGL
    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_remove_container_gl_data( cnum, vm );
    }
#endif
}

void fn_gl_draw_arrays( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;
    
    GLenum mode;
    GLint first;
    GLsizei count;
    int r, g, b, a;
    PIX_CID texture;
    PIX_CID vertex_array;
    PIX_CID color_array = -1;
    PIX_CID texcoord_array = -1;
    
    if( pars_num >= 9 )
    {
	GET_VAL_FROM_STACK( mode, 0, GLenum );
	GET_VAL_FROM_STACK( first, 1, GLint );
	GET_VAL_FROM_STACK( count, 2, GLsizei );
	GET_VAL_FROM_STACK( r, 3, int );
	GET_VAL_FROM_STACK( g, 4, int );
	GET_VAL_FROM_STACK( b, 5, int );
	GET_VAL_FROM_STACK( a, 6, int );
	GET_VAL_FROM_STACK( texture, 7, PIX_CID );
	GET_VAL_FROM_STACK( vertex_array, 8, PIX_CID );
	if( pars_num >= 10 ) GET_VAL_FROM_STACK( color_array, 9, PIX_CID );
	if( pars_num >= 11 ) GET_VAL_FROM_STACK( texcoord_array, 10, PIX_CID );
    }

    pix_vm_container* vertex_array_cont = pix_vm_get_container( vertex_array, vm );
    GLenum vertex_type;
    if( vertex_array_cont )
    {
	switch( vertex_array_cont->type )
	{
	    case PIX_CONTAINER_TYPE_INT8: vertex_type = GL_BYTE; break;
	    case PIX_CONTAINER_TYPE_INT16: vertex_type = GL_SHORT; break;
	    case PIX_CONTAINER_TYPE_FLOAT32: vertex_type = GL_FLOAT; break;
	    default:
		PIX_VM_LOG( "gl_draw_arrays() error: vertex_array can be INT8, INT16 or FLOAT32 only\n" );
		return;
		break;
	}
    }
    else return;

    pix_vm_container* color_array_cont = pix_vm_get_container( color_array, vm );
    GLenum color_type;
    bool color_normalize = false;
    if( color_array_cont )
    {
	switch( color_array_cont->type )
	{
	    case PIX_CONTAINER_TYPE_INT8: color_type = GL_UNSIGNED_BYTE; color_normalize = true; break;
	    case PIX_CONTAINER_TYPE_FLOAT32: color_type = GL_FLOAT; break;
	    default:
		PIX_VM_LOG( "gl_draw_arrays() error: color_array can be INT8 or FLOAT32 only\n" );
		color_array = -1;
		break;
	}
    }
    else color_array = -1;

    pix_vm_container_gl_data* texture_gl = 0;
    pix_vm_container* texcoord_array_cont = 0;
    GLenum texcoord_type;
    bool texcoord_normalize = false;
    if( texture >= 0 )
    {
	texture_gl = pix_vm_create_container_gl_data( texture, vm );
	texcoord_array_cont = pix_vm_get_container( texcoord_array, vm );
        if( texcoord_array_cont && texture_gl )
	{
	    switch( texcoord_array_cont->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: texcoord_type = GL_BYTE; texcoord_normalize = true; break;
		case PIX_CONTAINER_TYPE_INT16: texcoord_type = GL_SHORT; texcoord_normalize = true; break;
		case PIX_CONTAINER_TYPE_FLOAT32: texcoord_type = GL_FLOAT; break;
		default:
	    	    PIX_VM_LOG( "gl_draw_arrays() error: texcoord_array can be INT8, INT16 or FLOAT32 only\n" );
		    texture = -1;
		    break;
	    }
	}
	else texture = -1;
    }

    gl_program_struct* p;
    if( texture >= 0 )
    {
	if( texture_gl->texture_format == GL_ALPHA )
	{
	    if( color_array >= 0 )
    		p = vm->gl_prog_tex_alpha_gradient;
    	    else
    		p = vm->gl_prog_tex_alpha_solid;
    	}
        else
        {
	    if( color_array >= 0 )
		p = vm->gl_prog_tex_rgb_gradient;
	    else
		p = vm->gl_prog_tex_rgb_solid;
	}
    }
    else
    {
	if( color_array >= 0 )
    	    p = vm->gl_prog_gradient;
    	else
    	    p = vm->gl_prog_solid;
    }
    if( vm->gl_user_defined_prog ) p = vm->gl_user_defined_prog;
    if( vm->gl_current_prog != p )
    {
        pix_vm_gl_use_prog( p, vm );
        uint attr = 1 << GL_PROG_ATT_POSITION;
        if( texture >= 0 )
        {
    	    attr |= 1 << GL_PROG_ATT_TEX_COORD;
    	    glActiveTexture( GL_TEXTURE0 );
    	    glUniform1i( p->uniforms[ GL_PROG_UNI_TEXTURE ], 0 );
    	}
	if( color_array >= 0 )
	{
    	    attr |= 1 << GL_PROG_ATT_COLOR;
	}
	gl_enable_attributes( p, attr );
    }
    if( texture >= 0 )
    {
	glBindTexture( GL_TEXTURE_2D, texture_gl->texture_id );
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_TEX_COORD ], texcoord_array_cont->xsize, texcoord_type, texcoord_normalize, 0, texcoord_array_cont->data );
    }
    if( color_array >= 0 )
    {
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_COLOR ], color_array_cont->xsize, color_type, color_normalize, 0, color_array_cont->data );
    }
    else
    {
	if( r < 0 ) r = 0; else if( r > 255 ) r = 255;
	if( g < 0 ) g = 0; else if( g > 255 ) g = 255;
	if( b < 0 ) b = 0; else if( b > 255 ) b = 255;
	if( a < 0 ) a = 0; else if( a > 255 ) a = 255;
	glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)r / 255, (float)g / 255, (float)b / 255, (float)a / 255 );
    }
    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], vertex_array_cont->xsize, vertex_type, false, 0, vertex_array_cont->data );
    glDrawArrays( mode, first, count );

#endif //OPENGL
}

void fn_gl_blend_func( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;
    
#ifdef OPENGL
    if( pars_num < 2 )
    {
	//Set defaults:
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    }
    else
    {
	GLenum sfactor;
	GLenum dfactor;
	GET_VAL_FROM_STACK( sfactor, 0, GLenum );
	GET_VAL_FROM_STACK( dfactor, 1, GLenum );
	glBlendFunc( sfactor, dfactor );
    }
#endif
}

//Convert selected pixel container to the OpenGL framebuffer (with attached texture) and bind it.
//All rendering operations will be redirected to this framebuffer.
//To unbind - just call this function without parameters.
void fn_gl_bind_framebuffer( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;
    
    if( pars_num < 1 )
    {
	//Set defaults:
	gl_bind_framebuffer( 0, vm->wm );

	//Get previous viewport and WM transformation:
	gl_set_default_viewport( vm->wm );
	bmem_copy( vm->gl_wm_transform, vm->gl_wm_transform_prev, sizeof( vm->gl_wm_transform ) );
	pix_vm_gl_matrix_set( vm );
    }
    else
    {
	while( 1 )
	{
	    PIX_CID cnum;
	    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	    
	    pix_vm_container* c = pix_vm_get_container( cnum, vm );
	    if( c == 0 ) break;
	    c->flags |= PIX_CONTAINER_FLAG_GL_FRAMEBUFFER;
	    
	    pix_vm_container_gl_data* gl = pix_vm_create_container_gl_data( cnum, vm );
    	    if( gl == 0 ) break;

	    gl_bind_framebuffer( gl->framebuffer_id, vm->wm );

	    //Set new viewport and WM transformation:
	    bmem_copy( vm->gl_wm_transform_prev, vm->gl_wm_transform, sizeof( vm->gl_wm_transform ) );
	    glViewport( 0, 0, c->xsize, c->ysize );	    
	    matrix_4x4_reset( vm->gl_wm_transform );
	    matrix_4x4_ortho( -(float)c->xsize / 2 + 0.375, (float)c->xsize / 2 + 0.375, (float)c->ysize / 2 + 0.375, -(float)c->ysize / 2 + 0.375, -c->xsize * 10, c->xsize * 10, vm->gl_wm_transform );
	    pix_vm_gl_matrix_set( vm );

    	    break;
    	}
    }
#endif
}

void fn_gl_new_prog( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;
    
    PIX_CID rv = -1;

    if( pars_num >= 2 )
    {
	PIX_CID vshader;
	PIX_CID fshader;
	GET_VAL_FROM_STACK( vshader, 0, PIX_CID );
	GET_VAL_FROM_STACK( fshader, 1, PIX_CID );

	bool vshader_m = false;	
	bool fshader_m = false;	
	utf8_char* vshader_str = 0;
	utf8_char* fshader_str = 0;
	utf8_char ts1[ 2 ] = { 0, 0 };
	utf8_char ts2[ 2 ] = { 0, 0 };
	if( vshader < 0 )
	{
	    vshader = -vshader - 1;
	    if( vshader < GL_SHADER_MAX )
	    {
		ts1[ 0 ] = '0' + vshader;
		vshader_str = ts1;
	    }
	}
	else
	{	
	    vshader_str = pix_vm_make_cstring_from_container( vshader, &vshader_m, vm );
	}
	if( fshader < 0 )
	{
	    fshader = -fshader - 1;
	    if( fshader < GL_SHADER_MAX )
	    {
		ts2[ 0 ] = '0' + fshader;
		fshader_str = ts2;
	    }
	}
	else
	{	
	    fshader_str = pix_vm_make_cstring_from_container( fshader, &fshader_m, vm );
	}
	if( vshader_str && fshader_str )
	{
	    size_t len1 = bmem_strlen( vshader_str );
	    size_t len2 = bmem_strlen( fshader_str );
	    utf8_char* res = (utf8_char*)bmem_new( len1 + 1 + len2 + 1 );
	    bmem_copy( res, vshader_str, len1 + 1 );
	    bmem_copy( res + len1 + 1, fshader_str, len2 + 1 );
	    rv = pix_vm_new_container( -1, len1 + 1 + len2 + 1, 1, PIX_CONTAINER_TYPE_INT8, res, vm );
	    pix_vm_mix_container_flags( rv, PIX_CONTAINER_FLAG_GL_PROG, vm );
	}
	if( vshader_m ) bmem_free( vshader_str );
	if( fshader_m ) bmem_free( fshader_str );
    }    

    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
#endif
}

void fn_gl_use_prog( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;

    if( pars_num >= 1 )
    {
	//Use user-defined GLSL program:
	PIX_CID p;
	GET_VAL_FROM_STACK( p, 0, PIX_CID );
	pix_vm_container_gl_data* gl = pix_vm_create_container_gl_data( p, vm );
	if( gl && gl->prog )
	{
	    vm->gl_user_defined_prog = gl->prog;
	    glUseProgram( gl->prog->program );
	}
    }
    else
    {
	//Use default GLSL program:
	vm->gl_user_defined_prog = 0;
    }
#endif
}

void fn_gl_uniform( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;

    bool float_vals = false;
    if( pars_num >= 2 )
    {
	GLint uniform_loc = 0;
	GET_VAL_FROM_STACK( uniform_loc, 0, GLint );
	uniform_loc--;
	for( int i = 1; i < pars_num; i++ )
	{
	    if( stack_types[ sp + i ] != 0 )
	    {
		float_vals = true;
		break;
	    }
	}
	if( float_vals )
	{
	    GLfloat v0, v1, v2, v3;
	    while( 1 )
	    {
		GET_VAL_FROM_STACK( v0, 1, GLfloat );
		if( pars_num == 2 )
		{
		    glUniform1f( uniform_loc, v0 );
		    break;
		}
		GET_VAL_FROM_STACK( v1, 2, GLfloat );
		if( pars_num == 3 )
		{
		    glUniform2f( uniform_loc, v0, v1 );
		    break;
		}
		GET_VAL_FROM_STACK( v2, 3, GLfloat );
		if( pars_num == 4 )
		{
		    glUniform3f( uniform_loc, v0, v1, v2 );
		    break;
		}
		GET_VAL_FROM_STACK( v3, 4, GLfloat );
		if( pars_num == 5 )
		{
		    glUniform4f( uniform_loc, v0, v1, v2, v3 );
		    break;
		}
		break;
	    }
	}
	else
	{
	    GLint v0, v1, v2, v3;
	    while( 1 )
	    {
		GET_VAL_FROM_STACK( v0, 1, GLint );
		if( pars_num == 2 )
		{
		    glUniform1i( uniform_loc, v0 );
		    break;
		}
		GET_VAL_FROM_STACK( v1, 2, GLint );
		if( pars_num == 3 )
		{
		    glUniform2i( uniform_loc, v0, v1 );
		    break;
		}
		GET_VAL_FROM_STACK( v2, 3, GLint );
		if( pars_num == 4 )
		{
		    glUniform3i( uniform_loc, v0, v1, v2 );
		    break;
		}
		GET_VAL_FROM_STACK( v3, 4, GLint );
		if( pars_num == 5 )
		{
		    glUniform4i( uniform_loc, v0, v1, v2, v3 );
		    break;
		}
		break;
	    }
	}
    }
#endif
}

void fn_gl_uniform_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;

    if( pars_num >= 3 )
    {
	int size;
	GLint uniform_loc = 0;
	GLboolean transpose;
	PIX_CID cnum;
	GET_VAL_FROM_STACK( size, 0, int );
	GET_VAL_FROM_STACK( uniform_loc, 1, GLint );
	uniform_loc--;
	GET_VAL_FROM_STACK( transpose, 2, GLboolean );
	GET_VAL_FROM_STACK( cnum, 3, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c && size > 1 && size <= 4 )
	{
	    GLfloat temp[ 4 * 4 ];
	    GLfloat* p = 0;
	    if( c->type == PIX_CONTAINER_TYPE_FLOAT32 && sizeof( float ) == sizeof( GLfloat ) )
	    {
		p = (GLfloat*)c->data;
	    }
	    else
	    {
		p = temp;
		for( size_t i = 0; i < size * size; i++ )
		{
		    p[ i ] = pix_vm_get_container_float_element( cnum, i, vm );
		}
	    }
	    switch( size )
	    {
		case 2: glUniformMatrix2fv( uniform_loc, 1, transpose, p ); break;
		case 3: glUniformMatrix3fv( uniform_loc, 1, transpose, p ); break;
		case 4: glUniformMatrix4fv( uniform_loc, 1, transpose, p ); break;
	    }
	}
    }
#endif
}

//
// Animation
//

void fn_pixi_unpack_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    if( pars_num >= 1 ) 
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	rv = pix_vm_container_hdata_unpack_frame( cnum, vm );
    }
    
    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

void fn_pixi_pack_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    if( pars_num >= 1 ) 
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	rv = pix_vm_container_hdata_pack_frame( cnum, vm );
    }
    
    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

void fn_pixi_create_anim( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    while( 1 )
    {
	if( pars_num >= 1 ) 
	{
	    PIX_CID cnum;
	    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	    pix_vm_container* c = pix_vm_get_container( cnum, vm );
	    if( c == 0 ) break;
	    uchar* hdata_ptr = (uchar*)pix_vm_get_container_hdata( cnum, vm );
	    if( hdata_ptr )
	    {
		if( hdata_ptr[ 0 ] != pix_vm_container_hdata_type_anim )
		{
		    pix_vm_remove_container_hdata( cnum, vm );
		    hdata_ptr = 0;
		}
	    }
	    if( hdata_ptr == 0 )
	    {
		pix_vm_create_container_hdata( cnum, pix_vm_container_hdata_type_anim, sizeof( pix_vm_container_hdata_anim ), vm );
		hdata_ptr = (uchar*)pix_vm_get_container_hdata( cnum, vm );
		if( hdata_ptr )
		{
		    pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)hdata_ptr;
		    hdata->frame_count = 1;
		    PIX_VAL prop_val;
		    prop_val.i = 0; pix_vm_set_container_property( cnum, "frame", -1, 0, prop_val, vm );
		    prop_val.i = 10; pix_vm_set_container_property( cnum, "fps", -1, 0, prop_val, vm );
		    prop_val.i = 1; pix_vm_set_container_property( cnum, "frames", -1, 0, prop_val, vm );
		    prop_val.i = -1; pix_vm_set_container_property( cnum, "repeat", -1, 0, prop_val, vm );
		    hdata->frames = (pix_vm_anim_frame*)bmem_new( sizeof( pix_vm_anim_frame ) );
		    bmem_zero( hdata->frames );
		    if( hdata->frames )
		    {
			rv = pix_vm_container_hdata_pack_frame_from_buf( cnum, 0, (COLORPTR)c->data, c->type, c->xsize, c->ysize, vm );
		    }
		}
	    }
	}
	break;
    }
    
    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

void fn_pixi_remove_anim( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    while( 1 )
    {
	if( pars_num >= 1 ) 
	{
	    PIX_CID cnum;
	    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	    pix_vm_container* c = pix_vm_get_container( cnum, vm );
	    if( c == 0 ) break;
	    pix_vm_remove_container_hdata( cnum, vm );
	    rv = 0;
	}
	break;
    }
    
    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

void fn_pixi_clone_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    while( 1 )
    {
	if( pars_num >= 1 ) 
	{
	    PIX_CID cnum;
	    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	    rv = pix_vm_container_hdata_clone_frame( cnum, vm );
	}
	break;
    }
    
    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

void fn_pixi_remove_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    while( 1 )
    {
	if( pars_num >= 1 ) 
	{
	    PIX_CID cnum;
	    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	    rv = pix_vm_container_hdata_remove_frame( cnum, vm );
	}
	break;
    }
    
    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

void fn_pixi_play( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 ) 
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c && c->opt_data && c->opt_data->hdata )
	{
	    PIX_VAL prop_val;
	    prop_val.i = 1; pix_vm_set_container_property( cnum, "play", -1, 0, prop_val, vm );
	    prop_val.i = (uint)time_ticks_hires(); pix_vm_set_container_property( cnum, "start_time", -1, 0, prop_val, vm );
	    prop_val.i = pix_vm_get_container_property_i( cnum, "frame", -1, vm ); pix_vm_set_container_property( cnum, "start_frame", -1, 0, prop_val, vm );
	    //Unpack first frame:
	    pix_vm_container_hdata_unpack_frame( cnum, vm );
	}
    }
}

void fn_pixi_stop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 ) 
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c && c->opt_data && c->opt_data->hdata )
	{
	    PIX_VAL prop_val;
	    prop_val.i = 0; pix_vm_set_container_property( cnum, "play", -1, 0, prop_val, vm );
	}
    }
}

//
// Video
//

struct pix_video_struct
{
    bvideo_struct vid;
    PIX_CID video_container;
    pix_vm* vm;
    PIX_ADDR capture_callback;
    PIX_VAL capture_user_data;
    char capture_user_data_type;
    bool capture_callback_working;
};

void pix_video_capture_callback( bvideo_struct* vid )
{
    pix_video_struct* pix_vid = (pix_video_struct*)vid->capture_user_data;
    if( pix_vid->capture_callback != -1 )
    {
	pix_vm_function fun;
        PIX_VAL pp[ 2 ];
        char pp_types[ 2 ];
        fun.p = pp;
        fun.p_types = pp_types;
        fun.addr = pix_vid->capture_callback;
        fun.p[ 0 ].i = pix_vid->video_container;
        fun.p_types[ 0 ] = 0;
        fun.p[ 1 ] = pix_vid->capture_user_data;
        fun.p_types[ 1 ] = pix_vid->capture_user_data_type;
        fun.p_num = 2;
        pix_vid->capture_callback_working = 1;
        pix_vm_run( PIX_VM_THREADS - 3, 0, &fun, PIX_VM_CALL_FUNCTION, pix_vid->vm );
        pix_vid->capture_callback_working = 0;
    }
}

void fn_video_open( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;

    if( pars_num >= 2 ) 
    {
	PIX_CID name_cnum;
	uint flags;
	PIX_ADDR capture_callback = -1;
	PIX_VAL capture_user_data;
	char capture_user_data_type = 0;
	capture_user_data.i = 0;

	GET_VAL_FROM_STACK( name_cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( flags, 1, int );
	
	bool need_to_free = 0;
        utf8_char* name = pix_vm_make_cstring_from_container( name_cnum, &need_to_free, vm );

	if( pars_num >= 3 ) GET_VAL_FROM_STACK( capture_callback, 2, PIX_ADDR );
	if( pars_num >= 4 ) { capture_user_data = stack[ sp + 3 ]; capture_user_data_type = stack_types[ sp + 3 ]; }
        
        rv = pix_vm_new_container( -1, sizeof( pix_video_struct ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
        if( rv >= 0 )
        {
    	    pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( rv, vm );
    	    bmem_set( pix_vid, sizeof( pix_video_struct ), 0 );
    	    bvideo_struct* vid = &pix_vid->vid;
    	    pix_vid->video_container = rv;
    	    pix_vid->vm = vm;
    	    pix_vid->capture_callback = -1;
    	    if( IS_ADDRESS_CORRECT( capture_callback ) )
    	    {
    		pix_vid->capture_callback = capture_callback & PIX_INT_ADDRESS_MASK;
    	    }
    	    pix_vid->capture_user_data = capture_user_data;
    	    pix_vid->capture_user_data_type = capture_user_data_type;
    	    if( bvideo_open( vid, (const utf8_char*)name, flags, pix_video_capture_callback, pix_vid ) != 0 )
    	    {
    		pix_vm_remove_container( rv, vm );
    		rv = -1;
    	    }
        }        
        
        if( need_to_free ) bmem_free( name );
    }

    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

void fn_video_close( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	
	pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( cnum, vm );
	if( pix_vid )
	{
    	    bvideo_struct* vid = &pix_vid->vid;
    	    rv = bvideo_close( vid );
    	}
    }

    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

void fn_video_start( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	
	pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( cnum, vm );
	if( pix_vid )
	{
    	    bvideo_struct* vid = &pix_vid->vid;
    	    rv = bvideo_start( vid );
    	}
    }

    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

void fn_video_stop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	
	pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( cnum, vm );
	if( pix_vid )
	{
    	    bvideo_struct* vid = &pix_vid->vid;
    	    rv = bvideo_stop( vid );
    	}
    }

    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

void fn_video_props( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 2 )
    {
	PIX_CID vid_cnum;
	PIX_CID props_cnum;
	GET_VAL_FROM_STACK( vid_cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( props_cnum, 1, PIX_CID );
	
	pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( vid_cnum, vm );
	if( pix_vid )
	{
    	    bvideo_struct* vid = &pix_vid->vid;
    	    pix_vm_container* props_cont = pix_vm_get_container( props_cnum, vm );
    	    if( props_cont && props_cont->size >= 2 )
    	    {
    		bvideo_prop* props = (bvideo_prop*)bmem_new( sizeof( bvideo_prop ) * ( props_cont->size / 2 + 1 ) );
    		bmem_zero( props );
    		if( props )
    		{
		    for( int i = 0; i < props_cont->size; i += 2 )
		    {
			PIX_INT prop_id = pix_vm_get_container_int_element( props_cnum, i + 0, vm );
			PIX_INT prop_val = pix_vm_get_container_int_element( props_cnum, i + 1, vm );
			if( prop_id == BVIDEO_PROP_FRAME_WIDTH_I || prop_id == BVIDEO_PROP_FRAME_HEIGHT_I )
			{
			    int rotate = ( vm->wm->screen_angle - vid->orientation ) & 3;
			    if( rotate == 1 || rotate == 3 )
			    {
				if( prop_id == BVIDEO_PROP_FRAME_WIDTH_I )
				    prop_id = BVIDEO_PROP_FRAME_HEIGHT_I;
				else
				    prop_id = BVIDEO_PROP_FRAME_WIDTH_I;
			    }
			}
			props[ i / 2 ].id = (int)prop_id;
			if( fn_num == FN_VIDEO_SET_PROPS )
			    props[ i / 2 ].val.i = prop_val;
		    }
		    if( fn_num == FN_VIDEO_SET_PROPS )
		    {
			//Set:
    			rv = bvideo_set_props( vid, props );
    		    }
    		    else
    		    {
    			//Get:
    			rv = bvideo_get_props( vid, props );
    			if( rv == 0 )
    			{
    		    	    for( int i = 0; i < props_cont->size; i += 2 )
			    {
				pix_vm_set_container_int_element( props_cnum, i + 1, props[ i / 2 ].val.i, vm );
			    }
			}
    		    }
		    bmem_free( props );
		}
	    }	    
    	}
    }

    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

void fn_video_capture_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	PIX_CID dest_cnum;
	int pixel_format = 0; //0 - normal; 1 - grayscale8
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( dest_cnum, 1, PIX_CID );
	while( 1 )
	{
	    if( pars_num >= 3 ) { GET_VAL_FROM_STACK( pixel_format, 2, PIX_CID ); } else break;
	    break;
	}
	
	pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( cnum, vm );
	while( pix_vid )
	{
    	    bvideo_struct* vid = &pix_vid->vid;
    	    if( pix_vid->capture_callback_working == 0 )
    	    {
    		PIX_VM_LOG( "video_capture_frame() can't be called outsize of the capture callback\n" );
    		break;
    	    }
    	    int frame_xsize = vid->frame_width;
    	    int frame_ysize = vid->frame_height;
    	    int dest_pixel_format;
    	    int dest_pixel_size;
    	    int type;
    	    switch( pixel_format )
    	    {
    		case 1: dest_pixel_format = BVIDEO_PIXEL_FORMAT_GRAYSCALE8; dest_pixel_size = 1; type = PIX_CONTAINER_TYPE_INT8; break;
    		default: dest_pixel_format = BVIDEO_PIXEL_FORMAT_COLOR; dest_pixel_size = COLORLEN; type = 32; break;
    	    }
    	    pix_vm_container* dest_cont = pix_vm_get_container( dest_cnum, vm );
    	    if( dest_cont == 0 ) break;
    	    size_t frame_size = frame_xsize * frame_ysize;
    	    if( frame_size * dest_pixel_size > dest_cont->size * g_pix_container_type_sizes[ dest_cont->type ] )
    	    {
    		const utf8_char* type_str = 0;
    		if( type == 32 )
    		    type_str = "PIXEL";
    		else
    		    type_str = g_pix_container_type_names[ type ];
    		PIX_VM_LOG( "video_capture_frame(): wrong container size; expected: %dx%d %s\n", frame_xsize, frame_ysize, type_str );
    		break;
    	    }    	    
    	    if( vid->orientation == vm->wm->screen_angle )
    	    {
    		bvideo_pixel_convert( vid->capture_buffer, frame_xsize, frame_ysize, vid->pixel_format, dest_cont->data, dest_pixel_format );
    	    }
    	    else
    	    {
    		PIX_INT xsize = frame_xsize;
    		PIX_INT ysize = frame_ysize;
    		int rotate = ( vm->wm->screen_angle - vid->orientation ) & 3;
    		void* temp_buf = bmem_new( frame_size * dest_pixel_size );
    		if( temp_buf )
    		{
    		    bvideo_pixel_convert( vid->capture_buffer, xsize, ysize, vid->pixel_format, temp_buf, dest_pixel_format );
    		    pix_vm_rotate_block( &temp_buf, &xsize, &ysize, dest_cont->type, rotate, dest_cont->data );
    		    bmem_free( temp_buf );
    		}
    	    }
	    rv = 0;
    	    break;
    	}
    }

    stack[ sp + ( pars_num - 1 ) ].i = rv;
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
}

//
// Transformation
//
	    
void fn_t_reset( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    pix_vm_gfx_matrix_reset( vm );
    
#ifdef OPENGL
    pix_vm_gl_matrix_set( vm );
#endif
}

void fn_t_rotate( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 4 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT angle, x, y, z;
	GET_VAL_FROM_STACK( angle, 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( x, 1, PIX_FLOAT );
	GET_VAL_FROM_STACK( y, 2, PIX_FLOAT );
	GET_VAL_FROM_STACK( z, 3, PIX_FLOAT );
	angle /= 180;
	angle *= M_PI;
	
	//Normalize vector:
	PIX_FLOAT inv_length = 1.0f / sqrt( x * x + y * y + z * z );
	x *= inv_length;
	y *= inv_length;
	z *= inv_length;
	
	//Create the matrix:
	PIX_FLOAT c = cos( angle );
	PIX_FLOAT s = sin( angle );
	PIX_FLOAT cc = 1 - c;
	PIX_FLOAT r[ 4 * 4 ];
	PIX_FLOAT res[ 4 * 4 ];
	r[ 0 + 0 ] = x * x * cc + c;
	r[ 4 + 0 ] = x * y * cc - z * s;
	r[ 8 + 0 ] = x * z * cc + y * s;
	r[ 12 + 0 ] = 0;
	r[ 0 + 1 ] = y * x * cc + z * s;
	r[ 4 + 1 ] = y * y * cc + c;
	r[ 8 + 1 ] = y * z * cc - x * s;
	r[ 12 + 1 ] = 0;
	r[ 0 + 2 ] = x * z * cc - y * s;
	r[ 4 + 2 ] = y * z * cc + x * s;
	r[ 8 + 2 ] = z * z * cc + c;
	r[ 12 + 2 ] = 0;
	r[ 0 + 3 ] = 0;
	r[ 4 + 3 ] = 0;
	r[ 8 + 3 ] = 0;
	r[ 12 + 3 ] = 1;
	pix_vm_gfx_matrix_mul( res, m, r );
	bmem_copy( m, res, sizeof( PIX_FLOAT ) * 4 * 4 );

	vm->t_enabled = 1;

#ifdef OPENGL
	pix_vm_gl_matrix_set( vm );
#endif
    }
}

void fn_t_translate( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 3 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT x, y, z;
	GET_VAL_FROM_STACK( x, 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( y, 1, PIX_FLOAT );
	GET_VAL_FROM_STACK( z, 2, PIX_FLOAT );
	
	PIX_FLOAT m2[ 4 * 4 ];
	PIX_FLOAT res[ 4 * 4 ];
	bmem_set( m2, sizeof( PIX_FLOAT ) * 4 * 4, 0 );
	m2[ 0 ] = 1;
	m2[ 4 + 1 ] = 1;
	m2[ 8 + 2 ] = 1;
	m2[ 12 + 0 ] = x;
	m2[ 12 + 1 ] = y;
	m2[ 12 + 2 ] = z;
	m2[ 12 + 3 ] = 1;

	pix_vm_gfx_matrix_mul( res, m, m2 );
	bmem_copy( m, res, sizeof( PIX_FLOAT ) * 4 * 4 );
	
	vm->t_enabled = 1;

#ifdef OPENGL
	pix_vm_gl_matrix_set( vm );
#endif
    }
}

void fn_t_scale( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 3 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT x, y, z;
	GET_VAL_FROM_STACK( x, 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( y, 1, PIX_FLOAT );
	GET_VAL_FROM_STACK( z, 2, PIX_FLOAT );
	
	PIX_FLOAT m2[ 4 * 4 ];
	PIX_FLOAT res[ 4 * 4 ];
	bmem_set( m2, sizeof( PIX_FLOAT ) * 4 * 4, 0 );
	m2[ 0 ] = x;
	m2[ 4 + 1 ] = y;
	m2[ 8 + 2 ] = z;
	m2[ 12 + 3 ] = 1;
	
	pix_vm_gfx_matrix_mul( res, m, m2 );
	bmem_copy( m, res, sizeof( PIX_FLOAT ) * 4 * 4 );
	
	vm->t_enabled = 1;

#ifdef OPENGL
        pix_vm_gl_matrix_set( vm );
#endif
    }
}

void fn_t_push_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( vm->t_matrix_sp >= PIX_T_MATRIX_STACK_SIZE - 1 )
    {
	PIX_VM_LOG( "t_push_matrix(): stack overflow\n" );
    }
    else
    {
	bmem_copy( vm->t_matrix + ( vm->t_matrix_sp + 1 ) * 16, vm->t_matrix + vm->t_matrix_sp * 16, 4 * 4 * sizeof( PIX_FLOAT ) );
	vm->t_matrix_sp++;
    }
}

void fn_t_pop_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( vm->t_matrix_sp == 0 )
    {
	PIX_VM_LOG( "t_pop_matrix(): nothing to pop up froms stack\n" );
    }
    else
    {
	vm->t_matrix_sp--;
#ifdef OPENGL
	pix_vm_gl_matrix_set( vm );
#endif
    }
}

void fn_t_get_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	
	PIX_CID m2;
	GET_VAL_FROM_STACK( m2, 0, PIX_CID );
	
	if( (unsigned)m2 < (unsigned)vm->c_num && vm->c[ m2 ] )
	{
	    pix_vm_container* c = vm->c[ m2 ];
	    if( sizeof( PIX_FLOAT ) == 32 && c->type != PIX_CONTAINER_TYPE_FLOAT32 ) return;
	    if( sizeof( PIX_FLOAT ) == 64 && c->type != PIX_CONTAINER_TYPE_FLOAT64 ) return;
	    if( c->size < 4 * 4 ) return;
	    bmem_copy( c->data, m, sizeof( PIX_FLOAT ) * 4 * 4 );
	}
    }
}

void fn_t_set_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	
	PIX_CID m2;
	GET_VAL_FROM_STACK( m2, 0, PIX_CID );
	
	if( (unsigned)m2 < (unsigned)vm->c_num && vm->c[ m2 ] )
	{
	    pix_vm_container* c = vm->c[ m2 ];
	    if( sizeof( PIX_FLOAT ) == 32 && c->type != PIX_CONTAINER_TYPE_FLOAT32 ) return;
	    if( sizeof( PIX_FLOAT ) == 64 && c->type != PIX_CONTAINER_TYPE_FLOAT64 ) return;
	    if( c->size < 4 * 4 ) return;
	    bmem_copy( m, c->data, sizeof( PIX_FLOAT ) * 4 * 4 );
	    
	    vm->t_enabled = 1;

#ifdef OPENGL
	    pix_vm_gl_matrix_set( vm );
#endif
	}
    }
}

void fn_t_mul_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	
	PIX_CID m2;
	GET_VAL_FROM_STACK( m2, 0, PIX_CID );
	
	if( (unsigned)m2 < (unsigned)vm->c_num && vm->c[ m2 ] )
	{
	    pix_vm_container* c = vm->c[ m2 ];
	    if( sizeof( PIX_FLOAT ) == 32 && c->type != PIX_CONTAINER_TYPE_FLOAT32 ) return;
	    if( sizeof( PIX_FLOAT ) == 64 && c->type != PIX_CONTAINER_TYPE_FLOAT64 ) return;
	    if( c->size < 4 * 4 ) return;
	    PIX_FLOAT res_m[ 4 * 4 ];
	    pix_vm_gfx_matrix_mul( res_m, m, (PIX_FLOAT*)c->data );
	    bmem_copy( m, res_m, sizeof( PIX_FLOAT ) * 4 * 4 );
	    
	    vm->t_enabled = 1;

#ifdef OPENGL
	    pix_vm_gl_matrix_set( vm );
#endif
	}
    }
}

void fn_t_point( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	
	PIX_CID p;
	GET_VAL_FROM_STACK( p, 0, PIX_CID );
	
	if( (unsigned)p < (unsigned)vm->c_num && vm->c[ p ] )
	{
	    pix_vm_container* c = vm->c[ p ];
	    if( sizeof( PIX_FLOAT ) == 32 && c->type != PIX_CONTAINER_TYPE_FLOAT32 ) return;
	    if( sizeof( PIX_FLOAT ) == 64 && c->type != PIX_CONTAINER_TYPE_FLOAT64 ) return;
	    if( c->size < 3 ) return;
	    while( pars_num >= 2 )
	    {
		PIX_CID mc;
		GET_VAL_FROM_STACK( mc, 1, PIX_CID );
		if( (unsigned)mc < (unsigned)vm->c_num && vm->c[ mc ] )
		{
		    pix_vm_container* c2 = vm->c[ mc ];
		    if( sizeof( PIX_FLOAT ) == 32 && c2->type != PIX_CONTAINER_TYPE_FLOAT32 ) break;
		    if( sizeof( PIX_FLOAT ) == 64 && c2->type != PIX_CONTAINER_TYPE_FLOAT64 ) break;
		    if( c2->size < 4 * 4 ) break;
		    m = (PIX_FLOAT*)c2->data;
		}
		break;
	    }
	    pix_vm_gfx_vertex_transform( (PIX_FLOAT*)c->data, m );
	}
    }
}

//
// Audio
//	

void fn_set_audio_callback( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num > 0 )
    {
	PIX_ADDR callback;
	PIX_VAL userdata;
	userdata.i = 0;
	char userdata_type = 0;
	int freq = g_snd.freq;
	int format = PIX_CONTAINER_TYPE_INT16;
	int channels = 1;
	uint flags = 0;
	GET_VAL_FROM_STACK( callback, 0, PIX_ADDR );
	if( callback == -1 || IS_ADDRESS_CORRECT( callback ) )
	{
	    if( callback != -1 )
		callback &= PIX_INT_ADDRESS_MASK;
	    if( pars_num > 1 ) 
	    {
		userdata = stack[ sp + 1 ];
		userdata_type = stack_types[ sp + 1 ];
	    }
	    if( pars_num > 2 ) { GET_VAL_FROM_STACK( freq, 2, int ); }
	    if( pars_num > 3 ) { GET_VAL_FROM_STACK( format, 3, int ); }
	    if( pars_num > 4 ) { GET_VAL_FROM_STACK( channels, 4, int ); }
	    if( pars_num > 5 ) { GET_VAL_FROM_STACK( flags, 5, int ); }
	    stack[ sp + ( pars_num - 1 ) ].i = pix_vm_set_audio_callback( callback, userdata, userdata_type, freq, (pix_container_type)format, channels, flags, vm );
	    stack_types[ sp + ( pars_num - 1 ) ] = 0;
	}
	else
	{
	    stack[ sp + ( pars_num - 1 ) ].i = -1;
            stack_types[ sp + ( pars_num - 1 ) ] = 0;
            PIX_VM_LOG( "set_audio_callback() error: wrong callback address %d\n", (int)callback );
	}
    }
}

void fn_enable_audio_input( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = 0;

    if( pars_num > 0 )
    {
	int enable = 0;
	GET_VAL_FROM_STACK( enable, 0, int );
	sound_stream_input( enable );
	if( enable ) vm->audio_input_enabled++; else vm->audio_input_enabled--;
    }
}

void fn_get_note_freq( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = 0;

    if( pars_num > 0 )
    {
	int note = 0;
	int fine = 0;
	GET_VAL_FROM_STACK( note, 0, int );
	if( pars_num > 1 ) { GET_VAL_FROM_STACK( fine, 1, int ); }
	int p = 7680 - note * 64 - fine;
	if( p >= 0 )
	    rv = ( g_linear_freq_tab[ p % 768 ] >> ( p / 768 ) );
	else
	    rv = ( g_linear_freq_tab[ (7680*4+p) % 768 ] << -( ( (7680*4+p) / 768 ) - (7680*4)/768 ) ); //if pitch is negative
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

//
// MIDI
//

void fn_midi_open_client( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;
    
    if( pars_num > 0 )
    {
	PIX_CID client_name;
	GET_VAL_FROM_STACK( client_name, 0, PIX_CID );
	bool need_to_free = 0;
	utf8_char* name = pix_vm_make_cstring_from_container( client_name, &need_to_free, vm );
	if( name == 0 ) name = (utf8_char*)"Pixilang MIDI Client";
	
	rv = pix_vm_new_container( -1, sizeof( sundog_midi_client ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
	sundog_midi_client* c = (sundog_midi_client*)pix_vm_get_container_data( rv, vm );
	if( c )
	{
	    if( sundog_midi_client_open( c, name ) )
	    {
		//Error:
		pix_vm_remove_container( rv, vm );
		rv = -1;
	    }
	}
	
	if( need_to_free ) bmem_free( name );
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_midi_close_client( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;
    
    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	
	sundog_midi_client* c = (sundog_midi_client*)pix_vm_get_container_data( cnum, vm );
	if( c )
	{
	    if( sundog_midi_client_close( c ) == 0 )
	    {
		pix_vm_remove_container( cnum, vm );
		rv = 0;
	    }
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_midi_get_device( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;
    
    if( pars_num >= 3 )
    {
	PIX_CID cnum;
	int dev_num;
	uint flags;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( dev_num, 1, int );
	GET_VAL_FROM_STACK( flags, 2, int );

	sundog_midi_client* c = (sundog_midi_client*)pix_vm_get_container_data( cnum, vm );
	if( c )
	{
	    utf8_char** devices = 0;
	    int devs = sundog_midi_client_get_devices( c, &devices, flags );
	    if( devs > 0 && devices )
	    {
		if( dev_num < devs )
		{
		    utf8_char* name = devices[ dev_num ];
		    if( name )
		    {
			rv = pix_vm_new_container( -1, bmem_strlen( name ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
			bmem_copy( pix_vm_get_container_data( rv, vm ), name, bmem_strlen( name ) );
		    }
		}
		for( int i = 0; i < devs; i++ )
		{
		    bmem_free( devices[ i ] );
		}
		bmem_free( devices );
	    }
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}
    
void fn_midi_open_port( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;
    
    if( pars_num >= 4 )
    {
	PIX_CID cnum;
	PIX_CID port_name_cont;
	PIX_CID dev_name_cont;
	uint flags;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port_name_cont, 1, PIX_CID );
	GET_VAL_FROM_STACK( dev_name_cont, 2, PIX_CID );
	GET_VAL_FROM_STACK( flags, 3, int );
	bool need_to_free1 = 0;
	bool need_to_free2 = 0;
	utf8_char* port_name = pix_vm_make_cstring_from_container( port_name_cont, &need_to_free1, vm );
	utf8_char* dev_name = pix_vm_make_cstring_from_container( dev_name_cont, &need_to_free2, vm );
	
	sundog_midi_client* c = (sundog_midi_client*)pix_vm_get_container_data( cnum, vm );
	if( c )
	{
	    rv = sundog_midi_client_open_port( c, (const utf8_char*)port_name, (const utf8_char*)dev_name, flags );
	}
	
	if( need_to_free1 ) bmem_free( port_name );
	if( need_to_free2 ) bmem_free( dev_name );
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_midi_reopen_port( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;
    
    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int port;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );
	
	sundog_midi_client* c = (sundog_midi_client*)pix_vm_get_container_data( cnum, vm );
	if( c )
	{
	    rv = sundog_midi_client_reopen_port( c, port );
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_midi_close_port( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;
    
    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int port;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );
	
	sundog_midi_client* c = (sundog_midi_client*)pix_vm_get_container_data( cnum, vm );
	if( c )
	{
	    rv = sundog_midi_client_close_port( c, port );
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_midi_get_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = 0;
    
    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int port;
	PIX_CID data;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );
	GET_VAL_FROM_STACK( data, 2, PIX_CID );
	
	sundog_midi_client* c = (sundog_midi_client*)pix_vm_get_container_data( cnum, vm );
	if( c )
	{
	    sundog_midi_event* evt = sundog_midi_client_get_event( c, port );
	    if( evt && evt->size > 0 && evt->data )
	    {
		pix_vm_container* data_cont = pix_vm_get_container( data, vm );
		if( data_cont )
		{
		    if( data_cont->size * g_pix_container_type_sizes[ data_cont->type ] < evt->size )
		    {
			pix_vm_resize_container( data, evt->size, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
		    }
		    bmem_copy( data_cont->data, evt->data, evt->size );
		}
		rv = evt->size;
	    }
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_midi_get_event_time( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;
    
    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int port;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );
	
	sundog_midi_client* c = (sundog_midi_client*)pix_vm_get_container_data( cnum, vm );
	if( c )
	{
	    sundog_midi_event* evt = sundog_midi_client_get_event( c, port );
	    if( evt )
	    {
		rv = evt->t;
	    }
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_midi_next_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;
    
    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int port;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );
	
	sundog_midi_client* c = (sundog_midi_client*)pix_vm_get_container_data( cnum, vm );
	if( c )
	{
	    rv = sundog_midi_client_next_event( c, port );
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_midi_send_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;
    
    if( pars_num >= 5 )
    {
	PIX_CID cnum;
	int port;
	PIX_CID data;
	PIX_INT size;
	PIX_INT t;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );
	GET_VAL_FROM_STACK( data, 2, PIX_CID );
	GET_VAL_FROM_STACK( size, 3, PIX_INT );
	GET_VAL_FROM_STACK( t, 4, PIX_INT );
	
	sundog_midi_client* c = (sundog_midi_client*)pix_vm_get_container_data( cnum, vm );
	if( c )
	{
	    pix_vm_container* data_cont = pix_vm_get_container( data, vm );
	    if( data_cont && data_cont->data && data_cont->size * g_pix_container_type_sizes[ data_cont->type ] >= size )
	    {
		sundog_midi_event evt;
		evt.t = (ticks_hr_t)t;
		evt.size = (size_t)size;
		evt.data = (uchar*)data_cont->data;
	        rv = sundog_midi_client_send_event( c, port, &evt );
	    }
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

//
// Timers
//
	    
void fn_start_timer( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int tnum = 0;
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( tnum, 0, int );
    }
    if( (unsigned)tnum < (unsigned)( sizeof( vm->timers ) / sizeof( uint ) ) )
    {
	vm->timers[ tnum ] = time_ticks();
    }
}

void fn_get_timer( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int tnum = 0;
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( tnum, 0, int );
    }
    if( (unsigned)tnum < (unsigned)( sizeof( vm->timers ) / sizeof( uint ) ) )
    {
	uint t = time_ticks() - vm->timers[ tnum ];
	t *= 1000;
	t /= time_ticks_per_second();
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)t;
    }
}

void fn_get_year( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)time_year();
}

void fn_get_month( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)time_month();
}

void fn_get_day( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)time_day();
}

void fn_get_hours( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)time_hours();
}

void fn_get_minutes( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)time_minutes();
}

void fn_get_seconds( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)time_seconds();
}

void fn_get_ticks( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = (uint)time_ticks_hires();
}

void fn_get_tps( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = (uint)time_ticks_per_second_hires();
}

void fn_sleep( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int ms = 0;
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( ms, 0, int );
	time_sleep( ms );
    }
}

//
// Events
//
	    
void fn_get_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = pix_vm_get_event( vm );
}

void fn_set_quit_action( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( vm->quit_action, 0, char );
    }
}

//
// Threads
//
	    
void fn_thread_create( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_INT rv = -1;
    if( pars_num >= 2 )
    {
	rv = pix_vm_create_active_thread( -1, vm );
	if( rv >= 0 )
	{
	    PIX_ADDR function;
	    uint flags = 0;
	    GET_VAL_FROM_STACK( function, 0, PIX_ADDR );
	    if( pars_num >= 3 ) 
	    {
		GET_VAL_FROM_STACK( flags, 2, int );
		pix_vm_thread* th = vm->th[ rv ];
		th->flags = flags;
	    }
	    if( IS_ADDRESS_CORRECT( function ) )
	    {
		function &= PIX_INT_ADDRESS_MASK;
		pix_vm_function fun;
		PIX_VAL pp[ 2 ];
		char pp_types[ 2 ];
		fun.p = pp;
		fun.p_types = pp_types;
    		fun.addr = function;
		fun.p[ 0 ].i = rv;
		fun.p_types[ 0 ] = 0;
		fun.p[ 1 ] = stack[ sp + 1 ];
		fun.p_types[ 1 ] = stack_types[ sp + 1 ];
		fun.p_num = 2;
		pix_vm_run( rv, 1, &fun, PIX_VM_CALL_FUNCTION, vm );
	    }
	    else
	    {
		PIX_VM_LOG( "thread_create() error: wrong thread address %d\n", (int)function );
	    }
	}
    }
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_thread_destroy( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    if( pars_num >= 1 )
    {
	int thread_num;
	PIX_INT timeout = PIX_INT_MAX_POSITIVE;
	int time_counter = 0;
	GET_VAL_FROM_STACK( thread_num, 0, int );
	if( pars_num >= 2 ) GET_VAL_FROM_STACK( timeout, 1, int );
	rv = pix_vm_destroy_thread( thread_num, timeout, vm );
    }
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_mutex_create( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID rv = -1;
    
    rv = pix_vm_new_container( -1, sizeof( bmutex ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    if( rv >= 0 )
    {
	bmutex* m = (bmutex*)pix_vm_get_container_data( rv, vm );
	if( m == 0 || bmutex_init( m, 0 ) )
	{
	    pix_vm_remove_container( rv, vm );
	    rv = -1;
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_mutex_destroy( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = 0;

    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c )
	{
	    if( c->size >= sizeof( bmutex ) && c->data )
	    {
		rv = bmutex_destroy( (bmutex*)c->data );
		pix_vm_remove_container( cnum, vm );
	    }
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_mutex_lock( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = 0;
    
    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c )
	{
	    if( c->size >= sizeof( bmutex ) && c->data )
	    {
		rv = bmutex_lock( (bmutex*)c->data );
	    }
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_mutex_trylock( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = 0;
    
    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c )
	{
	    if( c->size >= sizeof( bmutex ) && c->data )
	    {
		rv = bmutex_trylock( (bmutex*)c->data );
	    }
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_mutex_unlock( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = 0;
    
    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c )
	{
	    if( c->size >= sizeof( bmutex ) && c->data )
	    {
		rv = bmutex_unlock( (bmutex*)c->data );
	    }
	}
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

//
// Mathematical functions
//

void fn_acos( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = acos( v );
    }
}

void fn_acosh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = acosh( v );
    }
}

void fn_asin( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = asin( v );
    }
}

void fn_asinh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = asinh( v );
    }
}

void fn_atan( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = atan( v );
    }
}

void fn_atanh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = atanh( v );
    }
}

void fn_ceil( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = ceil( v );
    }
}

void fn_cos( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = cos( v );
    }
}

void fn_cosh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
	    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = cosh( v );
    }
}

void fn_exp( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = exp( v );
    }
}

void fn_exp2( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = pow( 2.0, v );
    }
}

void fn_expm1( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = expm1( v );
    }
}

void fn_abs( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = abs( v );
    }
}

void fn_floor( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = floor( v );
    }
}

void fn_mod( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 2 )
    {
	PIX_FLOAT v1, v2;
	GET_VAL_FROM_STACK( v1, 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( v2, 1, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = fmod( v1, v2 );
    }
}

void fn_log( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = log( v );
    }
}

void fn_log2( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = LOG2( v );
    }
}

void fn_log10( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = log10( v );
    }
}

void fn_pow( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 2 )
    {
	PIX_FLOAT v1, v2;
	GET_VAL_FROM_STACK( v1, 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( v2, 1, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = pow( v1, v2 );
    }
}

void fn_sin( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = sin( v );
    }
}

void fn_sinh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = sinh( v );
    }
}

void fn_sqrt( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = sqrt( v );
    }
}

void fn_tan( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = tan( v );
    }
}

void fn_tanh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT v;
	GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
	stack_types[ sp + ( pars_num - 1 ) ] = 1;
	stack[ sp + ( pars_num - 1 ) ].f = tanh( v );
    }
}

void fn_rand( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = pseudo_random_with_seed( &vm->random );
}

void fn_rand_seed( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_INT v;
	GET_VAL_FROM_STACK( v, 0, PIX_INT );
	vm->random = v;
    }
}

//
// Data processing
//
	
void fn_op_cn( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_VAL retval;
    retval.i = 0;
    char retval_type = 0;
    
    if( pars_num >= 2 )
    {
	int opcode;
	PIX_CID cnum;
	char val_type = 0;
	PIX_VAL val;
	val.i = 0;
	PIX_INT x = 0;
	PIX_INT y = 0;
	PIX_INT xsize = 0;
	PIX_INT ysize = 0;
	
	GET_VAL_FROM_STACK( opcode, 0, int );
	GET_VAL_FROM_STACK( cnum, 1, PIX_CID );
	if( pars_num >= 3 )
	{
	    val_type = stack_types[ sp + 2 ];
	    val = stack[ sp + 2 ];
	}
	
	if( pars_num == 5 )
	{
	    //1D:
	    GET_VAL_FROM_STACK( x, 3, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 4, PIX_INT );
	    if( xsize <= 0 ) return;
	}
	if( pars_num == 7 )
	{
	    //2D:
	    GET_VAL_FROM_STACK( x, 3, PIX_INT );
	    GET_VAL_FROM_STACK( y, 4, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 5, PIX_INT );
	    GET_VAL_FROM_STACK( ysize, 6, PIX_INT );
	    if( xsize <= 0 ) return;
	    if( ysize <= 0 ) return;
	}
	
	pix_vm_op_cn( opcode, cnum, val_type, val, x, y, xsize, ysize, &retval, &retval_type, vm );
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = retval_type;
    stack[ sp + ( pars_num - 1 ) ] = retval;
}

void fn_op_cc( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 3 )
    {
	int opcode;
	PIX_CID cnum1;
	PIX_CID cnum2;
	PIX_INT dest_x = 0;
	PIX_INT dest_y = 0;
	PIX_INT src_x = 0;
	PIX_INT src_y = 0;
	PIX_INT xsize = 0;
	PIX_INT ysize = 0;
	
	GET_VAL_FROM_STACK( opcode, 0, int );
	GET_VAL_FROM_STACK( cnum1, 1, PIX_CID );
	GET_VAL_FROM_STACK( cnum2, 2, PIX_CID );
	
	if( pars_num == 6 )
	{
	    //1D:
	    GET_VAL_FROM_STACK( dest_x, 3, PIX_INT );
	    GET_VAL_FROM_STACK( src_x, 4, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 5, PIX_INT );
	    if( xsize <= 0 ) return;
	}
	if( pars_num == 9 )
	{
	    //2D:
	    GET_VAL_FROM_STACK( dest_x, 3, PIX_INT );
	    GET_VAL_FROM_STACK( dest_y, 4, PIX_INT );
	    GET_VAL_FROM_STACK( src_x, 5, PIX_INT );
	    GET_VAL_FROM_STACK( src_y, 6, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 7, PIX_INT );
	    GET_VAL_FROM_STACK( ysize, 8, PIX_INT );
	    if( xsize <= 0 ) return;
	    if( ysize <= 0 ) return;
	}
	
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
        stack[ sp + ( pars_num - 1 ) ].i = pix_vm_op_cc( opcode, cnum1, cnum2, dest_x, dest_y, src_x, src_y, xsize, ysize, vm );
    }
}

void fn_op_ccn( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 4 )
    {
	int opcode;
	PIX_CID cnum1;
	PIX_CID cnum2;
	char val_type;
	PIX_VAL val;
	PIX_INT dest_x = 0;
	PIX_INT dest_y = 0;
	PIX_INT src_x = 0;
	PIX_INT src_y = 0;
	PIX_INT xsize = 0;
	PIX_INT ysize = 0;
	
	GET_VAL_FROM_STACK( opcode, 0, int );
	GET_VAL_FROM_STACK( cnum1, 1, PIX_CID );
	GET_VAL_FROM_STACK( cnum2, 2, PIX_CID );
	val_type = stack_types[ sp + 3 ];
	val = stack[ sp + 3 ];
	
	if( pars_num == 7 )
	{
	    //1D:
	    GET_VAL_FROM_STACK( dest_x, 4, PIX_INT );
	    GET_VAL_FROM_STACK( src_x, 5, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 6, PIX_INT );
	    if( xsize <= 0 ) return;
	}
	if( pars_num == 10 )
	{
	    //2D:
	    GET_VAL_FROM_STACK( dest_x, 4, PIX_INT );
	    GET_VAL_FROM_STACK( dest_y, 5, PIX_INT );
	    GET_VAL_FROM_STACK( src_x, 6, PIX_INT );
	    GET_VAL_FROM_STACK( src_y, 7, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 8, PIX_INT );
	    GET_VAL_FROM_STACK( ysize, 9, PIX_INT );
	    if( xsize <= 0 ) return;
	    if( ysize <= 0 ) return;
	}
	
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
        stack[ sp + ( pars_num - 1 ) ].i = pix_vm_op_ccn( opcode, cnum1, cnum2, val_type, val, dest_x, dest_y, src_x, src_y, xsize, ysize, vm );
    }
}

void fn_generator( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 2 )
    {
	int opcode;
	PIX_CID cnum;
	PIX_FLOAT fval[ 4 ];
	fval[ 0 ] = 0; //Phase
	fval[ 1 ] = 1; //Amplitude
	fval[ 2 ] = 0; //Delta X
	fval[ 3 ] = 0; //Delta Y
	PIX_INT x = 0;
	PIX_INT y = 0;
	PIX_INT xsize = 0;
	PIX_INT ysize = 0;
	
	GET_VAL_FROM_STACK( opcode, 0, int );
	GET_VAL_FROM_STACK( cnum, 1, PIX_CID );
	if( pars_num >= 3 ) GET_VAL_FROM_STACK( fval[ 0 ], 2, PIX_FLOAT );
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( fval[ 1 ], 3, PIX_FLOAT );
	if( pars_num >= 5 ) GET_VAL_FROM_STACK( fval[ 2 ], 4, PIX_FLOAT );
	if( pars_num >= 6 ) GET_VAL_FROM_STACK( fval[ 3 ], 5, PIX_FLOAT );
	
	if( pars_num == 8 )
	{
	    //1D:
	    GET_VAL_FROM_STACK( x, 6, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 7, PIX_INT );
	    if( xsize <= 0 ) return;
	}
	if( pars_num == 10 )
	{
	    //2D:
	    GET_VAL_FROM_STACK( x, 6, PIX_INT );
	    GET_VAL_FROM_STACK( y, 7, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 8, PIX_INT );
	    GET_VAL_FROM_STACK( ysize, 9, PIX_INT );
	    if( xsize <= 0 ) return;
	    if( ysize <= 0 ) return;
	}
	
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
        stack[ sp + ( pars_num - 1 ) ].i = pix_vm_generator( opcode, cnum, fval, x, y, xsize, ysize, vm );
    }
}

void fn_wavetable_generator( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 8 )
    {
	PIX_CID dest; //Audio destination (INT16 or FLOAT32)
	PIX_INT dest_offset;
	PIX_INT dest_length;
	PIX_CID table; //Table with waveform (supported formats: 32768 x INT16, 32768 x FLOAT32)
	PIX_CID amp; //INT32 array of amplitudes (fixed point 16.16)
	PIX_CID amp_delta; //INT32 array of amplitude delta values (fixed point 16.16)
	PIX_CID pos; //INT32 array of wavetable positions (fixed point 16.16)
	PIX_CID pos_delta; //INT32 array of wavetable position delta values (fixed point 16.16)
	PIX_INT gen_offset; //Number of the first generator
	PIX_INT gen_step; //Play every gen_step generator
	PIX_INT gen_count; //Total number of generators to play
	GET_VAL_FROM_STACK( dest, 0, PIX_CID );
	GET_VAL_FROM_STACK( dest_offset, 1, PIX_INT );
	GET_VAL_FROM_STACK( dest_length, 2, PIX_INT );
	GET_VAL_FROM_STACK( table, 3, PIX_CID );
	GET_VAL_FROM_STACK( amp, 4, PIX_CID );
	GET_VAL_FROM_STACK( amp_delta, 5, PIX_CID );
	GET_VAL_FROM_STACK( pos, 6, PIX_CID );
	GET_VAL_FROM_STACK( pos_delta, 7, PIX_CID );
	GET_VAL_FROM_STACK( gen_offset, 8, PIX_INT );
	GET_VAL_FROM_STACK( gen_step, 9, PIX_INT );
	GET_VAL_FROM_STACK( gen_count, 10, PIX_INT );

	stack_types[ sp + ( pars_num - 1 ) ] = 0;
        stack[ sp + ( pars_num - 1 ) ].i = pix_vm_wavetable_generator( 
    	    dest, dest_offset, dest_length, 
    	    table, 
    	    amp, amp_delta, 
    	    pos, pos_delta, 
    	    gen_offset, gen_step, gen_count, 
    	    vm );
    }
}    

void fn_sampler( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	int pars;
	GET_VAL_FROM_STACK( pars, 0, int );
	if( (unsigned)pars >= (unsigned)vm->c_num ) return;
	pix_vm_container* pars_cont = vm->c[ pars ];
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
        stack[ sp + ( pars_num - 1 ) ].i = pix_vm_sampler( pars_cont, vm );
    }
}

void fn_envelope2p( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 3 )
    {
	PIX_CID cnum;
	PIX_INT v1;
	PIX_INT v2;
	PIX_INT offset = 0;
	PIX_INT size = -1;
	char dc_off1_type = 0;
	char dc_off2_type = 0; 
	PIX_VAL dc_off1; dc_off1.i = 0;
	PIX_VAL dc_off2; dc_off2.i = 0;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( v1, 1, PIX_INT );
	GET_VAL_FROM_STACK( v2, 2, PIX_INT );
	if( pars_num > 3 ) { GET_VAL_FROM_STACK( offset, 3, PIX_INT ); }
	if( pars_num > 4 ) { GET_VAL_FROM_STACK( size, 4, PIX_INT ); }
	if( pars_num > 5 ) { dc_off1_type = stack_types[ sp + 5 ]; dc_off1 = stack[ sp + 5 ]; }
	if( pars_num > 6 ) { dc_off2_type = stack_types[ sp + 6 ]; dc_off2 = stack[ sp + 6 ]; }
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
        stack[ sp + ( pars_num - 1 ) ].i = pix_vm_envelope2p( cnum, v1, v2, offset, size, dc_off1_type, dc_off1, dc_off2_type, dc_off2, vm );
    }
}

void fn_gradient( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;
    while( 1 )
    {
	if( pars_num < 5 ) break;
    
	PIX_VAL v[ 4 ];
	char v_types[ 4 ];
	PIX_CID cnum;
	PIX_INT x = 0;
	PIX_INT y = 0;
	PIX_INT x_step = 1;
	PIX_INT y_step = 1;

	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont == 0 ) break;
	PIX_INT xsize = cont->xsize;
	PIX_INT ysize = cont->ysize;

	for( int i = 0; i < 4; i++ )
	{
	    v[ i ] = stack[ sp + i + 1 ];
	    v_types[ i ] = stack_types[ sp + i + 1 ];
	    if( cont->type < PIX_CONTAINER_TYPE_FLOAT32 )
	    {
		if( v_types[ i ] == 1 )
		{
		    //Float to Int:
		    v_types[ i ] = 0;
		    v[ i ].i = v[ i ].f;
		}
	    }
	    else
	    {
		if( v_types[ i ] == 0 )
		{
		    //Int to Float:
		    v_types[ i ] = 1;
		    v[ i ].f = v[ i ].i;
		}
	    }
	}
	int pnum = 5;
	while( 1 )
	{
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( x, pnum, PIX_INT ); } else break; pnum++;
    	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( y, pnum, PIX_INT ); } else break; pnum++;
    	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( xsize, pnum, PIX_INT ); } else break; pnum++;
    	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( ysize, pnum, PIX_INT ); } else break; pnum++;
    	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( x_step, pnum, PIX_INT ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( y_step, pnum, PIX_INT ); } else break; pnum++;
	    break;
	}

	if( x + xsize < 0 ) break;
	if( y + ysize < 0 ) break;
	if( x >= cont->xsize ) break;
	if( y >= cont->ysize ) break;
	if( xsize <= 0 ) break;
	if( ysize <= 0 ) break;
	if( x_step <= 0 ) x_step = 1;
	if( y_step <= 0 ) y_step = 1;

	PIX_INT xd, yd, xstart, ystart;
	PIX_FLOAT xd_f, yd_f, xstart_f, ystart_f;
	if( cont->type < PIX_CONTAINER_TYPE_FLOAT32 )
	{
	    xd = ( 32768 << 15 ) / xsize;
	    xstart = 0;
	    yd = ( 32768 << 15 ) / ysize;
	    ystart = 0;
	    if( x < 0 ) { xsize -= -x; xstart = -x * xd; x = 0; }
	    if( y < 0 ) { ysize -= -y; ystart = -y * yd; y = 0; }
	    xd *= x_step;
	    yd *= y_step;
	}
	else
	{
	    xd_f = 1.0F / (PIX_FLOAT)xsize;
	    xstart_f = 0;
	    yd_f = 1.0F / (PIX_FLOAT)ysize;
	    ystart_f = 0;
	    if( x < 0 ) { xsize -= -x; xstart_f = -x * xd_f; x = 0; }
	    if( y < 0 ) { ysize -= -y; ystart_f = -y * yd_f; y = 0; }
	    xd_f *= x_step;
	    yd_f *= y_step;
	}
	
	if( x + xsize > cont->xsize ) xsize = cont->xsize - x;
	if( y + ysize > cont->ysize ) ysize = cont->ysize - y;

	if( cont->type < PIX_CONTAINER_TYPE_FLOAT32 )
	{
	    //Int:
	    PIX_INT yy = ystart;
	    for( PIX_INT cy = 0; cy < ysize; cy += y_step )
	    {
		PIX_INT xx = xstart;
		PIX_INT n = yy >> 15;
		PIX_INT nn = 32768 - n;
		switch( cont->type )
		{
		    case PIX_CONTAINER_TYPE_INT8:
			{
			    signed char* ptr = (signed char*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_INT v1 = ( v[ 0 ].i * nn + v[ 2 ].i * n ) >> 15;
			    PIX_INT v2 = ( v[ 1 ].i * nn + v[ 3 ].i * n ) >> 15;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx >> 15;
				nn = 32768 - n;
				*ptr = ( v1 * nn + v2 * n ) >> 15;
				ptr += x_step;
				xx += xd;
			    }
			}
			break;
		    case PIX_CONTAINER_TYPE_INT16:
			{
			    signed short* ptr = (signed short*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_INT v1 = ( v[ 0 ].i * nn + v[ 2 ].i * n ) >> 15;
			    PIX_INT v2 = ( v[ 1 ].i * nn + v[ 3 ].i * n ) >> 15;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx >> 15;
				nn = 32768 - n;
				*ptr = ( v1 * nn + v2 * n ) >> 15;
				ptr += x_step;
				xx += xd;
			    }
			}
			break;
		    case PIX_CONTAINER_TYPE_INT32:
			{
			    int* ptr = (int*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_INT v1 = ( v[ 0 ].i * nn + v[ 2 ].i * n ) >> 15;
			    PIX_INT v2 = ( v[ 1 ].i * nn + v[ 3 ].i * n ) >> 15;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx >> 15;
				nn = 32768 - n;
				*ptr = ( v1 * nn + v2 * n ) >> 15;
				ptr += x_step;
				xx += xd;
			    }
			}
			break;
#ifdef PIX_INT64_ENABLED
		    case PIX_CONTAINER_TYPE_INT64:
			{
			    int64* ptr = (int64*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_INT v1 = ( v[ 0 ].i * nn + v[ 2 ].i * n ) >> 15;
			    PIX_INT v2 = ( v[ 1 ].i * nn + v[ 3 ].i * n ) >> 15;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx >> 15;
				nn = 32768 - n;
				*ptr = ( v1 * nn + v2 * n ) >> 15;
				ptr += x_step;
				xx += xd;
			    }
			}
			break;
#endif
		}
		yy += yd;
	    }
	}
	else
	{
	    //Float:
	    PIX_FLOAT yy = ystart_f;
	    for( PIX_INT cy = 0; cy < ysize; cy += y_step )
	    {
		PIX_FLOAT xx = xstart_f;
		PIX_FLOAT n = yy;
		PIX_FLOAT nn = 1 - n;
		switch( cont->type )
		{
		    case PIX_CONTAINER_TYPE_FLOAT32:
			{
			    float* ptr = (float*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_FLOAT v1 = v[ 0 ].f * nn + v[ 2 ].f * n;
			    PIX_FLOAT v2 = v[ 1 ].f * nn + v[ 3 ].f * n;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx;
				nn = 1 - n;
				*ptr = v1 * nn + v2 * n;
				ptr += x_step;
				xx += xd_f;
			    }
			}
			break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64:
			{
			    double* ptr = (double*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_FLOAT v1 = v[ 0 ].f * nn + v[ 2 ].f * n;
			    PIX_FLOAT v2 = v[ 1 ].f * nn + v[ 3 ].f * n;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx;
				nn = 1 - n;
				*ptr = v1 * nn + v2 * n;
				ptr += x_step;
				xx += xd_f;
			    }
			}
			break;
#endif
		}
		yy += yd_f;
	    }
	}
	
	rv = 0;
	
	break;
    }

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_fft( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 3 )
    {
	uint flags;
	PIX_CID cnum1;
	PIX_CID cnum2;
	int size = 0;
	GET_VAL_FROM_STACK( flags, 0, int );
	GET_VAL_FROM_STACK( cnum1, 1, PIX_CID );
	GET_VAL_FROM_STACK( cnum2, 2, PIX_CID );
	if( pars_num >= 4 )
	    GET_VAL_FROM_STACK( size, 3, int );
	if( (unsigned)cnum1 >= (unsigned)vm->c_num ) return;
	if( (unsigned)cnum2 >= (unsigned)vm->c_num ) return;
	if( size < 0 ) return;
	pix_vm_container* cont1 = vm->c[ cnum1 ];
	pix_vm_container* cont2 = vm->c[ cnum2 ];
	if( cont1 && cont2 && cont1->data && cont2->data )
	{
	    if( cont1->type != cont2->type ) return;
	    if( size == 0 ) size = (int)cont1->size;
	    if( cont1->type == PIX_CONTAINER_TYPE_FLOAT32 )
	    {
		fft( flags, (float*)cont1->data, (float*)cont2->data, size );
	    }
	    if( cont1->type == PIX_CONTAINER_TYPE_FLOAT64 )
	    {
		fft( flags, (double*)cont1->data, (double*)cont2->data, size );
	    }
	}
    }
}

void fn_new_filter( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    uint flags = 0; //optional;

    if( pars_num >= 1 ) GET_VAL_FROM_STACK( flags, 0, int );

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = pix_vm_new_filter( flags, vm );
}

void fn_remove_filter( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_CID f;
	GET_VAL_FROM_STACK( f, 0, PIX_CID );
	pix_vm_remove_filter( f, vm );
    }
}

void fn_init_filter( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 2 )
    {
	PIX_CID f;
	PIX_CID a; //feedforward filter coefficients;
	PIX_CID b = -1; //feedback filter coefficients; optional; can be -1;
	int rshift = 0; //bitwise right shift for fixed point computations; optional;
	uint flags = 0; //optional;

	GET_VAL_FROM_STACK( f, 0, PIX_CID );
	GET_VAL_FROM_STACK( a, 1, PIX_CID );
	while( 1 )
	{
	    if( pars_num > 2 ) { GET_VAL_FROM_STACK( b, 2, PIX_CID ); } else break;
	    if( pars_num > 3 ) { GET_VAL_FROM_STACK( rshift, 3, int ); } else break;
	    if( pars_num > 4 ) { GET_VAL_FROM_STACK( flags, 4, int ); } else break;
	    break;
	}

	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = pix_vm_init_filter( f, a, b, rshift, flags, vm );
    }
    else
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}

void fn_reset_filter( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_CID f;
	GET_VAL_FROM_STACK( f, 0, PIX_CID );
	pix_vm_reset_filter( f, vm );
    }
}

void fn_apply_filter( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    // output[ n ] = ( a[ 0 ] * input[ n ] + a[ 1 ] * input[ n - 1 ] + ... + a[ a_count - 1 ] * input[ n - a_count - 1 ]
    //                                     + b[ 0 ] * output[ n - 1 ] + ... + b[ b_count - 1 ] * output[ n - b_count - 1 ] ) >> rshift;

    if( pars_num >= 3 )
    {
	PIX_CID f; //filter (created with new_filter());
	PIX_CID output;
	PIX_CID input;
	uint flags = 0; //optional;
	PIX_INT offset = 0; //optional;
	PIX_INT size = -1; //optional;

	GET_VAL_FROM_STACK( f, 0, PIX_CID );
	GET_VAL_FROM_STACK( output, 1, PIX_CID );
	GET_VAL_FROM_STACK( input, 2, PIX_CID );
	while( 1 )
	{
	    if( pars_num > 3 ) { GET_VAL_FROM_STACK( flags, 3, int ); } else break;
	    if( pars_num > 4 ) { GET_VAL_FROM_STACK( offset, 4, PIX_INT ); } else break;
	    if( pars_num > 5 ) { GET_VAL_FROM_STACK( size, 5, PIX_INT ); } else break;
	    break;
	}
	
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = pix_vm_apply_filter( f, output, input, flags, offset, size, vm );
    }
    else
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}    

void fn_replace_values( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    
    while( pars_num >= 3 )
    {
	PIX_CID dest;
	PIX_CID src;
	PIX_CID values;
	size_t dest_offset = 0;
	size_t src_offset = 0;
	size_t size = -1;
	GET_VAL_FROM_STACK( dest, 0, PIX_CID );
	GET_VAL_FROM_STACK( src, 1, PIX_CID );
	GET_VAL_FROM_STACK( values, 2, PIX_CID );
	while( 1 )
	{
	    if( pars_num >= 4 ) GET_VAL_FROM_STACK( dest_offset, 3, size_t ) else break;
	    if( pars_num >= 5 ) GET_VAL_FROM_STACK( src_offset, 4, size_t ) else break;
	    if( pars_num >= 6 ) GET_VAL_FROM_STACK( size, 5, size_t ) else break;
	    break;
	}
	if( (unsigned)dest >= (unsigned)vm->c_num ) break;
	if( (unsigned)src >= (unsigned)vm->c_num ) break;
	if( (unsigned)values >= (unsigned)vm->c_num ) break;
	pix_vm_container* dest_cont = vm->c[ dest ];
	pix_vm_container* src_cont = vm->c[ src ];
	pix_vm_container* values_cont = vm->c[ values ];
	if( dest_cont == 0 ) break;
	if( src_cont == 0 ) break;
	if( values_cont == 0 ) break;
	size_t values_num = values_cont->size;
	
	if( dest_cont->type != values_cont->type )
	{
	    PIX_VM_LOG( "replace_values(): destination type must be = values type\n" );
	    break;
	}

	if( size == -1 ) size = dest_cont->size;
	if( dest_offset >= dest_cont->size ) break;
	if( src_offset >= src_cont->size ) break;
	if( dest_offset + size > dest_cont->size )
	{
	    size = dest_cont->size - dest_offset;
	}
	if( src_offset + size > src_cont->size )
	{
	    size = src_cont->size - src_offset;
	}
	
	if( dest_cont->type == values_cont->type )
	{
	    switch( src_cont->type )
	    {
		case PIX_CONTAINER_TYPE_INT8:
		    {
			uchar* s = (uchar*)src_cont->data + src_offset;
			switch( dest_cont->type )
			{
		    	    case PIX_CONTAINER_TYPE_INT8: { uchar* d = (uchar*)dest_cont->data + dest_offset; uchar* v = (uchar*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT16: { uint16* d = (uint16*)dest_cont->data + dest_offset; uint16* v = (uint16*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break; 
		    	    case PIX_CONTAINER_TYPE_INT32:
		    	    case PIX_CONTAINER_TYPE_FLOAT32:
		    		{ uint* d = (uint*)dest_cont->data + dest_offset; uint* v = (uint*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
			    case PIX_CONTAINER_TYPE_INT64:
			    case PIX_CONTAINER_TYPE_FLOAT64:
		    		{ uint64* d = (uint64*)dest_cont->data + dest_offset; uint64* v = (uint64*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#endif
			    default:
				PIX_VM_LOG( "replace_values(): unsupported type of destination container\n" );
				break;
			}
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT16:
		    {
			uint16* s = (uint16*)src_cont->data + src_offset;
			switch( dest_cont->type )
			{
		    	    case PIX_CONTAINER_TYPE_INT8: { uchar* d = (uchar*)dest_cont->data + dest_offset; uchar* v = (uchar*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT16: { uint16* d = (uint16*)dest_cont->data + dest_offset; uint16* v = (uint16*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT32:
		    	    case PIX_CONTAINER_TYPE_FLOAT32:
		    		{ uint* d = (uint*)dest_cont->data + dest_offset; uint* v = (uint*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
			    case PIX_CONTAINER_TYPE_INT64:
			    case PIX_CONTAINER_TYPE_FLOAT64:
		    		{ uint64* d = (uint64*)dest_cont->data + dest_offset; uint64* v = (uint64*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#endif
			    default:
				PIX_VM_LOG( "replace_values(): unsupported type of destination container\n" );
				break;
			}
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT32:
		    {
			uint* s = (uint*)src_cont->data + src_offset;
			switch( dest_cont->type )
			{
		    	    case PIX_CONTAINER_TYPE_INT8: { uchar* d = (uchar*)dest_cont->data + dest_offset; uchar* v = (uchar*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT16: { uint16* d = (uint16*)dest_cont->data + dest_offset; uint16* v = (uint16*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT32:
		    	    case PIX_CONTAINER_TYPE_FLOAT32:
		    		{ uint* d = (uint*)dest_cont->data + dest_offset; uint* v = (uint*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
			    case PIX_CONTAINER_TYPE_INT64:
			    case PIX_CONTAINER_TYPE_FLOAT64:
		    		{ uint64* d = (uint64*)dest_cont->data + dest_offset; uint64* v = (uint64*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#endif
			    default:
				PIX_VM_LOG( "replace_values(): unsupported type of destination container\n" );
				break;
			}
		    }
		    break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64:
		    {
			uint64* s = (uint64*)src_cont->data + src_offset;
			switch( dest_cont->type )
			{
		    	    case PIX_CONTAINER_TYPE_INT8: { uchar* d = (uchar*)dest_cont->data + dest_offset; uchar* v = (uchar*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT16: { uint16* d = (uint16*)dest_cont->data + dest_offset; uint16* v = (uint16*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT32:
		    	    case PIX_CONTAINER_TYPE_FLOAT32:
		    		{ uint* d = (uint*)dest_cont->data + dest_offset; uint* v = (uint*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
			    case PIX_CONTAINER_TYPE_INT64:
			    case PIX_CONTAINER_TYPE_FLOAT64:
		    		{ uint64* d = (uint64*)dest_cont->data + dest_offset; uint64* v = (uint64*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#endif
			    default:
				PIX_VM_LOG( "replace_values(): unsupported type of destination container\n" );
				break;
			}
		    }
		    break;
#endif
		default:
		    PIX_VM_LOG( "replace_values(): unsupported type of source container\n" );
		    break;
	    }
	}
	break;
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_copy_and_resize( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    
    while( pars_num >= 2 )
    {
	pix_vm_resize_pars pars;
	bmem_set( &pars, sizeof( pars ), 0 );	
	
	PIX_CID dest_cnum;
	PIX_CID src_cnum;
	
	GET_VAL_FROM_STACK( dest_cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( src_cnum, 1, PIX_CID );
	
	pix_vm_container* dest = pix_vm_get_container( dest_cnum, vm );
	pix_vm_container* src = pix_vm_get_container( src_cnum, vm );
	if( dest == 0 || src == 0 ) break;
	
	if( dest->type != src->type )
	{
	    PIX_VM_LOG( "copy_and_stretch(): destination type must be = src type\n" );
	    break;
	}
	
	pars.dest = dest->data;
	pars.src = src->data;
	pars.type = dest->type;
	pars.dest_xsize = dest->xsize;
	pars.dest_ysize = dest->ysize;
	pars.src_xsize = src->xsize;
	pars.src_ysize = src->ysize;
	
	if( pars_num >= 3 ) { GET_VAL_FROM_STACK( pars.resize_flags, 2, int ); } else pars.resize_flags = PIX_RESIZE_INTERP1;
	if( pars_num == 11 )
	{
	    GET_VAL_FROM_STACK( pars.dest_x, 3, PIX_INT );
	    GET_VAL_FROM_STACK( pars.dest_y, 4, PIX_INT );
	    GET_VAL_FROM_STACK( pars.dest_rect_xsize, 5, PIX_INT );
	    GET_VAL_FROM_STACK( pars.dest_rect_ysize, 6, PIX_INT );
	    GET_VAL_FROM_STACK( pars.src_x, 7, PIX_INT );
	    GET_VAL_FROM_STACK( pars.src_y, 8, PIX_INT );
	    GET_VAL_FROM_STACK( pars.src_rect_xsize, 9, PIX_INT );
	    GET_VAL_FROM_STACK( pars.src_rect_ysize, 10, PIX_INT );
	}
	
	pix_vm_copy_and_resize( &pars );
	
	rv = 0;
	
	break;
    }

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

//
// Dialogs
//
	    
void fn_file_dialog( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    bool name_need_to_free = 0;
    bool mask_need_to_free = 0;
    bool id_need_to_free = 0;
    bool defname_need_to_free = 0;
    utf8_char* name = 0;
    utf8_char* mask = 0;
    utf8_char* id = 0;
    utf8_char* defname = 0;
        
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 3 ) { err = 1; break; }
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	name = pix_vm_make_cstring_from_container( cnum, &name_need_to_free, vm );
	if( name == 0 ) { err = 1; break; }
	GET_VAL_FROM_STACK( cnum, 1, PIX_CID );
	mask = pix_vm_make_cstring_from_container( cnum, &mask_need_to_free, vm );
	if( mask == 0 ) { err = 1; break; }
	GET_VAL_FROM_STACK( cnum, 2, PIX_CID );
	id = pix_vm_make_cstring_from_container( cnum, &id_need_to_free, vm );
	if( id == 0 ) { err = 1; break; }
	if( pars_num > 3 )
	{
	    GET_VAL_FROM_STACK( cnum, 3, PIX_CID );
	    defname = pix_vm_make_cstring_from_container( cnum, &defname_need_to_free, vm );
	}
	break;
    }

    //Execute:
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = -1;
    if( err == 0 )
    {
	vm->file_dialog_name = name;
	if( mask[ 0 ] == 0 )
	    vm->file_dialog_mask = 0;
	else
	    vm->file_dialog_mask = mask;
	vm->file_dialog_id = id;
	vm->file_dialog_def_name = defname;
	vm->file_dialog_request = 1;
	while( 1 )
	{
	    if( vm->file_dialog_request == 0 ) break;
	    time_sleep( 100 );
	}
	utf8_char* filename = vm->file_dialog_result;
	if( filename )
	{
	    size_t size = bmem_strlen( filename );
	    if( size > 0 )
	    {
		void* data = bmem_new( size );
		bmem_copy( data, filename, size );
		stack[ sp + ( pars_num - 1 ) ].i = pix_vm_new_container( -1, (int)size, 1, PIX_CONTAINER_TYPE_INT8, data, vm );
	    }
	}
	
	if( name_need_to_free ) bmem_free( name );
	if( mask_need_to_free ) bmem_free( mask );
	if( id_need_to_free ) bmem_free( id );
	if( defname_need_to_free ) bmem_free( defname );
    }
}

void fn_prefs_dialog( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    //Execute:
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = 0;

    vm->prefs_dialog_request = 1;
    while( 1 )
    {
        if( vm->prefs_dialog_request == 0 ) break;
        time_sleep( 100 );
    }
}

//
// Posix compatibility
//

//Issue a command:
void fn_system( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
#ifndef WINCE
    PIX_CID name;
    pix_vm_container* name_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	name_cont = vm->c[ name ];
	break;
    }
    
    //Execute:
    if( err == 0 )
    {
	bool need_to_free = 0;
	utf8_char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );

	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)system( ts );
	
	if( need_to_free ) bmem_free( ts );
    }
    else 
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)system( 0 );
    }
#else
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = 0;
#endif
}

void fn_argc( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = (PIX_INT)vm->wm->sd->argc;
}

void fn_argv( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    bool err = 1;
    if( pars_num >= 1 )
    {
	PIX_INT arg_num;
	GET_VAL_FROM_STACK( arg_num, 0, PIX_INT );
	if( (unsigned)arg_num < vm->wm->sd->argc && vm->wm->sd->argv && vm->wm->sd->argv[ arg_num ] )
	{
	    int arg_len = (int)bmem_strlen( vm->wm->sd->argv[ arg_num ] );
	    PIX_CID arg = pix_vm_new_container( -1, arg_len, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
	    if( arg >= 0 )
	    {
		pix_vm_container* arg_cont = vm->c[ arg ];
		bmem_copy( arg_cont->data, vm->wm->sd->argv[ arg_num ], arg_len );
		stack_types[ sp + ( pars_num - 1 ) ] = 0;
		stack[ sp + ( pars_num - 1 ) ].i = arg;
		err = 0;
	    }
	}
    }
    if( err )
    {
	stack_types[ sp + ( pars_num - 1 ) ] = 0;
	stack[ sp + ( pars_num - 1 ) ].i = -1;
    }
}

void fn_exit( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( vm->wm->sd->exit_code, 0, int );
    }
    th->active = 0;
    vm->wm->exit_request = 1;
}

//
// Private API
//
	
void fn_system_copy_OR_open_url( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name;
    pix_vm_container* name_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	name_cont = vm->c[ name ];
	break;
    }
    
    //Execute:
    if( err == 0 )
    {
	bool need_to_free = 0;
	utf8_char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
    
	if( fn_num == FN_SYSTEM_COPY )
	    system_copy( ts );
	if( fn_num == FN_OPEN_URL )
	    open_url( ts );
	
	if( need_to_free ) bmem_free( ts );
    }
}

void fn_system_paste( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = -1;
    
    utf8_char* fname = system_paste( 0 );
    if( fname )
    {
        PIX_CID name = pix_vm_new_container( -1, bmem_strlen( fname ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
        if( name >= 0 )
        {
            pix_vm_container* name_cont = vm->c[ name ];
            bmem_copy( name_cont->data, fname, bmem_strlen( fname ) );
            stack_types[ sp + ( pars_num - 1 ) ] = 0;
            stack[ sp + ( pars_num - 1 ) ].i = name;
        }
        bmem_free( fname );
    }
}

void fn_send_file_to( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    
    while( pars_num >= 1 )
    {
	PIX_CID file_path;
	
	GET_VAL_FROM_STACK( file_path, 0, PIX_CID );
	
	bool need_to_free;
	utf8_char* file_str = pix_vm_make_cstring_from_container( file_path, &need_to_free, vm );
	
	if( file_str )
	{
	    switch( fn_num )
	    {
		case FN_SEND_FILE_TO_EMAIL: rv = send_file_to_email( file_str ); break;
		case FN_SEND_FILE_TO_GALLERY: rv = send_file_to_gallery( file_str ); break;
		default: break;
	    }
	}
	
	if( need_to_free ) bmem_free( file_str );

	break;
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_webserver( PIX_BUILTIN_FN_PARAMETERS )
{
    vm->webserver_request = 1;
    while( vm->webserver_request != 0 )
    {
        time_sleep( 100 );
    }
}

void fn_set_audio_play_status( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int status;
    GET_VAL_FROM_STACK( status, 0, int );
    g_snd_play_status = status;
}

void fn_get_audio_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = 0;
    while( 1 )
    {
	if( g_snd_play_request )
	{
	    rv = 1;
	    g_snd_play_request = 0;
	    break;
	}
	if( g_snd_stop_request )
	{
	    rv = 2;
	    g_snd_stop_request = 0;
	    break;
	}
	if( g_snd_rewind_request )
	{
	    rv = 3;
	    g_snd_rewind_request = 0;
	    break;
	}
	break;
    }
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_wm_video_capture_supported( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = video_capture_supported( vm->wm );
}

void fn_wm_video_capture_start( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    while( 1 )
    {
	if( pars_num >= 1 ) GET_VAL_FROM_STACK( vm->vcap_in_fps, 0, int ) else vm->vcap_in_fps = 30;
	if( pars_num >= 2 ) GET_VAL_FROM_STACK( vm->vcap_in_bitrate_kb, 1, int ) else vm->vcap_in_bitrate_kb = 1000;

	vm->vcap_request = 1;
	while( vm->vcap_request == 1 )
	{
    	    time_sleep( 10 );
	}
    	rv = vm->vcap_out_err;
    	
    	break;
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_wm_video_capture_stop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    vm->vcap_request = 2;
    while( vm->vcap_request == 2 )
    {
        time_sleep( 10 );
    }

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = vm->vcap_out_err;
}

void fn_wm_video_capture_get_ext( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = -1;
    
    const utf8_char* ext = video_capture_get_file_ext( vm->wm );
    if( ext )
    {
	PIX_CID str = pix_vm_new_container( -1, bmem_strlen( ext ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	if( str >= 0 )
    	{
    	    pix_vm_container* str_cont = vm->c[ str ];
    	    bmem_copy( str_cont->data, ext, bmem_strlen( ext ) );
    	    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    	    stack[ sp + ( pars_num - 1 ) ].i = str;
    	}
    }
}

void fn_wm_video_capture_encode( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    
    while( pars_num >= 1 )
    {
	bool name_need_to_free = 0;
	utf8_char* name = 0;
	PIX_CID name_cont;
	
	GET_VAL_FROM_STACK( name_cont, 0, PIX_CID );
	name = pix_vm_make_cstring_from_container( name_cont, &name_need_to_free, vm );
	if( name == 0 ) break;

	video_capture_set_in_name( name, vm->wm );
        rv = video_capture_encode( vm->wm );

	if( name_need_to_free ) bmem_free( name );
	
	break;
    }
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}
