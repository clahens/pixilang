#ifndef __PIXILANG_HEADER__
#define __PIXILANG_HEADER__

//********************************************************************
// Main Pixilang Virtual Machine Configuration: **********************
//********************************************************************

#define PIXILANG_VERSION ( ( 3 << 24 ) | ( 6 << 16 ) | ( 0 << 8 ) | ( 0 << 0 ) )
#define PIXILANG_VERSION_STR "v3.6"

#define PIX_VM_THREADS		    16
#define PIX_VM_SYSTEM_THREADS	    3 /* threads-1 == audio; threads-2 == opengl; threads-3 == video capture callback */
#define PIX_VM_STACK_SIZE	    8192 /* number of PIX_VALs */
#define PIX_VM_EVENTS		    64

#define PIX_VM_FONTS		    8
#define PIX_VM_AUDIO_CHANNELS	    2

typedef int PIX_INT; //Pixilang integer type
typedef float PIX_FLOAT; //Pixilang floating point type
typedef unsigned int PIX_OPCODE; //VM opcode type
typedef int PIX_CID; //Container ID
typedef int PIX_ADDR; //Pixilang code address (instruction offset)
#define PIX_INT_BITS ( sizeof( PIX_INT ) * 8 )
#define PIX_FLOAT_BITS ( sizeof( PIX_FLOAT ) * 8 )

//#define PIX_INT64_ENABLED
//#define PIX_FLOAT64_ENABLED

#define PIX_OPCODE_BITS		    6
#define PIX_OPCODE_MASK		    ( ( 1 << PIX_OPCODE_BITS ) - 1 )

#define PIX_FN_BITS		    8
#define PIX_FN_MASK		    ( ( 1 << PIX_FN_BITS ) - 1 )

#define PIX_INT_MAX_POSITIVE	    ( (unsigned)( (PIX_INT)(-1) ) >> 1 )
#define PIX_INT_ADDRESS_MARKER	    (unsigned int)0xA0000000
#define PIX_INT_ADDRESS_MASK	    (unsigned int)0x0FFFFFFF
#define IS_ADDRESS_CORRECT( v )     ( ( ( v ) & ~PIX_INT_ADDRESS_MASK ) == PIX_INT_ADDRESS_MARKER )

//Fixed point precision for math operations:
#define PIX_FIXED_MATH_PREC	    12
#define PIX_TEX_FIXED_MATH_PREC	    15
#define PIX_AUDIO_PTR_PREC	    14
#define PIX_MAX_FIXED_MATH 	    ( 1 << ( PIX_INT_BITS - PIX_FIXED_MATH_PREC - 1 ) )

#define PIX_T_MATRIX_STACK_SIZE	    16

//********************************************************************
//********************************************************************
//********************************************************************

//Pixilang program format:
// address | instruction
// 0       | HALT
// 1       | start of the main user function 
// ...     | ...
// ...     | RET_i - return from the main function

//Pixilang stack structure:
// [ top of the stack ]
// ...
// function parameter X;
// ...
// function parameter 1;
// number of parameters; <============= current FP
// previous FP (frame pointer);
// previous PC (program counter);
// local variables;
// ...
// [ bottom of the stack - zero offset ]

//Pixilang transformation matrix:
// | 0  4  8  12 |
// | 1  5  9  13 |
// | 2  6  10 14 |
// | 3  7  11 15 |

union PIX_VAL
{
    PIX_INT i;
    PIX_FLOAT f;
    void* p;
};

//Pixilang VM opcodes:
// small letter - number is in the opcode body;
// V - variable (type = PIX_OPCODE);
// N - short number (type = PIX_OPCODE);
// I - PIX_INT (type = one or several PIX_OPCODEs);
// F - PIX_FLOAT (type = one or several PIX_OPCODEs).
enum pix_vm_opcode
{
    OPCODE_NOP = 0,

    OPCODE_HALT,

    OPCODE_PUSH_I,
    OPCODE_PUSH_i,
    OPCODE_PUSH_F,
    OPCODE_PUSH_v,

    //Size of these opcodes must be less or equal g_comp->statlist_header_size: *****
    OPCODE_GO,
    OPCODE_JMP_i,
    OPCODE_JMP_IF_FALSE_i,
    //*******************************************************************************

    OPCODE_SAVE_TO_VAR_v,

    OPCODE_SAVE_TO_PROP_I,
    OPCODE_LOAD_FROM_PROP_I,
    
    OPCODE_SAVE_TO_MEM,
    OPCODE_SAVE_TO_MEM_2D,
    OPCODE_LOAD_FROM_MEM,
    OPCODE_LOAD_FROM_MEM_2D,
    
    OPCODE_SAVE_TO_STACKFRAME_i,
    OPCODE_LOAD_FROM_STACKFRAME_i,

    OPCODE_SUB,
    OPCODE_ADD,
    OPCODE_MUL,
    OPCODE_IDIV,
    OPCODE_DIV,
    OPCODE_MOD,
    OPCODE_AND,
    OPCODE_OR,
    OPCODE_XOR,
    OPCODE_ANDAND,
    OPCODE_OROR,
    OPCODE_EQ,
    OPCODE_NEQ,
    OPCODE_LESS,
    OPCODE_LEQ,
    OPCODE_GREATER,
    OPCODE_GEQ,
    OPCODE_LSHIFT,
    OPCODE_RSHIFT,

    OPCODE_NEG,

    OPCODE_CALL_BUILTIN_FN,
    OPCODE_CALL_BUILTIN_FN_VOID,
    OPCODE_CALL_i,
    OPCODE_INC_SP_i,
    OPCODE_RET_i,
    OPCODE_RET_I,
    OPCODE_RET,
    
    NUMBER_OF_OPCODES
};

//Buildin functions:
enum
{
    //Containers (memory management):
    
    FN_NEW_PIXI = 0,
    FN_REMOVE_PIXI,
    FN_REMOVE_PIXI_WITH_ALPHA,
    FN_RESIZE_PIXI,
    FN_ROTATE_PIXI,
    FN_CLEAN_PIXI,
    FN_CLONE_PIXI,
    FN_COPY_PIXI,
    FN_GET_PIXI_SIZE,
    FN_GET_PIXI_XSIZE,
    FN_GET_PIXI_YSIZE,
    FN_GET_PIXI_ESIZE,
    FN_GET_PIXI_TYPE,
    FN_GET_PIXI_FLAGS,
    FN_SET_PIXI_FLAGS,
    FN_RESET_PIXI_FLAGS,
    FN_GET_PIXI_PROP,
    FN_SET_PIXI_PROP,
    FN_REMOVE_PIXI_PROPS,
    FN_CONVERT_PIXI_TYPE,
    FN_SHOW_MEM_DEBUG_MESSAGES,
    FN_ZLIB_PACK,
    FN_ZLIB_UNPACK,
    
