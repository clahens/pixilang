#pragma once

typedef uint ticks_t;
typedef uint ticks_hr_t; //Hires ticks

#if defined(WIN) || defined(WINCE)
    ticks_hr_t time_ticks_per_second_hires( void );
#else
    #define time_ticks_per_second_hires() (ticks_hr_t)(50000)
#endif
ticks_hr_t time_ticks_hires( void );

#if defined(IPHONE) || defined(OSX)
    extern double g_timebase_nanosec;
#endif

void time_global_init( void );
void time_global_deinit( void );
uint time_year( void );
uint time_month( void );
const utf8_char* time_month_string( void );
uint time_day( void );
uint time_hours( void );
uint time_minutes( void );
uint time_seconds( void );
ticks_t time_ticks_per_second( void );
ticks_t time_ticks( void );
void time_sleep( int milliseconds );
