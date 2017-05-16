#pragma once

#include "regions/device.h"
#include "struct.h"

extern uchar g_font0[ 8 * 256 + 257 ];
extern uchar g_font1[ 12 * 256 + 257 ];

#define COLOR_THEMES 25
extern uchar g_color_themes[ 3 * 4 * COLOR_THEMES ];

extern const utf8_char g_text_up[ 2 ];
extern const utf8_char g_text_down[ 2 ];
extern const utf8_char g_text_left[ 2 ];
extern const utf8_char g_text_right[ 2 ];

//###################################
//## MAIN FUNCTIONS:               ##
//###################################

int win_init( 
    const utf8_char* windowname, 
    int xsize, 
    int ysize, 
    uint flags, 
    sundog_engine* sd );
void win_colors_init( window_manager* wm );
void win_reinit( window_manager* wm );
int win_calc_font_zoom( int screen_ppi, int screen_zoom, float screen_scale, float screen_font_scale );
void win_zoom_init( window_manager* wm );
void win_vsync( bool vsync, window_manager* wm );
void win_exit_request( window_manager* wm );
void win_suspend( bool suspend, window_manager* wm );
void win_close( window_manager* wm ); //close window manager

//###################################
//## WINDOWS:                      ##
//###################################

WINDOWPTR new_window( 
    const utf8_char* name, 
    int x, 
    int y, 
    int xsize, 
    int ysize, 
    COLOR color,
    WINDOWPTR parent, 
    win_handler_t win_handler,
    window_manager* wm );
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
    window_manager* wm );
void set_window_controller( WINDOWPTR win, int ctrl_num, window_manager* wm, ... );
void remove_window( WINDOWPTR win, window_manager* wm );
void add_child( WINDOWPTR win, WINDOWPTR child, window_manager* wm );
void remove_child( WINDOWPTR win, WINDOWPTR child, window_manager* wm );
WINDOWPTR get_parent_by_win_handler( WINDOWPTR win, win_handler_t win_handler );
WINDOWPTR get_child_by_win_handler( WINDOWPTR win, win_handler_t win_handler );
void set_handler( WINDOWPTR win, win_action_handler_t handler, void* handler_data, window_manager* wm );
bool is_window_visible( WINDOWPTR win, window_manager* wm );
void draw_window( WINDOWPTR win, window_manager* wm );
void show_window( WINDOWPTR win, window_manager* wm );
void hide_window( WINDOWPTR win, window_manager* wm );
void bring_to_front( WINDOWPTR win, window_manager* wm );
void recalc_regions( window_manager* wm );

void set_focus_win( WINDOWPTR win, window_manager* wm );

//mode:
//  0 - touch down;
//  1 - touch move;
//  2 - touch up.
//flags:
#define TOUCH_FLAG_LIMIT		( 1 << 0 ) /*Don't allow touches outside of the screen*/
#define TOUCH_FLAG_REALWINDOW_XY	( 1 << 1 )
int collect_touch_events( int mode, uint touch_flags, uint evt_flags, int x, int y, int pressure, WM_TOUCH_ID touch_id, window_manager* wm );
void convert_real_window_xy( int& x, int& y, window_manager* wm );
int send_touch_events( window_manager* wm );
int send_events(
    sundog_event* events,
    int events_num,
    window_manager* wm );
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
    window_manager* wm );
int check_event( sundog_event* evt, window_manager* wm );
void handle_event( sundog_event* evt, window_manager* wm );
int handle_event_by_window( sundog_event* evt, window_manager* wm );

int EVENT_LOOP_BEGIN( sundog_event* evt, window_manager* wm );
int EVENT_LOOP_END( window_manager* wm );

inline int window_handler_check_data( sundog_event* evt, window_manager* wm )
{
    if( evt->type != EVT_GETDATASIZE )
    {
        if( evt->win->data == 0 )
        {
            blog( "ERROR: Event to window (%s) without data\n", evt->win->name );
            return 1;
        }
    }
    return 0;
}

//###################################
//## STATUS MESSAGES:              ##
//###################################

void show_status_message( const utf8_char* msg, int t, window_manager* wm );
void hide_status_message( window_manager* wm );