    //Working with strings:
    
    FN_NUM_TO_STRING,
    FN_STRING_TO_NUM,

    //Working with strings (posix):

    FN_STRCAT,
    FN_STRCMP,
    FN_STRLEN,
    FN_STRSTR,
    FN_SPRINTF,
    FN_PRINTF,
    FN_FPRINTF,

    //Log management:

    FN_LOGF,
    FN_GET_LOG,
    FN_GET_SYSTEM_LOG,
    
    //Files:
    
    FN_LOAD,
    FN_FLOAD,
    FN_SAVE,
    FN_FSAVE,
    FN_GET_REAL_PATH,
    FN_NEW_FLIST,
    FN_REMOVE_FLIST,
    FN_GET_FLIST_NAME,
    FN_GET_FLIST_TYPE,
    FN_FLIST_NEXT,
    FN_GET_FILE_SIZE,
    FN_REMOVE_FILE,
    FN_RENAME_FILE,
    FN_COPY_FILE,
    FN_CREATE_DIRECTORY,
    FN_SET_DISK0,
    FN_GET_DISK0,

    //Files (posix):

    FN_FOPEN,
    FN_FOPEN_MEM,
    FN_FCLOSE,
    FN_FPUTC,
    FN_FPUTS,
    FN_FWRITE,
    FN_FGETC,
    FN_FGETS,
    FN_FREAD,
    FN_FEOF,
    FN_FFLUSH,
    FN_FSEEK,
    FN_FTELL,
    FN_SETXATTR,
    
    //Graphics:
    
    FN_FRAME,
    FN_VSYNC,
    FN_SET_PIXEL_SIZE,
    FN_GET_PIXEL_SIZE,
    FN_SET_SCREEN,
    FN_GET_SCREEN,
    FN_SET_ZBUF,
    FN_GET_ZBUF,
    FN_CLEAR_ZBUF,
    FN_GET_COLOR,
    FN_GET_RED,
    FN_GET_GREEN,
    FN_GET_BLUE,
    FN_GET_BLEND,
    FN_TRANSP,
    FN_GET_TRANSP,
    FN_CLEAR,
    FN_DOT,
    FN_DOT3D,
    FN_GET_DOT,
    FN_GET_DOT3D,
    FN_LINE,
    FN_LINE3D,
    FN_BOX,
    FN_FBOX,
    FN_PIXI,
    FN_TRIANGLES,
    FN_SORT_TRIANGLES,
    FN_SET_KEY_COLOR,
    FN_GET_KEY_COLOR,
    FN_SET_ALPHA,
    FN_GET_ALPHA,
    FN_PRINT,
    FN_GET_TEXT_XSIZE,
    FN_GET_TEXT_YSIZE,
    FN_SET_FONT,
    FN_GET_FONT,
    FN_EFFECTOR,
    FN_COLOR_GRADIENT,
    FN_SPLIT_RGB,
    FN_SPLIT_YCBCR,
    
    //OpenGL graphics:
    
    FN_SET_GL_CALLBACK,
    FN_REMOVE_GL_DATA,
    FN_GL_DRAW_ARRAYS,
    FN_GL_BLEND_FUNC,
    FN_GL_BIND_FRAMEBUFFER,
    FN_GL_NEW_PROG,
    FN_GL_USE_PROG,
    FN_GL_UNIFORM,
    FN_GL_UNIFORM_MATRIX,
    
    //Animation:
    
    FN_PIXI_UNPACK_FRAME,
    FN_PIXI_PACK_FRAME,
    FN_PIXI_CREATE_ANIM,
    FN_PIXI_REMOVE_ANIM,
    FN_PIXI_CLONE_FRAME,
    FN_PIXI_REMOVE_FRAME,
    FN_PIXI_PLAY,
    FN_PIXI_STOP,
    
    //Video:
    
    FN_VIDEO_OPEN,
    FN_VIDEO_CLOSE,
    FN_VIDEO_START,
    FN_VIDEO_STOP,
    FN_VIDEO_SET_PROPS,
    FN_VIDEO_GET_PROPS,
    FN_VIDEO_CAPTURE_FRAME,
    
    //Transformation:
    
    FN_T_RESET,
    FN_T_ROTATE,
    FN_T_TRANSLATE,
    FN_T_SCALE,
    FN_T_PUSH_MATRIX,
    FN_T_POP_MATRIX,
    FN_T_GET_MATRIX,
    FN_T_SET_MATRIX,
    FN_T_MUL_MATRIX,
    FN_T_POINT,
    
    //Audio:
    
    FN_SET_AUDIO_CALLBACK,
    FN_ENABLE_AUDIO_INPUT,
    FN_GET_NOTE_FREQ,
    
    //MIDI:
    
    FN_MIDI_OPEN_CLIENT,
    FN_MIDI_CLOSE_CLIENT,
    FN_MIDI_GET_DEVICE,
    FN_MIDI_OPEN_PORT,
    FN_MIDI_REOPEN_PORT,
    FN_MIDI_CLOSE_PORT,
    FN_MIDI_GET_EVENT,
    FN_MIDI_GET_EVENT_TIME,
    FN_MIDI_NEXT_EVENT,
    FN_MIDI_SEND_EVENT,
    
    //Time:
    
    FN_START_TIMER,
    FN_GET_TIMER,
    FN_GET_YEAR,
    FN_GET_MONTH,
    FN_GET_DAY,
    FN_GET_HOURS,
    FN_GET_MINUTES,
    FN_GET_SECONDS,
    FN_GET_TICKS,
    FN_GET_TPS,
    FN_SLEEP,
    
    //Events:
    
    FN_GET_EVENT,
    FN_SET_QUIT_ACTION,
    
    //Threads:
    
    FN_THREAD_CREATE,
    FN_THREAD_DESTROY,
    FN_MUTEX_CREATE,
    FN_MUTEX_DESTROY,
    FN_MUTEX_LOCK,
    FN_MUTEX_TRYLOCK,
    FN_MUTEX_UNLOCK,
    
    //Mathematical functions:
    
    FN_ACOS,
    FN_ACOSH,
    FN_ASIN,
    FN_ASINH,
    FN_ATAN,
    FN_ATANH,
    FN_CEIL,
    FN_COS,
    FN_COSH,
    FN_EXP,
    FN_EXP2,
    FN_EXPM1,
    FN_ABS,
    FN_FLOOR,
    FN_MOD,
    FN_LOG,
    FN_LOG2,
    FN_LOG10,
    FN_POW,
    FN_SIN,
    FN_SINH,
    FN_SQRT,
    FN_TAN,
    FN_TANH,
    FN_RAND,
    FN_RAND_SEED,
    
