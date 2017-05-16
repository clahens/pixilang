#pragma once                          

#define WM_EVENTS	64 //must be 2/4/8/16/32...
#define WM_TIMERS	16
#define WM_FONTS	2
#define WM_TOUCHES	10
#define WM_TOUCH_EVENTS	16

#define WM_INLINE 	inline
#define WM_TOUCH_ID	size_t

#ifdef UNIX
    #include <pthread.h>
#endif

#ifdef OPENGL
    #ifndef OPENGLES
	#ifdef UNIX
	    //Include GL >1.1 entry points:
	    #define GL_GLEXT_PROTOTYPES
        #else
    	    //Windows OpenGL library only exposes OpenGL 1.1 entry points, 
    	    //so all functions beyond that version are loaded with wglGetProcAddress.
        #endif
    #endif
    #ifdef IPHONE
	#include <OpenGLES/ES2/gl.h>
    	#include <OpenGLES/ES2/glext.h>
    #endif
    #ifdef OSX
	#include <OpenGL/gl.h>
	#include <OpenGL/glext.h>
        #include <OpenGL/glu.h>
        #include <OpenGL/OpenGL.h>
    #endif
    #if defined(LINUX) || defined(FREEBSD) || defined(WIN)
	#ifdef OPENGLES
	    #include <GLES2/gl2.h>
	    #include <GLES2/gl2ext.h>
	    #include <EGL/egl.h>
	#else
	    #include <GL/gl.h>
	    #include <GL/glext.h>
	    #include <GL/glu.h>
	#endif
    #endif
    #ifdef WIN
	//Framebuffer:
	extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
        extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
	extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
	extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
	//Shaders:
	extern PFNGLATTACHSHADERPROC glAttachShader;
	extern PFNGLCOMPILESHADERPROC glCompileShader;
	extern PFNGLCREATEPROGRAMPROC glCreateProgram;
	extern PFNGLCREATESHADERPROC glCreateShader;
	extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
	extern PFNGLDELETESHADERPROC glDeleteShader;
	extern PFNGLDETACHSHADERPROC glDetachShader;
	extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
	extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
	extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
	extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
	extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
	extern PFNGLGETSHADERIVPROC glGetShaderiv;
	extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
	extern PFNGLLINKPROGRAMPROC glLinkProgram;
	extern PFNGLSHADERSOURCEPROC glShaderSource;
	extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
	extern PFNGLUSEPROGRAMPROC glUseProgram;
	extern PFNGLUNIFORM1FPROC glUniform1f;
	extern PFNGLUNIFORM2FPROC glUniform2f;
	extern PFNGLUNIFORM3FPROC glUniform3f;
	extern PFNGLUNIFORM4FPROC glUniform4f;
	extern PFNGLUNIFORM1IPROC glUniform1i;
	extern PFNGLUNIFORM2IPROC glUniform2i;
	extern PFNGLUNIFORM3IPROC glUniform3i;
	extern PFNGLUNIFORM4IPROC glUniform4i;
	extern PFNGLUNIFORM1FVPROC glUniform1fv;
	extern PFNGLUNIFORM2FVPROC glUniform2fv;
	extern PFNGLUNIFORM3FVPROC glUniform3fv;
	extern PFNGLUNIFORM4FVPROC glUniform4fv;
	extern PFNGLUNIFORM1IVPROC glUniform1iv;
	extern PFNGLUNIFORM2IVPROC glUniform2iv;
	extern PFNGLUNIFORM3IVPROC glUniform3iv;
	extern PFNGLUNIFORM4IVPROC glUniform4iv;
	extern PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
	extern PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
	extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
	extern PFNGLVALIDATEPROGRAMPROC glValidateProgram;
	extern PFNGLVERTEXATTRIB1DPROC glVertexAttrib1d;
	extern PFNGLVERTEXATTRIB1DVPROC glVertexAttrib1dv;
	extern PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f;
	extern PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv;
	extern PFNGLVERTEXATTRIB1SPROC glVertexAttrib1s;
	extern PFNGLVERTEXATTRIB1SVPROC glVertexAttrib1sv;
	extern PFNGLVERTEXATTRIB2DPROC glVertexAttrib2d;
	extern PFNGLVERTEXATTRIB2DVPROC glVertexAttrib2dv;
	extern PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f;
	extern PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv;
	extern PFNGLVERTEXATTRIB2SPROC glVertexAttrib2s;
	extern PFNGLVERTEXATTRIB2SVPROC glVertexAttrib2sv;
	extern PFNGLVERTEXATTRIB3DPROC glVertexAttrib3d;
	extern PFNGLVERTEXATTRIB3DVPROC glVertexAttrib3dv;
	extern PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f;
	extern PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv;
	extern PFNGLVERTEXATTRIB3SPROC glVertexAttrib3s;
	extern PFNGLVERTEXATTRIB3SVPROC glVertexAttrib3sv;
	extern PFNGLVERTEXATTRIB4NBVPROC glVertexAttrib4Nbv;
	extern PFNGLVERTEXATTRIB4NIVPROC glVertexAttrib4Niv;
	extern PFNGLVERTEXATTRIB4NSVPROC glVertexAttrib4Nsv;
	extern PFNGLVERTEXATTRIB4NUBPROC glVertexAttrib4Nub;
	extern PFNGLVERTEXATTRIB4NUBVPROC glVertexAttrib4Nubv;
	extern PFNGLVERTEXATTRIB4NUIVPROC glVertexAttrib4Nuiv;
	extern PFNGLVERTEXATTRIB4NUSVPROC glVertexAttrib4Nusv;
	extern PFNGLVERTEXATTRIB4BVPROC glVertexAttrib4bv;
	extern PFNGLVERTEXATTRIB4DPROC glVertexAttrib4d;
	extern PFNGLVERTEXATTRIB4DVPROC glVertexAttrib4dv;
	extern PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
	extern PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;
	extern PFNGLVERTEXATTRIB4IVPROC glVertexAttrib4iv;
	extern PFNGLVERTEXATTRIB4SPROC glVertexAttrib4s;
	extern PFNGLVERTEXATTRIB4SVPROC glVertexAttrib4sv;
	extern PFNGLVERTEXATTRIB4UBVPROC glVertexAttrib4ubv;
	extern PFNGLVERTEXATTRIB4UIVPROC glVertexAttrib4uiv;
	extern PFNGLVERTEXATTRIB4USVPROC glVertexAttrib4usv;
	extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
	extern PFNGLACTIVETEXTUREPROC glActiveTexture;
    #endif
