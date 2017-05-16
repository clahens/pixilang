/*
    dsp_functions.cpp
    Copyright (C) 2008 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

//#define USE_COMPLEX

#include <math.h>
#ifdef USE_COMPLEX
    #include <complex.h>
#endif

#include "core/core.h"
#include "dsp.h"

template < typename T >
static void do_fft( uint flags, T* fi, T* fr, int size )
{
    int size2 = size / 2;
    int m = 0;
    while( 1 )
    {
	if( 1 << m == size ) break;
	m++;
    }    

    T isign = 1;
    if( flags & FFT_FLAG_INVERSE )
    {
	isign = -1;
    }

    //Bit-reversal permutation:
    for( int i = 1, j = size2; i < size - 1; i++ )
    {
	if( i < j )
	{
	    T tr = fr[ j ];
	    T ti = fi[ j ];
	    fr[ j ] = fr[ i ];
	    fi[ j ] = fi[ i ];
	    fr[ i ] = tr;
	    fi[ i ] = ti;
	}
	int k = size2;
	while( k <= j )
	{
	    j -= k;
	    k >>= 1;
	}
	j += k;
    }
    
    //Danielson-Lanczos section:
    int mmax = 1;
    int istep;
    while( mmax < size )
    {
	istep = mmax << 1; //istep = 2; mmax = 1; ... istep = size; mmax = size >> 1;	
	T theta = isign * ( M_PI / mmax ); //Initialize the trigonometric recurrence.
	T wtemp = sin( 0.5 * theta ); 
#ifdef USE_COMPLEX
	float _Complex wp = -2.0F * wtemp * wtemp + sin( theta ) * _Complex_I;
	float _Complex w = 1.0 + 0.0 * I;
#else
	T wpr = -2.0F * wtemp * wtemp; 
	T wpi = sin( theta );
	T wr = 1.0;
	T wi = 0.0;
#endif
	for( m = 0; m < mmax; m++ ) //m = 0..1; 0..2; ... 0..size/2;
	{
	    for( int i = m; i < size; i += istep ) 
	    {
		int j = i + mmax;
#ifdef USE_COMPLEX		
		float _Complex jj = fr[ j ] + fi[ j ] * _Complex_I;
		float _Complex ii = fr[ i ] + fi[ i ] * _Complex_I;
		float _Complex t = w * jj;
		float _Complex ii1 = ii - t;
		float _Complex ii2 = ii + t;
		fr[ j ] = crealf( ii1 );
		fi[ j ] = cimagf( ii1 );
		fr[ i ] = crealf( ii2 );
		fi[ i ] = cimagf( ii2 );
#else
		T tempr = wr * fr[ j ] - wi * fi[ j ];
		T tempi = wr * fi[ j ] + wi * fr[ j ];
		fr[ j ] = fr[ i ] - tempr;
		fi[ j ] = fi[ i ] - tempi;
		fr[ i ] += tempr;
		fi[ i ] += tempi;
#endif		
	    }
#ifdef USE_COMPLEX
	    w = w + ( w * wp );
#else
	    wtemp = wr;
	    wr = wr * wpr - wi * wpi + wr;
	    wi = wi * wpr + wtemp * wpi + wi;
#endif
	}
	mmax = istep;
    }
    
    if( flags & FFT_FLAG_INVERSE )
    {
	for( int i = 0; i < size; i++ ) 
	{
	    fr[ i ] = fr[ i ] / size;
	    fi[ i ] = -fi[ i ] / size;
	}
    }
}

void fft( uint flags, float* fi, float* fr, int size )
{
    do_fft( flags, fi, fr, size );
}

void fft( uint flags, double* fi, double* fr, int size )
{
    do_fft( flags, fi, fr, size );
}

int dsp_curve( int x, dsp_curve_type type ) //Input: 0...32768; Output: 0...32768
{
    int y = x;
    switch( type )
    {
        case dsp_curve_type_exponential1:
            {
                int y2 = 32768 - x;
                y = 32768 - ( ( y2 * y2 ) >> 15 );
            }
            break;
        case dsp_curve_type_exponential2:
    	    y = ( x * x ) >> 15;
            break;
        case dsp_curve_type_spline:
            y = (int)( ( ( sin( ( (float)x / 32768.0F ) * M_PI - M_PI / 2.0F ) + 1.0F ) / 2.0F ) * 32768.0F );
            break;
        case dsp_curve_type_rectangular:
            if( x < 16384 ) y = 0; else y = 32768;
            break;
        default:
    	    break;
    }
    return y;
}