    //Data processing:
    
    FN_OP_CN,
    FN_OP_CC,
    FN_OP_CCN,
    FN_GENERATOR,
    FN_WAVETABLE_GENERATOR,
    FN_SAMPLER,
    FN_ENVELOPE2P,
    FN_GRADIENT,
    FN_FFT,
    FN_NEW_FILTER,
    FN_REMOVE_FILTER,
    FN_INIT_FILTER,
    FN_RESET_FILTER,
    FN_APPLY_FILTER,
    FN_REPLACE_VALUES,
    FN_COPY_AND_RESIZE,
    
    //Dialogs:
    
    FN_FILE_DIALOG,
    FN_PREFS_DIALOG,
    
    //Network:
    
    FN_OPEN_URL,
    
    //Native code:
    
    FN_DLOPEN,
    FN_DLCLOSE,
    FN_DLSYM,
    FN_DLCALL,

    //Posix compatibility:
    
    FN_SYSTEM,
    FN_ARGC,
    FN_ARGV,
    FN_EXIT,
    
    //Private API,
    
    FN_SYSTEM_COPY,
    FN_SYSTEM_PASTE,
    FN_SEND_FILE_TO_EMAIL,
    FN_SEND_FILE_TO_GALLERY,
    FN_OPEN_WEBSERVER,
    FN_SET_AUDIO_PLAY_STATUS,
    FN_GET_AUDIO_EVENT,
    FN_WM_VIDEO_CAPTURE_SUPPORTED,
    FN_WM_VIDEO_CAPTURE_START,
    FN_WM_VIDEO_CAPTURE_STOP,
    FN_WM_VIDEO_CAPTURE_GET_EXT,
    FN_WM_VIDEO_CAPTURE_ENCODE,
    
    //Number of builtin functions:
    
    FN_NUM
};

//pix_vm_run() modes:
enum pix_vm_run_mode
{
    PIX_VM_CONTINUE,
    PIX_VM_CALL_FUNCTION,
    PIX_VM_CALL_MAIN,
};

enum pix_sym_type
{
    SYMTYPE_LVAR,
    SYMTYPE_GVAR,
    SYMTYPE_NUM_I,
    SYMTYPE_NUM_F,
    SYMTYPE_WHILE,
    SYMTYPE_BREAK,
    SYMTYPE_BREAK2,
    SYMTYPE_BREAK3,
    SYMTYPE_BREAK4,
    SYMTYPE_BREAKALL,
    SYMTYPE_CONTINUE,
    SYMTYPE_IF,
    SYMTYPE_ELSE,
    SYMTYPE_GO,
    SYMTYPE_RET,
    SYMTYPE_IDIV,
    SYMTYPE_FNNUM,
    SYMTYPE_FNDEF,
    SYMTYPE_INCLUDE,
    SYMTYPE_HALT
};

enum pix_container_type
{
    PIX_CONTAINER_TYPE_INT8 = 0,
    PIX_CONTAINER_TYPE_INT16,
    PIX_CONTAINER_TYPE_INT32,
    PIX_CONTAINER_TYPE_INT64,
    PIX_CONTAINER_TYPE_FLOAT32,
    PIX_CONTAINER_TYPE_FLOAT64,
    PIX_CONTAINER_TYPES
};

enum
{
    PIX_EFFECT_NOISE,
    PIX_EFFECT_SPREAD_LEFT,
    PIX_EFFECT_SPREAD_RIGHT,
    PIX_EFFECT_SPREAD_UP,
    PIX_EFFECT_SPREAD_DOWN,
    PIX_EFFECT_HBLUR,
    PIX_EFFECT_VBLUR,
    PIX_EFFECT_COLOR
};

enum
{
    GL_SHADER_SOLID = 0,
    GL_SHADER_GRAD,
    GL_SHADER_TEX_ALPHA_SOLID,
    GL_SHADER_TEX_ALPHA_GRAD,
    GL_SHADER_TEX_RGB_SOLID,
    GL_SHADER_TEX_RGB_GRAD,
    GL_SHADER_MAX
};

#define PIX_CONTAINER_FLAG_USES_KEY	 	( 1 << 0 )
#define PIX_CONTAINER_FLAG_STATIC_DATA	 	( 1 << 1 )
#define PIX_CONTAINER_FLAG_SYSTEM_MANAGED 	( 1 << 2 )
#define PIX_CONTAINER_FLAG_GL_MIN_LINEAR	( 1 << 3 )
#define PIX_CONTAINER_FLAG_GL_MAG_LINEAR	( 1 << 4 )
#define PIX_CONTAINER_FLAG_GL_NO_XREPEAT	( 1 << 5 )
#define PIX_CONTAINER_FLAG_GL_NO_YREPEAT	( 1 << 6 )
#define PIX_CONTAINER_FLAG_GL_NICEST		( 1 << 7 )
#define PIX_CONTAINER_FLAG_GL_NO_ALPHA		( 1 << 8 )
#define PIX_CONTAINER_FLAG_GL_FRAMEBUFFER	( 1 << 9 )
#define PIX_CONTAINER_FLAG_GL_PROG		( 1 << 10 )
#define PIX_CONTAINER_FLAG_INTERP		( 1 << 11 )