#endif

#ifdef X11
    #ifdef OPENGL
	#ifndef OPENGLES
	    #include <GL/glx.h>
	#endif
    #endif
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/Xatom.h>
    #include <X11/keysym.h>
    #include <X11/XKBlib.h>
#ifdef MULTITOUCH
    #include <X11/extensions/XInput.h>
    #include <X11/extensions/XInput2.h>
#endif
#endif

#if defined(DIRECTDRAW) && defined(UNIX)
    #include "SDL/SDL.h"
#endif

#if defined(DIRECTDRAW) && defined(WIN)
    #include "ddraw.h"
#endif

#ifdef COLOR8BITS
    #define COLOR	uchar
    #define COLORSIGNED	signed char
    #define COLORPTR	uchar*
    #define COLORLEN	(int)1
    #define COLORBITS	8
    #define COLORMASK	0xFF
    #define CLEARCOLOR	0
#endif
#ifdef COLOR16BITS
    #define COLOR	uint16
    #define COLORSIGNED	int16
    #define COLORPTR  	uint16*
    #define COLORLEN  	(int)2
    #define COLORBITS 	16
    #define COLORMASK 	0xFFFF
    #define CLEARCOLOR 	0
#endif
#ifdef COLOR32BITS
    #define COLOR     	uint
    #define COLORSIGNED	int
    #define COLORPTR  	uint*
    #define COLORLEN  	(int)4
    #define COLORBITS 	32
    #define COLORMASK 	0x00FFFFFF
    #define CLEARCOLOR 	0x00000000
#endif

#ifdef OPENGL
    #define SCREEN_ZOOM_SUPPORTED 1
#endif
#if defined(DIRECTDRAW) && defined(UNIX)
    #define SCREEN_ZOOM_SUPPORTED 1    
    #define SCREEN_ROTATE_SUPPORTED 1
#endif
#ifdef WINCE
    #define SCREEN_ZOOM_SUPPORTED 1
    #define SCREEN_ROTATE_SUPPORTED 1
    enum
    {
	VIDEODRIVER_NONE = 0,
        VIDEODRIVER_GAPI,
    	VIDEODRIVER_RAW,
	VIDEODRIVER_DDRAW,
        VIDEODRIVER_GDI
    };
#endif
#ifdef ANDROID
    #define SCREEN_ROTATE_SUPPORTED 1
#endif

//WM strings:
enum wm_string
{
    STR_WM_OK,
    STR_WM_OKCANCEL,
    STR_WM_CANCEL,
    STR_WM_YESNO,
    STR_WM_YES_CAP,
    STR_WM_NO_CAP,
    STR_WM_ENABLED_CAP,
    STR_WM_DISABLED_CAP,
    STR_WM_CLOSE,
    STR_WM_RESET,
    STR_WM_RESET_ALL,
    STR_WM_LOAD,
    STR_WM_SAVE,
    STR_WM_INFO,
    STR_WM_AUTO,
    STR_WM_FIND,
    STR_WM_EDIT,
    STR_WM_NEW,
    STR_WM_DELETE,
    STR_WM_RENAME,
    STR_WM_RENAME_FILE,
    STR_WM_CUT,
    STR_WM_CUT2,
    STR_WM_COPY,
    STR_WM_COPY2,
    STR_WM_PASTE,
    STR_WM_PASTE2,
    STR_WM_CREATE_DIR,
    STR_WM_DELETE_DIR,
    STR_WM_RECURS,
    STR_WM_ERROR,
    STR_WM_NOT_FOUND,

    STR_WM_MS,
    STR_WM_SEC,
    STR_WM_HZ,
    STR_WM_INCH,
    STR_WM_DECIBEL,
    STR_WM_BIT,
    STR_WM_BYTES,
    
    STR_WM_DEMOVERSION,
    