//###################################
//## TIMERS:                       ##
//###################################

// -1 - empty timer;
// Timer is endlessly looped (with auto-repeat) by default; so if you want - just remove it in the callback
int add_timer( void (*)( void*, sundog_timer*, window_manager* ), void* data, ticks_t delay, window_manager* wm );
void reset_timer( int timer, window_manager* wm );
void reset_timer( int timer, ticks_t new_delay, window_manager* wm );
void remove_timer( int timer, window_manager* wm );

//###################################
//## WINDOWS DECORATIONS:          ##
//###################################

int decorator_handler( sundog_event* evt, window_manager* wm );
void change_decorator(
    WINDOWPTR dec,
    int x, 
    int y, 
    int xsize, 
    int ysize, 
    uint flags,
    window_manager* wm );
void resize_window_with_decorator( WINDOWPTR win, int xsize, int ysize, window_manager* wm );
WINDOWPTR new_window_with_decorator( 
    const utf8_char* name, 
    int x, 
    int y, 
    int xsize, 
    int ysize, 
    COLOR color,
    WINDOWPTR parent, 
    win_handler_t win_handler,
    uint flags, //DECOR_FLAG_xxx
    window_manager* wm );
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
    uint flags, //DECOR_FLAG_xxx
    window_manager* wm );

//###################################
//## DRAWING FUNCTIONS:            ##
//###################################

WM_INLINE COLOR get_color( uchar r, uchar g, uchar b )
{
    COLOR res = 0; //resulted color

#ifdef COLOR8BITS
    #ifdef GRAYSCALE
	res = (COLOR)( (int)( ( r + g + b + b ) >> 2 ) );
    #else
	res += ( b & 192 );
	res += ( g & 224 ) >> 2;
	res += ( r >> 5 );
    #endif
#endif
    
#ifdef COLOR16BITS

#ifdef RGB556
    res += ( r >> 3 ) << 11;
    res += ( g >> 3 ) << 6;
    res += ( b >> 2 );
#endif
#ifdef RGB555
    res += ( r >> 3 ) << 10;
    res += ( g >> 3 ) << 5;
    res += ( b >> 3 );
#endif
#ifdef RGB565
    res += ( r >> 3 ) << 11;
    res += ( g >> 2 ) << 5;
    res += ( b >> 3 );
#endif

#endif
    
#ifdef COLOR32BITS
    #ifdef OPENGL
	res += r;
	res += g << 8;
	res += b << 16;
    #else
	res += r << 16;
	res += g << 8;
	res += b;
    #endif
#endif
    
    return res;
}

WM_INLINE COLOR get_color_from_string( utf8_char* str )
{
    uint c = 0;
    if( bmem_strlen( str ) != 7 ) return COLORMASK;
    for( int i = 1; i < 7; i++ )
    {
	c <<= 4;
	if( str[ i ] < 58 ) c += str[ i ] - '0';
	else if( str[ i ] > 64 && str[ i ] < 91 ) c += str[ i ] - 'A' + 10;
	else c += str[ i ] - 'a' + 10;
    }
    return get_color( ( c >> 16 ) & 255, ( c >> 8 ) & 255, c & 255 );
}

#ifdef RGB556
#define GET_RED16 \
    res = ( ( c >> 11 ) << 3 ) & 0xF8; \
    if( res ) res |= 7;
#endif
#ifdef RGB555
#define GET_RED16 \
    res = ( ( c >> 10 ) << 3 ) & 0xF8; \
    if( res ) res |= 7;
#endif
#ifdef RGB565
#define GET_RED16 \
    res = ( ( c >> 11 ) << 3 ) & 0xF8; \
    if( res ) res |= 7;
#endif

WM_INLINE int red( COLOR c )
{
    int res = 0;
    
#ifdef COLOR8BITS
    #ifdef GRAYSCALE
	res = c;
    #else
	res = ( c << 5 ) & 255;
	if( res ) res |= 0x1F;
    #endif
#endif
    
#ifdef COLOR16BITS
    GET_RED16;
#endif

#ifdef COLOR32BITS
    #ifdef OPENGL
	res = c & 255;
    #else
	res = ( c >> 16 ) & 255;
    #endif
#endif

    return res;
}