enum pix_data_opcode
{
    //op_cn():
    PIX_DATA_OPCODE_MIN,
    PIX_DATA_OPCODE_MAX,
    PIX_DATA_OPCODE_MAXABS,
    PIX_DATA_OPCODE_SUM,
    PIX_DATA_OPCODE_LIMIT_TOP,
    PIX_DATA_OPCODE_LIMIT_BOTTOM,
    PIX_DATA_OPCODE_ABS,
    PIX_DATA_OPCODE_SUB2,
    PIX_DATA_OPCODE_COLOR_SUB2,
    PIX_DATA_OPCODE_DIV2,
    PIX_DATA_OPCODE_H_INTEGRAL,
    PIX_DATA_OPCODE_V_INTEGRAL,
    PIX_DATA_OPCODE_H_DERIVATIVE,
    PIX_DATA_OPCODE_V_DERIVATIVE,
    PIX_DATA_OPCODE_H_FLIP,
    PIX_DATA_OPCODE_V_FLIP,
    //op_cn(), op_cc():
    PIX_DATA_OPCODE_ADD,
    PIX_DATA_OPCODE_SADD,
    PIX_DATA_OPCODE_COLOR_ADD,
    PIX_DATA_OPCODE_SUB,
    PIX_DATA_OPCODE_SSUB,
    PIX_DATA_OPCODE_COLOR_SUB,
    PIX_DATA_OPCODE_MUL,
    PIX_DATA_OPCODE_SMUL,
    PIX_DATA_OPCODE_MUL_RSHIFT15,
    PIX_DATA_OPCODE_COLOR_MUL,
    PIX_DATA_OPCODE_DIV,
    PIX_DATA_OPCODE_COLOR_DIV,
    PIX_DATA_OPCODE_AND,
    PIX_DATA_OPCODE_OR,
    PIX_DATA_OPCODE_XOR,
    PIX_DATA_OPCODE_LSHIFT,
    PIX_DATA_OPCODE_RSHIFT,
    PIX_DATA_OPCODE_EQUAL,
    PIX_DATA_OPCODE_LESS,
    PIX_DATA_OPCODE_GREATER,
    PIX_DATA_OPCODE_COPY,
    PIX_DATA_OPCODE_COPY_LESS,
    PIX_DATA_OPCODE_COPY_GREATER,
    //op_cc():
    PIX_DATA_OPCODE_BMUL,
    PIX_DATA_OPCODE_EXCHANGE,
    PIX_DATA_OPCODE_COMPARE,
    //op_ccn():
    PIX_DATA_OPCODE_MUL_DIV, //container1 = ( container1 * container2 ) / number
    PIX_DATA_OPCODE_MUL_RSHIFT, //container1 = ( container1 * container2 ) >> number
    //generator():
    PIX_DATA_OPCODE_SIN,
    PIX_DATA_OPCODE_SIN8,
    PIX_DATA_OPCODE_RAND,
};

enum
{
    PIX_SAMPLER_DEST = 0,
    PIX_SAMPLER_DEST_OFF,
    PIX_SAMPLER_DEST_LEN,
    PIX_SAMPLER_SRC,
    PIX_SAMPLER_SRC_OFF_H,
    PIX_SAMPLER_SRC_OFF_L,
    PIX_SAMPLER_SRC_SIZE,
    PIX_SAMPLER_LOOP,
    PIX_SAMPLER_LOOP_LEN,
    PIX_SAMPLER_VOL1,
    PIX_SAMPLER_VOL2,
    PIX_SAMPLER_DELTA,
    PIX_SAMPLER_FLAGS,
    PIX_SAMPLER_PARAMETERS,
};

#define PIX_SAMPLER_FLAG_INTERP0	0
#define PIX_SAMPLER_FLAG_INTERP2	1
#define PIX_SAMPLER_FLAG_INTERP4	2
#define PIX_SAMPLER_FLAG_INTERP_MASK	7
#define PIX_SAMPLER_FLAG_PINGPONG	( 1 << 3 )
#define PIX_SAMPLER_FLAG_REVERSE	( 1 << 4 )
#define PIX_SAMPLER_FLAG_INLOOP		( 1 << 5 )

#define PIX_AUDIO_FLAG_INTERP2		1

#define PIX_COPY_NO_AUTOROTATE		1
#define PIX_COPY_CLIPPING		2

#define PIX_RESIZE_INTERP1			1
#define PIX_RESIZE_INTERP2			2
#define PIX_RESIZE_INTERP_TYPE( flags )		( flags & 15 )
#define PIX_RESIZE_INTERP_COLOR			( 1 << 4 )
#define PIX_RESIZE_INTERP_UNSIGNED		( 2 << 4 )
#define PIX_RESIZE_INTERP_OPTIONS( flags )	( flags & ( 15 << 4 ) )
#define PIX_RESIZE_MASK_INTERPOLATION		255

#define PIX_THREAD_FLAG_AUTO_DESTROY		( 1 << 0 )

enum 
{
    PIX_EVT_NULL = 0,
    PIX_EVT_MOUSEBUTTONDOWN,
    PIX_EVT_MOUSEBUTTONUP,
    PIX_EVT_MOUSEMOVE,
    PIX_EVT_TOUCHBEGIN,
    PIX_EVT_TOUCHEND,
    PIX_EVT_TOUCHMOVE,
    PIX_EVT_BUTTONDOWN,
    PIX_EVT_BUTTONUP,
    PIX_EVT_SCREENRESIZE,
    PIX_EVT_QUIT,
};

enum
{
    PIX_FORMAT_RAW = 0,
    PIX_FORMAT_JPEG,
    PIX_FORMAT_PNG,
    PIX_FORMAT_GIF,
    PIX_FORMAT_WAVE,
    PIX_FORMAT_AIFF,
    PIX_FORMAT_PIXICONTAINER
};

//Load/Save options (flags):
#define PIX_GIF_GRAYSCALE		1
#define PIX_GIF_PALETTE_MASK		3
#define PIX_GIF_DITHER			4
#define PIX_JPEG_QUALITY( opt )		( opt & 127 )
#define PIX_JPEG_H1V1			( 1 << 7 )
#define PIX_JPEG_H2V1			( 2 << 7 )
#define PIX_JPEG_H2V2			( 3 << 7 )
#define PIX_JPEG_SUBSAMPLING_MASK	( 3 << 7 )
#define PIX_JPEG_TWOPASS		( 1 << ( 7 + 3 ) )
#define PIX_LOAD_FIRST_FRAME		( 1 << 0 )

#define PIX_GVAR_WINDOW_XSIZE		128
#define PIX_GVAR_WINDOW_YSIZE		129
#define PIX_GVAR_FPS			130
#define PIX_GVAR_PPI			131
#define PIX_GVAR_SCALE			132
#define PIX_GVAR_FONT_SCALE		133
#define PIX_GVARS			134

#define PIX_COMPILER_SYMTAB_SIZE	6151
#define PIX_CONTAINER_SYMTAB_SIZE	53

enum
{
    pix_vm_container_hdata_type_anim = 0,
};

#define PIX_CCONV_DEFAULT		0
#define PIX_CCONV_CDECL			1
#define PIX_CCONV_STDCALL		2
#define PIX_CCONV_UNIX_AMD64		3
#define PIX_CCONV_WIN64			4

#define PIX_GL_SCREEN			-2
#define PIX_GL_ZBUF			-3

#define PIX_CODE_ANALYZER_SHOW_OPCODES	( 1 << 0 )
#define PIX_CODE_ANALYZER_SHOW_ADDRESS	( 1 << 1 )
#define PIX_CODE_ANALYZER_SHOW_STATS	( 1 << 2 )

struct pix_vm;

struct pix_sym //Symbol
{
    utf8_char*		name;
    pix_sym_type	type;
    PIX_VAL		val;
    pix_sym*		next;
};

struct pix_symtab //Symbol table
{
    size_t		size;
    pix_sym**		symtab;
};