    STR_WM_PREFERENCES,
    STR_WM_PREFS_CHANGED,
    STR_WM_INTERFACE,
    STR_WM_AUDIO,
    STR_WM_VIDEO,
    STR_WM_CAMERA,
    STR_WM_BACK_CAM,
    STR_WM_FRONT_CAM,
    STR_WM_MAXFPS,
    STR_WM_ANGLE,
    STR_WM_DOUBLE_CLICK_TIME,
    STR_WM_CTL_TYPE,
    STR_WM_CTL_FINGERS,
    STR_WM_CTL_PEN,
    STR_WM_SHOW_KBD,
    STR_WM_WINDOW_PARS,
    STR_WM_WINDOW_WIDTH,
    STR_WM_WINDOW_HEIGHT,
    STR_WM_WINDOW_FULLSCREEN,
    STR_WM_SET_COLOR_THEME,
    STR_WM_SET_UI_SCALE,
    STR_WM_SHORTCUTS_SHORT,
    STR_WM_SHORTCUTS,
    STR_WM_UI_SCALE,
    STR_WM_COLOR_THEME,
    STR_WM_COLOR_THEME_MSG_RESTART,
    STR_WM_LANG,
    STR_WM_DRIVER,
    STR_WM_OUTPUT,
    STR_WM_INPUT,
    STR_WM_BUFFER,
    STR_WM_FREQ,
    STR_WM_DEVICE,
    STR_WM_INPUT_DEVICE,
    STR_WM_BUFFER_SIZE,
    STR_WM_ASIO_OPTIONS,
    STR_WM_FIRST_OUT_CH,
    STR_WM_FIRST_IN_CH,
    STR_WM_OPTIONS,
    STR_WM_MORE_OPTIONS,
    STR_WM_CUR_DRIVER,
    STR_WM_CUR_FREQ,
    STR_WM_CUR_LATENCY,
    STR_WM_ACTION,
    STR_WM_SHORTCUTS2,
    STR_WM_SHORTCUT,
    STR_WM_RESET_TO_DEF,
    STR_WM_ENTER_SHORTCUT,
    STR_WM_AUTO_SCALE,
    STR_WM_PPI,
    STR_WM_BUTTON_SCALE,
    STR_WM_FONT_SCALE,
    STR_WM_BUTTON,

    STR_WM_DISABLED_ENABLED_MENU,
    STR_WM_AUTO_YES_NO_MENU,
    STR_WM_CTL_TYPE_MENU,
    
    STR_WM_FILE_NAME,
    STR_WM_FILE_PATH,
    STR_WM_FILE_MSG_NONAME,
    STR_WM_FILE_OVERWRITE,
};

#define CW 				COLORMASK
#define CB 				CLEARCOLOR

#define WIN_INIT_FLAG_SCALABLE	    	( 1 << 0 )
#define WIN_INIT_FLAG_NOBORDER	    	( 1 << 1 )
#define WIN_INIT_FLAG_FULLSCREEN    	( 1 << 2 )
#define WIN_INIT_FLAG_FULL_CPU_USAGE	( 1 << 3 )
#define WIN_INIT_FLAGOFF_ANGLE		4
#define WIN_INIT_FLAG_TOUCHCONTROL	( 1 << 6 )
#define WIN_INIT_FLAG_OPTIMIZE_MOVE_EVENTS	( 1 << 7 )
#define WIN_INIT_FLAG_NOSCREENROTATE	( 1 << 8 )
#define WIN_INIT_FLAG_LANDSCAPE_ONLY	( 1 << 9 )
#define WIN_INIT_FLAG_PORTRAIT_ONLY     ( 1 << 10 )
#define WIN_INIT_FLAG_NOCURSOR		( 1 << 11 )
#define WIN_INIT_FLAG_VSYNC		( 1 << 12 )
#define WIN_INIT_FLAG_FRAMEBUFFER	( 1 << 13 )
#define WIN_INIT_FLAG_NOWINDOW		( 1 << 14 )
#define WIN_INIT_FLAGOFF_ZOOM           15

enum 
{
    EVT_NULL = 0,
    EVT_GETDATASIZE,
    EVT_AFTERCREATE,
    EVT_BEFORECLOSE,
    EVT_BEFORESHOW,
    EVT_DRAW,
    EVT_FRAME,
    EVT_FOCUS,
    EVT_UNFOCUS, //When user click on other window
    EVT_MOUSEBUTTONDOWN,
    EVT_MOUSEBUTTONUP,
    EVT_MOUSEMOVE,
    EVT_TOUCHBEGIN,
    EVT_TOUCHEND,
    EVT_TOUCHMOVE,
    EVT_BUTTONDOWN,
    EVT_BUTTONUP,
    EVT_SCREENRESIZE, //After the main screen resize
    EVT_SCREENFOCUS,
    EVT_SCREENUNFOCUS,
    EVT_CLOSEREQUEST, //Close Window request. Can be ignored
    EVT_QUIT, //Close Application request. E.g. when user click Main Window Close button. Can be ignored
              //Available not for all systems.
    EVT_USERDEFINED1
};

//Event flags:
#define EVT_FLAG_SHIFT 			( 1 << 0 )
#define EVT_FLAG_CTRL 			( 1 << 1 )
#define EVT_FLAG_ALT 			( 1 << 2 )
#define EVT_FLAG_MODE 			( 1 << 3 )
#define EVT_FLAG_CMD			( 1 << 4 )
#define EVT_FLAG_MODS			( EVT_FLAG_SHIFT | EVT_FLAG_CTRL | EVT_FLAG_ALT | EVT_FLAG_MODE | EVT_FLAG_CMD )
#define EVT_FLAG_DOUBLECLICK		( 1 << 5 )
#define EVT_FLAG_DONTDRAW		( 1 << 6 )
#define EVT_FLAG_HANDLING		( 1 << 7 )
#define EVT_FLAGS_NUM			7

