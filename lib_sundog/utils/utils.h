#pragma once

#ifdef UNIX
    #include <pthread.h>
#endif

void utils_global_init( void );
void utils_global_deinit( void );

//LIST:

struct list_data
{
    utf8_char* items; //Items data
    int* items_ptr; //Item pointers
    int items_num; //Number of items; 
    int selected_item;
    int start_item;
};

void list_init( list_data* data );
void list_close( list_data* data );
void list_clear( list_data* data );
void list_reset_items( list_data* data );
void list_reset_selection( list_data* data );
void list_add_item( const utf8_char* item, char attr, list_data* data );
void list_delete_item( int item_num, list_data* data );
void list_move_item_up( int item_num, list_data* data );
void list_move_item_down( int item_num, list_data* data );
utf8_char* list_get_item( int item_num, list_data* data );
char list_get_attr( int item_num, list_data* data );
int list_get_selected_num( list_data* data );
void list_set_selected_num( int sel, list_data* data );
int list_compare_items( int item1, int item2, list_data* data );
void list_sort( list_data* data );

//MUTEXES:

struct bmutex
{
#ifdef UNIX
    pthread_mutex_t mutex;
#endif
#if defined(WIN) || defined(WINCE)
    CRITICAL_SECTION mutex;
#endif
};

// mutex attributes:
// 0 - recursive.

int bmutex_init( bmutex* mutex, int attr ); 
int bmutex_destroy( bmutex* mutex );
int bmutex_lock( bmutex* mutex ); //Return 0 if successful
int bmutex_trylock( bmutex* mutex ); // ...
int bmutex_unlock( bmutex* mutex );

//THREADS:

struct bthread
{
#ifdef UNIX
    pthread_t th;
#endif
#if defined(WIN) || defined(WINCE)
    HANDLE th;
#endif
    void* arg;
    void* (*proc)(void*);
    volatile bool finished;
};

#define BTHREAD_INFINITE        0x7FFFFFFF

int bthread_create( bthread* th, void* (*proc)(void*), void* arg, uint flags );
int bthread_destroy( bthread* th, int timeout ); //timeout (ms): >0 - timeout; <0 - don't destroy after timeout; 
int bthread_clean( bthread* th );
int bthread_is_empty( bthread* th );
int bthread_is_finished( bthread* th );

//RING BUFFER:

struct ring_buf
{
    bmutex m;
    uint flags;
    uchar* buf;
    size_t buf_size;
    volatile size_t wp;
    volatile size_t rp;
};

ring_buf* ring_buf_new( size_t size, uint flags );
void ring_buf_remove( ring_buf* b );
void ring_buf_lock( ring_buf* b );
void ring_buf_unlock( ring_buf* b );
size_t ring_buf_write( ring_buf* b, void* data, size_t size ); //Use with Lock+Unlock
size_t ring_buf_read( ring_buf* b, void* data, size_t size ); //Use with Lock+Unlock
void ring_buf_next( ring_buf* b, size_t size ); //Use with Lock+Unlock
size_t ring_buf_avail( ring_buf* b );

//PROFILES:

#define KEY_LANG		"locale_lang"
#define KEY_SCREENX		"width"
#define KEY_SCREENY		"height"
#define KEY_ROTATE		"rotate"
#define KEY_FULLSCREEN		"fullscreen"
#define KEY_ZOOM		"zoom"
#define KEY_PPI			"ppi"
#define KEY_SCALE		"scale"
#define KEY_FONT_SCALE		"fscale"
#define KEY_NOCURSOR		"nocursor"
#define KEY_NOAUTOLOCK		"noautolock"
#define KEY_NOBORDER		"noborder"
#define KEY_WINDOWNAME		"windowname"
#define KEY_VIDEODRIVER		"videodriver"
#define KEY_TOUCHCONTROL	"touchcontrol"
#define KEY_PENCONTROL		"pencontrol"
#define KEY_DOUBLECLICK		"doubleclick"
#define KEY_MAXFPS              "maxfps"
#define KEY_VSYNC		"vsync"
#define KEY_FRAMEBUFFER		"framebuffer"
#define KEY_NOWINDOW		"nowin"
#define KEY_SHOW_VIRT_KBD	"show_virt_kbd"
#define KEY_HIDE_VIRT_KBD	"hide_virt_kbd"
#define KEY_FILE_PREVIEW	"fpreview"
#define KEY_COLOR_THEME		"builtin_theme"
#define KEY_SOUNDBUFFER		"buffer"
#define KEY_AUDIODEVICE		"audiodevice"
#define KEY_AUDIODEVICE_IN	"audiodevice_in"
#define KEY_AUDIODRIVER		"audiodriver"
#define KEY_FREQ		"frequency"
#define KEY_CAMERA		"camera"

struct profile_key
{
    utf8_char* key;
    utf8_char* value;
    int line_num;
    bool deleted;
};

struct profile_data
{
    int file_num;
    utf8_char* file_name;
    utf8_char* source;
    profile_key* keys;
    int num;
};

void profile_new( profile_data* p );
int profile_resize( int new_num, profile_data* p );
int profile_add_key( const utf8_char* key, const utf8_char* value, int line_num, profile_data* p );
void profile_remove_key( const utf8_char* key, profile_data* p );
void profile_set_str_value( const utf8_char* key, const utf8_char* value, profile_data* p );
void profile_set_int_value( const utf8_char* key, int value, profile_data* p );
int profile_get_int_value( const utf8_char* key, int default_value, profile_data* p );
utf8_char* profile_get_str_value( const utf8_char* key, const utf8_char* default_value, profile_data* p );
void profile_close( profile_data* p );
void profile_load( const utf8_char* filename, profile_data* p );
int profile_save( profile_data* p );
void remove_all_profile_files( void );