struct pix_vm_thread
{
    volatile bool	active;

    uint		flags;    
    bthread		th;
    bool		thread_open; //TRUE if it is a separate non-blocking thread
    int			thread_num;
    pix_vm*		vm;
    
    size_t		pc; //Program counter
    size_t		sp; //Stack pointer (grows down)
    size_t		fp; //Stack frame pointer

    PIX_VAL		stack[ PIX_VM_STACK_SIZE ];
    char		stack_types[ PIX_VM_STACK_SIZE ];
};

struct pix_vm_function
{
    size_t		addr;
    PIX_VAL*		p;
    char*		p_types;
    int			p_num;
};

struct pix_vm_anim_frame
{
    pix_container_type	type;
    PIX_INT		xsize;
    PIX_INT		ysize;
    COLORPTR		pixels;
};

struct pix_vm_container_hdata_anim
{
    uchar		type;
    uint		frame_count;
    pix_vm_anim_frame*	frames;
};

#ifdef OPENGL
struct pix_vm_container_gl_data
{
    int			xsize;
    int			ysize;
    uint		texture_id;
    uint		texture_format;
    uint		framebuffer_id;
    gl_program_struct*	prog;
};
#endif

struct pix_vm_container_opt_data
{
    pix_symtab		props; //Properties
    void*		hdata; //Hidden data
#ifdef OPENGL
    pix_vm_container_gl_data* gl;
#endif
};

struct pix_vm_container //Universal container - base component of Pixilang
{
    pix_container_type	type;
    uint		flags;
    PIX_INT		xsize;
    PIX_INT		ysize;
    size_t		size;
    void*		data;
    COLOR		key;
    PIX_CID		alpha; //Container with alpha-channel
    pix_vm_container_opt_data* opt_data; //Optional data (properties, animation etc)
};

struct pix_vm_font
{
    PIX_CID 		font; //container
    int			xchars;
    int 		ychars;
    utf32_char		first;
    utf32_char		last;
};

struct pix_vm_text_line
{
    size_t		offset;
    size_t		end;
    int			xsize;
    int			ysize;
};

struct pix_vm_event
{
    uint16          	type; //event type
    uint16	    	flags;
    ticks_t	    	time;
    int16           	x;
    int16           	y;
    uint16	    	key; //virtual key code: standart ASCII (0..127) and additional (see KEY_xxx defines)
    uint16	    	scancode; //device dependent
    uint16	    	pressure; //key pressure (0..1024)
    utf32_char	    	unicode; //unicode if possible, or zero
};

struct pix_vm_ivertex
{
    PIX_INT		x; //fixed point (PIX_FIXED_MATH_PREC)
    PIX_INT		y; //...
    PIX_INT		z; //...
};

struct pix_vm_ivertex_t
{
    PIX_INT		x; //fixed point (PIX_FIXED_MATH_PREC)
    PIX_INT		y; //... 
    PIX_INT		z; //...
    PIX_INT		tx; //...
    PIX_INT		ty; //...
};

struct pix_vm_filter
{
    int a_count; //number of feedforward filter coefficients
    int b_count; //number of feedback filter coefficients
    bool int_coefs;
    int rshift;
    PIX_INT* a_i; //feedforward filter coefficients
    PIX_INT* b_i; //feedback filter coefficients
    PIX_FLOAT* a_f;
    PIX_FLOAT* b_f;
    int input_state_size;
    int output_state_size;
    uint input_state_ptr;
    uint output_state_ptr;
    void* input_state;
    void* output_state;
};

struct pix_vm_resize_pars
{
    void* dest;
    void* src;
    int type;
    uint resize_flags;
    PIX_INT dest_xsize;
    PIX_INT dest_ysize;
    PIX_INT src_xsize;
    PIX_INT src_ysize;
    PIX_INT dest_x;
    PIX_INT dest_y;
    PIX_INT dest_rect_xsize;
    PIX_INT dest_rect_ysize;
    PIX_INT src_x;
    PIX_INT src_y;
    PIX_INT src_rect_xsize;
    PIX_INT src_rect_ysize;
};

struct pix_vm //Pixilang virtual machine
{
    bool		ready;
    
    PIX_OPCODE*		code;
    size_t		code_ptr;
    size_t		code_size;
    size_t		halt_addr; //Address of HALT instruction; functions must returns to this addr.
    
    PIX_VAL*		vars; //Global variables
    char*		var_types; //Global variable types
    utf8_char**		var_names; //Global variable names
    size_t		vars_num; //Number of global variables
    
    pix_vm_container**	c; //Containers
    size_t		c_num;
    bmutex		c_mutex;
    PIX_CID		c_counter;
    bool		c_show_debug_messages;
    bool		c_ignore_mutex;
    
    PIX_CID		last_displayed_screen;
    int			fps;
    int			fps_counter;
    ticks_t		fps_time;
    PIX_CID		screen; //Container with the main Pixilang screen
    COLORPTR		screen_ptr;
    volatile int	screen_redraw_request;
    volatile int	screen_redraw_answer;
    volatile int	screen_redraw_counter;
    int			screen_change_x;
    int			screen_change_y;
    int			screen_change_xsize;
    int			screen_change_ysize;
    int			screen_xsize;
    int			screen_ysize;
    uint16		pixel_size;
    PIX_CID		zbuf;
    
    uchar		transp; //Opacity
    
    pix_vm_font		fonts[ PIX_VM_FONTS ];
    utf32_char*		text;
    pix_vm_text_line*	text_lines;

    int*		effector_colors_r;
    int*		effector_colors_g;
    int*		effector_colors_b;

    //Coordinate transformation:
    bool		t_enabled;
    PIX_FLOAT		t_matrix[ 4 * 4 * PIX_T_MATRIX_STACK_SIZE ];
    int			t_matrix_sp;
    
    PIX_ADDR		gl_callback;
    PIX_VAL		gl_userdata;
    char		gl_userdata_type;
    COLOR		gl_temp_screen;
    
    //Audio stream:
    PIX_ADDR		audio_callback;
    PIX_VAL		audio_userdata;
    char		audio_userdata_type;
    int			audio_freq;
    pix_container_type	audio_format;
    int			audio_channels;
    uint		audio_flags;
    int			audio_input_enabled;
    uint		audio_src_ptr; //infinite; fixed point
    uint		audio_src_rendered; //infinite
    void*		audio_src_buffers[ PIX_VM_AUDIO_CHANNELS ];
    uint		audio_src_buffer_size;
    uint		audio_src_buffer_ptr; //fixed point;
    void*		audio_input_buffers[ PIX_VM_AUDIO_CHANNELS ];
    PIX_CID		audio_channels_cont; //Container with audio output channels
    PIX_CID		audio_input_channels_cont; //Container with audio input channels
    PIX_CID		audio_buffers_conts[ PIX_VM_AUDIO_CHANNELS ]; //Containers with audio output buffers;
    PIX_CID		audio_input_buffers_conts[ PIX_VM_AUDIO_CHANNELS ]; //Containers with audio input buffers;
    