//Mouse buttons:
#define MOUSE_BUTTON_LEFT 		1
#define MOUSE_BUTTON_MIDDLE 		2
#define MOUSE_BUTTON_RIGHT 		4
#define MOUSE_BUTTON_SCROLLUP 		8
#define MOUSE_BUTTON_SCROLLDOWN 	16

//Virtual key codes (ASCII):
//(scancode and unicode may be set for each virtual key)
// 00      : empty;
// 01 - 1F : control codes;
// 20 - 7E : standart ASCII symbols (all letters are small - from 61);
// 7F      : not defined;
// 80 - xx : additional key codes
#define KEY_BACKSPACE   		0x08
#define KEY_TAB         		0x09
#define KEY_ENTER       		0x0D
#define KEY_ESCAPE      		0x1B
#define KEY_SPACE       		0x20

//Additional key codes:
enum 
{
    KEY_F1 = 0x80,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_INSERT,
    KEY_DELETE,
    KEY_HOME,
    KEY_END,
    KEY_PAGEUP,
    KEY_PAGEDOWN,
    KEY_CAPS,
    KEY_MIDI_NOTE,
    KEY_MIDI_CTL,
    KEY_MIDI_NRPN,
    KEY_MIDI_RPN,
    KEY_MIDI_PROG,
    KEY_SHIFT,
    KEY_CTRL,
    KEY_ALT,
    KEY_MENU,
    KEY_CMD,
    KEY_FN,
    KEY_UNKNOWN, //virtual key code = code - KEY_UNKNOWN
};

//Scroll flags:
#define SF_LEFT         		1
#define SF_RIGHT        		2
#define SF_UP           		4
#define SF_DOWN         		8
#define SF_STATICWIN    		16

#define WINDOWPTR			sundog_window*
#define WINDOW				sundog_window

#define DECOR_FLAG_CENTERED		( 1 << 0 )
#define DECOR_FLAG_CHECK_SIZE		( 1 << 1 )
#define DECOR_FLAG_STATIC		( 1 << 2 )
#define DECOR_FLAG_NOBORDER		( 1 << 3 )
#define DECOR_FLAG_NOHEADER		( 1 << 4 )
#define DECOR_FLAG_NORESIZE		( 1 << 5 )
#define DECOR_FLAG_FULLSCREEN		( 1 << 6 )
#define DECOR_FLAG_WITH_CLOSE		( 1 << 7 )
#define DECOR_FLAG_WITH_MINIMIZE	( 1 << 8 )

#define BORDER_OPACITY			32
#define BORDER_COLOR_WITHOUT_OPACITY	wm->color3
#define PUSHED_OPACITY			40
#define PUSHED_COLOR_WITHOUT_OPACITY	wm->color3
#define PUSHED_COLOR( orig ) 		blend( orig, PUSHED_COLOR_WITHOUT_OPACITY, PUSHED_OPACITY )
#define BORDER_COLOR( orig )		blend( orig, BORDER_COLOR_WITHOUT_OPACITY, BORDER_OPACITY )
#define SELECTED_BUTTON_COLOR		blend( wm->button_color, wm->selection_color, 80 )
#define LABEL_OPACITY			128

#define SCROLLBAR_SIZE_COEFF		0.245
#define BIG_BUTTON_YSIZE_COEFF 		0.27
#define TEXT_YSIZE_COEFF		0.18

#define DEFAULT_DOUBLE_CLICK_TIME	200

typedef size_t                          WCMD;
#define CWIN		    		(WCMD)30000
#define CX1		    		(WCMD)30001
#define CX2		    		(WCMD)30002
#define CY1		    		(WCMD)30003
#define CY2		    		(WCMD)30004
#define CXSIZE		    		(WCMD)30005
#define CYSIZE		    		(WCMD)30006
#define CSUB		    		(WCMD)30007
#define CADD		    		(WCMD)30008
#define CPERC		    		(WCMD)30009
#define CBACKVAL0	    		(WCMD)30010
#define CBACKVAL1	    		(WCMD)30011
#define CMAXVAL		    		(WCMD)30012
#define CMINVAL		    		(WCMD)30013
#define CMULDIV256			(WCMD)30014
#define CPUTR0		    		(WCMD)30015
#define CGETR0		    		(WCMD)30016
#define CR0		    		(WCMD)30017
#define CEND		    		(WCMD)30018

#define IMAGE_NATIVE_RGB		1
#define IMAGE_ALPHA8			2
#define IMAGE_STATIC_SOURCE		4
#define IMAGE_INTERPOLATION		8
#define IMAGE_CLEAN			16
#define IMAGE_NO_REPEAT			32

struct window_manager;
struct sundog_timer;
struct sundog_window;
struct sundog_event;

struct sundog_image
{
    window_manager* 	wm;
    int			xsize;
    int			ysize;
    int			int_xsize; //internal xsize; may be != xsize; OpenGL texture size, for example.
    int			int_ysize; //internal ysize...
    void*		data;
    COLOR		color;
    COLOR		backcolor;
    uchar		opacity; //0..255
#ifdef OPENGL
    unsigned int	gl_texture_id;
#endif
    uint		flags;
};

