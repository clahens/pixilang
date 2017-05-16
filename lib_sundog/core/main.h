#pragma once

/*
  SunDog stand-alone app structure:
    SUNDOG_GLOBAL_VARS;
    sundog_engine* sd = zero-filled structure;
    sundog_main( sd, true );
*/

/*
  SunDog module structure:
    SUNDOG_GLOBAL_VARS;
    global init:
      SUNDOG_GLOBAL_INIT();
    global deinit:
      SUNDOG_GLOBAL_DEINIT();
    module thread:
      sundog_engine* sd = zero-filled structure;
      sundog_main( sd, false );
*/

extern const utf8_char* user_window_name;
extern const utf8_char* user_window_name_short;
extern const utf8_char* user_profile_names[];
extern const utf8_char* user_debug_log_file_name;
extern int user_window_xsize;
extern int user_window_ysize;
extern uint user_window_flags; //WIN_INIT_FLAG_xxx
extern const utf8_char* user_options[];
extern int user_options_val[];

extern utf8_char* g_open_document_request;

#define SUNDOG_GLOBAL_VARS \
    utf8_char* g_open_document_request = 0;

inline void SUNDOG_GLOBAL_INIT( void )
{
    time_global_init();
    bmem_global_init();
    bfs_global_init();
    blog_global_init( user_debug_log_file_name );
    utils_global_init();
    bvideo_global_init( 0 );
}

inline void SUNDOG_GLOBAL_DEINIT( void )
{
    bvideo_global_deinit( 0 );
    utils_global_deinit();
    bfs_global_deinit();
    bmem_free_all();
    blog_global_deinit();
    bmem_global_deinit();
    time_global_deinit();
}

//SunDog Engine structure.
//Must be cleared and initialized before sundog_main().
struct sundog_engine
{
    window_manager*		wm;
    int				exit_code;

    //DEVICE DEPENDENT PART:
    
    int                 	argc; //Minimum - 1
    utf8_char**         	argv; // "program name"; "arg1"; "arg2"; ...
#ifdef WIN
    HINSTANCE           	hCurrentInst;
    HINSTANCE           	hPreviousInst;
    LPSTR               	lpszCmdLine;
    int                 	nCmdShow;
#endif
#ifdef WINCE
    HINSTANCE           	hCurrentInst;
    HINSTANCE           	hPreviousInst;
    LPWSTR              	lpszCmdLine;
    int                 	nCmdShow;
#endif
};

int sundog_main( sundog_engine* sd, bool global_init );

int user_init( window_manager* wm );
int user_event_handler( sundog_event* evt, window_manager* wm );
void user_close( window_manager* wm );
