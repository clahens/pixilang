/*
    timemanager.cpp. Time functions (thread safe)
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100% with global init/deinit

#include "core/core.h"
#include "../timemanager.h"

#ifdef UNIX
    #include <time.h>
#endif
#if defined(__APPLE__) || defined(OSX) || defined(IPHONE)
    #include <mach/mach.h>	    //mach_absolute_time() ...
    #include <mach/mach_time.h>	    //mach_absolute_time() ...
    #include <unistd.h>
#endif
#ifdef FREEBSD
    #include <sys/time.h>
#endif
#ifdef LINUX
    #include <sys/select.h>
#endif
#if defined(WIN) || defined(WINCE)
    #include <time.h>
#endif

void time_global_init( void )
{
    time_ticks();
    time_ticks_hires();
}

void time_global_deinit( void )
{
}

uint time_year( void )
{
#ifdef UNIX
    //UNIX:
    time_t t;
    time( &t );
    return localtime( &t )->tm_year + 1900;
#endif
#ifdef WIN
    //WINDOWS:
    time_t t;
    time( &t );
    return localtime( &t )->tm_year + 1900;
#endif
#ifdef WINCE
    //WINDOWS CE:
    SYSTEMTIME st;
    GetLocalTime( &st );
    return st.wYear;
#endif
}

uint time_month( void )
{
#ifdef UNIX
    //UNIX:
    time_t t;
    time( &t );
    return localtime( &t )->tm_mon + 1;
#endif
#ifdef WIN
    //WINDOWS:
    time_t t;
    time( &t );
    return localtime( &t )->tm_mon + 1;
#endif
#ifdef WINCE
    //WINDOWS CE:
    SYSTEMTIME st;
    GetLocalTime( &st );
    return st.wMonth;
#endif
}

const utf8_char* time_month_string( void )
{
    switch( time_month() )
    {
	case 1: return "jan"; break;
	case 2: return "feb"; break;
	case 3: return "mar"; break;
	case 4: return "apr"; break;
	case 5: return "may"; break;
	case 6: return "jun"; break;
	case 7: return "jul"; break;
	case 8: return "aug"; break;
	case 9: return "sep"; break;
	case 10: return "oct"; break;
	case 11: return "nov"; break;
	case 12: return "dec"; break;
	default: return ""; break;
    }
}

uint time_day( void )
{
#ifdef UNIX
    //UNIX:
    time_t t;
    time( &t );
    return localtime( &t )->tm_mday;
#endif
#ifdef WIN
    //WINDOWS:
    time_t t;
    time( &t );
    return localtime( &t )->tm_mday;
#endif
#ifdef WINCE
    //WINDOWS CE:
    SYSTEMTIME st;
    GetLocalTime( &st );
    return st.wDay;
#endif
}

uint time_hours( void )
{
#ifdef UNIX
    //UNIX:
    time_t t;
    time( &t );
    return localtime( &t )->tm_hour;
#endif
#ifdef WIN
    //WINDOWS:
    time_t t;
    time( &t );
    return localtime( &t )->tm_hour;
#endif
#ifdef WINCE
    //WINDOWS CE:
    SYSTEMTIME st;
    GetLocalTime( &st );
    return st.wHour;
#endif
}

uint time_minutes( void )
{
#ifdef UNIX
    //UNIX:
    time_t t;
    time( &t );
    return localtime( &t )->tm_min;
#endif
#ifdef WIN
    //WINDOWS:
    time_t t;
    time( &t );
    return localtime( &t )->tm_min;
#endif
#ifdef WINCE
    //WINDOWS CE:
    SYSTEMTIME st;
    GetLocalTime( &st );
    return st.wMinute;
#endif
}

uint time_seconds( void )
{
#ifdef UNIX
    //UNIX:
    time_t t = 0;
    time( &t );
    return localtime( &t )->tm_sec;
#endif
#ifdef WIN
    //WINDOWS:
    time_t t;
    time( &t );
    return localtime( &t )->tm_sec;
#endif
#ifdef WINCE
    //WINDOWS CE:
    SYSTEMTIME st;
    GetLocalTime( &st );
    return st.wSecond;
#endif
}

ticks_t time_ticks_per_second( void )
{
#ifdef UNIX
    //UNIX:
    return (ticks_t)1000;
#endif
#if defined(WIN) || defined(WINCE)
    //WINDOWS:
    return (ticks_t)1000;
#endif
}

#if defined(__APPLE__) || defined(OSX) || defined(IPHONE)
bool g_timebase_ready = 0;
double g_timebase_nanosec;
#endif

ticks_t time_ticks( void )
{
#ifdef UNIX
    #if defined(__APPLE__) || defined(OSX) || defined(IPHONE)
	//OSX || IPHONE:
	if( g_timebase_ready == 0 ) 
	{
	    mach_timebase_info_data_t timebase_info;
	    mach_timebase_info( &timebase_info );
	    g_timebase_nanosec = (double)timebase_info.numer / (double)timebase_info.denom;
	    g_timebase_ready = 1;
	}
	double t = (double)mach_absolute_time() * g_timebase_nanosec / 1000000;
	return (ticks_t)t;
    #else
	//Other UNIX systems:
	timespec t;
	clock_gettime( CLOCK_REALTIME, &t );
	return (ticks_t)( t.tv_nsec / 1000000 ) + t.tv_sec * 1000;
    #endif
#endif
#if defined(WIN) || defined(WINCE)
    return (ticks_t)GetTickCount();
#endif
}

#if defined(WIN) || defined(WINCE)
static unsigned long long g_ticks_per_second = 0;
static ticks_hr_t g_ticks_per_second_norm = 0;
static unsigned int g_ticks_div = 1;
static unsigned int g_ticks_mul = 0;
ticks_hr_t time_ticks_per_second_hires( void )
{
    return g_ticks_per_second_norm;
}
#endif
#ifdef WIN
ticks_hr_t __attribute__ ((force_align_arg_pointer)) time_ticks_hires( void )
#else
ticks_hr_t time_ticks_hires( void )
#endif
{
#ifdef LINUX
    timespec t;
    clock_gettime( CLOCK_REALTIME, &t );
    return (ticks_hr_t)( t.tv_nsec / ( 1000000000 / time_ticks_per_second_hires() ) ) + t.tv_sec * time_ticks_per_second_hires();
#endif
#if defined(WIN) || defined(WINCE)
    unsigned long long tick;
    if( g_ticks_per_second == 0 )
    {
	QueryPerformanceFrequency( (LARGE_INTEGER*)&g_ticks_per_second );
	if( g_ticks_per_second > 50000 )
	{
	    g_ticks_div = g_ticks_per_second / 50000;
	    g_ticks_per_second_norm = (ticks_hr_t)( g_ticks_per_second / g_ticks_div );
	}
	else 
	{
	    g_ticks_mul = 50000 / g_ticks_per_second;
	    if( g_ticks_mul * g_ticks_per_second < 50000 ) g_ticks_mul++;
	    g_ticks_per_second_norm = (ticks_hr_t)( g_ticks_per_second * g_ticks_mul );
	}
	/*
	printf( "TPS: %d\n", (int)g_ticks_per_second );
	printf( "TPSN: %d\n", (int)g_ticks_per_second_norm );
	printf( "DIV: %d\n", (int)g_ticks_div );
	printf( "MUL: %d\n", (int)g_ticks_mul );
	*/
    }
    QueryPerformanceCounter( (LARGE_INTEGER*)&tick );
    if( g_ticks_mul ) 
	return (ticks_hr_t)( tick * g_ticks_mul );
    else
	return (ticks_hr_t)( tick / g_ticks_div );
#endif
#if defined(__APPLE__) || defined(OSX) || defined(IPHONE)
    double t = (double)mach_absolute_time() * g_timebase_nanosec / 20000;
    unsigned long long tl = (unsigned long long)t;
    return (ticks_hr_t)tl;
#endif
}

void time_sleep( int milliseconds )
{
#ifdef UNIX
#if defined(OSX) || defined(IPHONE)
    while( 1 )
    {
	int t = milliseconds;
	if( t > 1000 ) t = 1000;
	usleep( t * 1000 );
	milliseconds -= t;
	if( milliseconds <= 0 ) break;
    }
#else
    timeval t;
    t.tv_sec = 0;
    t.tv_usec = (int) milliseconds * 1000;
    select( 0 + 1, 0, 0, 0, &t );
#endif
#endif
#if defined(WIN) || defined(WINCE)
    Sleep( milliseconds );
#endif
}