struct sundog_image_scaled
{
    sundog_image*	img;
    int 		src_x; //fixed point (IMG_PREC)
    int 		src_y; //fixed point
    int 		src_xsize; //fixed point
    int			src_ysize; //fixed point
    int 		dest_xsize;
    int 		dest_ysize;
};

#define KEYMAP_BIND_DEFAULT 		1
#define KEYMAP_BIND_OVERWRITE		2
#define KEYMAP_BIND_RESET_TO_DEFAULT	4
#define KEYMAP_ACTION_KEYS		4

struct sundog_keymap_key
{
    int16			key;
    uint16			flags;
    uint			pars1;
    uint			pars2;
};

struct sundog_keymap_action
{
    const utf8_char*		name;
    const utf8_char*		id;
    sundog_keymap_key		keys[ KEYMAP_ACTION_KEYS ];
    sundog_keymap_key		default_keys[ KEYMAP_ACTION_KEYS ];
};

struct sundog_keymap_section
{
    const utf8_char*		name;
    const utf8_char*		id;
    sundog_keymap_action*	actions;
    bsymtab*			bindings;
};

struct sundog_keymap
{
    bool 			silent;
    sundog_keymap_section*	sections;
    uint			midi_notes[ 128 / ( 8 * sizeof( uint ) ) ];
};

#define WBD_FLAG_ONE_COLOR		1
#define WBD_FLAG_ONE_OPACITY		2

struct sundog_vertex
{
    int16 x;
    int16 y;
    COLOR c;
    uchar t;
};

struct sundog_polygon
{
    int vnum;
    sundog_vertex* v;
};

#define WIN_FLAG_ALWAYS_INVISIBLE	( 1 << 0 )
#define WIN_FLAG_ALWAYS_ON_TOP		( 1 << 1 )
#define WIN_FLAG_ALWAYS_UNFOCUSED	( 1 << 2 )
#define WIN_FLAG_TRASH			( 1 << 3 )
#define WIN_FLAG_DOUBLECLICK		( 1 << 4 )
#define WIN_FLAG_DONT_USE_CONTROLLERS	( 1 << 5 )
#define WIN_FLAG_UNFOCUS_HANDLING	( 1 << 6 )
#define WIN_FLAG_FOCUS_HANDLING		( 1 << 7 )
#define WIN_FLAG_ALWAYS_HANDLE_DRAW_EVT	( 1 << 8 )

typedef int (*win_handler_t)( sundog_event*, window_manager* );
typedef int (*win_action_handler_t)( void*, WINDOWPTR, window_manager* );

struct sundog_window
{
    window_manager*	wm;
    bool		visible;
    uint16	    	flags;
    uint16	    	id;
    const utf8_char*	name; //window name
    int16	    	x, y; //x,y (relative)
    int16	    	screen_x, screen_y; //real x,y (global screen coordinates)
    int16	    	xsize, ysize; //window size
    COLOR	    	color;
    int	    		font;
    MWCLIPREGION*	reg;
    WINDOWPTR	    	parent;
    void*		host; //hosting object - engine or some window; parent != host!
    WINDOWPTR*		childs;
    int		    	childs_num;
    win_handler_t 	win_handler;
    void*		data;

    WCMD*		x1com; //Controller of the window coordinate x1 - sequence of commands for the controller VM (cvm)
    WCMD*		y1com; //...
    WCMD*		x2com; //...
    WCMD*		y2com; //...
    int16	    	controllers_defined;
    int16	    	controllers_calculated;

    win_action_handler_t action_handler;
    int		    	action_result;
    void*		handler_data;

    ticks_hr_t	    	click_time;
};

//SunDog event handling:
// EVENT 
//   |
// user_event_handler()
//   | (if not handled)
// window 
//   | (if not handled)
// window children
//   | (if not handled)
// handler_of_unhandled_events()

struct sundog_event
{
    uint16          	type; //event type
    uint16	    	flags;
    ticks_hr_t	   	time;
    WINDOWPTR	    	win;
    int16           	x; //x OR midi parameter (note/ctl)
    int16           	y; //y OR midi channel
    uint16	    	key; //virtual key code (ASCII 0..127) OR additional key code (see KEY_xxx defines)
    uint16	    	scancode; //device dependent OR touch count for multitouch devices
    uint16	    	pressure; //key pressure (0..1024)
    uint16	    	reserved;
    utf32_char	    	unicode; //unicode if possible, or zero
};

struct sundog_timer
{
    void            	(*handler)( void*, sundog_timer*, window_manager* );
    void*		data;
    ticks_t	    	deadline;
    ticks_t	    	delay;
    int			id;
};

enum
{
    DIALOG_ITEM_NONE = 0,
    DIALOG_ITEM_NUMBER,
    DIALOG_ITEM_NUMBER_HEX,
    DIALOG_ITEM_SLIDER,
    DIALOG_ITEM_TEXT,
    DIALOG_ITEM_LABEL,
    DIALOG_ITEM_POPUP,
};

#define DIALOG_ITEM_FLAG_FOCUS		( 1 << 0 )

#define DIALOG_ITEM_TEXT_SIZE 		wm->text_ysize
#define DIALOG_ITEM_NUMBER_SIZE 	DIALOG_ITEM_TEXT_SIZE
#define DIALOG_ITEM_SLIDER_SIZE 	wm->controller_ysize
#define DIALOG_ITEM_LABEL_SIZE 		font_char_y_size( win->font, wm )
#define DIALOG_ITEM_POPUP_SIZE 		wm->popup_button_ysize

