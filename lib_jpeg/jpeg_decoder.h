#pragma once

#ifdef PALMOS
    #define SIMPLE_JPEG_DECODER 1
#endif

#ifdef SIMPLE_JPEG_DECODER
    #include "simple_jpeg_decoder.h"
#else
    #include "jpgd.h"
#endif

enum jd_pixel_format
{
    JD_GRAYSCALE,
    JD_RGB,
    JD_SUNDOG_COLOR,
};

void* load_jpeg( const utf8_char* filename, bfs_file f, int* width, int* height, int* num_components, jd_pixel_format pixel_format );
