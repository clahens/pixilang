#pragma once

//Examples of file names for different systems:
//Linux:
// /prog.txt		- prog.txt file in the root directory 
//Windows:
// C:/prog.txt		- prog.txt on disk C
//Any OS:
// vfsX:/prop.txt	- prog.txt in the packed file system; where the X - file descriptor of this filesystem (supported formats: TAR)
// 1:/prog.txt		- ... current working directory (iOS:docs; WinCE:/; Android:external card; ...)
// 2:/prog.txt		- ... user directory for configs and caches (Linux: /home/username/; iOS:caches; Android: internal files directory);
//                            if possible - use 1: for the backups and last sessions, and use 2: for the configs only.
// 3:/prog.txt		- ... temp directory

#if defined(WIN) || defined(WINCE)
    #include <shlobj.h>
#endif

#ifdef UNIX
    #include <dirent.h> //for file find
#endif

#define MAX_DISKS	    128
#define DISKNAME_SIZE	    4
#define MAX_DIR_LEN	    2048

void bfs_refresh_disks( void ); //get info about local disks
const utf8_char* bfs_get_disk_name( uint n ); //get disk name
uint bfs_get_current_disk( void ); //get number of the current disk
uint bfs_get_disk_count( void );
const utf8_char* bfs_get_work_path( void ); //get current working directory (example: "C:/mydir/"); virtual disk 1:/
const utf8_char* bfs_get_conf_path( void ); //get config directory; virtual disk 2:/
const utf8_char* bfs_get_temp_path( void ); //virtual disk 3:/

//
// Main functions:
//

#define BFS_MAX_DESCRIPTORS	256
#define BFS_FOPEN_MAX		( BFS_MAX_DESCRIPTORS - 3 )

//Std files:
#define BFS_STDIN		( BFS_MAX_DESCRIPTORS - 0 )
#define BFS_STDOUT		( BFS_MAX_DESCRIPTORS - 1 )
#define BFS_STDERR		( BFS_MAX_DESCRIPTORS - 2 )

//Seek access:
#define BFS_SEEK_SET            0
#define BFS_SEEK_CUR            1
#define BFS_SEEK_END            2

enum bfs_fd_type
{
    BFS_FILE_NORMAL,
    BFS_FILE_IN_MEMORY,
};

enum bfs_file_type
{
    BFS_FILE_TYPE_UNKNOWN = 0,
    BFS_FILE_TYPE_WAVE,
    BFS_FILE_TYPE_AIFF,
    BFS_FILE_TYPE_OGG,
    BFS_FILE_TYPE_MP3,
    BFS_FILE_TYPE_FLAC,
    BFS_FILE_TYPE_MIDI,
    BFS_FILE_TYPE_JPEG,
    BFS_FILE_TYPE_PNG,
    BFS_FILE_TYPE_GIF,
    BFS_FILE_TYPE_AVI,
    BFS_FILE_TYPE_MP4,
    BFS_FILE_TYPE_ZIP,
    BFS_FILE_TYPE_PIXICONTAINER,
};

struct bfs_fd_struct
{
    utf8_char*	    	filename;
    void*	    	f;
    bfs_fd_type    	type;
    char*	    	virt_file_data;
    bool		virt_file_data_autofree;
    size_t	    	virt_file_ptr;
    size_t	    	virt_file_size;
    size_t		user_data; //Some user-defined parameter
};

typedef uint bfs_file;

void bfs_global_init( void );
void bfs_global_deinit( void );
utf8_char* bfs_make_filename( const utf8_char* filename );
const utf8_char* bfs_get_filename_without_dir( const utf8_char* filename );
const utf8_char* bfs_get_filename_extension( const utf8_char* filename );
bfs_fd_type bfs_get_type( bfs_file f );
void* bfs_get_data( bfs_file f );
size_t bfs_get_data_size( bfs_file f );
void bfs_set_user_data( bfs_file f, size_t user_data );
size_t bfs_get_user_data( bfs_file f );
bfs_file bfs_open_in_memory( void* data, size_t size );
bfs_file bfs_open( const utf8_char* filename, const utf8_char* filemode );
int bfs_close( bfs_file f );
void bfs_rewind( bfs_file f );
int bfs_getc( bfs_file f );
size_t bfs_tell( bfs_file f );
int bfs_seek( bfs_file f, long offset, int access );
int bfs_eof( bfs_file f );
int bfs_flush( bfs_file f );
size_t bfs_read( void* ptr, size_t el_size, size_t elements, bfs_file f ); //Return value: total number of elements (NOT bytes!) successfully read
size_t bfs_write( const void* ptr, size_t el_size, size_t elements, bfs_file f ); //Return value: total number of elements (NOT bytes!) successfully written
int bfs_putc( int val, bfs_file f );
int bfs_remove( const utf8_char* filename );
int bfs_rename( const utf8_char* old_name, const utf8_char* new_name );
int bfs_mkdir( const utf8_char* pathname, uint mode );
size_t bfs_get_file_size( const utf8_char* filename );
int bfs_copy_file( const utf8_char* dest, const utf8_char* src );
bfs_file_type bfs_get_file_type( const utf8_char* filename, bfs_file f );
const utf8_char* bfs_get_mime_type( bfs_file_type type );

//
// Searching files:
//

//type in bfs_find_struct:
enum bfs_find_item_type
{
    BFS_FILE = 0,
    BFS_DIR
};

struct bfs_find_struct
{ //structure for file searching functions
    const utf8_char* start_dir; //Example: "c:/mydir/" "d:/"
    const utf8_char* mask; //Example: "xm/mod/it" (or NULL for all files)
    
    utf8_char name[ MAX_DIR_LEN ]; //Found file's name
    utf8_char temp_name[ MAX_DIR_LEN ];
    bfs_find_item_type type; //Found file's type

#if defined(WIN)
    WIN32_FIND_DATAW find_data;
#endif
#if defined(WINCE)
    WIN32_FIND_DATA find_data;
#endif
#if defined(WIN) | defined(WINCE)
    HANDLE find_handle;
    utf8_char win_mask[ MAX_DIR_LEN ]; //Example: "*.xm *.mod *.it"
    utf8_char* win_start_dir; //Example: "mydir\*.xm"
#endif
#ifdef UNIX
    DIR* dir;
    struct dirent* current_file;
    utf8_char new_start_dir[ MAX_DIR_LEN ];
#endif
};

int bfs_find_first( bfs_find_struct* ); //Return values: 0 - no files
int bfs_find_next( bfs_find_struct* ); //Return values: 0 - no files
void bfs_find_close( bfs_find_struct* );