struct dialog_item
{
    int			type;
    int			min;
    int			max;
    int			int_val;
    int			normal_val;
    utf8_char*		str_val; //DIALOG_ITEM_TEXT will remove (with bmem_free()) this string and create the new one
    const utf8_char*	menu;
    uint		flags;
    
    WINDOWPTR		win;
};

#ifdef OPENGL
enum {
    GL_PROG_ATT_POSITION = 0,
    GL_PROG_ATT_COLOR,
    GL_PROG_ATT_TEX_COORD,
    GL_PROG_ATT_MAX
};
enum {
    GL_PROG_UNI_TRANSFORM1 = 0,
    GL_PROG_UNI_TRANSFORM2,
    GL_PROG_UNI_COLOR,
    GL_PROG_UNI_TEXTURE,
    GL_PROG_UNI_MAX
};
struct gl_program_struct
{
    GLuint		program;
    GLint		attributes[ GL_PROG_ATT_MAX ]; //Vertex attributes
    GLint		uniforms[ GL_PROG_UNI_MAX ]; //Global parameters for all vertex and fragment shaders
    uint		attributes_enabled; //Enabled attributes bits: ( 1 << GL_PROG_ATT_POSITION ), etc.
    int			transform_counter;
};
#endif

enum files_preview_status
{
    FPREVIEW_NONE = 0,
    FPREVIEW_OPEN,
    FPREVIEW_CLOSE,
    FPREVIEW_FILE_SELECTED,
};
struct files_preview_data //Zero-filled by default (after the file dialog window creation)
{
    files_preview_status status;
    utf8_char* name;
    WINDOWPTR win;
    void* user_data;
    int user_pars[ 16 ];
};

//Control type: 0 - mouse, pen, stylus; 1 - touch/multitouch screen.
#define PENCONTROL			0
#define TOUCHCONTROL			1

typedef void (*tdevice_event_handler)( window_manager* );
typedef int (*tdevice_start)( const utf8_char* windowname, int xsize, int ysize, window_manager* wm ); //before main loop
typedef void (*tdevice_end)( window_manager* ); //after main loop
typedef void (*tdevice_draw_line)( int x1, int y1, int x2, int y2, COLOR color, window_manager* wm );
typedef void (*tdevice_draw_frect)( int x, int y, int xsize, int ysize, COLOR color, window_manager* wm );
typedef void (*tdevice_draw_image)( 
    int dest_x, int dest_y,
    int dest_xs, int dest_ys,
    int src_x, int src_y,
    sundog_image* img,
    window_manager* wm );
typedef void (*tdevice_screen_lock)( WINDOWPTR win, window_manager* wm );
typedef void (*tdevice_screen_unlock)( WINDOWPTR win, window_manager* wm );
typedef void (*tdevice_vsync)( bool vsync, window_manager* wm );
typedef void (*tdevice_redraw_framebuffer)( window_manager* wm );

struct window_manager
{
    sundog_engine*	sd; //Parent SunDog engine
    
    volatile int       	wm_initialized;
    bool		suspended; //App is inactive, WM suspended
    bool		restart_request;
    bool 	    	exit_request;

    uint	    	flags;
    
    volatile int       	events_count; //Number of events to execute
    int	        	current_event_num;
    sundog_event    	events[ WM_EVENTS ];
    bmutex    		events_mutex;
    sundog_event 	frame_evt;
    sundog_event 	empty_evt; //Fields that can be changed: win; type; time;
    
#ifdef MULTITOUCH
    sundog_event	touch_evts[ WM_TOUCH_EVENTS ];
    uint		touch_evts_cnt;
    WM_TOUCH_ID		touch_id[ WM_TOUCHES ];
    bool 		touch_busy[ WM_TOUCHES ];
#endif

    sundog_keymap*	km; //Default keymap (shortcuts)

    WINDOWPTR*		trash;
    uint	    	window_counter; //For ID calculation.
    WINDOWPTR	    	root_win;
    int			creg0; //Temp register for the window XY controller calculation.

    sundog_timer    	timers[ WM_TIMERS ];
    int		    	timers_num;
    int			timers_id_counter;

    int             	screen_xsize;
    int             	screen_ysize;
    int			screen_angle; //degrees = acreen_angle * 90
    bool		screen_angle_lock;
    int 		screen_zoom; //pixel size; don't use it - will be removed soon!
    int			screen_ppi; //pixels per inch
    float		screen_scale; //UI scale factor; normal = 1.0
    float		screen_font_scale; //font scale factor; normal = 1.0
    bool		screen_buffer_preserved; //TRUE - screen buffer will be preserved in the new frame 
						 // (for framebuffer modes and for single-buffer OpenGL);
						 //FALSE - can't preserve the screen content, so WM will redraw all windows on every frame.
    int		    	screen_lock_counter;
    bool            	screen_is_active;
    int			screen_changed;
    
    int			vcap; //Video capture status
    int 		vcap_in_fps;
    int 		vcap_in_bitrate_kb;
    uint		vcap_in_flags;
    utf8_char* 		vcap_in_name; //Encoding: final file name    
    
    utf8_char* 		status_message;
    WINDOWPTR 		status_window;
    int 		status_timer;
    
