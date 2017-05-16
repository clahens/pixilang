#pragma once

#include "jpge.h"

enum je_subsampling 
{ 
    JE_Y_ONLY, 
    JE_H1V1, 
    JE_H2V1, 
    JE_H2V2,
};

enum je_pixel_format
{
    JE_GRAYSCALE,
    JE_RGB,
    JE_SUNDOG_COLOR,
};

struct je_params
{
    // Quality: 1-100, higher is better. Typical values are around 50-95.
    int quality;

    // m_subsampling:
    // Y (grayscale) only
    // YCbCr, no subsampling (H1V1, YCbCr 1x1x1, 3 blocks per MCU)
    // YCbCr, H2V1 subsampling (YCbCr 2x1x1, 4 blocks per MCU)
    // YCbCr, H2V2 subsampling (YCbCr 4x1x1, 6 blocks per MCU-- very common)
    je_subsampling subsampling;

    bool two_pass_flag;
    
    je_pixel_format pixel_format;
};

void init_je_params( je_params* pars );
  
bool save_jpeg( const utf8_char* filename, bfs_file f, int width, int height, const uchar* image_data, je_params* pars );