#ifdef RGB556
#define GET_GREEN16 \
    res = ( ( c >> 6 ) << 3 ) & 0xF8; \
    if( res ) res |= 7;
#endif
#ifdef RGB555
#define GET_GREEN16 \
    res = ( ( c >> 5 ) << 3 ) & 0xF8; \
    if( res ) res |= 7;
#endif
#ifdef RGB565
#define GET_GREEN16 \
    res = ( ( c >> 5 ) << 2 ) & 0xFC; \
    if( res ) res |= 3;
#endif

WM_INLINE int green( COLOR c )
{
    int res = 0;
    
#ifdef COLOR8BITS
    #ifdef GRAYSCALE
	res = c;
    #else
	res = ( c << 2 ) & 0xE0;
	if( res ) res |= 0x1F;
    #endif
#endif
    
#ifdef COLOR16BITS
    GET_GREEN16;
#endif

#ifdef COLOR32BITS
    res = ( c >> 8 ) & 255;
#endif
    
    return res;
}

#ifdef RGB556
#define GET_BLUE16 \
    res = ( c << 2 ) & 0xFC; \
    if( res ) res |= 3;
#endif
#ifdef RGB555
#define GET_BLUE16 \
    res = ( c << 3 ) & 0xF8; \
    if( res ) res |= 7;
#endif
#ifdef RGB565
#define GET_BLUE16 \
    res = ( c << 3 ) & 0xF8; \
    if( res ) res |= 7;
#endif

WM_INLINE int blue( COLOR c )
{
    int res = 0;
    
#ifdef COLOR8BITS
    #ifdef GRAYSCALE
	res = c;
    #else
	res = ( c & 192 );
	if( res ) res |= 0x3F;
    #endif
#endif
    
#ifdef COLOR16BITS
    GET_BLUE16;
#endif
    
#ifdef COLOR32BITS
    #ifdef OPENGL
	res = ( c >> 16 ) & 255;
    #else
	res = c & 255;
    #endif
#endif
    
    return res;
}

