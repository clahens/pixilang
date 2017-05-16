#include "core/core.h"
#include "pixilang.h"

#if defined(UNIX)
    #if ( defined(ARCH_X86) || defined(ARCH_X86_64) || defined(ARCH_ARM) ) && !defined(IPHONE)
	#include <dlfcn.h>
	#define DYNAMIC_LIB_SUPPORT
    #endif
    
    #include <sys/stat.h>
    #include <sys/types.h>
    
    #ifdef ANDROID
	#define NO_SETXATTR
    #else
	#include <sys/xattr.h>
    #endif
#endif

#ifdef WIN
    #include <windows.h>
    #define DYNAMIC_LIB_SUPPORT
#endif

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
#define DPRINT( fmt, ARGS... ) blog( fmt, ## ARGS )
#else
#define DPRINT( fmt, ARGS... ) {}
#endif

#define GET_VAL_FROM_STACK( v, snum, type ) { if( stack_types[ sp + snum ] == 0 ) v = (type)stack[ sp + snum ].i; else v = (type)stack[ sp + snum ].f; }

#define FN_HEADER PIX_VAL* stack = th->stack; char* stack_types = th->stack_types;

void fn_new_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_pixi_with_alpha( PIX_BUILTIN_FN_PARAMETERS );
void fn_resize_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_rotate_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_clean_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_clone_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_copy_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_pixi_info( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_pixi_flags( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_pixi_flags( PIX_BUILTIN_FN_PARAMETERS );
void fn_reset_pixi_flags( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_pixi_prop_OR_set_pixi_prop( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_pixi_props( PIX_BUILTIN_FN_PARAMETERS );
void fn_convert_pixi_type( PIX_BUILTIN_FN_PARAMETERS );
void fn_show_mem_debug_messages( PIX_BUILTIN_FN_PARAMETERS );
void fn_zlib_pack( PIX_BUILTIN_FN_PARAMETERS );
void fn_zlib_unpack( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_num_to_string( PIX_BUILTIN_FN_PARAMETERS );
void fn_string_to_num( PIX_BUILTIN_FN_PARAMETERS );

void fn_strcat( PIX_BUILTIN_FN_PARAMETERS );
void fn_strcmp_OR_strstr( PIX_BUILTIN_FN_PARAMETERS );
void fn_strlen( PIX_BUILTIN_FN_PARAMETERS );
void fn_sprintf( PIX_BUILTIN_FN_PARAMETERS );

void fn_get_log( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_system_log( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_load( PIX_BUILTIN_FN_PARAMETERS );
void fn_fload( PIX_BUILTIN_FN_PARAMETERS );
void fn_save( PIX_BUILTIN_FN_PARAMETERS );
void fn_fsave( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_real_path( PIX_BUILTIN_FN_PARAMETERS );
void fn_new_flist( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_flist( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_flist_name( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_flist_type( PIX_BUILTIN_FN_PARAMETERS );
void fn_flist_next( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_file_size( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_file( PIX_BUILTIN_FN_PARAMETERS );
void fn_rename_file( PIX_BUILTIN_FN_PARAMETERS );
void fn_copy_file( PIX_BUILTIN_FN_PARAMETERS );
void fn_create_directory( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_disk0( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_disk0( PIX_BUILTIN_FN_PARAMETERS );

void fn_fopen( PIX_BUILTIN_FN_PARAMETERS );
void fn_fopen_mem( PIX_BUILTIN_FN_PARAMETERS );
void fn_fclose( PIX_BUILTIN_FN_PARAMETERS );
void fn_fputc( PIX_BUILTIN_FN_PARAMETERS );
void fn_fputs( PIX_BUILTIN_FN_PARAMETERS );
void fn_fgets_OR_fwrite_OR_fread( PIX_BUILTIN_FN_PARAMETERS );
void fn_fgetc( PIX_BUILTIN_FN_PARAMETERS );
void fn_feof_OF_fflush_OR_ftell( PIX_BUILTIN_FN_PARAMETERS );
void fn_fseek( PIX_BUILTIN_FN_PARAMETERS );
void fn_setxattr( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_frame( PIX_BUILTIN_FN_PARAMETERS );
void fn_vsync( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_pixel_size( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_pixel_size( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_screen( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_screen( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_zbuf( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_zbuf( PIX_BUILTIN_FN_PARAMETERS );
void fn_clear_zbuf( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_color( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_red( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_green( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_blue( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_blend( PIX_BUILTIN_FN_PARAMETERS );
void fn_transp( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_transp( PIX_BUILTIN_FN_PARAMETERS );
void fn_clear( PIX_BUILTIN_FN_PARAMETERS );
void fn_dot( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_dot( PIX_BUILTIN_FN_PARAMETERS );
void fn_line( PIX_BUILTIN_FN_PARAMETERS );
void fn_box( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_triangles3d( PIX_BUILTIN_FN_PARAMETERS );
void fn_sort_triangles3d( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_key_color( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_key_color( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_alpha( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_alpha( PIX_BUILTIN_FN_PARAMETERS );
void fn_print( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_text_xsize( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_text_ysize( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_font( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_font( PIX_BUILTIN_FN_PARAMETERS );
void fn_effector( PIX_BUILTIN_FN_PARAMETERS );
void fn_color_gradient( PIX_BUILTIN_FN_PARAMETERS );
void fn_split_rgb( PIX_BUILTIN_FN_PARAMETERS );

void fn_set_gl_callback( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_gl_data( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_draw_arrays( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_blend_func( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_bind_framebuffer( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_new_prog( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_use_prog( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_uniform( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_uniform_matrix( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_pixi_unpack_frame( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_pack_frame( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_create_anim( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_remove_anim( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_clone_frame( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_remove_frame( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_play( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_stop( PIX_BUILTIN_FN_PARAMETERS );

void fn_video_open( PIX_BUILTIN_FN_PARAMETERS );
void fn_video_close( PIX_BUILTIN_FN_PARAMETERS );
void fn_video_start( PIX_BUILTIN_FN_PARAMETERS );
void fn_video_stop( PIX_BUILTIN_FN_PARAMETERS );
void fn_video_props( PIX_BUILTIN_FN_PARAMETERS );
void fn_video_capture_frame( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_t_reset( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_rotate( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_translate( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_scale( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_push_matrix( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_pop_matrix( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_get_matrix( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_set_matrix( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_mul_matrix( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_point( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_set_audio_callback( PIX_BUILTIN_FN_PARAMETERS );
void fn_enable_audio_input( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_note_freq( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_midi_open_client( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_close_client( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_get_device( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_open_port( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_reopen_port( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_close_port( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_get_event( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_get_event_time( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_next_event( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_send_event( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_start_timer( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_timer( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_year( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_month( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_day( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_hours( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_minutes( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_seconds( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_ticks( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_tps( PIX_BUILTIN_FN_PARAMETERS );
void fn_sleep( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_get_event( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_quit_action( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_thread_create( PIX_BUILTIN_FN_PARAMETERS );
void fn_thread_destroy( PIX_BUILTIN_FN_PARAMETERS );
void fn_mutex_create( PIX_BUILTIN_FN_PARAMETERS );
void fn_mutex_destroy( PIX_BUILTIN_FN_PARAMETERS );
void fn_mutex_lock( PIX_BUILTIN_FN_PARAMETERS );
void fn_mutex_trylock( PIX_BUILTIN_FN_PARAMETERS );
void fn_mutex_unlock( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_acos( PIX_BUILTIN_FN_PARAMETERS );
void fn_acosh( PIX_BUILTIN_FN_PARAMETERS );
void fn_asin( PIX_BUILTIN_FN_PARAMETERS );
void fn_asinh( PIX_BUILTIN_FN_PARAMETERS );
void fn_atan( PIX_BUILTIN_FN_PARAMETERS );
void fn_atanh( PIX_BUILTIN_FN_PARAMETERS );
void fn_ceil( PIX_BUILTIN_FN_PARAMETERS );
void fn_cos( PIX_BUILTIN_FN_PARAMETERS );
void fn_cosh( PIX_BUILTIN_FN_PARAMETERS );
void fn_exp( PIX_BUILTIN_FN_PARAMETERS );
void fn_exp2( PIX_BUILTIN_FN_PARAMETERS );
void fn_expm1( PIX_BUILTIN_FN_PARAMETERS );
void fn_abs( PIX_BUILTIN_FN_PARAMETERS );
void fn_floor( PIX_BUILTIN_FN_PARAMETERS );
void fn_mod( PIX_BUILTIN_FN_PARAMETERS );
void fn_log( PIX_BUILTIN_FN_PARAMETERS );
void fn_log2( PIX_BUILTIN_FN_PARAMETERS );
void fn_log10( PIX_BUILTIN_FN_PARAMETERS );
void fn_pow( PIX_BUILTIN_FN_PARAMETERS );
void fn_sin( PIX_BUILTIN_FN_PARAMETERS );
void fn_sinh( PIX_BUILTIN_FN_PARAMETERS );
void fn_sqrt( PIX_BUILTIN_FN_PARAMETERS );
void fn_tan( PIX_BUILTIN_FN_PARAMETERS );
void fn_tanh( PIX_BUILTIN_FN_PARAMETERS );
void fn_rand( PIX_BUILTIN_FN_PARAMETERS );
void fn_rand_seed( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_op_cn( PIX_BUILTIN_FN_PARAMETERS );
void fn_op_cc( PIX_BUILTIN_FN_PARAMETERS );
void fn_op_ccn( PIX_BUILTIN_FN_PARAMETERS );
void fn_generator( PIX_BUILTIN_FN_PARAMETERS );
void fn_wavetable_generator( PIX_BUILTIN_FN_PARAMETERS );
void fn_sampler( PIX_BUILTIN_FN_PARAMETERS );
void fn_envelope2p( PIX_BUILTIN_FN_PARAMETERS );
void fn_gradient( PIX_BUILTIN_FN_PARAMETERS );
void fn_fft( PIX_BUILTIN_FN_PARAMETERS );
void fn_new_filter( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_filter( PIX_BUILTIN_FN_PARAMETERS );
void fn_init_filter( PIX_BUILTIN_FN_PARAMETERS );
void fn_reset_filter( PIX_BUILTIN_FN_PARAMETERS );
void fn_apply_filter( PIX_BUILTIN_FN_PARAMETERS );
void fn_replace_values( PIX_BUILTIN_FN_PARAMETERS );
void fn_copy_and_resize( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_file_dialog( PIX_BUILTIN_FN_PARAMETERS );
void fn_prefs_dialog( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_dlopen( PIX_BUILTIN_FN_PARAMETERS );
void fn_dlclose( PIX_BUILTIN_FN_PARAMETERS );
void fn_dlsym( PIX_BUILTIN_FN_PARAMETERS );
void fn_dlcall( PIX_BUILTIN_FN_PARAMETERS );

void fn_system( PIX_BUILTIN_FN_PARAMETERS );
void fn_argc( PIX_BUILTIN_FN_PARAMETERS );
void fn_argv( PIX_BUILTIN_FN_PARAMETERS );
void fn_exit( PIX_BUILTIN_FN_PARAMETERS );
    
void fn_system_copy_OR_open_url( PIX_BUILTIN_FN_PARAMETERS );
void fn_system_paste( PIX_BUILTIN_FN_PARAMETERS );
void fn_send_file_to( PIX_BUILTIN_FN_PARAMETERS );
void fn_webserver( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_audio_play_status( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_audio_event( PIX_BUILTIN_FN_PARAMETERS );
void fn_wm_video_capture_supported( PIX_BUILTIN_FN_PARAMETERS );
void fn_wm_video_capture_start( PIX_BUILTIN_FN_PARAMETERS );
void fn_wm_video_capture_stop( PIX_BUILTIN_FN_PARAMETERS );
void fn_wm_video_capture_get_ext( PIX_BUILTIN_FN_PARAMETERS );
void fn_wm_video_capture_encode( PIX_BUILTIN_FN_PARAMETERS );
