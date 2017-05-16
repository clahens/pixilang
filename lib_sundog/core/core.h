#pragma once

#define SUNDOG_VERSION	"SunDog Engine"
#define SUNDOG_DATE	__DATE__
#define SUNDOG_TIME	__TIME__

//Possible defines:
//  COLOR8BITS		//8 bit color
//  COLOR16BITS		//16 bit color
//  COLOR32BITS		//32 bit color
//  RGB565		//Default for 16bit color
//  RGB555		//16bit color in Windows GDI
//  RGB556		//16bit color in OSX Quartz
//  GRAYSCALE		//Used with COLOR8BITS for grayscale palette
//  UNIX		//OS = some UNIX variation (Linux, FreeBSD...)
//  LINUX		//OS = Linux
//  WIN			//OS = Windows
//  WINCE		//OS = WindowsCE
//  OSX			//OS = OSX
//  IPHONE		//OS = iPhone OS
//  ANDROID		//OS = Android
//  X11			//X11 support
//  OPENGL		//OpenGL support
//  OPENGLES		//OpenGL ES
//  GDI			//GDI support
//  DIRECTDRAW		//DirectDraw (Windows), GAPI (WinCE), SDL (Linux)
//  FRAMEBUFFER		//Use framebuffer (may be virtual, without DIRECTDRAW)
//  ONLY44100		//Sample rate other then 44100 is not allowed
//  NOLOG		//No log messages
//  CPUMARK		//0..10: 0 - slow CPU (e.g. 100 MHz ARM without FPU); 10 - fast CPU (e.g. Core2Duo)
//  HEAPSIZE		//Average memory size available for application (in Mb)

typedef unsigned char   	uchar;
typedef unsigned short  	uint16;
typedef signed short    	int16;
typedef unsigned int    	uint;
typedef unsigned long long 	uint64;
typedef long long		int64;

typedef char			utf8_char;
typedef uint16	        	utf16_char;
typedef uint	        	utf32_char;

#if defined(__ICC) || defined(__INTEL_COMPILER)
    #define RESTRICT		restrict
#elif defined(__GNUC__) || defined(__llvm__) || defined(__clang__)
    #define RESTRICT		__restrict__
#elif defined(_MSC_VER)
    #define RESTRICT		__restrict
#else
    #define RESTRICT
#endif

#if defined(__ICC) || defined(__INTEL_COMPILER)
    #define COMPILER_MEMORY_BARRIER() __memory_barrier()
#elif defined(__GNUC__) || defined(__llvm__) || defined(__clang__)
    #define COMPILER_MEMORY_BARRIER() asm volatile( "" ::: "memory" )
#elif defined(_MSC_VER)
    #define COMPILER_MEMORY_BARRIER() _ReadWriteBarrier()
#else
    #define COMPILER_MEMORY_BARRIER()
#endif

#ifdef COLOR16BITS
    #ifndef RGB556
	#ifndef RGB555
	    #ifndef RGB565
		#define RGB565
	    #endif
	#endif
    #endif
#endif

#ifdef DIRECTDRAW
    #define FRAMEBUFFER
#endif

#if defined(LINUX) || defined(FREEBSD) || defined(OSX) || defined(IPHONE)
    #define UNIX
#endif

#ifdef MAEMO
    #define OS_NAME "maemo linux"
    #ifdef OPENGL
	#define OPENGLES
    #endif
    #define HEAPSIZE 32
#endif

#ifdef DINGUX
    #define OS_NAME "dingux linux"
    #define HEAPSIZE 16
    #define ONLY44100
    #define VIRTUALKEYBOARD
    #define NOMIDI
#endif

#ifdef ANDROID
    #define OS_NAME "android linux"
    #ifdef OPENGL
	#define OPENGLES
    #endif
    #define HEAPSIZE 32
    #define VIRTUALKEYBOARD
    #define NOMIDI
    #define MULTITOUCH
#endif

#ifdef RASPBERRY_PI
    #define OS_NAME "raspberry pi linux"
    #ifdef OPENGL
	#define OPENGLES
    #endif
    #define HEAPSIZE 64
#endif

#ifdef LINUX
    #ifndef OS_NAME
	//Common Linux:
	#define OS_NAME "linux"
	#define HEAPSIZE 256
    #endif
#endif

#ifdef OSX
    #define OS_NAME "osx"
    #define HEAPSIZE 256
    #define HANDLE_THREAD_EVENTS CFRunLoopRunInMode( kCFRunLoopDefaultMode, 0, 1 )
    #if !defined(ARCH_NAME)
	#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
	    #define ARCH_NAME "x86_64"
	    #define ARCH_X86_64
	#endif
	#if defined(i386) || defined(__i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
	    #define ARCH_NAME "x86"
	    #define ARCH_X86
	#endif
    #endif
#endif

#ifdef IPHONE
    #define OS_NAME "ios"
    #ifdef OPENGL
	#define OPENGLES
    #endif
    #define HEAPSIZE 32
    #define VIRTUALKEYBOARD
    #define MULTITOUCH
    #define HANDLE_THREAD_EVENTS CFRunLoopRunInMode( kCFRunLoopDefaultMode, 0, 1 )
    #if !defined(ARCH_NAME)
	#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
	    #define ARCH_NAME "x86_64"
	    #define ARCH_X86_64
	#endif
	#if defined(i386) || defined(__i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
	    #define ARCH_NAME "x86"
	    #define ARCH_X86
	#endif
    #endif
    #if !defined(ARCH_NAME)
	#define ARCH_NAME "arm"
	#define ARCH_ARM
    #endif
#endif

#ifdef WIN
    #define OS_NAME "win32"
    #define HEAPSIZE 256
#ifdef MULTITOUCH
    #define _WIN32_WINNT 0x0601 //Windows7 API support
#endif
    #include <windows.h>
#endif

#ifdef WINCE
    #define OS_NAME "wince"
    #define HEAPSIZE 16
    #define ONLY44100
    #define VIRTUALKEYBOARD
    #define NOMIDI
    #include <windows.h>
#endif

#ifndef HANDLE_THREAD_EVENTS
    #define HANDLE_THREAD_EVENTS /**/
#endif

#ifndef OS_NAME
    #error OS_NAME not defined
#endif

#ifndef ARCH_NAME
    #error ARCH_NAME not defined
#endif

#ifndef CPUMARK
    #error CPUMARK not defined
#endif

#ifndef HEAPSIZE
    #error HEAPSIZE not defined
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#ifdef ANDROID
    #define LOG2( V )	( log( V ) / log( 2 ) )
    #define LOG2F( V )	( logf( V ) / logf( 2 ) )
#else
    #define LOG2( V )	log2( V )
    #define LOG2F( V )	log2f( V )
#endif

struct sundog_engine;

#include "filesystem/filesystem.h"
#include "memory/memory.h"
#include "time/timemanager.h"
#include "utils/utils.h"
#include "sound/sound.h"
#include "video/video.h"
#include "log.h"
#include "window_manager/wmanager.h"
#include "main.h"