WM_INLINE COLOR fast_blend( COLOR c1, COLOR c2, uchar val )
{
    uint ival = 256 - val;
#ifdef COLOR32BITS
    uint v1_1 = c1 & 0xFF00FF;
    uint v1_2 = c1 & 0x00FF00;
    uint v2_1 = c2 & 0xFF00FF;
    uint v2_2 = c2 & 0x00FF00;
    uint res = 
	( ( ( ( v1_1 * ival ) + ( v2_1 * val ) ) >> 8 ) & 0xFF00FF ) |
	( ( ( ( v1_2 * ival ) + ( v2_2 * val ) ) >> 8 ) & 0x00FF00 );
    return res;
#endif
#ifdef COLOR16BITS
    #ifdef FAST_BLEND
        ival >>= 3;
        val >>= 3;
        #ifdef RGB556
            uint res = 
                ( ( ( ( ( c1 & 0xF83F ) * ival ) + ( ( c2 & 0xF83F ) * val ) ) >> 5 ) & 0xF83F ) |
                ( ( ( ( ( c1 & 0x7C0 ) * ival ) + ( ( c2 & 0x7C0 ) * val ) ) >> 5 ) & 0x7C0 );
        #endif
        #ifdef RGB555
            uint res = 
                ( ( ( ( ( c1 & 0x7C1F ) * ival ) + ( ( c2 & 0x7C1F ) * val ) ) >> 5 ) & 0x7C1F ) |
                ( ( ( ( ( c1 & 0x1E0 ) * ival ) + ( ( c2 & 0x1E0 ) * val ) ) >> 5 ) & 0x1E0 );
        #endif
        #ifdef RGB565
            uint res = 
                ( ( ( ( ( c1 & 0xF81F ) * ival ) + ( ( c2 & 0xF81F ) * val ) ) >> 5 ) & 0xF81F ) |
                ( ( ( ( ( c1 & 0x7E0 ) * ival ) + ( ( c2 & 0x7E0 ) * val ) ) >> 5 ) & 0x7E0 );
        #endif
    #else
        #ifdef RGB556
            int r1 = ( c1 >> 8 ) | 7;
            int g1 = ( ( c1 >> 3 ) & 255 ) | 7;
            int b1 = ( ( c1 << 2 ) & 255 ) | 3;
            int r2 = ( c2 >> 8 ) | 7;
            int g2 = ( ( c2 >> 3 ) & 255 ) | 7;
            int b2 = ( ( c2 << 2 ) & 255 ) | 3;
        #endif
        #ifdef RGB555
            int r1 = ( c1 >> 7 ) | 7;
            int g1 = ( ( c1 >> 2 ) & 255 ) | 7;
            int b1 = ( ( c1 << 3 ) & 255 ) | 7;
            int r2 = ( c2 >> 7 ) | 7;
            int g2 = ( ( c2 >> 2 ) & 255 ) | 7;
            int b2 = ( ( c2 << 3 ) & 255 ) | 7;
        #endif
        #ifdef RGB565
            int r1 = ( c1 >> 8 ) | 7;
            int g1 = ( ( c1 >> 3 ) & 255 ) | 3;
            int b1 = ( ( c1 << 3 ) & 255 ) | 7;
            int r2 = ( c2 >> 8 ) | 7;
            int g2 = ( ( c2 >> 3 ) & 255 ) | 3;
            int b2 = ( ( c2 << 3 ) & 255 ) | 7;
        #endif
            int r3 = r1 * ival + r2 * val;
            int g3 = g1 * ival + g2 * val;
            int b3 = b1 * ival + b2 * val;
        #ifdef RGB556
            r3 >>= 8 + 3;
            g3 >>= 8 + 3;
            b3 >>= 8 + 2;
            uint res = ( r3 << 11 ) | ( g3 << 6 ) | b3;
        #endif
        #ifdef RGB555
            r3 >>= 8 + 3;
            g3 >>= 8 + 3;
            b3 >>= 8 + 3;
            uint res = ( r3 << 10 ) | ( g3 << 5 ) | b3;
        #endif
        #ifdef RGB565
            r3 >>= 8 + 3;
            g3 >>= 8 + 2;
            b3 >>= 8 + 3;
            uint res = ( r3 << 11 ) | ( g3 << 5 ) | b3;
        #endif
    #endif
    return res;
#endif
#ifdef COLOR8BITS
    #ifdef GRAYSCALE
	return (COLOR)( ( c1 * ival + c2 * val ) >> 8 );
    #else
	int b1 = c1 >> 5; b1 &= ~1; if( b1 ) b1 |= 1; //because B is 2 bits only
	int g1 = ( c1 >> 3 ) & 7; 
	int r1 = c1 & 7; 
	int b2 = c2 >> 5; b2 &= ~1; if( b2 ) b2 |= 1;
	int g2 = ( c2 >> 3 ) & 7; 
	int r2 = c2 & 7; 
	int r3 = r1 * ival + r2 * val;
	int g3 = g1 * ival + g2 * val;
	int b3 = b1 * ival + b2 * val;
	b3 >>= 9;
	g3 >>= 8;
	r3 >>= 8;
	return ( b3 << 6 ) | ( g3 << 3 ) | r3;
    #endif
#endif
    return 0;
}

WM_INLINE COLOR blend( COLOR c1, COLOR c2, int val )
{
    if( val <= 0 ) return c1;
    if( val >= 255 ) return c2;
    return fast_blend( c1, c2, (uchar)val );
}

void win_draw_lock( WINDOWPTR win, window_manager* wm );    //Only draw_window() and event_handler (during the EVT_DRAW handling) do this automaticaly!
void win_draw_unlock( WINDOWPTR win, window_manager* wm );  //Only draw_window() and event_handler (during the EVT_DRAW handling) do this automaticaly!

void win_draw_rect( WINDOWPTR win, int x, int y, int xsize, int ysize, COLOR color, window_manager* wm );
void win_draw_frect( WINDOWPTR win, int x, int y, int xsize, int ysize, COLOR color, window_manager* wm );
void win_draw_image_ext( 
    WINDOWPTR win, 
    int x, 
    int y, 
    int dest_xsize, 
    int dest_ysize,
    int source_x,
    int source_y,
    sundog_image* img, 
    window_manager* wm );