//STRINGS:

void int_to_string( int value, utf8_char* str );
void hex_int_to_string( uint value, utf8_char* str );
int string_to_int( const utf8_char* str );
int hex_string_to_int( const utf8_char* str );
char int_to_hchar( int value );

utf16_char* utf8_to_utf16( utf16_char* dest, int dest_chars, const utf8_char* s );
utf32_char* utf8_to_utf32( utf32_char* dest, int dest_chars, const utf8_char* s );
utf8_char* utf16_to_utf8( utf8_char* dst, int dest_chars, const utf16_char* src );
utf8_char* utf32_to_utf8( utf8_char* dst, int dest_chars, const utf32_char* src );

int utf8_to_utf32_char( const utf8_char* str, utf32_char* res );
int utf8_to_utf32_char_safe( utf8_char* str, size_t str_size, utf32_char* res );

void utf8_unix_slash_to_windows( utf8_char* str );
void utf16_unix_slash_to_windows( utf16_char* str );
void utf32_unix_slash_to_windows( utf32_char* str );

int make_string_lowercase( utf8_char* dest, size_t dest_size, utf8_char* src );
int make_string_uppercase( utf8_char* dest, size_t dest_size, utf8_char* src );

//LOCALE:

int blocale_init( void );
void blocale_deinit( void );
const utf8_char* blocale_get_lang( void ); //Return language in POSIX format

//UNDO MANAGER:

#define UNDO_HANDLER_PARS bool redo, undo_action* action, undo_data* u

enum 
{
    UNDO_STATUS_NONE = 0,
    UNDO_STATUS_ADD_ACTION,
};

struct undo_action
{
    size_t level;
    uint type; //Action type
    uint par[ 5 ]; //User defined parameters...
    void* data;
};

struct undo_data
{
    int status;
    
    size_t data_size;
    size_t data_size_limit;
    
    size_t actions_num_limit;
    
    size_t level;
    size_t first_action;
    size_t cur_action; //Relative to first_action
    size_t actions_num;
    undo_action* actions;
    int (*action_handler)( UNDO_HANDLER_PARS );
    
    void* user_data;
};

void undo_init( size_t size_limit, int (*action_handler)( UNDO_HANDLER_PARS ), void* user_data, undo_data* u );
void undo_deinit( undo_data* u );
void undo_reset( undo_data* u );
int undo_add_action( undo_action* action, undo_data* u );
void undo_next_level( undo_data* u );
void execute_undo( undo_data* u );
void execute_redo( undo_data* u );

//SYMBOL TABLE (hash table)

union BSYMTAB_VAL
{
    int i;
    float f;
    void* p;
};

struct bsymtab_item //Symbol
{
    utf8_char*		name;
    int        		type;
    BSYMTAB_VAL		val;
    bsymtab_item*	next;
};

struct bsymtab //Symbol table
{
    size_t		size;
    bsymtab_item**	symtab;
};

bsymtab* bsymtab_new( size_t size );
int bsymtab_remove( bsymtab* st );
int bsymtab_hash( const utf8_char* name, size_t size );
//auto hash calculation: -1
bsymtab_item* bsymtab_lookup( const utf8_char* name, int hash, bool create, int initial_type, BSYMTAB_VAL initial_val, bool* created, bsymtab* st );
bsymtab_item* bsymtab_get_list( bsymtab* st );
int bsymtab_iset( const utf8_char* sym_name, int val, bsymtab* st ); //set int value of the symbol
int bsymtab_iset( uint sym_name, int val, bsymtab* st );
int bsymtab_iget( const utf8_char* sym_name, int notfound_val, bsymtab* st ); //get int value of the symbol
int bsymtab_iget( uint sym_name, int notfound_val, bsymtab* st );

//COPY / PASTE:

int system_copy( const utf8_char* filename );
utf8_char* system_paste( uint flags );

//URL:

void open_url( const utf8_char* url );
void send_mail( const utf8_char* email, const utf8_char* subj, const utf8_char* body );

//SEND FILE:

int send_file_to_email( const utf8_char* filename );
int send_file_to_gallery( const utf8_char* filename );

//RANDOM GENERATOR:

void set_seed( unsigned int seed );
unsigned int pseudo_random( void ); //OUT: 0...32767
unsigned int pseudo_random_with_seed( unsigned int* s ); //OUT: 0...32767

//3D TRANSFORMATION MATRIX OPERATIONS:

//Matrix structure:
// | 0  4  8  12 |
// | 1  5  9  13 |
// | 2  6  10 14 |
// | 3  7  11 15 |

void matrix_4x4_reset( float* m );
void matrix_4x4_mul( float* res, float* m1, float* m2 );
void matrix_4x4_rotate( float angle, float x, float y, float z, float* m );
void matrix_4x4_translate( float x, float y, float z, float* m );
void matrix_4x4_scale( float x, float y, float z, float* m );
void matrix_4x4_ortho( float left, float right, float bottom, float top, float z_near, float z_far, float* m ); //Multiply by an orthographic matrix

//MISC:

size_t round_to_power_of_two( size_t v );
uint sqrt_newton( uint l );

#define INT32_SWAP(n) ( ( (((unsigned int) n) << 24 ) & 0xFF000000 ) | \
                        ( (((unsigned int) n) << 8 ) & 0x00FF0000 ) | \
                        ( (((unsigned int) n) >> 8 ) & 0x0000FF00 ) | \
                        ( (((unsigned int) n) >> 24 ) & 0x000000FF ) )

#define INT16_SWAP(n) ( ( (((unsigned short) n) << 8 ) & 0xFF00 ) | \
                        ( (((unsigned short) n) >> 8 ) & 0x00FF ) )
