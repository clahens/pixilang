#include "core/core.h"
#include "pixilang.h"

const utf8_char* user_window_name = "Pixilang " PIXILANG_VERSION_STR " (" __DATE__ ")";
const utf8_char* user_window_name_short = "Pixilang";
const utf8_char* user_profile_names[] = { "1:/pixilang_config.ini", "2:/pixilang_config.ini", 0 };
#if defined(ANDROID) || defined(WIN) || defined(OSX)
    const utf8_char* user_debug_log_file_name = "1:/pixilang_log.txt";
#else
    const utf8_char* user_debug_log_file_name = "3:/pixilang_log.txt";
#endif
int user_window_xsize = 480;
int user_window_ysize = 320;
uint user_window_flags = WIN_INIT_FLAG_SCALABLE;
const utf8_char* user_options[] = { 0 };
int user_options_val[] = { 0 };