void win_draw_image( 
    WINDOWPTR win, 
    int x, 
    int y, 
    sundog_image* img, 
    window_manager* wm );
bool line_clip( int* x1, int* y1, int* x2, int* y2, int clip_x1, int clip_y1, int clip_x2, int clip_y2 );
void win_draw_line( WINDOWPTR win, int x1, int y1, int x2, int y2, COLOR color, window_manager* wm );

void screen_changed( window_manager* wm );

//###################################
//## VIDEO CAPTURING:              ##
//###################################

bool video_capture_supported( window_manager* wm );
void video_capture_set_in_name( const utf8_char* name, window_manager* wm );
const utf8_char* video_capture_get_file_ext( window_manager* wm ); //example: "avi"
int video_capture_start( window_manager* wm );
int video_capture_frame_begin( window_manager* wm );
int video_capture_frame_end( window_manager* wm );
int video_capture_stop( window_manager* wm );
int video_capture_encode( window_manager* wm );

//###################################
//## FONTS:                        ##
//###################################

#define utf32_to_win1251( dest, src ) \
{ \
    while( 1 ) \
    { \
	if( src > 0x7F ) \
	{ \
    	    if( ( src >> 8 ) == 0x04 ) \
    	    { \
    		/* Russian: */ \
		utf32_char src_t = src & 255; \
    		if( src_t >= 0x10 && src_t <= 0x4F ) \
		{ \
	    	    dest = src_t + 0xB0; \
		    break; \
		} \
	    } \
	} \
	else \
	{ \
	    if( src >= 0x10 ) { dest = src; break; } \
	} \
	dest = '*'; \
	break; \
    } \
}

//###################################
//## FRAMEBUFFER:                  ##
//###################################

void fb_draw_line( int x1, int y1, int x2, int y2, COLOR color, window_manager* wm );
void fb_draw_frect( int x, int y, int xsize, int ysize, COLOR color, window_manager* wm );
void fb_draw_image(
    int dest_x, int dest_y,
    int dest_xs, int dest_ys,
    int src_x, int src_y,
    sundog_image* img,
    window_manager* wm );

//###################################
//## IMAGE FUNCTIONS:              ##
//###################################

sundog_image* new_image( 
    int xsize, 
    int ysize, 
    void* src,
    int src_x,
    int src_y,
    int src_xsize,
    int src_ysize,
    uint flags,
    window_manager* wm );
void flush_image( sundog_image* img );
sundog_image* resize_image( int resize_flags, int new_xsize, int new_ysize, sundog_image* img );
void remove_image( sundog_image* img );

//###################################
//## KEYMAP FUNCTIONS:             ##
//###################################

sundog_keymap* keymap_new( window_manager* wm );
void keymap_remove( sundog_keymap* km, window_manager* wm );
void keymap_silent( sundog_keymap* km, bool silent, window_manager* wm );
int keymap_add_section( sundog_keymap* km, const utf8_char* section_name, const utf8_char* section_id, int section_num, window_manager* wm );
int keymap_add_action( sundog_keymap* km, int section_num, const utf8_char* action_name, const utf8_char* action_id, int action_num, window_manager* wm );
const utf8_char* keymap_get_action_name( sundog_keymap* km, int section_num, int action_num, window_manager* wm );
int keymap_bind2( sundog_keymap* km, int section_num, int key, uint flags, uint pars1, uint pars2, int action_num, int bind_num, int bind_flags, window_manager* wm );
int keymap_bind( sundog_keymap* km, int section_num, int key, uint flags, int action_num, int bind_num, int bind_flags, window_manager* wm );
int keymap_get_action( sundog_keymap* km, int section_num, int key, uint flags, uint pars1, uint pars2, window_manager* wm );
sundog_keymap_key* keymap_get_key( sundog_keymap* km, int section_num, int action_num, int key_num, window_manager* wm );
bool keymap_midi_note_assigned( sundog_keymap* km, int note, window_manager* wm );
int keymap_save( sundog_keymap* km, profile_data* profile, window_manager* wm );
int keymap_load( sundog_keymap* km, profile_data* profile, window_manager* wm );

