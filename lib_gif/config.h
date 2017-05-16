#pragma once

#define HAVE_FCNTL_H 1
#define HAVE_STDARG_H 1
#define UINT32 unsigned int

#ifdef ANDROID
    #define S_IWRITE S_IWUSR
    #define S_IREAD S_IRUSR
#endif