    int			color_theme;
    COLOR	    	color0;
    COLOR		color1;
    COLOR		color2;
    COLOR	    	color3;
    bool		color3_is_brighter;
    int			color1_darkness;
    COLOR	    	yellow;
    COLOR	    	green;
    COLOR	    	red;
    COLOR	    	blue;
    COLOR		header_text_color;
    COLOR		alternative_text_color;
    COLOR	    	dialog_color;
    COLOR	    	decorator_color;
    COLOR	    	decorator_border;
    COLOR	    	button_color;
    COLOR	    	menu_color;
    COLOR	    	selection_color;
    COLOR	    	text_background;
    COLOR	    	list_background;
    COLOR	    	scroll_color;
    COLOR	    	scroll_background_color;
    
    sundog_image*	texture0;

    int			control_type;
    int			double_click_time; //ms
    bool		show_virtual_keyboard;
    int                 max_fps; //Max FPS, provided by window manager. Less value - less CPU usage.
    bool		frame_event_request;
    
    int			normal_window_xsize;
    int			normal_window_ysize;
    int			large_window_xsize;
    int			large_window_ysize;
    int		    	decor_border_size;
    int		    	decor_header_size;
    int		    	scrollbar_size; //Scrollbar, divider, normal button
    int			small_button_xsize; //Smallest width of the button; at least one char of the biggest font
    int		    	button_xsize; //Big button
    int		    	button_ysize; //Big button
    int			popup_button_ysize; //Button with popup menu and visible current value; at least two lines of text
    int			controller_ysize; //Two text lines with the smallest font
    int			interelement_space;
    int			interelement_space2; //Smallest
    int			list_item_ysize;
    int			text_ysize; //Width of the text editor or the small button with text
    int			corners_size;
    int			corners_len;
    					  
    // WBD (Window Buffer Draw):
    WINDOWPTR		cur_window;
    bool		cur_window_invisible;
    COLOR*		screen_pixels; //Pixel screen buffer for direct read/write
    COLOR           	cur_font_color;
    int		    	cur_font_scale; //256 - normal; 512 - x2
    uchar		cur_opacity;
    uint		cur_flags;
    void*		points_array;
    sundog_vertex 	poly_vertices1[ 8 ];
    sundog_vertex 	poly_vertices2[ 8 ];

    sundog_image* 	font_img[ WM_FONTS ];
    int 		font_cxsize[ WM_FONTS ];
    int 		font_cysize[ WM_FONTS ];
    int			font_zoom; //Global font zoom
    int			default_font;

    int		    	pen_x;
    int		    	pen_y;
    WINDOWPTR	    	focus_win;
    WINDOWPTR	    	prev_focus_win;
    uint16	    	focus_win_id;
    uint16	    	prev_focus_win_id;
    WINDOWPTR	    	last_unfocused_window;

    WINDOWPTR	    	handler_of_unhandled_events;
    
    //Dialog windows data:
    int 		dialog_open; //"Dialog open" counter; 0 - no dialogs
    bool 		fdialog_open;
    utf8_char* 		fdialog_filename;
    utf8_char* 		fdialog_copy_file_name;  // 1:/dir/file.txt
    utf8_char* 		fdialog_copy_file_name2; // file.txt
    bool 		fdialog_cut_file_flag;
    WINDOWPTR 		prefs_win;
    const utf8_char* 	prefs_section_names[ 32 ];
    int 		prefs_sections;
    void* 		prefs_section_handlers[ 32 ];
    int			prefs_section_ysize;
    bool 		prefs_restart_request;
    uint		prefs_flags;
    WINDOWPTR		colortheme_win;
    WINDOWPTR		ui_scale_win;
    WINDOWPTR		keymap_win;
    WINDOWPTR		vk_win; //virtual keyboard
    
    //Window creation options:
    int 			opt_divider_vscroll_min;
    int 			opt_divider_vscroll_max;
    bool 			opt_divider_vertical;
    bool 			opt_divider_with_time;
    bool 			opt_text_no_virtual_keyboard;
    bool 			opt_text_call_handler_on_any_changes;
    bool 			opt_text_ro; //Read Only
    int 			opt_text_numeric;
    int 			opt_text_num_min;
    int 			opt_text_num_max;
    bool 			opt_text_num_hide_zero;
    bool 			opt_button_autorepeat;
    char 			opt_button_flat;
    int 			(*opt_button_end_handler)( void*, WINDOWPTR );
    void* 			opt_button_end_handler_data;
    uchar 			opt_button_flags;
    sundog_image_scaled        	opt_button_image1;
    sundog_image_scaled        	opt_button_image2;
    bool 			opt_scrollbar_vertical;
    bool 			opt_scrollbar_reverse;
    bool 			opt_scrollbar_compact;
    bool 			opt_scrollbar_flat;
    int 			(*opt_scrollbar_begin_handler)( void*, WINDOWPTR, int );
    int 			(*opt_scrollbar_end_handler)( void*, WINDOWPTR, int );
    int 			(*opt_scrollbar_opt_handler)( void*, WINDOWPTR, int ); //"options" (right mouse button for example)
    void* 			opt_scrollbar_begin_handler_data;
    void* 			opt_scrollbar_end_handler_data;
    void* 			opt_scrollbar_opt_handler_data;
    dialog_item* 		opt_dialog_items;
    const utf8_char* 		opt_dialog_buttons_text;
    const utf8_char* 		opt_dialog_text;
    int* 			opt_dialog_result_ptr;
    const utf8_char* 		opt_files_props;
    const utf8_char* 		opt_files_mask; //Example: "xm/mod/it" (or NULL for all files)
    const utf8_char* 		opt_files_preview;
    int 			(*opt_files_preview_handler)( files_preview_data*, window_manager* );
    void* 			opt_files_preview_user_data;
    const utf8_char* 		opt_files_user_button[ 4 ];
    int 			(*opt_files_user_button_handler[ 4 ])( void* user_data, WINDOWPTR win, window_manager* );
    void* 			opt_files_user_button_data[ 4 ];
    const utf8_char* 		opt_files_def_filename;
    bool 			opt_list_numbered;
    bool 			opt_list_without_scrollbar;
    bool 			opt_list_without_extensions;
    const utf8_char*		opt_popup_text;
    int*			opt_popup_exit_ptr;
    int*			opt_popup_result_ptr;
        