//###################################
//## WBD FUNCTIONS:                ##
//###################################

#define IMG_PREC 11 /* fixed point precision */

//See code in wbd.cpp
void wbd_lock( WINDOWPTR win );
void wbd_unlock( window_manager* wm );
void wbd_draw( window_manager* wm );

void draw_points( int16* coord2d, COLOR color, uint count, window_manager* wm );
void draw_line( int x1, int y1, int x2, int y2, COLOR color, window_manager* wm );
void draw_rect( int x, int y, int xsize, int ysize, COLOR color, window_manager* wm );
void draw_frect( int x, int y, int xsize, int ysize, COLOR color, window_manager* wm );
void draw_hgradient( int x, int y, int xsize, int ysize, COLOR c, int t1, int t2, window_manager* wm );
void draw_vgradient( int x, int y, int xsize, int ysize, COLOR c, int t1, int t2, window_manager* wm );
void draw_corners( int x, int y, int xsize, int ysize, int size, int len, COLOR c, window_manager* wm );
void draw_polygon( sundog_polygon* p, window_manager* wm );
int draw_char( utf32_char c, int x, int y, window_manager* wm );
void draw_string( const utf8_char* str, int x, int y, window_manager* wm );
void draw_string_wordwrap( const utf8_char* str, int x, int y, int xsize, int* out_xsize, int* out_ysize, bool dont_draw, window_manager* wm );
void draw_string_limited( const utf8_char* str, int x, int y, int limit, window_manager* wm ); //limit in utf8 chars
void draw_image_scaled( int dest_x, int dest_y, sundog_image_scaled* img, window_manager* wm );

int font_char_x_size( utf32_char c, int font, window_manager* wm );
int font_char_y_size( int font, window_manager* wm );
int font_string_x_size( const utf8_char* str, int font, window_manager* wm );
int font_string_x_size_limited( const utf8_char* str, int limit, int font, window_manager* wm ); //limit in utf8 chars
int char_x_size( utf32_char c, window_manager* wm );
int char_y_size( window_manager* wm );
int string_x_size( const utf8_char* str, window_manager* wm );
int string_x_size_limited( const utf8_char* str, int limit, window_manager* wm ); //limit in utf8 chars

//###################################
//## STANDARD WINDOW HANDLERS:     ##
//###################################

