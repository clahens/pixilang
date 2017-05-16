#pragma once

//Linear table for frequency calculation:
extern unsigned int g_linear_freq_tab[ 768 ];

//Half of sinus:
extern unsigned char g_hsin_tab[ 256 ];

#define FFT_FLAG_INVERSE	1

void fft( uint flags, float* fi, float* fr, int size );
void fft( uint flags, double* fi, double* fr, int size );

enum dsp_curve_type
{
    dsp_curve_type_linear,
    dsp_curve_type_exponential1, 
    dsp_curve_type_exponential2, 
    dsp_curve_type_spline, 
    dsp_curve_type_rectangular,
    dsp_curve_types
};

int dsp_curve( int x, dsp_curve_type type ); //Input: 0...32768; Output: 0...32768

//Catmull-Rom cubic spline interpolation:
// y1 - current point;
// x - 0...32767;
inline int catmull_rom_spline_interp_int16( int y0, int y1, int y2, int y3, int x )
{
    x >>= 2;
    int a = ( 3 * ( y1 - y2 ) - y0 + y3 ) / 2;
    int b = 2 * y2 + y0 - ( 5 * y1 + y3 ) / 2;
    int c2 = ( y2 - y0 ) / 2;
    return ( ( ( ( ( ( ( ( a * x ) >> 13 ) + b ) * x ) >> 13 ) + c2 ) * x ) >> 13 ) + y1;
}