    uint		timers[ 16 ]; //User defined timers
    
    int16            	events_count; //Number of events to execute
    int16	       	current_event_num;
    pix_vm_event    	events[ PIX_VM_EVENTS ];
    bmutex    	events_mutex;
    PIX_CID		event; //(container)
    char		quit_action; //Action on the QUIT event: 0 - none; 1 - close virtual machine (default).

    unsigned int	random;
    
    utf8_char*		log_buffer; //Cyclic buffer for the log messages
    size_t		log_filled; //Number of filled bytes
    size_t		log_ptr;
    utf8_char*		log_prev_msg; //Previous message
    utf8_char		log_temp_str[ 4096 ];
    bmutex		log_mutex;
    
    bfs_file		virt_disk0;
    
    utf8_char*		base_path;
    
    PIX_CID		current_path; //Container
    PIX_CID		user_path; //Container
    PIX_CID		temp_path; //Container
    
    PIX_CID		os_name; //Container
    PIX_CID		arch_name; //...
    PIX_CID		lang_name; //...
    
    pix_vm_thread*	th[ PIX_VM_THREADS ]; //Threads
    bmutex		th_mutex;

    utf8_char*		file_dialog_name;
    utf8_char*		file_dialog_mask;
    utf8_char*		file_dialog_id;
    utf8_char*		file_dialog_def_name;
    utf8_char*		file_dialog_result;
    volatile int	file_dialog_request;
    volatile int	prefs_dialog_request; //Show global preferences
    volatile int	vsync_request;
    volatile int        webserver_request;
    int                 vcap_in_fps;
    int                 vcap_in_bitrate_kb;
    uint                vcap_in_flags;
    utf8_char*		vcap_out_file_extension;
    int			vcap_out_err;
    volatile int        vcap_request;

    window_manager*	wm;
    
#ifdef OPENGL
    bmutex		gl_mutex;
    uint*		gl_unused_textures;
    volatile int	gl_unused_textures_count;
    uint*		gl_unused_framebuffers;
    volatile int	gl_unused_framebuffers_count;
    gl_program_struct**	gl_unused_progs;
    volatile int	gl_unused_progs_count;
    int                 gl_transform_counter;
    float		gl_wm_transform[ 4 * 4 ];
    float		gl_wm_transform_prev[ 4 * 4 ];
    GLuint              gl_vshader_solid;
    GLuint              gl_vshader_gradient;
    GLuint              gl_vshader_tex_solid;
    GLuint              gl_vshader_tex_gradient;
    GLuint              gl_fshader_solid;
    GLuint              gl_fshader_gradient;
    GLuint              gl_fshader_tex_alpha_solid;
    GLuint              gl_fshader_tex_alpha_gradient;
    GLuint              gl_fshader_tex_rgb_solid;
    GLuint              gl_fshader_tex_rgb_gradient;
    gl_program_struct*  gl_current_prog;
    gl_program_struct*  gl_user_defined_prog;
    gl_program_struct*  gl_prog_solid;
    gl_program_struct*  gl_prog_gradient;
    gl_program_struct*  gl_prog_tex_alpha_solid;
    gl_program_struct*  gl_prog_tex_alpha_gradient;
    gl_program_struct*  gl_prog_tex_rgb_solid;
    gl_program_struct*  gl_prog_tex_rgb_gradient;
#endif
};

struct pix_data //Global Pixilang structure. One for different virtual machines
{
    bmutex compiler_mutex; //One compiler at the time only
};

#define PIX_BUILTIN_FN_PARAMETERS int fn_num, int pars_num, size_t sp, pix_vm_thread* th, pix_vm* vm
typedef void (*pix_builtin_fn)( PIX_BUILTIN_FN_PARAMETERS );

extern const utf8_char* g_pix_fn_names[];
extern pix_builtin_fn g_pix_fns[];

//
// LEVEL 0. Pixilang main.
//

int pix_init( pix_data* pd );
int pix_deinit( pix_data* pd );

//
// LEVEL 1. Virtual machine (process).
//

//Thread-safe functions:
//  pix_vm_new_container;
//  pix_vm_send_event;
//  pix_vm_get_event.

extern const utf8_char* g_pix_container_type_names[];
extern const int g_pix_container_type_sizes[];