int null_handler( sundog_event* evt, window_manager* wm );
int desktop_handler( sundog_event* evt, window_manager* wm );
int divider_handler( sundog_event* evt, window_manager* wm );
void bind_divider_to( WINDOWPTR win, WINDOWPTR bind_to, int bind_num, window_manager* wm );
void set_divider_push( WINDOWPTR win, WINDOWPTR push_win, window_manager* wm );
void set_divider_vscroll_parameters( WINDOWPTR win, int min, int max, window_manager* wm );
int get_divider_vscroll_value( WINDOWPTR win, window_manager* wm );
int label_handler( sundog_event* evt, window_manager* wm );
int text_handler( sundog_event* evt, window_manager* wm );
void text_set_text( WINDOWPTR win, const utf8_char* text, window_manager* wm );
void text_set_cursor_position( WINDOWPTR win, int cur_pos, window_manager* wm );
void text_set_value( WINDOWPTR win, int val, window_manager* wm );
utf8_char* text_get_text( WINDOWPTR win, window_manager* wm );
int text_get_cursor_position( WINDOWPTR win, window_manager* wm );
int text_get_value( WINDOWPTR win, window_manager* wm );
void text_changed( WINDOWPTR win, window_manager* wm );
void text_set_zoom( WINDOWPTR win, int zoom, window_manager* wm );
void text_set_range( WINDOWPTR win, int min, int max );
void text_set_step( WINDOWPTR win, int step );
bool text_get_editing_state( WINDOWPTR win );
bool text_is_readonly( WINDOWPTR win );
#define BUTTON_FLAG_HANDLER_CALLED_FROM_TIMER	1
#define BUTTON_FLAG_AUTOBACKCOLOR		2
//Show current selected item (data->menu_val) of the popup menu:
#define BUTTON_FLAG_SHOW_VALUE			4 
#define BUTTON_FLAG_LEFT_ALIGNMENT_ON_OVERFLOW	8
int button_handler( sundog_event* evt, window_manager* wm );
void button_set_menu( WINDOWPTR win, const utf8_char* menu, window_manager* wm );
void button_set_menu_val( WINDOWPTR win, int val, window_manager* wm );
int button_get_menu_val( WINDOWPTR win, window_manager* wm );
void button_set_text( WINDOWPTR win, const utf8_char* text, window_manager* wm );
void button_set_text_color( WINDOWPTR win, COLOR c, window_manager* wm );
void button_set_text_opacity( WINDOWPTR win, uchar opacity, window_manager* wm );
int button_get_text_opacity( WINDOWPTR win, window_manager* wm );
void button_set_images( WINDOWPTR win, sundog_image_scaled* img1, sundog_image_scaled* img2 );
void button_set_flags( WINDOWPTR win, uchar flags );
uchar button_get_flags( WINDOWPTR win );
int button_get_evt_flags( WINDOWPTR win );
int button_get_optimal_xsize( const utf8_char* button_name, int font, bool smallest_as_possible, window_manager* wm );
#define LIST_ACTION_UPDOWN	1
#define LIST_ACTION_ENTER	2
#define LIST_ACTION_CLICK	3
#define LIST_ACTION_RCLICK	4
#define LIST_ACTION_DOUBLECLICK	5
#define LIST_ACTION_ESCAPE	6
int list_handler( sundog_event *evt, window_manager *wm );
list_data* list_get_data( WINDOWPTR win, window_manager *wm );
int list_get_last_action( WINDOWPTR win, window_manager *wm );
void list_select_item( WINDOWPTR win, int item_num, bool make_it_visible );
int list_get_selected_item( WINDOWPTR win );
int list_get_pressed( WINDOWPTR win );
enum scrollbar_hex
{
    scrollbar_hex_off,
    scrollbar_hex_scaled, //scaled: 0 (0%) ... 0x8000 (100%);
    scrollbar_hex_normal, //normal hex mode without show_offset;
    scrollbar_hex_normal_with_offset, //normal with show_offset;
};
#define SCROLLBAR_FLAG_INPUT	1
void draw_scrollbar_horizontal_selection( WINDOWPTR win, int x );
void draw_scrollbar_vertical_selection( WINDOWPTR win, int y );
int scrollbar_handler( sundog_event* evt, window_manager* wm );
void scrollbar_set_parameters( WINDOWPTR win, int cur, int max_value, int page_size, int step_size, window_manager* wm );
void scrollbar_set_value( WINDOWPTR win, int val, window_manager* wm );
int scrollbar_get_value( WINDOWPTR win, window_manager* wm );
int scrollbar_get_evt_flags( WINDOWPTR win );
int scrollbar_get_step( WINDOWPTR win );
void scrollbar_set_name( WINDOWPTR win, const utf8_char* name, window_manager* wm );
void scrollbar_set_values( WINDOWPTR win, const utf8_char* values, window_manager* wm );
void scrollbar_set_showing_offset( WINDOWPTR win, int offset, window_manager* wm );
void scrollbar_set_hex_mode( WINDOWPTR win, scrollbar_hex hex, window_manager* wm );
void scrollbar_set_normal_value( WINDOWPTR win, int normal_value, window_manager* wm );
void scrollbar_set_flags( WINDOWPTR win, uint flags );
uint scrollbar_get_flags( WINDOWPTR win );
void scrollbar_call_handler( WINDOWPTR win );
bool scrollbar_get_editing_state( WINDOWPTR win );
int keyboard_handler( sundog_event* evt, window_manager* wm );
void hide_keyboard_for_text_window( window_manager* wm );
void show_keyboard_for_text_window( WINDOWPTR text, window_manager* wm );
int files_handler( sundog_event* evt, window_manager* wm );
int dialog_handler( sundog_event* evt, window_manager* wm );
int popup_handler( sundog_event* evt, window_manager* wm );