    COLORPTR		fb; //Pixel buffer for direct read/write
    int		    	fb_xpitch; //framebuffer xpitch
    int		    	fb_ypitch; //...
    int		    	fb_offset; //...
    int			fb_xsize;
    int			fb_ysize;
    int		    	real_window_width;
    int		    	real_window_height;

    //DEVICE DEPENDENT PART:
    
    //Device init/deinit:
    tdevice_event_handler	device_event_handler;
    tdevice_start		device_start; //before main loop
    tdevice_end			device_end; //after main loop
    //Device window drawing functions:
    tdevice_draw_line		device_draw_line;
    tdevice_draw_frect		device_draw_frect;
    tdevice_draw_image		device_draw_image;
    //...
    tdevice_screen_lock		device_screen_lock;
    tdevice_screen_unlock	device_screen_unlock;
    tdevice_vsync		device_vsync;
    tdevice_redraw_framebuffer	device_redraw_framebuffer;

#ifdef OPENGL
    bmutex        	gl_mutex;
    float		gl_xscale;
    float		gl_yscale;
    int			gl_transform_counter;
    float		gl_projection_matrix[ 4 * 4 ];
    gl_program_struct*	gl_current_prog;
    gl_program_struct*	gl_prog_solid;
    gl_program_struct*	gl_prog_gradient;
    gl_program_struct*	gl_prog_tex_alpha;
    gl_program_struct*	gl_prog_tex_rgb;
    void*		gl_points_array;
    bool		gl_no_points; //no GL_POINTS
    int16 		gl_array_s[ 16 ];
    uchar 		gl_array_c[ 16 ];
    float 		gl_array_f[ 16 ];
#endif //OPENGL

#if defined(DIRECTDRAW) && defined(WIN)
    LPDIRECTDRAWSURFACE	lpDDSPrimary; //DirectDraw primary surface (for win32).
    LPDIRECTDRAWSURFACE	lpDDSBack; //DirectDraw back surface (for win32).
#endif //DIRECTDRAW

#ifdef GDI
    uint           	gdi_bitmap_info[ 512 ]; //GDI bitmap info (for win32).
#endif //GDI

#ifdef WIN
    HDC	            	hdc; //Graphics content handler.
    HWND	    	hwnd;
    bool		ignore_mouse_evts;
    ticks_t		ignore_mouse_evts_before;
#ifdef OPENGL
    BOOL 		(WINAPI *wglSwapIntervalEXT)( int interval );
#endif
#endif //WIN

#ifdef WINCE
    int 		hires;
    int			vd; //video-driver
    char		rfb[ 32 ]; //raw framebuffer info
    HDC             	hdc; //graphics content handler
    HWND	    	hwnd;
    int		    	os_version;
    int		    	gx_suspended;
    int			raw_fb_xpitch;
    int			raw_fb_ypitch;
    void* 		wince_zoom_buffer;
#endif //WINCE

#ifdef UNIX
    pthread_mutex_t 	sdl_lock_mutex;
#ifdef DIRECTDRAW
    SDL_Surface*	sdl_screen;
    pthread_t 		sdl_evt_pth;
    int 		sdl_event_loop_sem;
    bthread 		sdl_frames_thread;
    volatile bool 	sdl_frames_thread_exit_request;
    bthread 		sdl_screen_control_thread;
    volatile bool 	sdl_screen_control_thread_exit_request;
    void* 		sdl_zoom_buffer;
    volatile int 	sdl_thread_finished;
    volatile int 	sdl_thread_initialized;
    uint 		sdl_wm_flags;
    const utf8_char* 	sdl_wm_windowname;
    int 		sdl_wm_xsize;
    int 		sdl_wm_ysize;
#endif
#ifdef X11
    Display*		dpy;
    Colormap	    	cmap;
    Window          	win;
    Visual*		win_visual;
    GC              	win_gc;
    XImage*		win_img;
    XImage*		win_img_back_pattern;
    char*		win_img_back_pattern_data;
    int 	    	win_img_depth;
    char*		win_buffer;
    int	            	win_depth;
#ifdef MULTITOUCH
    bool		xi; //X Input Extension
    int			xi_opcode;
    int			xi_event;
    int			xi_error;
#endif
#if defined(OPENGL) && !defined(OPENGLES)
    int 		(*glXSwapIntervalEXT)( Display* dpy, GLXDrawable drw, int interval );
    void		(*glXSwapIntervalMESA)( GLint );
    void		(*glXSwapIntervalSGI)( GLint );
#endif //OPENGL
#endif //X11
#endif //UNIX
};