int pix_vm_init( pix_vm* vm, window_manager* wm );
int pix_vm_deinit( pix_vm* vm );
void pix_vm_log( utf8_char* message, pix_vm* vm );
#define PIX_VM_LOG( fmt, ARGS... ) \
{ \
    bool use_mutex; if( vm->log_buffer ) use_mutex = 1; else use_mutex = 0; \
    if( use_mutex ) bmutex_lock( &vm->log_mutex ); \
    sprintf( vm->log_temp_str, fmt, ## ARGS ); pix_vm_log( vm->log_temp_str, vm ); \
    if( use_mutex) bmutex_unlock( &vm->log_mutex ); \
}
void pix_vm_put_opcode( PIX_OPCODE opcode, pix_vm* vm );
void pix_vm_put_int( PIX_INT v, pix_vm* vm );
void pix_vm_put_float( PIX_FLOAT v, pix_vm* vm );
utf8_char* pix_vm_get_variable_name( pix_vm* vm, size_t vnum );
void pix_vm_resize_variables( pix_vm* vm );
int pix_vm_send_event(
    int16 type,
    int16 flags,
    int16 x,
    int16 y,
    int16 key,
    int16 scancode,
    int16 pressure,
    utf32_char unicode,
    pix_vm* vm );
int pix_vm_get_event( pix_vm* vm );
int pix_vm_create_active_thread( int thread_num, pix_vm* vm );
int pix_vm_destroy_thread( int thread_num, PIX_INT timeout, pix_vm* vm );
int pix_vm_get_thread_retval( int thread_num, pix_vm* vm, PIX_VAL* retval, char* retval_type );
int pix_vm_run( 
    int thread_num, 
    bool open_new_thread, 
    pix_vm_function* fun, 
    pix_vm_run_mode mode, 
    pix_vm* vm );
void pix_vm_set_systeminfo_containers( pix_vm* vm );
int pix_vm_save_code( const utf8_char* name, pix_vm* vm );
int pix_vm_load_code( const utf8_char* name, utf8_char* base_path, pix_vm* vm );

int pix_vm_set_audio_callback( PIX_ADDR callback, PIX_VAL userdata, char userdata_type, uint freq, pix_container_type format, int channels, uint flags, pix_vm* vm );

void pix_vm_call_builtin_function( int fn_num, int pars_num, size_t sp, pix_vm_thread* th, pix_vm* vm );

PIX_CID pix_vm_new_container( PIX_CID cnum, PIX_INT xsize, PIX_INT ysize, int type, void* data, pix_vm* vm );
void pix_vm_remove_container( PIX_CID cnum, pix_vm* vm );
int pix_vm_resize_container( PIX_CID cnum, PIX_INT xsize, PIX_INT ysize, int type, uint flags, pix_vm* vm );
int pix_vm_rotate_block( void** ptr, PIX_INT* xsize, PIX_INT* ysize, int type, int angle, void* save_to );
int pix_vm_rotate_container( PIX_CID cnum, int angle, pix_vm* vm );
int pix_vm_convert_container_type( PIX_CID cnum, int type, pix_vm* vm );
void pix_vm_clean_container( PIX_CID cnum, char v_type, PIX_VAL v, PIX_INT offset, PIX_INT size, pix_vm* vm );
PIX_CID pix_vm_clone_container( PIX_CID cnum, pix_vm* vm );
PIX_CID pix_vm_zlib_pack_container( PIX_CID cnum, int level, pix_vm* vm );
PIX_CID pix_vm_zlib_unpack_container( PIX_CID cnum, pix_vm* vm );
PIX_INT pix_vm_get_container_int_element( PIX_CID cnum, size_t elnum, pix_vm* vm );
PIX_FLOAT pix_vm_get_container_float_element( PIX_CID cnum, size_t elnum, pix_vm* vm );
void pix_vm_set_container_int_element( PIX_CID cnum, size_t elnum, PIX_INT val, pix_vm* vm );
void pix_vm_set_container_float_element( PIX_CID cnum, size_t elnum, PIX_FLOAT val, pix_vm* vm );
size_t pix_vm_get_container_strlen( PIX_CID cnum, size_t offset, pix_vm* vm );
utf8_char* pix_vm_make_cstring_from_container( PIX_CID cnum, bool* need_to_free, pix_vm* vm );
pix_sym* pix_vm_get_container_property( PIX_CID cnum, const utf8_char* prop_name, int prop_hash, pix_vm* vm );
PIX_INT pix_vm_get_container_property_i( PIX_CID cnum, const utf8_char* prop_name, int prop_hash, pix_vm* vm );
void pix_vm_set_container_property( PIX_CID cnum, const utf8_char* prop_name, int prop_hash, char val_type, PIX_VAL val, pix_vm* vm );
void* pix_vm_get_container_hdata( PIX_CID cnum, pix_vm* vm );
int pix_vm_create_container_hdata( PIX_CID cnum, uchar hdata_type, size_t hdata_size, pix_vm* vm );
void pix_vm_remove_container_hdata( PIX_CID cnum, pix_vm* vm );
size_t pix_vm_get_container_hdata_size( PIX_CID cnum, pix_vm* vm );
size_t pix_vm_save_container_hdata( PIX_CID cnum, bfs_file f, pix_vm* vm );
size_t pix_vm_load_container_hdata( PIX_CID cnum, bfs_file f, pix_vm* vm );
int pix_vm_clone_container_hdata( PIX_CID new_cnum, PIX_CID old_cnum, pix_vm* vm );
PIX_INT pix_vm_container_get_cur_frame( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_get_frame_count( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_get_frame_size( PIX_CID cnum, int cur_frame, pix_container_type* type, int* xsize, int* ysize, pix_vm* vm );
int pix_vm_container_hdata_unpack_frame_to_buf( PIX_CID cnum, int cur_frame, COLORPTR buf, pix_vm* vm );
int pix_vm_container_hdata_unpack_frame( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_pack_frame_from_buf( PIX_CID cnum, int cur_frame, COLORPTR buf, pix_container_type type, int xsize, int ysize, pix_vm* vm );
int pix_vm_container_hdata_pack_frame( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_clone_frame( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_remove_frame( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_autoplay_control( PIX_CID cnum, pix_vm* vm );

inline pix_vm_container* pix_vm_get_container( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        return vm->c[ cnum ];
    }        
    return 0;
}
inline void* pix_vm_get_container_data( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            return c->data;
        }
    }
    return 0;
}
inline int pix_vm_get_container_flags( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            return c->flags;
        }
    }
    return 0;
}
inline void pix_vm_set_container_flags( PIX_CID cnum, uint flags, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            c->flags = flags;
        }
    }
}
inline void pix_vm_mix_container_flags( PIX_CID cnum, uint flags, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            c->flags |= flags;
        }
    }
}
inline COLOR pix_vm_get_container_key_color( PIX_CID cnum, pix_vm* vm )
{   
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            return c->key;
        }
    }
    return 0;
}
inline void pix_vm_set_container_key_color( PIX_CID cnum, COLOR key, pix_vm* vm )
{   
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            c->key = key;
            c->flags |= PIX_CONTAINER_FLAG_USES_KEY;
        }
    }
}
inline void pix_vm_remove_container_key_color( PIX_CID cnum, pix_vm* vm )
{   
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            c->flags &= ~PIX_CONTAINER_FLAG_USES_KEY;
        }
    }
}
inline PIX_CID pix_vm_get_container_alpha( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            return c->alpha;
        }
    }    
    return 0;
}
inline void* pix_vm_get_container_alpha_data( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            if( (unsigned)c->alpha < (unsigned)vm->c_num )
            {
                pix_vm_container* alpha_cont = vm->c[ c->alpha ];
                if( alpha_cont )
                {
            	    return alpha_cont->data;
            	}
            }
        }
    }    
    return 0;
}
inline void pix_vm_set_container_alpha( PIX_CID cnum, PIX_CID alpha, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            c->alpha = alpha;
        }
    }
}

void pix_vm_op_cn( int opcode, PIX_CID cnum, char val_type, PIX_VAL val, PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, PIX_VAL* retval, char* retval_type, pix_vm* vm );
PIX_INT pix_vm_op_cc( int opcode, PIX_CID cnum1, PIX_CID cnum2, PIX_INT dest_x, PIX_INT dest_y, PIX_INT src_x, PIX_INT src_y, PIX_INT xsize, PIX_INT ysize, pix_vm* vm );
PIX_INT pix_vm_op_ccn( int opcode, PIX_CID cnum1, PIX_CID cnum2, char val_type, PIX_VAL val, PIX_INT dest_x, PIX_INT dest_y, PIX_INT src_x, PIX_INT src_y, PIX_INT xsize, PIX_INT ysize, pix_vm* vm );
PIX_INT pix_vm_generator( int opcode, PIX_CID cnum, PIX_FLOAT* fval, PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, pix_vm* vm );
PIX_INT pix_vm_wavetable_generator(
    PIX_CID dest_cnum,
    PIX_INT dest_off,
    PIX_INT dest_len,
    PIX_CID table_cnum,
    PIX_CID amp_cnum,
    PIX_CID amp_delta_cnum,
    PIX_CID pos_cnum,
    PIX_CID pos_delta_cnum,
    PIX_INT gen_offset,
    PIX_INT gen_step,
    PIX_INT gen_count,
    pix_vm* vm );