extern bool g_color_theme_changed;
int colortheme_handler( sundog_event* evt, window_manager* wm );

int ui_scale_handler( sundog_event* evt, window_manager* wm );

int keymap_handler( sundog_event* evt, window_manager* wm );

#define PREFS_FLAG_NO_COLOR_THEME		1
#define PREFS_FLAG_NO_CONTROL_TYPE		2
#define PREFS_FLAG_NO_KEYMAP			4
int prefs_ui_handler( sundog_event* evt, window_manager* wm );
int prefs_video_handler( sundog_event* evt, window_manager* wm );
int prefs_audio_handler( sundog_event* evt, window_manager* wm );
int prefs_handler( sundog_event* evt, window_manager* wm );

//###################################
//## DIALOGS & BUILT-IN WINDOWS:   ##
//###################################

const utf8_char* wm_get_string( wm_string str_id );
//mask (file formats) example: "xm/mod/it" (or NULL for all files)
utf8_char* dialog_open_file( const utf8_char* name, const utf8_char* mask, const utf8_char* id, const utf8_char* def_filename, window_manager* wm );
int dialog_overwrite( utf8_char* filename, window_manager* wm ); //0 - YES; 1 - NO;
int dialog( const utf8_char* name, const utf8_char* buttons, window_manager* wm );
int popup_menu( const utf8_char* name, const utf8_char* items, int x, int y, COLOR color, window_manager* wm );
void prefs_clear( window_manager* wm );
void prefs_add_sections( const utf8_char** names, void** handlers, int num, window_manager* wm );
void prefs_add_default_sections( window_manager* wm );
void prefs_open( void* host, window_manager* wm );
void colortheme_open( window_manager* wm );
void ui_scale_open( window_manager* wm );
void keymap_open( window_manager* wm );

//###################################
//## DEVICE DEPENDENT:             ##
//###################################

#ifdef OPENGL
//Thread safe:
int gl_lock( window_manager* wm );
void gl_unlock( window_manager* wm );
int gl_init( window_manager* wm );
void gl_deinit( window_manager* wm );
void gl_resize( window_manager* wm );
void gl_draw_points( int16* coord2d, COLOR color, int count, window_manager* wm );
void gl_draw_triangles( int16* coord2d, COLOR color, int count, window_manager* wm );
void gl_draw_polygon( sundog_polygon* p, window_manager* wm );
void gl_draw_image_scaled( 
    int dest_x, int dest_y, 
    int dest_xs, int dest_ys,
    float src_x, float src_y,
    float src_xs, float src_ys,
    sundog_image* img,
    window_manager* wm );
void gl_draw_line( int x1, int y1, int x2, int y2, COLOR color, window_manager* wm );
void gl_draw_frect( int x, int y, int xsize, int ysize, COLOR color, window_manager* wm );
void gl_draw_image(
    int dest_x, int dest_y,
    int dest_xs, int dest_ys,
    int src_x, int src_y,
    sundog_image* img,
    window_manager* wm );
//Not thread safe:
void gl_bind_framebuffer( GLuint fb, window_manager* wm );
void gl_set_default_viewport( window_manager* wm );
void gl_print_shader_info_log( GLuint shader );
void gl_print_program_info_log( GLuint program );
GLuint gl_make_shader( const utf8_char* shader_source, GLenum type );
GLuint gl_make_program( GLuint vertex_shader, GLuint fragment_shader );
gl_program_struct* gl_program_new( GLuint vertex_shader, GLuint fragment_shader );
void gl_program_remove( gl_program_struct* p );
void gl_init_uniform( gl_program_struct* prog, int n, const utf8_char* name );
void gl_init_attribute( gl_program_struct* prog, int n, const utf8_char* name );
void gl_enable_attributes( gl_program_struct* prog, uint attr );
void gl_program_reset( window_manager* wm );
#else
WM_INLINE int gl_lock( window_manager* wm ) { return 0; }
#define gl_unlock( wm ) {}
#define gl_resize( wm ) {}
#endif