PIX_INT pix_vm_sampler( pix_vm_container* pars_cont, pix_vm* vm );
PIX_INT pix_vm_envelope2p( PIX_CID cnum, PIX_INT v1, PIX_INT v2, PIX_INT offset, PIX_INT size, char dc_off1_type, PIX_VAL dc_off1, char dc_off2_type, PIX_VAL dc_off2, pix_vm* vm );
PIX_CID pix_vm_new_filter( uint flags, pix_vm* vm );
void pix_vm_remove_filter( PIX_CID f_c, pix_vm* vm );
PIX_INT pix_vm_init_filter( PIX_CID f_c, PIX_CID a_c, PIX_CID b_c, int rshift, uint flags, pix_vm* vm );
void pix_vm_reset_filter( PIX_CID f_c, pix_vm* vm );
PIX_INT pix_vm_apply_filter( PIX_CID f_c, PIX_CID output_c, PIX_CID input_c, uint flags, PIX_INT offset, PIX_INT size, pix_vm* vm );
void pix_vm_copy_and_resize( pix_vm_resize_pars* pars );

void pix_vm_gfx_set_screen( PIX_CID cnum, pix_vm* vm );
void pix_vm_gfx_matrix_reset( pix_vm* vm );
void pix_vm_gfx_matrix_mul( PIX_FLOAT* res, PIX_FLOAT* m1, PIX_FLOAT* m2 );
void pix_vm_gfx_vertex_transform( PIX_FLOAT* v, PIX_FLOAT* m );
void pix_vm_gfx_draw_line( PIX_INT x1, PIX_INT y1, PIX_INT x2, PIX_INT y2, COLOR color, pix_vm* vm );
void pix_vm_gfx_draw_line_zbuf( PIX_INT x1, PIX_INT y1, PIX_INT z1, PIX_INT x2, PIX_INT y2, PIX_INT z2, COLOR color, int* zbuf, pix_vm* vm );
void pix_vm_gfx_draw_box( PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, COLOR color, pix_vm* vm );
void pix_vm_gfx_draw_fbox( PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, COLOR color, pix_vm* vm );
void pix_vm_gfx_draw_container( PIX_CID cnum, PIX_FLOAT x, PIX_FLOAT y, PIX_FLOAT z, PIX_FLOAT xsize, PIX_FLOAT ysize, PIX_INT tx, PIX_INT ty, PIX_INT txsize, PIX_INT tysize, COLOR color, pix_vm* vm );
int* pix_vm_gfx_get_zbuf( pix_vm* vm );
void pix_vm_gfx_draw_text( utf8_char* str, size_t str_size, PIX_FLOAT x, PIX_FLOAT y, PIX_FLOAT z, int align, COLOR color, int max_xsize, int* out_xsize, int* out_ysize, bool dont_draw, pix_vm* vm );
void pix_vm_gfx_draw_triangle( pix_vm_ivertex* v1, pix_vm_ivertex* v2, pix_vm_ivertex* v3, COLOR color, pix_vm* vm );
void pix_vm_gfx_draw_triangle_zbuf( pix_vm_ivertex* v1, pix_vm_ivertex* v2, pix_vm_ivertex* v3, COLOR color, pix_vm* vm );
void pix_vm_gfx_draw_triangle_t( PIX_FLOAT* v1f, PIX_FLOAT* v2f, PIX_FLOAT* v3f, PIX_CID cnum, COLOR color, pix_vm* vm );

pix_vm_font* pix_vm_get_font_for_char( utf32_char c, pix_vm* vm );
int pix_vm_set_font( utf32_char first_char, PIX_CID cnum, int xchars, int ychars, pix_vm* vm );

PIX_CID pix_vm_load( const utf8_char* filename, bfs_file f, int par1, pix_vm* vm );
int pix_vm_save( PIX_CID cnum, const utf8_char* filename, bfs_file f, int format, int par1, pix_vm* vm );

int pix_vm_code_analyzer( uint flags, pix_vm* vm );

#ifdef OPENGL
int pix_vm_gl_init( pix_vm* vm );
void pix_vm_gl_deinit( pix_vm* vm );
void pix_vm_gl_matrix_set( pix_vm* vm );
void pix_vm_gl_program_reset( pix_vm* vm );
void pix_vm_gl_use_prog( gl_program_struct* p, pix_vm* vm );
pix_vm_container_gl_data* pix_vm_get_container_gl_data( PIX_CID cnum, pix_vm* vm );
pix_vm_container_gl_data* pix_vm_create_container_gl_data( PIX_CID cnum, pix_vm* vm );
void pix_vm_remove_container_gl_data( PIX_CID cnum, pix_vm* vm );
void pix_vm_empty_gl_trash( pix_vm* vm ); //Call this function in the thread with active OpenGL context only!
#endif

//
// LEVEL 2. Pixilang compiler (text source -> virtual code).
//

int pix_compile_from_memory( utf8_char* src, int src_size, utf8_char* src_name, utf8_char* base_path, pix_vm* vm, pix_data* pd );
int pix_compile( const utf8_char* name, pix_vm* vm, pix_data* pd );

//
// LEVEL X. Symbol table.
//

int pix_symtab_init( size_t size, pix_symtab* st );
int pix_symtab_hash( const utf8_char* name, size_t size );
pix_sym* pix_symtab_lookup( const utf8_char* name, int hash, bool create, pix_sym_type type, PIX_INT ival, PIX_FLOAT fval, bool* created, pix_symtab* st );
pix_sym* pix_sym_clone( pix_sym* s );
int pix_symtab_clone( pix_symtab* dest_st, pix_symtab* src_st );
pix_sym* pix_symtab_get_list( pix_symtab* st );
int pix_symtab_deinit( pix_symtab* st );

//
// LEVEL X. Various tools (utilities).
//

utf8_char* pix_get_base_path( const utf8_char* src_name );
utf8_char* pix_compose_full_path( utf8_char* base_path, utf8_char* file_name, pix_vm* vm );

#endif
