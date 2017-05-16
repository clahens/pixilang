// JPEG Decoder
// SunDog port by Alexander Zolotov (2013 - 2016)

// based on:
// jpgd.cpp - C++ class for JPEG decompression.
// Public domain, Rich Geldreich <richgel99@gmail.com>
// Alex Evans: Linear memory allocator (taken from jpge.h).
// v1.04, May. 19, 2012: Code tweaks to fix VS2008 static code analysis warnings (all looked harmless)
//
// Supports progressive and baseline sequential JPEG image files, and the most common chroma subsampling factors: Y, H1V1, H2V1, H1V2, and H2V2.
//
// Chroma upsampling quality: H2V2 is upsampled in the frequency domain, H2V1 and H1V2 are upsampled using point sampling.
// Chroma upsampling reference: "Fast Scheme for Image Size Change in the Compressed Domain"
// http://vision.ai.uiuc.edu/~dugad/research/dct/index.html

//Modularity: 100%

#include "core/core.h"
#include "jpgd.h"

#include <assert.h>
#define JPGD_ASSERT( x ) assert( x )

// Set to 1 to enable freq. domain chroma upsampling on images using H2V2 subsampling (0=faster nearest neighbor sampling).
// This is slower, but results in higher quality on images with highly saturated colors.
#define JPGD_SUPPORT_FREQ_DOMAIN_UPSAMPLING 1

#define JPGD_TRUE (1)
#define JPGD_FALSE (0)

#define JPGD_MAX(a,b) (((a)>(b)) ? (a) : (b))
#define JPGD_MIN(a,b) (((a)<(b)) ? (a) : (b))

static inline void* jpgd_malloc( size_t nSize ) { return bmem_new( nSize ); }
static inline void jpgd_free( void* p ) { bmem_free( p ); }

inline jpgd_block_t* jd_coeff_buf_getp( coeff_buf* cb, int block_x, int block_y );
inline uint jd_get_char( jpeg_decoder* jd );
inline uint jd_get_char( bool* pPadding_flag, jpeg_decoder* jd );
inline void jd_stuff_char( uchar q, jpeg_decoder* jd );
inline uchar jd_get_octet( jpeg_decoder* jd );
inline uint jd_get_bits( int num_bits, jpeg_decoder* jd );
inline uint jd_get_bits_no_markers( int numbits, jpeg_decoder* jd );
inline int jd_huff_decode( huff_tables* pH, jpeg_decoder* jd );
inline int jd_huff_decode( huff_tables* pH, int& extrabits, jpeg_decoder* jd );
inline uchar jd_clamp( int i );
static void jd_prep_in_buffer( jpeg_decoder* jd );

// DCT coefficients are stored in this sequence.
static int g_ZAG[ 64 ] = { 0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63 };

enum JPEG_MARKER
{
    M_SOF0  = 0xC0, M_SOF1  = 0xC1, M_SOF2  = 0xC2, M_SOF3  = 0xC3, M_SOF5  = 0xC5, M_SOF6  = 0xC6, M_SOF7  = 0xC7, M_JPG   = 0xC8,
    M_SOF9  = 0xC9, M_SOF10 = 0xCA, M_SOF11 = 0xCB, M_SOF13 = 0xCD, M_SOF14 = 0xCE, M_SOF15 = 0xCF, M_DHT   = 0xC4, M_DAC   = 0xCC,
    M_RST0  = 0xD0, M_RST1  = 0xD1, M_RST2  = 0xD2, M_RST3  = 0xD3, M_RST4  = 0xD4, M_RST5  = 0xD5, M_RST6  = 0xD6, M_RST7  = 0xD7,
    M_SOI   = 0xD8, M_EOI   = 0xD9, M_SOS   = 0xDA, M_DQT   = 0xDB, M_DNL   = 0xDC, M_DRI   = 0xDD, M_DHP   = 0xDE, M_EXP   = 0xDF,
    M_APP0  = 0xE0, M_APP15 = 0xEF, M_JPG0  = 0xF0, M_JPG13 = 0xFD, M_COM   = 0xFE, M_TEM   = 0x01, M_ERROR = 0x100, RST0   = 0xD0
};

enum JPEG_SUBSAMPLING 
{ 
    JPGD_GRAYSCALE = 0, 
    JPGD_YH1V1, 
    JPGD_YH2V1, 
    JPGD_YH1V2, 
    JPGD_YH2V2 
};

#define CONST_BITS  13
#define PASS1_BITS  2
#define SCALEDONE ((int)1)

#define FIX_0_298631336  ((int)2446)        /* FIX(0.298631336) */
#define FIX_0_390180644  ((int)3196)        /* FIX(0.390180644) */
#define FIX_0_541196100  ((int)4433)        /* FIX(0.541196100) */
#define FIX_0_765366865  ((int)6270)        /* FIX(0.765366865) */
#define FIX_0_899976223  ((int)7373)        /* FIX(0.899976223) */
#define FIX_1_175875602  ((int)9633)        /* FIX(1.175875602) */
#define FIX_1_501321110  ((int)12299)       /* FIX(1.501321110) */
#define FIX_1_847759065  ((int)15137)       /* FIX(1.847759065) */
#define FIX_1_961570560  ((int)16069)       /* FIX(1.961570560) */
#define FIX_2_053119869  ((int)16819)       /* FIX(2.053119869) */
#define FIX_2_562915447  ((int)20995)       /* FIX(2.562915447) */
#define FIX_3_072711026  ((int)25172)       /* FIX(3.072711026) */

#define DESCALE(x,n)  (((x) + (SCALEDONE << ((n)-1))) >> (n))
#define DESCALE_ZEROSHIFT(x,n)  (((x) + (128 << (n)) + (SCALEDONE << ((n)-1))) >> (n))

#define MULTIPLY(var, cnst)  ((var) * (cnst))

#define CLAMP(i) ((static_cast<uint>(i) > 255) ? (((~i) >> 31) & 0xFF) : (i))

// Compiler creates a fast path 1D IDCT for X non-zero columns
template <int NONZERO_COLS>
struct Row
{
    static void idct( int* pTemp, const jpgd_block_t* pSrc )
    {
	// ACCESS_COL() will be optimized at compile time to either an array access, or 0.
	#define ACCESS_COL(x) (((x) < NONZERO_COLS) ? (int)pSrc[x] : 0)

	const int z2 = ACCESS_COL(2), z3 = ACCESS_COL(6);

	const int z1 = MULTIPLY(z2 + z3, FIX_0_541196100);
	const int tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065);
	const int tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);

	const int tmp0 = (ACCESS_COL(0) + ACCESS_COL(4)) << CONST_BITS;
	const int tmp1 = (ACCESS_COL(0) - ACCESS_COL(4)) << CONST_BITS;

	const int tmp10 = tmp0 + tmp3, tmp13 = tmp0 - tmp3, tmp11 = tmp1 + tmp2, tmp12 = tmp1 - tmp2;

	const int atmp0 = ACCESS_COL(7), atmp1 = ACCESS_COL(5), atmp2 = ACCESS_COL(3), atmp3 = ACCESS_COL(1);

	const int bz1 = atmp0 + atmp3, bz2 = atmp1 + atmp2, bz3 = atmp0 + atmp2, bz4 = atmp1 + atmp3;
	const int bz5 = MULTIPLY(bz3 + bz4, FIX_1_175875602);

	const int az1 = MULTIPLY(bz1, - FIX_0_899976223);
	const int az2 = MULTIPLY(bz2, - FIX_2_562915447);
	const int az3 = MULTIPLY(bz3, - FIX_1_961570560) + bz5;
	const int az4 = MULTIPLY(bz4, - FIX_0_390180644) + bz5;

	const int btmp0 = MULTIPLY(atmp0, FIX_0_298631336) + az1 + az3;
	const int btmp1 = MULTIPLY(atmp1, FIX_2_053119869) + az2 + az4;
	const int btmp2 = MULTIPLY(atmp2, FIX_3_072711026) + az2 + az3;
	const int btmp3 = MULTIPLY(atmp3, FIX_1_501321110) + az1 + az4;

	pTemp[0] = DESCALE(tmp10 + btmp3, CONST_BITS-PASS1_BITS);
	pTemp[7] = DESCALE(tmp10 - btmp3, CONST_BITS-PASS1_BITS);
	pTemp[1] = DESCALE(tmp11 + btmp2, CONST_BITS-PASS1_BITS);
	pTemp[6] = DESCALE(tmp11 - btmp2, CONST_BITS-PASS1_BITS);
	pTemp[2] = DESCALE(tmp12 + btmp1, CONST_BITS-PASS1_BITS);
	pTemp[5] = DESCALE(tmp12 - btmp1, CONST_BITS-PASS1_BITS);
	pTemp[3] = DESCALE(tmp13 + btmp0, CONST_BITS-PASS1_BITS);
	pTemp[4] = DESCALE(tmp13 - btmp0, CONST_BITS-PASS1_BITS);
    }
};

template <>
struct Row<0>
{
    static void idct( int* pTemp, const jpgd_block_t* pSrc )
    {
#ifdef _MSC_VER
	pTemp; pSrc;
#endif
    }
};

template <>
struct Row<1>
{
    static void idct( int* pTemp, const jpgd_block_t* pSrc )
    {
	const int dcval = (pSrc[0] << PASS1_BITS);

	pTemp[0] = dcval;
	pTemp[1] = dcval;
	pTemp[2] = dcval;
	pTemp[3] = dcval;
	pTemp[4] = dcval;
	pTemp[5] = dcval;
	pTemp[6] = dcval;
	pTemp[7] = dcval;
    }
};

// Compiler creates a fast path 1D IDCT for X non-zero rows
template <int NONZERO_ROWS>
struct Col
{
    static void idct( uchar* pDst_ptr, const int* pTemp )
    {
	// ACCESS_ROW() will be optimized at compile time to either an array access, or 0.
	#define ACCESS_ROW(x) (((x) < NONZERO_ROWS) ? pTemp[x * 8] : 0)

	const int z2 = ACCESS_ROW(2);
	const int z3 = ACCESS_ROW(6);

	const int z1 = MULTIPLY(z2 + z3, FIX_0_541196100);
	const int tmp2 = z1 + MULTIPLY(z3, - FIX_1_847759065);
	const int tmp3 = z1 + MULTIPLY(z2, FIX_0_765366865);

	const int tmp0 = (ACCESS_ROW(0) + ACCESS_ROW(4)) << CONST_BITS;
	const int tmp1 = (ACCESS_ROW(0) - ACCESS_ROW(4)) << CONST_BITS;

	const int tmp10 = tmp0 + tmp3, tmp13 = tmp0 - tmp3, tmp11 = tmp1 + tmp2, tmp12 = tmp1 - tmp2;

	const int atmp0 = ACCESS_ROW(7), atmp1 = ACCESS_ROW(5), atmp2 = ACCESS_ROW(3), atmp3 = ACCESS_ROW(1);

	const int bz1 = atmp0 + atmp3, bz2 = atmp1 + atmp2, bz3 = atmp0 + atmp2, bz4 = atmp1 + atmp3;
	const int bz5 = MULTIPLY(bz3 + bz4, FIX_1_175875602);

	const int az1 = MULTIPLY(bz1, - FIX_0_899976223);
	const int az2 = MULTIPLY(bz2, - FIX_2_562915447);
	const int az3 = MULTIPLY(bz3, - FIX_1_961570560) + bz5;
	const int az4 = MULTIPLY(bz4, - FIX_0_390180644) + bz5;

	const int btmp0 = MULTIPLY(atmp0, FIX_0_298631336) + az1 + az3;
	const int btmp1 = MULTIPLY(atmp1, FIX_2_053119869) + az2 + az4;
	const int btmp2 = MULTIPLY(atmp2, FIX_3_072711026) + az2 + az3;
	const int btmp3 = MULTIPLY(atmp3, FIX_1_501321110) + az1 + az4;

	int i = DESCALE_ZEROSHIFT(tmp10 + btmp3, CONST_BITS+PASS1_BITS+3);
	pDst_ptr[8*0] = (uchar)CLAMP(i);

	i = DESCALE_ZEROSHIFT(tmp10 - btmp3, CONST_BITS+PASS1_BITS+3);
	pDst_ptr[8*7] = (uchar)CLAMP(i);

	i = DESCALE_ZEROSHIFT(tmp11 + btmp2, CONST_BITS+PASS1_BITS+3);
	pDst_ptr[8*1] = (uchar)CLAMP(i);

	i = DESCALE_ZEROSHIFT(tmp11 - btmp2, CONST_BITS+PASS1_BITS+3);
	pDst_ptr[8*6] = (uchar)CLAMP(i);

	i = DESCALE_ZEROSHIFT(tmp12 + btmp1, CONST_BITS+PASS1_BITS+3);
	pDst_ptr[8*2] = (uchar)CLAMP(i);

	i = DESCALE_ZEROSHIFT(tmp12 - btmp1, CONST_BITS+PASS1_BITS+3);
	pDst_ptr[8*5] = (uchar)CLAMP(i);

	i = DESCALE_ZEROSHIFT(tmp13 + btmp0, CONST_BITS+PASS1_BITS+3);
	pDst_ptr[8*3] = (uchar)CLAMP(i);

	i = DESCALE_ZEROSHIFT(tmp13 - btmp0, CONST_BITS+PASS1_BITS+3);
	pDst_ptr[8*4] = (uchar)CLAMP(i);
    }
};

template <>
struct Col<1>
{
    static void idct( uchar* pDst_ptr, const int* pTemp )
    {
	int dcval = DESCALE_ZEROSHIFT(pTemp[0], PASS1_BITS+3);
	const uchar dcval_clamped = (uchar)CLAMP(dcval);
	pDst_ptr[0*8] = dcval_clamped;
	pDst_ptr[1*8] = dcval_clamped;
	pDst_ptr[2*8] = dcval_clamped;
	pDst_ptr[3*8] = dcval_clamped;
	pDst_ptr[4*8] = dcval_clamped;
	pDst_ptr[5*8] = dcval_clamped;
	pDst_ptr[6*8] = dcval_clamped;
	pDst_ptr[7*8] = dcval_clamped;
    }
};

static const uchar s_idct_row_table[] =
{
    1,0,0,0,0,0,0,0, 2,0,0,0,0,0,0,0, 2,1,0,0,0,0,0,0, 2,1,1,0,0,0,0,0, 2,2,1,0,0,0,0,0, 3,2,1,0,0,0,0,0, 4,2,1,0,0,0,0,0, 4,3,1,0,0,0,0,0,
    4,3,2,0,0,0,0,0, 4,3,2,1,0,0,0,0, 4,3,2,1,1,0,0,0, 4,3,2,2,1,0,0,0, 4,3,3,2,1,0,0,0, 4,4,3,2,1,0,0,0, 5,4,3,2,1,0,0,0, 6,4,3,2,1,0,0,0,
    6,5,3,2,1,0,0,0, 6,5,4,2,1,0,0,0, 6,5,4,3,1,0,0,0, 6,5,4,3,2,0,0,0, 6,5,4,3,2,1,0,0, 6,5,4,3,2,1,1,0, 6,5,4,3,2,2,1,0, 6,5,4,3,3,2,1,0,
    6,5,4,4,3,2,1,0, 6,5,5,4,3,2,1,0, 6,6,5,4,3,2,1,0, 7,6,5,4,3,2,1,0, 8,6,5,4,3,2,1,0, 8,7,5,4,3,2,1,0, 8,7,6,4,3,2,1,0, 8,7,6,5,3,2,1,0,
    8,7,6,5,4,2,1,0, 8,7,6,5,4,3,1,0, 8,7,6,5,4,3,2,0, 8,7,6,5,4,3,2,1, 8,7,6,5,4,3,2,2, 8,7,6,5,4,3,3,2, 8,7,6,5,4,4,3,2, 8,7,6,5,5,4,3,2,
    8,7,6,6,5,4,3,2, 8,7,7,6,5,4,3,2, 8,8,7,6,5,4,3,2, 8,8,8,6,5,4,3,2, 8,8,8,7,5,4,3,2, 8,8,8,7,6,4,3,2, 8,8,8,7,6,5,3,2, 8,8,8,7,6,5,4,2,
    8,8,8,7,6,5,4,3, 8,8,8,7,6,5,4,4, 8,8,8,7,6,5,5,4, 8,8,8,7,6,6,5,4, 8,8,8,7,7,6,5,4, 8,8,8,8,7,6,5,4, 8,8,8,8,8,6,5,4, 8,8,8,8,8,7,5,4,
    8,8,8,8,8,7,6,4, 8,8,8,8,8,7,6,5, 8,8,8,8,8,7,6,6, 8,8,8,8,8,7,7,6, 8,8,8,8,8,8,7,6, 8,8,8,8,8,8,8,6, 8,8,8,8,8,8,8,7, 8,8,8,8,8,8,8,8,
};

static const uchar s_idct_col_table[] = { 1, 1, 2, 3, 3, 3, 3, 3, 3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };

static void jd_idct( const jpgd_block_t* pSrc_ptr, uchar* pDst_ptr, int block_max_zag )
{
    JPGD_ASSERT( block_max_zag >= 1 );
    JPGD_ASSERT( block_max_zag <= 64 );
    
    if( block_max_zag <= 1 )
    {
	int k = ( ( pSrc_ptr[ 0 ] + 4 ) >> 3 ) + 128;
	k = CLAMP(k);
	k = k | ( k << 8 );
	k = k | ( k << 16 );

	for( int i = 8; i > 0; i-- )
	{
    	    *(int*)&pDst_ptr[ 0 ] = k;
    	    *(int*)&pDst_ptr[ 4 ] = k;
    	    pDst_ptr += 8;
	}
	return;
    }

    int temp[ 64 ];

    const jpgd_block_t* pSrc = pSrc_ptr;
    int* pTemp = temp;

    const uchar* pRow_tab = &s_idct_row_table[ ( block_max_zag - 1 ) * 8 ];
    int i;
    for( i = 8; i > 0; i--, pRow_tab++ )
    {
	switch( *pRow_tab )
	{
    	    case 0: Row<0>::idct( pTemp, pSrc ); break;
    	    case 1: Row<1>::idct( pTemp, pSrc ); break;
    	    case 2: Row<2>::idct( pTemp, pSrc ); break;
    	    case 3: Row<3>::idct( pTemp, pSrc ); break;
    	    case 4: Row<4>::idct( pTemp, pSrc ); break;
    	    case 5: Row<5>::idct( pTemp, pSrc ); break;
    	    case 6: Row<6>::idct( pTemp, pSrc ); break;
    	    case 7: Row<7>::idct( pTemp, pSrc ); break;
    	    case 8: Row<8>::idct( pTemp, pSrc ); break;
	}

	pSrc += 8;
	pTemp += 8;
    }

    pTemp = temp;

    const int nonzero_rows = s_idct_col_table[ block_max_zag - 1 ];
    for( i = 8; i > 0; i-- )
    {
	switch( nonzero_rows )
	{
    	    case 1: Col<1>::idct( pDst_ptr, pTemp ); break;
    	    case 2: Col<2>::idct( pDst_ptr, pTemp ); break;
    	    case 3: Col<3>::idct( pDst_ptr, pTemp ); break;
    	    case 4: Col<4>::idct( pDst_ptr, pTemp ); break;
    	    case 5: Col<5>::idct( pDst_ptr, pTemp ); break;
    	    case 6: Col<6>::idct( pDst_ptr, pTemp ); break;
    	    case 7: Col<7>::idct( pDst_ptr, pTemp ); break;
    	    case 8: Col<8>::idct( pDst_ptr, pTemp ); break;
	}

	pTemp++;
	pDst_ptr++;
    }
}

static void jd_idct_4x4( const jpgd_block_t* pSrc_ptr, uchar* pDst_ptr )
{
    int temp[ 64 ];
    int* pTemp = temp;
    const jpgd_block_t* pSrc = pSrc_ptr;

    for( int i = 4; i > 0; i-- )
    {
	Row<4>::idct( pTemp, pSrc );
	pSrc += 8;
	pTemp += 8;
    }

    pTemp = temp;
    for( int i = 8; i > 0; i-- )
    {
	Col<4>::idct( pDst_ptr, pTemp );
	pTemp++;
	pDst_ptr++;
    }
}

// Retrieve one character from the input stream.
inline uint jd_get_char( jpeg_decoder* jd )
{
    // Any bytes remaining in buffer?
    if( !jd->m_in_buf_left )
    {
	// Try to get more bytes.
	jd_prep_in_buffer( jd );
	// Still nothing to get?
	if( !jd->m_in_buf_left )
	{
    	    // Pad the end of the stream with 0xFF 0xD9 (EOI marker)
    	    int t = jd->m_tem_flag;
    	    jd->m_tem_flag ^= 1;
    	    if( t )
    		return 0xD9;
    	    else
    		return 0xFF;
	}
    }

    uint c = *(jd->m_pIn_buf_ofs);
    jd->m_pIn_buf_ofs++;
    jd-> m_in_buf_left--;

    return c;
}

// Same as previous method, except can indicate if the character is a pad character or not.
inline uint jd_get_char( bool* pPadding_flag, jpeg_decoder* jd )
{
    if( !jd->m_in_buf_left )
    {
	jd_prep_in_buffer( jd );
	if( !jd->m_in_buf_left )
	{
    	    *pPadding_flag = true;
    	    int t = jd->m_tem_flag;
    	    jd->m_tem_flag ^= 1;
    	    if( t )
    		return 0xD9;
    	    else
    		return 0xFF;
	}
    }

    *pPadding_flag = false;

    uint c = *(jd->m_pIn_buf_ofs);
    jd->m_pIn_buf_ofs++;
    jd->m_in_buf_left--;

    return c;
}

// Inserts a previously retrieved character back into the input buffer.
inline void jd_stuff_char( uchar q, jpeg_decoder* jd )
{
    jd->m_pIn_buf_ofs--;
    *(jd->m_pIn_buf_ofs) = q;
    jd->m_in_buf_left++;
}

// Retrieves one character from the input stream, but does not read past markers. Will continue to return 0xFF when a marker is encountered.
inline uchar jd_get_octet( jpeg_decoder* jd )
{
    bool padding_flag;
    int c = jd_get_char( &padding_flag, jd );

    if( c == 0xFF )
    {
	if( padding_flag )
    	    return 0xFF;

	c = jd_get_char( &padding_flag, jd );
	if( padding_flag )
	{
    	    jd_stuff_char( 0xFF, jd );
    	    return 0xFF;
	}

	if( c == 0x00 )
    	    return 0xFF;
	else
	{
    	    jd_stuff_char( static_cast<uchar>(c), jd );
    	    jd_stuff_char( 0xFF, jd );
    	    return 0xFF;
	}
    }

    return static_cast<uchar>(c);
}

// Retrieves a variable number of bits from the input stream. Does not recognize markers.
inline uint jd_get_bits( int num_bits, jpeg_decoder* jd )
{
    if( !num_bits )
	return 0;

    uint i = jd->m_bit_buf >> ( 32 - num_bits );

    if( ( jd->m_bits_left -= num_bits ) <= 0 )
    {
	jd->m_bit_buf <<= ( num_bits += jd->m_bits_left );

	uint c1 = jd_get_char( jd );
	uint c2 = jd_get_char( jd );
	jd->m_bit_buf = ( jd->m_bit_buf & 0xFFFF0000 ) | ( c1 << 8 ) | c2;

	jd->m_bit_buf <<= -jd->m_bits_left;

	jd->m_bits_left += 16;
	
	JPGD_ASSERT( jd->m_bits_left >= 0 );
    }
    else
	jd->m_bit_buf <<= num_bits;

    return i;
}

// Retrieves a variable number of bits from the input stream. Markers will not be read into the input bit buffer. Instead, an infinite number of all 1's will be returned when a marker is encountered.
inline uint jd_get_bits_no_markers( int num_bits, jpeg_decoder* jd )
{
    if( !num_bits )
	return 0;

    uint i = jd->m_bit_buf >> ( 32 - num_bits );

    if( ( jd->m_bits_left -= num_bits ) <= 0 )
    {
	jd->m_bit_buf <<= ( num_bits += jd->m_bits_left );

	if( ( jd->m_in_buf_left < 2 ) || ( jd->m_pIn_buf_ofs[ 0 ] == 0xFF ) || ( jd->m_pIn_buf_ofs[ 1 ] == 0xFF ) )
	{
    	    uint c1 = jd_get_octet( jd );
    	    uint c2 = jd_get_octet( jd );
    	    jd->m_bit_buf |= ( c1 << 8 ) | c2;
	}
	else
	{
    	    jd->m_bit_buf |= ( (uint)jd->m_pIn_buf_ofs[ 0 ] << 8 ) | jd->m_pIn_buf_ofs[ 1 ];
    	    jd->m_in_buf_left -= 2;
    	    jd->m_pIn_buf_ofs += 2;
	}

	jd->m_bit_buf <<= -jd->m_bits_left;

	jd->m_bits_left += 16;

	JPGD_ASSERT( jd->m_bits_left >= 0 );
    }
    else
	jd->m_bit_buf <<= num_bits;

    return i;
}

// Decodes a Huffman encoded symbol.
inline int jd_huff_decode( huff_tables* pH, jpeg_decoder* jd )
{
    int symbol;

    // Check first 8-bits: do we have a complete symbol?
    if( ( symbol = pH->look_up[ jd->m_bit_buf >> 24 ] ) < 0 )
    {
	// Decode more bits, use a tree traversal to find symbol.
	int ofs = 23;
	do
	{
    	    symbol = pH->tree[ -(int)( symbol + ( ( jd->m_bit_buf >> ofs ) & 1 ) ) ];
    	    ofs--;
	} while( symbol < 0 );

	jd_get_bits_no_markers( 8 + ( 23 - ofs ), jd );
    }
    else
	jd_get_bits_no_markers( pH->code_size[ symbol ], jd );

    return symbol;
}

// Decodes a Huffman encoded symbol.
inline int jd_huff_decode( huff_tables* pH, int& extra_bits, jpeg_decoder* jd )
{
    int symbol;

    // Check first 8-bits: do we have a complete symbol?
    if( ( symbol = pH->look_up2[ jd->m_bit_buf >> 24 ] ) < 0 )
    {
	// Use a tree traversal to find symbol.
	int ofs = 23;
	do
	{
    	    symbol = pH->tree[ -(int)( symbol + ( ( jd->m_bit_buf >> ofs ) & 1 ) ) ];
	    ofs--;
	} while( symbol < 0 );

	jd_get_bits_no_markers( 8 + ( 23 - ofs ), jd );

	extra_bits = jd_get_bits_no_markers( symbol & 0xF, jd );
    }
    else
    {
	JPGD_ASSERT( ( ( symbol >> 8 ) & 31 ) == pH->code_size[ symbol & 255 ] + ( ( symbol & 0x8000 ) ? ( symbol & 15 ) : 0 ) );

	if( symbol & 0x8000 )
	{
    	    jd_get_bits_no_markers( ( symbol >> 8 ) & 31, jd );
    	    extra_bits = symbol >> 16;
	}
	else
	{
    	    int code_size = ( symbol >> 8 ) & 31;
    	    int num_extra_bits = symbol & 0xF;
    	    int bits = code_size + num_extra_bits;
    	    if( bits <= ( jd->m_bits_left + 16 ) )
    		extra_bits = jd_get_bits_no_markers( bits, jd ) & ( ( 1 << num_extra_bits ) - 1 );
    	    else
    	    {
    		jd_get_bits_no_markers( code_size, jd );
    		extra_bits = jd_get_bits_no_markers( num_extra_bits, jd );
    	    }
	}

	symbol &= 0xFF;
    }

    return symbol;
}

// Tables and macro used to fully decode the DPCM differences.
static const int s_extend_test[ 16 ] = { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };
static const int s_extend_offset[ 16 ] = { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1, ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1, ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1, ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };
static const int s_extend_mask[] = { 0, (1<<0), (1<<1), (1<<2), (1<<3), (1<<4), (1<<5), (1<<6), (1<<7), (1<<8), (1<<9), (1<<10), (1<<11), (1<<12), (1<<13), (1<<14), (1<<15), (1<<16) };
// The logical AND's in this macro are to shut up static code analysis (aren't really necessary - couldn't find another way to do this)
#define JPGD_HUFF_EXTEND( x, s ) ( ( ( x ) < s_extend_test[ s & 15 ] ) ? ( ( x ) + s_extend_offset[ s & 15 ] ) : ( x ) )

// Clamps a value between 0-255.
inline uchar jd_clamp( int i )
{
    if( static_cast<uint>( i ) > 255 )
    i = ( ( ( ~i ) >> 31 ) & 0xFF );

    return static_cast<uchar>( i );
}

struct Matrix44
{
    typedef int Element_Type;
    enum { NUM_ROWS = 4, NUM_COLS = 4 };

    Element_Type v[ NUM_ROWS ][ NUM_COLS ];

    inline int rows() const { return NUM_ROWS; }
    inline int cols() const { return NUM_COLS; }

    inline const Element_Type & at( int r, int c ) const { return v[ r ][ c ]; }
    inline       Element_Type & at( int r, int c )       { return v[ r ][ c ]; }

    inline Matrix44() { }

    inline Matrix44& operator += (const Matrix44& a)
    {
	for (int r = 0; r < NUM_ROWS; r++)
        {
    	    at(r, 0) += a.at(r, 0);
    	    at(r, 1) += a.at(r, 1);
    	    at(r, 2) += a.at(r, 2);
    	    at(r, 3) += a.at(r, 3);
    	}
        return *this;
    }

    inline Matrix44& operator -= (const Matrix44& a)
    {
        for (int r = 0; r < NUM_ROWS; r++)
        {
    	    at(r, 0) -= a.at(r, 0);
    	    at(r, 1) -= a.at(r, 1);
    	    at(r, 2) -= a.at(r, 2);
    	    at(r, 3) -= a.at(r, 3);
        }
        return *this;
    }

    friend inline Matrix44 operator + (const Matrix44& a, const Matrix44& b)
    {
        Matrix44 ret;
        for (int r = 0; r < NUM_ROWS; r++)
        {
    	    ret.at(r, 0) = a.at(r, 0) + b.at(r, 0);
    	    ret.at(r, 1) = a.at(r, 1) + b.at(r, 1);
    	    ret.at(r, 2) = a.at(r, 2) + b.at(r, 2);
    	    ret.at(r, 3) = a.at(r, 3) + b.at(r, 3);
        }
        return ret;
    }

    friend inline Matrix44 operator - (const Matrix44& a, const Matrix44& b)
    {
	Matrix44 ret;
        for (int r = 0; r < NUM_ROWS; r++)
        {
    	    ret.at(r, 0) = a.at(r, 0) - b.at(r, 0);
    	    ret.at(r, 1) = a.at(r, 1) - b.at(r, 1);
    	    ret.at(r, 2) = a.at(r, 2) - b.at(r, 2);
    	    ret.at(r, 3) = a.at(r, 3) - b.at(r, 3);
        }
        return ret;
    }

    static inline void add_and_store(jpgd_block_t* pDst, const Matrix44& a, const Matrix44& b)
    {
        for (int r = 0; r < 4; r++)
        {
    	    pDst[0*8 + r] = static_cast<jpgd_block_t>(a.at(r, 0) + b.at(r, 0));
    	    pDst[1*8 + r] = static_cast<jpgd_block_t>(a.at(r, 1) + b.at(r, 1));
    	    pDst[2*8 + r] = static_cast<jpgd_block_t>(a.at(r, 2) + b.at(r, 2));
    	    pDst[3*8 + r] = static_cast<jpgd_block_t>(a.at(r, 3) + b.at(r, 3));
        }
    }

    static inline void sub_and_store(jpgd_block_t* pDst, const Matrix44& a, const Matrix44& b)
    {
        for (int r = 0; r < 4; r++)
        {
    	    pDst[0*8 + r] = static_cast<jpgd_block_t>(a.at(r, 0) - b.at(r, 0));
    	    pDst[1*8 + r] = static_cast<jpgd_block_t>(a.at(r, 1) - b.at(r, 1));
    	    pDst[2*8 + r] = static_cast<jpgd_block_t>(a.at(r, 2) - b.at(r, 2));
    	    pDst[3*8 + r] = static_cast<jpgd_block_t>(a.at(r, 3) - b.at(r, 3));
        }
    }
};

const int FRACT_BITS = 10;
const int SCALE = 1 << FRACT_BITS;

typedef int Temp_Type;
#define D(i) (((i) + (SCALE >> 1)) >> FRACT_BITS)
#define F(i) ((int)((i) * SCALE + .5f))

// Any decent C++ compiler will optimize this at compile time to a 0, or an array access.
#define AT(c, r) ((((c)>=NUM_COLS)||((r)>=NUM_ROWS)) ? 0 : pSrc[(c)+(r)*8])

// NUM_ROWS/NUM_COLS = # of non-zero rows/cols in input matrix
template<int NUM_ROWS, int NUM_COLS>
struct P_Q
{
    static void calc(Matrix44& P, Matrix44& Q, const jpgd_block_t* pSrc)
    {
	// 4x8 = 4x8 times 8x8, matrix 0 is constant
        const Temp_Type X000 = AT(0, 0);
        const Temp_Type X001 = AT(0, 1);
        const Temp_Type X002 = AT(0, 2);
        const Temp_Type X003 = AT(0, 3);
        const Temp_Type X004 = AT(0, 4);
        const Temp_Type X005 = AT(0, 5);
        const Temp_Type X006 = AT(0, 6);
        const Temp_Type X007 = AT(0, 7);
        const Temp_Type X010 = D(F(0.415735f) * AT(1, 0) + F(0.791065f) * AT(3, 0) + F(-0.352443f) * AT(5, 0) + F(0.277785f) * AT(7, 0));
        const Temp_Type X011 = D(F(0.415735f) * AT(1, 1) + F(0.791065f) * AT(3, 1) + F(-0.352443f) * AT(5, 1) + F(0.277785f) * AT(7, 1));
        const Temp_Type X012 = D(F(0.415735f) * AT(1, 2) + F(0.791065f) * AT(3, 2) + F(-0.352443f) * AT(5, 2) + F(0.277785f) * AT(7, 2));
        const Temp_Type X013 = D(F(0.415735f) * AT(1, 3) + F(0.791065f) * AT(3, 3) + F(-0.352443f) * AT(5, 3) + F(0.277785f) * AT(7, 3));
        const Temp_Type X014 = D(F(0.415735f) * AT(1, 4) + F(0.791065f) * AT(3, 4) + F(-0.352443f) * AT(5, 4) + F(0.277785f) * AT(7, 4));
        const Temp_Type X015 = D(F(0.415735f) * AT(1, 5) + F(0.791065f) * AT(3, 5) + F(-0.352443f) * AT(5, 5) + F(0.277785f) * AT(7, 5));
        const Temp_Type X016 = D(F(0.415735f) * AT(1, 6) + F(0.791065f) * AT(3, 6) + F(-0.352443f) * AT(5, 6) + F(0.277785f) * AT(7, 6));
        const Temp_Type X017 = D(F(0.415735f) * AT(1, 7) + F(0.791065f) * AT(3, 7) + F(-0.352443f) * AT(5, 7) + F(0.277785f) * AT(7, 7));
        const Temp_Type X020 = AT(4, 0);
        const Temp_Type X021 = AT(4, 1);
        const Temp_Type X022 = AT(4, 2);
        const Temp_Type X023 = AT(4, 3);
        const Temp_Type X024 = AT(4, 4);
        const Temp_Type X025 = AT(4, 5);
        const Temp_Type X026 = AT(4, 6);
        const Temp_Type X027 = AT(4, 7);
        const Temp_Type X030 = D(F(0.022887f) * AT(1, 0) + F(-0.097545f) * AT(3, 0) + F(0.490393f) * AT(5, 0) + F(0.865723f) * AT(7, 0));
        const Temp_Type X031 = D(F(0.022887f) * AT(1, 1) + F(-0.097545f) * AT(3, 1) + F(0.490393f) * AT(5, 1) + F(0.865723f) * AT(7, 1));
        const Temp_Type X032 = D(F(0.022887f) * AT(1, 2) + F(-0.097545f) * AT(3, 2) + F(0.490393f) * AT(5, 2) + F(0.865723f) * AT(7, 2));
        const Temp_Type X033 = D(F(0.022887f) * AT(1, 3) + F(-0.097545f) * AT(3, 3) + F(0.490393f) * AT(5, 3) + F(0.865723f) * AT(7, 3));
        const Temp_Type X034 = D(F(0.022887f) * AT(1, 4) + F(-0.097545f) * AT(3, 4) + F(0.490393f) * AT(5, 4) + F(0.865723f) * AT(7, 4));
        const Temp_Type X035 = D(F(0.022887f) * AT(1, 5) + F(-0.097545f) * AT(3, 5) + F(0.490393f) * AT(5, 5) + F(0.865723f) * AT(7, 5));
        const Temp_Type X036 = D(F(0.022887f) * AT(1, 6) + F(-0.097545f) * AT(3, 6) + F(0.490393f) * AT(5, 6) + F(0.865723f) * AT(7, 6));
        const Temp_Type X037 = D(F(0.022887f) * AT(1, 7) + F(-0.097545f) * AT(3, 7) + F(0.490393f) * AT(5, 7) + F(0.865723f) * AT(7, 7));

        // 4x4 = 4x8 times 8x4, matrix 1 is constant
        P.at(0, 0) = X000;
        P.at(0, 1) = D(X001 * F(0.415735f) + X003 * F(0.791065f) + X005 * F(-0.352443f) + X007 * F(0.277785f));
        P.at(0, 2) = X004;
        P.at(0, 3) = D(X001 * F(0.022887f) + X003 * F(-0.097545f) + X005 * F(0.490393f) + X007 * F(0.865723f));
        P.at(1, 0) = X010;
        P.at(1, 1) = D(X011 * F(0.415735f) + X013 * F(0.791065f) + X015 * F(-0.352443f) + X017 * F(0.277785f));
        P.at(1, 2) = X014;
        P.at(1, 3) = D(X011 * F(0.022887f) + X013 * F(-0.097545f) + X015 * F(0.490393f) + X017 * F(0.865723f));
        P.at(2, 0) = X020;
        P.at(2, 1) = D(X021 * F(0.415735f) + X023 * F(0.791065f) + X025 * F(-0.352443f) + X027 * F(0.277785f));
        P.at(2, 2) = X024;
        P.at(2, 3) = D(X021 * F(0.022887f) + X023 * F(-0.097545f) + X025 * F(0.490393f) + X027 * F(0.865723f));
        P.at(3, 0) = X030;
        P.at(3, 1) = D(X031 * F(0.415735f) + X033 * F(0.791065f) + X035 * F(-0.352443f) + X037 * F(0.277785f));
        P.at(3, 2) = X034;
        P.at(3, 3) = D(X031 * F(0.022887f) + X033 * F(-0.097545f) + X035 * F(0.490393f) + X037 * F(0.865723f));
        // 40 muls 24 adds

        // 4x4 = 4x8 times 8x4, matrix 1 is constant
        Q.at(0, 0) = D(X001 * F(0.906127f) + X003 * F(-0.318190f) + X005 * F(0.212608f) + X007 * F(-0.180240f));
        Q.at(0, 1) = X002;
        Q.at(0, 2) = D(X001 * F(-0.074658f) + X003 * F(0.513280f) + X005 * F(0.768178f) + X007 * F(-0.375330f));
        Q.at(0, 3) = X006;
        Q.at(1, 0) = D(X011 * F(0.906127f) + X013 * F(-0.318190f) + X015 * F(0.212608f) + X017 * F(-0.180240f));
        Q.at(1, 1) = X012;
        Q.at(1, 2) = D(X011 * F(-0.074658f) + X013 * F(0.513280f) + X015 * F(0.768178f) + X017 * F(-0.375330f));
        Q.at(1, 3) = X016;
        Q.at(2, 0) = D(X021 * F(0.906127f) + X023 * F(-0.318190f) + X025 * F(0.212608f) + X027 * F(-0.180240f));
        Q.at(2, 1) = X022;
        Q.at(2, 2) = D(X021 * F(-0.074658f) + X023 * F(0.513280f) + X025 * F(0.768178f) + X027 * F(-0.375330f));
        Q.at(2, 3) = X026;
        Q.at(3, 0) = D(X031 * F(0.906127f) + X033 * F(-0.318190f) + X035 * F(0.212608f) + X037 * F(-0.180240f));
        Q.at(3, 1) = X032;
        Q.at(3, 2) = D(X031 * F(-0.074658f) + X033 * F(0.513280f) + X035 * F(0.768178f) + X037 * F(-0.375330f));
        Q.at(3, 3) = X036;
        // 40 muls 24 adds
    }
};

template<int NUM_ROWS, int NUM_COLS>
struct R_S
{
    static void calc(Matrix44& R, Matrix44& S, const jpgd_block_t* pSrc)
    {
	// 4x8 = 4x8 times 8x8, matrix 0 is constant
        const Temp_Type X100 = D(F(0.906127f) * AT(1, 0) + F(-0.318190f) * AT(3, 0) + F(0.212608f) * AT(5, 0) + F(-0.180240f) * AT(7, 0));
        const Temp_Type X101 = D(F(0.906127f) * AT(1, 1) + F(-0.318190f) * AT(3, 1) + F(0.212608f) * AT(5, 1) + F(-0.180240f) * AT(7, 1));
        const Temp_Type X102 = D(F(0.906127f) * AT(1, 2) + F(-0.318190f) * AT(3, 2) + F(0.212608f) * AT(5, 2) + F(-0.180240f) * AT(7, 2));
        const Temp_Type X103 = D(F(0.906127f) * AT(1, 3) + F(-0.318190f) * AT(3, 3) + F(0.212608f) * AT(5, 3) + F(-0.180240f) * AT(7, 3));
        const Temp_Type X104 = D(F(0.906127f) * AT(1, 4) + F(-0.318190f) * AT(3, 4) + F(0.212608f) * AT(5, 4) + F(-0.180240f) * AT(7, 4));
        const Temp_Type X105 = D(F(0.906127f) * AT(1, 5) + F(-0.318190f) * AT(3, 5) + F(0.212608f) * AT(5, 5) + F(-0.180240f) * AT(7, 5));
        const Temp_Type X106 = D(F(0.906127f) * AT(1, 6) + F(-0.318190f) * AT(3, 6) + F(0.212608f) * AT(5, 6) + F(-0.180240f) * AT(7, 6));
        const Temp_Type X107 = D(F(0.906127f) * AT(1, 7) + F(-0.318190f) * AT(3, 7) + F(0.212608f) * AT(5, 7) + F(-0.180240f) * AT(7, 7));
        const Temp_Type X110 = AT(2, 0);
        const Temp_Type X111 = AT(2, 1);
        const Temp_Type X112 = AT(2, 2);
        const Temp_Type X113 = AT(2, 3);
        const Temp_Type X114 = AT(2, 4);
        const Temp_Type X115 = AT(2, 5);
        const Temp_Type X116 = AT(2, 6);
        const Temp_Type X117 = AT(2, 7);
        const Temp_Type X120 = D(F(-0.074658f) * AT(1, 0) + F(0.513280f) * AT(3, 0) + F(0.768178f) * AT(5, 0) + F(-0.375330f) * AT(7, 0));
        const Temp_Type X121 = D(F(-0.074658f) * AT(1, 1) + F(0.513280f) * AT(3, 1) + F(0.768178f) * AT(5, 1) + F(-0.375330f) * AT(7, 1));
        const Temp_Type X122 = D(F(-0.074658f) * AT(1, 2) + F(0.513280f) * AT(3, 2) + F(0.768178f) * AT(5, 2) + F(-0.375330f) * AT(7, 2));
        const Temp_Type X123 = D(F(-0.074658f) * AT(1, 3) + F(0.513280f) * AT(3, 3) + F(0.768178f) * AT(5, 3) + F(-0.375330f) * AT(7, 3));
        const Temp_Type X124 = D(F(-0.074658f) * AT(1, 4) + F(0.513280f) * AT(3, 4) + F(0.768178f) * AT(5, 4) + F(-0.375330f) * AT(7, 4));
        const Temp_Type X125 = D(F(-0.074658f) * AT(1, 5) + F(0.513280f) * AT(3, 5) + F(0.768178f) * AT(5, 5) + F(-0.375330f) * AT(7, 5));
        const Temp_Type X126 = D(F(-0.074658f) * AT(1, 6) + F(0.513280f) * AT(3, 6) + F(0.768178f) * AT(5, 6) + F(-0.375330f) * AT(7, 6));
        const Temp_Type X127 = D(F(-0.074658f) * AT(1, 7) + F(0.513280f) * AT(3, 7) + F(0.768178f) * AT(5, 7) + F(-0.375330f) * AT(7, 7));
        const Temp_Type X130 = AT(6, 0);
        const Temp_Type X131 = AT(6, 1);
        const Temp_Type X132 = AT(6, 2);
        const Temp_Type X133 = AT(6, 3);
        const Temp_Type X134 = AT(6, 4);
        const Temp_Type X135 = AT(6, 5);
        const Temp_Type X136 = AT(6, 6);
        const Temp_Type X137 = AT(6, 7);
        // 80 muls 48 adds

        // 4x4 = 4x8 times 8x4, matrix 1 is constant
        R.at(0, 0) = X100;
        R.at(0, 1) = D(X101 * F(0.415735f) + X103 * F(0.791065f) + X105 * F(-0.352443f) + X107 * F(0.277785f));
        R.at(0, 2) = X104;
        R.at(0, 3) = D(X101 * F(0.022887f) + X103 * F(-0.097545f) + X105 * F(0.490393f) + X107 * F(0.865723f));
        R.at(1, 0) = X110;
        R.at(1, 1) = D(X111 * F(0.415735f) + X113 * F(0.791065f) + X115 * F(-0.352443f) + X117 * F(0.277785f));
        R.at(1, 2) = X114;
        R.at(1, 3) = D(X111 * F(0.022887f) + X113 * F(-0.097545f) + X115 * F(0.490393f) + X117 * F(0.865723f));
        R.at(2, 0) = X120;
        R.at(2, 1) = D(X121 * F(0.415735f) + X123 * F(0.791065f) + X125 * F(-0.352443f) + X127 * F(0.277785f));
        R.at(2, 2) = X124;
        R.at(2, 3) = D(X121 * F(0.022887f) + X123 * F(-0.097545f) + X125 * F(0.490393f) + X127 * F(0.865723f));
        R.at(3, 0) = X130;
        R.at(3, 1) = D(X131 * F(0.415735f) + X133 * F(0.791065f) + X135 * F(-0.352443f) + X137 * F(0.277785f));
        R.at(3, 2) = X134;
        R.at(3, 3) = D(X131 * F(0.022887f) + X133 * F(-0.097545f) + X135 * F(0.490393f) + X137 * F(0.865723f));
        // 40 muls 24 adds
        // 4x4 = 4x8 times 8x4, matrix 1 is constant
        S.at(0, 0) = D(X101 * F(0.906127f) + X103 * F(-0.318190f) + X105 * F(0.212608f) + X107 * F(-0.180240f));
        S.at(0, 1) = X102;
        S.at(0, 2) = D(X101 * F(-0.074658f) + X103 * F(0.513280f) + X105 * F(0.768178f) + X107 * F(-0.375330f));
        S.at(0, 3) = X106;
        S.at(1, 0) = D(X111 * F(0.906127f) + X113 * F(-0.318190f) + X115 * F(0.212608f) + X117 * F(-0.180240f));
        S.at(1, 1) = X112;
        S.at(1, 2) = D(X111 * F(-0.074658f) + X113 * F(0.513280f) + X115 * F(0.768178f) + X117 * F(-0.375330f));
        S.at(1, 3) = X116;
        S.at(2, 0) = D(X121 * F(0.906127f) + X123 * F(-0.318190f) + X125 * F(0.212608f) + X127 * F(-0.180240f));
        S.at(2, 1) = X122;
        S.at(2, 2) = D(X121 * F(-0.074658f) + X123 * F(0.513280f) + X125 * F(0.768178f) + X127 * F(-0.375330f));
        S.at(2, 3) = X126;
        S.at(3, 0) = D(X131 * F(0.906127f) + X133 * F(-0.318190f) + X135 * F(0.212608f) + X137 * F(-0.180240f));
        S.at(3, 1) = X132;
        S.at(3, 2) = D(X131 * F(-0.074658f) + X133 * F(0.513280f) + X135 * F(0.768178f) + X137 * F(-0.375330f));
        S.at(3, 3) = X136;
        // 40 muls 24 adds
    }
};

// Unconditionally frees all allocated m_blocks.
static void jd_free_all_blocks( jpeg_decoder* jd )
{
    jd->m_pStream = 0;
    for( mem_block* b = jd->m_pMem_blocks; b; )
    {
	mem_block* n = b->m_pNext;
	jpgd_free( b );
	b = n;
    }
    jd->m_pMem_blocks = NULL;
}

// This method handles all errors. It will never return.
// It could easily be changed to use C++ exceptions.
JPGD_NORETURN void jd_stop_decoding( jpgd_status status, jpeg_decoder* jd )
{
    jd->m_error_code = status;
    jd_free_all_blocks( jd );
    longjmp( jd->m_jmp_state, status );
}

static void* jd_alloc( size_t nSize, bool zero, jpeg_decoder* jd )
{
    nSize = ( JPGD_MAX( nSize, 1 ) + 3 ) & ~3;
    char* rv = NULL;
    for( mem_block* b = jd->m_pMem_blocks; b; b = b->m_pNext )
    {
	if( ( b->m_used_count + nSize ) <= b->m_size )
	{
    	    rv = b->m_data + b->m_used_count;
    	    b->m_used_count += nSize;
    	    break;
	}
    }
    if( !rv )
    {
	int capacity = JPGD_MAX( 32768 - 256, ( nSize + 2047 ) & ~2047 );
	mem_block* b = (mem_block*)jpgd_malloc( sizeof( mem_block ) + capacity );
	if( !b ) { jd_stop_decoding( JPGD_NOTENOUGHMEM, jd ); }
	b->m_pNext = jd->m_pMem_blocks; jd->m_pMem_blocks = b;
	b->m_used_count = nSize;
	b->m_size = capacity;
	rv = b->m_data;
    }
    if( zero ) memset( rv, 0, nSize );
    return rv;
}

static void jd_word_clear( void* p, uint16 c, uint n )
{
    uchar* pD = (uchar*)p;
    const uchar l = c & 0xFF, h = (c >> 8) & 0xFF;
    while( n )
    {
	pD[0] = l; pD[1] = h; pD += 2;
	n--;
    }
}

static int jd_read( uchar* pBuf, int max_bytes_to_read, jpeg_decoder* jd )
{
    if( !jd->m_pStream ) return -1;

    if( jd->m_eof_flag )
    {
        return 0;
    }
        
    int bytes_read = static_cast<int>( bfs_read( pBuf, 1, max_bytes_to_read, jd->m_pStream ) );
    if( bytes_read < max_bytes_to_read )
    {
        jd->m_eof_flag = true;
    }
        
    return bytes_read;
}

// Refill the input buffer.
// This method will sit in a loop until (A) the buffer is full or (B)
// the stream's read() method reports and end of file condition.
static void jd_prep_in_buffer( jpeg_decoder* jd )
{
    jd->m_in_buf_left = 0;
    jd->m_pIn_buf_ofs = jd->m_in_buf;

    if( jd->m_eof_flag )
	return;

    do
    {
	int bytes_read = jd_read( jd->m_in_buf + jd->m_in_buf_left, JPGD_IN_BUF_SIZE - jd->m_in_buf_left, jd );
	if( bytes_read == -1 )
    	    jd_stop_decoding( JPGD_STREAM_READ, jd );

	jd->m_in_buf_left += bytes_read;
    } while( ( jd->m_in_buf_left < JPGD_IN_BUF_SIZE ) && ( !jd->m_eof_flag ) );

    jd->m_total_bytes_read += jd->m_in_buf_left;

    // Pad the end of the block with M_EOI (prevents the decompressor from going off the rails if the stream is invalid).
    // (This dates way back to when this decompressor was written in C/asm, and the all-asm Huffman decoder did some fancy things to increase perf.)
    jd_word_clear( jd->m_pIn_buf_ofs + jd->m_in_buf_left, 0xD9FF, 64 );
}

// Read a Huffman code table.
static void jd_read_dht_marker( jpeg_decoder* jd )
{
    int i, index, count;
    uchar huff_num[ 17 ];
    uchar huff_val[ 256 ];

    uint num_left = jd_get_bits( 16, jd );

    if( num_left < 2 )
	jd_stop_decoding( JPGD_BAD_DHT_MARKER, jd );

    num_left -= 2;

    while( num_left )
    {
	index = jd_get_bits( 8, jd );

	huff_num[ 0 ] = 0;

	count = 0;

	for( i = 1; i <= 16; i++ )
	{
    	    huff_num[ i ] = static_cast<uchar>( jd_get_bits( 8, jd ) );
    	    count += huff_num[ i ];
	}

	if( count > 255 )
    	    jd_stop_decoding( JPGD_BAD_DHT_COUNTS, jd );

	for( i = 0; i < count; i++ )
    	    huff_val[ i ] = static_cast<uchar>( jd_get_bits( 8, jd ) );

	i = 1 + 16 + count;

	if( num_left < (uint)i )
    	    jd_stop_decoding( JPGD_BAD_DHT_MARKER, jd );

	num_left -= i;

	if( ( index & 0x10 ) > 0x10 )
    	    jd_stop_decoding( JPGD_BAD_DHT_INDEX, jd );

	index = ( index & 0x0F ) + ( ( index & 0x10 ) >> 4 ) * ( JPGD_MAX_HUFF_TABLES >> 1 );

	if( index >= JPGD_MAX_HUFF_TABLES )
    	    jd_stop_decoding( JPGD_BAD_DHT_INDEX, jd );

	if( !jd->m_huff_num[ index ] )
    	    jd->m_huff_num[ index ] = (uchar*)jd_alloc( 17, 0, jd );

	if( !jd->m_huff_val[ index ] )
    	    jd->m_huff_val[ index ] = (uchar*)jd_alloc( 256, 0, jd );

	jd->m_huff_ac[ index ] = ( index & 0x10 ) != 0;
	memcpy( jd->m_huff_num[ index ], huff_num, 17 );
	memcpy( jd->m_huff_val[ index ], huff_val, 256 );
    }
}

// Read a quantization table.
static void jd_read_dqt_marker( jpeg_decoder* jd )
{
    int n, i, prec;
    uint num_left;
    uint temp;

    num_left = jd_get_bits( 16, jd );

    if( num_left < 2 )
	jd_stop_decoding( JPGD_BAD_DQT_MARKER, jd );

    num_left -= 2;

    while( num_left )
    {
	n = jd_get_bits( 8, jd );
	prec = n >> 4;
	n &= 0x0F;

	if( n >= JPGD_MAX_QUANT_TABLES )
    	    jd_stop_decoding( JPGD_BAD_DQT_TABLE, jd );

	if( !jd->m_quant[ n ] )
    	    jd->m_quant[ n ] = (jpgd_quant_t*)jd_alloc( 64 * sizeof( jpgd_quant_t ), 0, jd );

	// read quantization entries, in zag order
	for( i = 0; i < 64; i++ )
	{
    	    temp = jd_get_bits( 8, jd );

	    if( prec )
    		temp = ( temp << 8 ) + jd_get_bits( 8, jd );

    	    jd->m_quant[ n ][ i ] = static_cast<jpgd_quant_t>( temp );
	}

	i = 64 + 1;

	if( prec )
    	    i += 64;

	if( num_left < (uint)i )
    	    jd_stop_decoding( JPGD_BAD_DQT_LENGTH, jd );

	num_left -= i;
    }
}

// Read the start of frame (SOF) marker.
static void jd_read_sof_marker( jpeg_decoder* jd )
{
    int i;
    uint num_left;

    num_left = jd_get_bits( 16, jd );

    if( jd_get_bits( 8, jd ) != 8 )   /* precision: sorry, only 8-bit precision is supported right now */
	jd_stop_decoding( JPGD_BAD_PRECISION, jd );

    jd->m_image_y_size = jd_get_bits( 16, jd );

    if( ( jd->m_image_y_size < 1 ) || ( jd->m_image_y_size > JPGD_MAX_HEIGHT ) )
	jd_stop_decoding( JPGD_BAD_HEIGHT, jd );

    jd->m_image_x_size = jd_get_bits( 16, jd );

    if( ( jd->m_image_x_size < 1 ) || ( jd->m_image_x_size > JPGD_MAX_WIDTH ) )
	jd_stop_decoding( JPGD_BAD_WIDTH, jd );

    jd->m_comps_in_frame = jd_get_bits( 8, jd );

    if( jd->m_comps_in_frame > JPGD_MAX_COMPONENTS )
	jd_stop_decoding( JPGD_TOO_MANY_COMPONENTS, jd );

    if( num_left != (uint)( jd->m_comps_in_frame * 3 + 8 ) )
	jd_stop_decoding( JPGD_BAD_SOF_LENGTH, jd );

    for( i = 0; i < jd->m_comps_in_frame; i++ )
    {
	jd->m_comp_ident[ i ]  = jd_get_bits( 8, jd );
	jd->m_comp_h_samp[ i ] = jd_get_bits( 4, jd );
	jd->m_comp_v_samp[ i ] = jd_get_bits( 4, jd );
	jd->m_comp_quant[ i ]  = jd_get_bits( 8, jd );
    }
}

// Used to skip unrecognized markers.
static void jd_skip_variable_marker( jpeg_decoder* jd )
{
    uint num_left;

    num_left = jd_get_bits( 16, jd );

    if( num_left < 2 )
	jd_stop_decoding( JPGD_BAD_VARIABLE_MARKER, jd );

    num_left -= 2;

    while( num_left )
    {
	jd_get_bits( 8, jd );
	num_left--;
    }
}

// Read a define restart interval (DRI) marker.
static void jd_read_dri_marker( jpeg_decoder* jd )
{
    if( jd_get_bits( 16, jd ) != 4 )
	jd_stop_decoding( JPGD_BAD_DRI_LENGTH, jd );

    jd->m_restart_interval = jd_get_bits( 16, jd );
}

// Read a start of scan (SOS) marker.
static void jd_read_sos_marker( jpeg_decoder* jd )
{
    uint num_left;
    int i, ci, n, c, cc;

    num_left = jd_get_bits( 16, jd );

    n = jd_get_bits( 8, jd );

    jd->m_comps_in_scan = n;

    num_left -= 3;

    if ( ( num_left != (uint)( n * 2 + 3 ) ) || ( n < 1 ) || ( n > JPGD_MAX_COMPS_IN_SCAN ) )
	jd_stop_decoding( JPGD_BAD_SOS_LENGTH, jd );

    for( i = 0; i < n; i++ )
    {
	cc = jd_get_bits( 8, jd );
	c = jd_get_bits( 8, jd );
	num_left -= 2;

	for( ci = 0; ci < jd->m_comps_in_frame; ci++ )
    	    if( cc == jd->m_comp_ident[ ci ] )
    		break;

	if( ci >= jd->m_comps_in_frame )
    	    jd_stop_decoding( JPGD_BAD_SOS_COMP_ID, jd );

	jd->m_comp_list[ i ]    = ci;
	jd->m_comp_dc_tab[ ci ] = (c >> 4) & 15;
	jd->m_comp_ac_tab[ ci ] = (c & 15) + (JPGD_MAX_HUFF_TABLES >> 1);
    }

    jd->m_spectral_start  = jd_get_bits( 8, jd );
    jd->m_spectral_end    = jd_get_bits( 8, jd );
    jd->m_successive_high = jd_get_bits( 4, jd );
    jd->m_successive_low  = jd_get_bits( 4, jd );

    if( !jd->m_progressive_flag )
    {
	jd->m_spectral_start = 0;
	jd->m_spectral_end = 63;
    }

    num_left -= 3;

    while( num_left )                  /* read past whatever is num_left */
    {
	jd_get_bits( 8, jd );
	num_left--;
    }
}

// Finds the next marker.
static int jd_next_marker( jpeg_decoder* jd )
{
    uint c, bytes;

    bytes = 0;

    do
    {
	do
	{
    	    bytes++;
    	    c = jd_get_bits( 8, jd );
	} while( c != 0xFF );

	do
	{
    	    c = jd_get_bits( 8, jd );
	} while( c == 0xFF );

    } while( c == 0 );

    // If bytes > 0 here, there where extra bytes before the marker (not good).

    return c;
}

// Process markers. Returns when an SOFx, SOI, EOI, or SOS marker is
// encountered.
static int jd_process_markers( jpeg_decoder* jd )
{
    int c;

    for ( ; ; )
    {
	c = jd_next_marker( jd );

	/*switch( c )
	{
    	    case M_SOF0: printf( "SOF0\n" ); break;
    	    case M_SOF1: printf( "SOF1\n" ); break;
    	    case M_SOF2: printf( "SOF2\n" ); break;
    	    case M_SOF3: printf( "SOF3\n" ); break;
    	    case M_SOF5: printf( "SOF5\n" ); break;
    	    case M_SOF6: printf( "SOF6\n" ); break;
    	    case M_SOF7: printf( "SOF7\n" ); break;
    	    case M_SOF9: printf( "SOF9\n" ); break;
    	    case M_SOF10: printf( "SOF10\n" ); break;
    	    case M_SOF11: printf( "SOF11\n" ); break;
    	    case M_SOF13: printf( "SOF13\n" ); break;
    	    case M_SOF14: printf( "SOF14\n" ); break;
    	    case M_SOF15: printf( "SOF15\n" ); break;
    	    case M_SOI: printf( "SOI\n" ); break;
    	    case M_EOI: printf( "EOI\n" ); break;
    	    case M_SOS: printf( "SOS\n" ); break;
    	    case M_DHT: printf( "DHT\n" ); break;
    	    case M_DAC: printf( "DAC\n" ); break;
    	    case M_DQT: printf( "DQT\n" ); break;
    	    case M_DRI: printf( "DRI\n" ); break;
    	    case M_JPG: printf( "JPG\n" ); break;
    	    case M_RST0: printf( "RST0\n" ); break;
    	    case M_RST1: printf( "RST1\n" ); break;
    	    case M_RST2: printf( "RST2\n" ); break;
    	    case M_RST3: printf( "RST3\n" ); break;
    	    case M_RST4: printf( "RST4\n" ); break;
    	    case M_RST5: printf( "RST5\n" ); break;
    	    case M_RST6: printf( "RST6\n" ); break;
    	    case M_RST7: printf( "RST7\n" ); break;
    	    case M_TEM: printf( "TEM\n" ); break;
	}*/

	switch( c )
	{
    	    case M_SOF0:
    	    case M_SOF1:
    	    case M_SOF2:
    	    case M_SOF3:
    	    case M_SOF5:
    	    case M_SOF6:
    	    case M_SOF7:
//	    case M_JPG:
    	    case M_SOF9:
    	    case M_SOF10:
    	    case M_SOF11:
    	    case M_SOF13:
    	    case M_SOF14:
    	    case M_SOF15:
    	    case M_SOI:
    	    case M_EOI:
    	    case M_SOS:
    		    return c;
    	    case M_DHT:
    		    jd_read_dht_marker( jd );
    		    break;
    	    // No arithmitic support - dumb patents!
    	    case M_DAC:
    		jd_stop_decoding( JPGD_NO_ARITHMITIC_SUPPORT, jd );
    		break;
    	    case M_DQT:
    		jd_read_dqt_marker( jd );
    		break;
    	    case M_DRI:
    		jd_read_dri_marker( jd );
    		break;
//	    case M_APP0:  /* no need to read the JFIF marker */
    	    case M_JPG:
    	    case M_RST0:    /* no parameters */
    	    case M_RST1:
    	    case M_RST2:
    	    case M_RST3:
    	    case M_RST4:
    	    case M_RST5:
    	    case M_RST6:
    	    case M_RST7:
    	    case M_TEM:
    		jd_stop_decoding( JPGD_UNEXPECTED_MARKER, jd );
    		break;
    	    default:    /* must be DNL, DHP, EXP, APPn, JPGn, COM, or RESn or APP0 */
    		jd_skip_variable_marker( jd );
    		break;
	}
    }
}

// Finds the start of image (SOI) marker.
// This code is rather defensive: it only checks the first 512 bytes to avoid
// false positives.
static void jd_locate_soi_marker( jpeg_decoder* jd )
{
    uint lastchar, thischar;
    uint bytesleft;

    lastchar = jd_get_bits( 8, jd );

    thischar = jd_get_bits( 8, jd );

    /* ok if it's a normal JPEG file without a special header */

    if( ( lastchar == 0xFF ) && ( thischar == M_SOI ) )
	return;

    bytesleft = 4096; //512;

    for ( ; ; )
    {
	if( --bytesleft == 0 )
    	    jd_stop_decoding( JPGD_NOT_JPEG, jd );

	lastchar = thischar;

	thischar = jd_get_bits( 8, jd );

	if( lastchar == 0xFF )
	{
    	    if( thischar == M_SOI )
    		break;
    	    else if( thischar == M_EOI ) // get_bits will keep returning M_EOI if we read past the end
    		jd_stop_decoding( JPGD_NOT_JPEG, jd );
	}
    }

    // Check the next character after marker: if it's not 0xFF, it can't be the start of the next marker, so the file is bad.
    thischar = ( jd->m_bit_buf >> 24 ) & 0xFF;

    if( thischar != 0xFF )
	jd_stop_decoding( JPGD_NOT_JPEG, jd );
}

// Find a start of frame (SOF) marker.
static void jd_locate_sof_marker( jpeg_decoder* jd )
{
    jd_locate_soi_marker( jd );

    int c = jd_process_markers( jd );

    switch( c )
    {
	case M_SOF2:
    	    jd->m_progressive_flag = JPGD_TRUE;
	case M_SOF0:  /* baseline DCT */
	case M_SOF1:  /* extended sequential DCT */
    	    jd_read_sof_marker( jd );
    	    break;
	case M_SOF9:  /* Arithmitic coding */
    	    jd_stop_decoding( JPGD_NO_ARITHMITIC_SUPPORT, jd );
    	    break;
	default:
            jd_stop_decoding( JPGD_UNSUPPORTED_MARKER, jd );
    	    break;
    }
}

// Find a start of scan (SOS) marker.
static int jd_locate_sos_marker( jpeg_decoder* jd )
{
    int c;

    c = jd_process_markers( jd );

    if( c == M_EOI )
	return JPGD_FALSE;
    else if( c != M_SOS )
	jd_stop_decoding( JPGD_UNEXPECTED_MARKER, jd );

    jd_read_sos_marker( jd );

    return JPGD_TRUE;
}

#define SCALEBITS 16
#define ONE_HALF  ((int) 1 << (SCALEBITS-1))
#define FIX(x)    ((int) ((x) * (1L<<SCALEBITS) + 0.5f))

// Create a few tables that allow us to quickly convert YCbCr to RGB.
static void jd_create_look_ups( jpeg_decoder* jd )
{
    for( int i = 0; i <= 255; i++ )
    {
	int k = i - 128;
	jd->m_crr[ i ] = ( FIX(1.40200f) * k + ONE_HALF ) >> SCALEBITS;
	jd->m_cbb[ i ] = ( FIX(1.77200f) * k + ONE_HALF ) >> SCALEBITS;
	jd->m_crg[ i ] = ( -FIX(0.71414f) ) * k;
	jd->m_cbg[ i ] = ( -FIX(0.34414f) ) * k + ONE_HALF;
    }
}

// This method throws back into the stream any bytes that where read
// into the bit buffer during initial marker scanning.
static void jd_fix_in_buffer( jpeg_decoder* jd )
{
    // In case any 0xFF's where pulled into the buffer during marker scanning.
    JPGD_ASSERT( ( jd->m_bits_left & 7 ) == 0 );

    if( jd->m_bits_left == 16 )
	jd_stuff_char( (uchar)( jd->m_bit_buf & 0xFF ), jd );

    if( jd->m_bits_left >= 8)
	jd_stuff_char( (uchar)( ( jd->m_bit_buf >> 8 ) & 0xFF ), jd );

    jd_stuff_char( (uchar)( ( jd->m_bit_buf >> 16 ) & 0xFF ), jd );
    jd_stuff_char( (uchar)( ( jd->m_bit_buf >> 24 ) & 0xFF ), jd );

    jd->m_bits_left = 16;
    jd_get_bits_no_markers( 16, jd );
    jd_get_bits_no_markers( 16, jd );
}

static void jd_transform_mcu( int mcu_row, jpeg_decoder* jd )
{
    jpgd_block_t* pSrc_ptr = jd->m_pMCU_coefficients;
    uchar* pDst_ptr = jd->m_pSample_buf + mcu_row * jd->m_blocks_per_mcu * 64;

    for( int mcu_block = 0; mcu_block < jd->m_blocks_per_mcu; mcu_block++ )
    {
	jd_idct( pSrc_ptr, pDst_ptr, jd->m_mcu_block_max_zag[ mcu_block ] );
	pSrc_ptr += 64;
	pDst_ptr += 64;
    }
}

static const uchar s_max_rc[ 64 ] =
{
    17, 18, 34, 50, 50, 51, 52, 52, 52, 68, 84, 84, 84, 84, 85, 86, 86, 86, 86, 86,
    102, 118, 118, 118, 118, 118, 118, 119, 120, 120, 120, 120, 120, 120, 120, 136,
    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136
};

static void jd_transform_mcu_expand( int mcu_row, jpeg_decoder* jd )
{
    jpgd_block_t* pSrc_ptr = jd->m_pMCU_coefficients;
    uchar* pDst_ptr = jd->m_pSample_buf + mcu_row * jd->m_expanded_blocks_per_mcu * 64;

    // Y IDCT
    int mcu_block;
    for( mcu_block = 0; mcu_block < jd->m_expanded_blocks_per_component; mcu_block++ )
    {
	jd_idct( pSrc_ptr, pDst_ptr, jd->m_mcu_block_max_zag[ mcu_block ] );
	pSrc_ptr += 64;
	pDst_ptr += 64;
    }

    // Chroma IDCT, with upsampling
    jpgd_block_t temp_block[ 64 ];

    for( int i = 0; i < 2; i++ )
    {
	Matrix44 P, Q, R, S;

	JPGD_ASSERT( jd->m_mcu_block_max_zag[ mcu_block ] >= 1 );
	JPGD_ASSERT( jd->m_mcu_block_max_zag[ mcu_block ] <= 64 );

	int max_zag = jd->m_mcu_block_max_zag[ mcu_block++ ] - 1; 
	if( max_zag <= 0 ) max_zag = 0; // should never happen, only here to shut up static analysis
	switch( s_max_rc[ max_zag ] )
	{
	    case 1*16+1:
    		P_Q<1, 1>::calc(P, Q, pSrc_ptr);
    		R_S<1, 1>::calc(R, S, pSrc_ptr);
    		break;
	    case 1*16+2:
    		P_Q<1, 2>::calc(P, Q, pSrc_ptr);
    		R_S<1, 2>::calc(R, S, pSrc_ptr);
    		break;
	    case 2*16+2:
		P_Q<2, 2>::calc(P, Q, pSrc_ptr);
		R_S<2, 2>::calc(R, S, pSrc_ptr);
		break;
	    case 3*16+2:
		P_Q<3, 2>::calc(P, Q, pSrc_ptr);
		R_S<3, 2>::calc(R, S, pSrc_ptr);
		break;
	    case 3*16+3:
		P_Q<3, 3>::calc(P, Q, pSrc_ptr);
		R_S<3, 3>::calc(R, S, pSrc_ptr);
		break;
	    case 3*16+4:
		P_Q<3, 4>::calc(P, Q, pSrc_ptr);
		R_S<3, 4>::calc(R, S, pSrc_ptr);
		break;
	    case 4*16+4:
		P_Q<4, 4>::calc(P, Q, pSrc_ptr);
		R_S<4, 4>::calc(R, S, pSrc_ptr);
		break;
	    case 5*16+4:
		P_Q<5, 4>::calc(P, Q, pSrc_ptr);
		R_S<5, 4>::calc(R, S, pSrc_ptr);
		break;
	    case 5*16+5:
    		P_Q<5, 5>::calc(P, Q, pSrc_ptr);
		R_S<5, 5>::calc(R, S, pSrc_ptr);
		break;
	    case 5*16+6:
		P_Q<5, 6>::calc(P, Q, pSrc_ptr);
		R_S<5, 6>::calc(R, S, pSrc_ptr);
		break;
	    case 6*16+6:
		P_Q<6, 6>::calc(P, Q, pSrc_ptr);
		R_S<6, 6>::calc(R, S, pSrc_ptr);
		break;
	    case 7*16+6:
		P_Q<7, 6>::calc(P, Q, pSrc_ptr);
		R_S<7, 6>::calc(R, S, pSrc_ptr);
		break;
	    case 7*16+7:
		P_Q<7, 7>::calc(P, Q, pSrc_ptr);
		R_S<7, 7>::calc(R, S, pSrc_ptr);
		break;
	    case 7*16+8:
		P_Q<7, 8>::calc(P, Q, pSrc_ptr);
		R_S<7, 8>::calc(R, S, pSrc_ptr);
		break;
	    case 8*16+8:
		P_Q<8, 8>::calc(P, Q, pSrc_ptr);
		R_S<8, 8>::calc(R, S, pSrc_ptr);
		break;
	    default:
    		JPGD_ASSERT( false );
	}

	Matrix44 a(P + Q); P -= Q;
	Matrix44& b = P;
	Matrix44 c(R + S); R -= S;
	Matrix44& d = R;

	Matrix44::add_and_store(temp_block, a, c);
	jd_idct_4x4(temp_block, pDst_ptr);
	pDst_ptr += 64;

	Matrix44::sub_and_store(temp_block, a, c);
	jd_idct_4x4(temp_block, pDst_ptr);
	pDst_ptr += 64;

	Matrix44::add_and_store(temp_block, b, d);
	jd_idct_4x4(temp_block, pDst_ptr);
	pDst_ptr += 64;

	Matrix44::sub_and_store(temp_block, b, d);
	jd_idct_4x4(temp_block, pDst_ptr);
	pDst_ptr += 64;

	pSrc_ptr += 64;
    }
}

// Loads and dequantizes the next row of (already decoded) coefficients.
// Progressive images only.
static void jd_load_next_row( jpeg_decoder* jd )
{
    int i;
    jpgd_block_t* p;
    jpgd_quant_t* q;
    int mcu_row, mcu_block, row_block = 0;
    int component_num, component_id;
    int block_x_mcu[ JPGD_MAX_COMPONENTS ];

    memset( block_x_mcu, 0, JPGD_MAX_COMPONENTS * sizeof( int ) );

    for( mcu_row = 0; mcu_row < jd->m_mcus_per_row; mcu_row++ )
    {
	int block_x_mcu_ofs = 0, block_y_mcu_ofs = 0;

	for( mcu_block = 0; mcu_block < jd->m_blocks_per_mcu; mcu_block++ )
	{
    	    component_id = jd->m_mcu_org[ mcu_block ];
	    q = jd->m_quant[ jd->m_comp_quant[ component_id ] ];

	    p = jd->m_pMCU_coefficients + 64 * mcu_block;

	    jpgd_block_t* pAC = jd_coeff_buf_getp( jd->m_ac_coeffs[ component_id ], block_x_mcu[ component_id ] + block_x_mcu_ofs, jd->m_block_y_mcu[ component_id ] + block_y_mcu_ofs );
    	    jpgd_block_t* pDC = jd_coeff_buf_getp( jd->m_dc_coeffs[ component_id ], block_x_mcu[ component_id ] + block_x_mcu_ofs, jd->m_block_y_mcu[ component_id ] + block_y_mcu_ofs );
    	    p[ 0 ] = pDC[ 0 ];
    	    memcpy( &p[ 1 ], &pAC[ 1 ], 63 * sizeof( jpgd_block_t ) );

    	    for( i = 63; i > 0; i-- )
    		if( p[ g_ZAG[ i ] ] )
        	    break;

    	    jd->m_mcu_block_max_zag[ mcu_block ] = i + 1;

    	    for ( ; i >= 0; i-- )
		if( p[ g_ZAG[ i ] ] )
		    p[ g_ZAG[ i ] ] = static_cast<jpgd_block_t>( p[ g_ZAG[ i ] ] * q[ i ] );

    	    row_block++;

    	    if( jd->m_comps_in_scan == 1 )
    		block_x_mcu[ component_id ]++;
	    else
	    {
		if( ++block_x_mcu_ofs == jd->m_comp_h_samp[ component_id ] )
    		{
        	    block_x_mcu_ofs = 0;

        	    if( ++block_y_mcu_ofs == jd->m_comp_v_samp[ component_id ] )
        	    {
        		block_y_mcu_ofs = 0;

        		block_x_mcu[ component_id ] += jd->m_comp_h_samp[ component_id ];
        	    }
    		}
    	    }
	}

	if( jd->m_freq_domain_chroma_upsample )
    	    jd_transform_mcu_expand( mcu_row, jd );
	else
    	    jd_transform_mcu( mcu_row, jd );
    }

    if( jd->m_comps_in_scan == 1 )
	jd->m_block_y_mcu[ jd->m_comp_list[ 0 ] ]++;
    else
    {
	for( component_num = 0; component_num < jd->m_comps_in_scan; component_num++ )
	{
    	    component_id = jd->m_comp_list[ component_num ];

    	    jd->m_block_y_mcu[ component_id ] += jd->m_comp_v_samp[ component_id ];
	}
    }
}

// Restart interval processing.
static void jd_process_restart( jpeg_decoder* jd )
{
    int i;
    int c = 0;

    // Align to a byte boundry
    // FIXME: Is this really necessary? get_bits_no_markers() never reads in markers!
    //get_bits_no_markers(m_bits_left & 7);

    // Let's scan a little bit to find the marker, but not _too_ far.
    // 1536 is a "fudge factor" that determines how much to scan.
    for( i = 1536; i > 0; i-- )
	if( jd_get_char( jd ) == 0xFF )
    	    break;

    if( i == 0 )
	jd_stop_decoding( JPGD_BAD_RESTART_MARKER, jd );

    for( ; i > 0; i-- )
	if( ( c = jd_get_char( jd ) ) != 0xFF )
    	    break;

    if( i == 0 )
	jd_stop_decoding( JPGD_BAD_RESTART_MARKER, jd );

    // Is it the expected marker? If not, something bad happened.
    if( c != ( jd->m_next_restart_num + M_RST0 ) )
	jd_stop_decoding( JPGD_BAD_RESTART_MARKER, jd );

    // Reset each component's DC prediction values.
    memset( &jd->m_last_dc_val, 0, jd->m_comps_in_frame * sizeof( uint ) );

    jd->m_eob_run = 0;

    jd->m_restarts_left = jd->m_restart_interval;

    jd->m_next_restart_num = ( jd->m_next_restart_num + 1 ) & 7;

    // Get the bit buffer going again...

    jd->m_bits_left = 16;
    jd_get_bits_no_markers( 16, jd );
    jd_get_bits_no_markers( 16, jd );
}

static inline int jd_dequantize_ac( int c, int q ) { c *= q; return c; }

// Decodes and dequantizes the next row of coefficients.
static void jd_decode_next_row( jpeg_decoder* jd )
{
    int row_block = 0;

    for( int mcu_row = 0; mcu_row < jd->m_mcus_per_row; mcu_row++ )
    {
	if( ( jd->m_restart_interval ) && ( jd->m_restarts_left == 0 ) )
	    jd_process_restart( jd );

	jpgd_block_t* p = jd->m_pMCU_coefficients;
	for( int mcu_block = 0; mcu_block < jd->m_blocks_per_mcu; mcu_block++, p += 64 )
	{
    	    int component_id = jd->m_mcu_org[ mcu_block ];
	    jpgd_quant_t* q = jd->m_quant[ jd->m_comp_quant[ component_id ] ];
	    
	    int r, s;
	    s = jd_huff_decode( jd->m_pHuff_tabs[ jd->m_comp_dc_tab[ component_id ] ], r, jd );
	    s = JPGD_HUFF_EXTEND( r, s );

    	    jd->m_last_dc_val[ component_id ] = ( s += jd->m_last_dc_val[ component_id ] );

    	    p[ 0 ] = static_cast<jpgd_block_t>( s * q[ 0 ] );

	    int prev_num_set = jd->m_mcu_block_max_zag[ mcu_block ];

    	    huff_tables* pH = jd->m_pHuff_tabs[ jd->m_comp_ac_tab[ component_id ] ];

    	    int k;
    	    for( k = 1; k < 64; k++ )
	    {
    		int extra_bits;
    		s = jd_huff_decode( pH, extra_bits, jd );

		r = s >> 4;
		s &= 15;

		if( s )
		{
        	    if( r )
        	    {
        		if( ( k + r ) > 63 )
            		    jd_stop_decoding( JPGD_DECODE_ERROR, jd );

        		if( k < prev_num_set )
        		{
            		    int n = JPGD_MIN( r, prev_num_set - k );
            		    int kt = k;
            		    while( n-- )
            			p[ g_ZAG[ kt++ ] ] = 0;
        		}

        		k += r;
        	    }
          
        	    s = JPGD_HUFF_EXTEND( extra_bits, s );

        	    JPGD_ASSERT( k < 64 );

        	    p[ g_ZAG[ k ] ] = static_cast<jpgd_block_t>( jd_dequantize_ac( s, q[ k ] ) ); //s * q[k];
    		}
    		else
        	{
        	    if( r == 15 )
        	    {
        		if( ( k + 16 ) > 64)
            		    jd_stop_decoding( JPGD_DECODE_ERROR, jd );

        		if( k < prev_num_set )
			{
            		    int n = JPGD_MIN( 16, prev_num_set - k );
            		    int kt = k;
            		    while( n-- )
            		    {
            			JPGD_ASSERT( kt <= 63 );
            			p[ g_ZAG[ kt++ ] ] = 0;
            		    }
        		}

        		k += 16 - 1; // - 1 because the loop counter is k
        		JPGD_ASSERT( p[ g_ZAG[ k ] ] == 0 );
        	    }
        	    else
        		break;
    		}
    	    }

    	    if( k < prev_num_set )
	    {
    		int kt = k;
    		while( kt < prev_num_set )
        	p[ g_ZAG[ kt++ ] ] = 0;
    	    }

    	    jd->m_mcu_block_max_zag[ mcu_block ] = k;

	    row_block++;
	}

	if( jd->m_freq_domain_chroma_upsample )
	    jd_transform_mcu_expand( mcu_row, jd );
	else
    	    jd_transform_mcu( mcu_row, jd );

	jd->m_restarts_left--;
    }
}

// YCbCr H1V1 (1x1:1:1, 3 m_blocks per MCU) to RGB
static void jd_H1V1Convert( jpeg_decoder* jd )
{
    int row = jd->m_max_mcu_y_size - jd->m_mcu_lines_left;
    uchar* d = jd->m_pScan_line_0;
    uchar* s = jd->m_pSample_buf + row * 8;

    for( int i = jd->m_max_mcus_per_row; i > 0; i-- )
    {
	for( int j = 0; j < 8; j++ )
	{
    	    int y = s[ j ];
    	    int cb = s[ 64 + j ];
    	    int cr = s[ 128 + j ];

    	    d[ 0 ] = jd_clamp( y + jd->m_crr[ cr ] );
    	    d[ 1 ] = jd_clamp( y + ( ( jd->m_crg[ cr ] + jd->m_cbg[ cb ] ) >> 16 ) );
	    d[ 2 ] = jd_clamp( y + jd->m_cbb[ cb ] );
    	    d[ 3 ] = 255;

    	    d += 4;
	}

	s += 64 * 3;
    }
}

// YCbCr H2V1 (2x1:1:1, 4 m_blocks per MCU) to RGB
static void jd_H2V1Convert( jpeg_decoder* jd )
{
    int row = jd->m_max_mcu_y_size - jd->m_mcu_lines_left;
    uchar* d0 = jd->m_pScan_line_0;
    uchar* y = jd->m_pSample_buf + row * 8;
    uchar* c = jd->m_pSample_buf + 2*64 + row * 8;

    for( int i = jd->m_max_mcus_per_row; i > 0; i-- )
    {
	for( int l = 0; l < 2; l++ )
	{
    	    for( int j = 0; j < 4; j++ )
    	    {
    		int cb = c[ 0 ];
    		int cr = c[ 64 ];

    		int rc = jd->m_crr[ cr ];
    		int gc = ( ( jd->m_crg[ cr ] + jd->m_cbg[ cb ] ) >> 16 );
    		int bc = jd->m_cbb[ cb ];

    		int yy = y[ j << 1 ];
    		d0[ 0 ] = jd_clamp( yy + rc );
    		d0[ 1 ] = jd_clamp( yy + gc );
    		d0[ 2 ] = jd_clamp( yy + bc );
    		d0[ 3 ] = 255;

    		yy = y[ ( j << 1 ) + 1 ];
    		d0[ 4 ] = jd_clamp( yy + rc );
    		d0[ 5 ] = jd_clamp( yy + gc );
    		d0[ 6 ] = jd_clamp( yy + bc );
    		d0[ 7 ] = 255;

    		d0 += 8;

    		c++;
    	    }
    	    y += 64;
	}

	y += 64*4 - 64*2;
	c += 64*4 - 8;
    }
}

// YCbCr H2V1 (1x2:1:1, 4 m_blocks per MCU) to RGB
static void jd_H1V2Convert( jpeg_decoder* jd )
{
    int row = jd->m_max_mcu_y_size - jd->m_mcu_lines_left;
    uchar* d0 = jd->m_pScan_line_0;
    uchar* d1 = jd->m_pScan_line_1;
    uchar* y;
    uchar* c;

    if( row < 8 )
	y = jd->m_pSample_buf + row * 8;
    else
	y = jd->m_pSample_buf + 64 * 1 + ( row & 7 ) * 8;

    c = jd->m_pSample_buf + 64 * 2 + ( row >> 1 ) * 8;

    for( int i = jd->m_max_mcus_per_row; i > 0; i-- )
    {
	for( int j = 0; j < 8; j++ )
	{
    	    int cb = c[ 0 + j ];
    	    int cr = c[ 64 + j ];

    	    int rc = jd->m_crr[ cr ];
    	    int gc = ( ( jd->m_crg[ cr ] + jd->m_cbg[ cb ] ) >> 16 );
    	    int bc = jd->m_cbb[ cb ];

	    int yy = y[ j ];
    	    d0[ 0 ] = jd_clamp( yy + rc );
	    d0[ 1 ] = jd_clamp( yy + gc );
	    d0[ 2 ] = jd_clamp( yy + bc );
	    d0[ 3 ] = 255;

	    yy = y[ 8 + j ];
	    d1[ 0 ] = jd_clamp( yy + rc );
	    d1[ 1 ] = jd_clamp( yy + gc );
	    d1[ 2 ] = jd_clamp( yy + bc );
	    d1[ 3 ] = 255;

	    d0 += 4;
	    d1 += 4;
	}

	y += 64*4;
	c += 64*4;
    }
}

// YCbCr H2V2 (2x2:1:1, 6 m_blocks per MCU) to RGB
static void jd_H2V2Convert( jpeg_decoder* jd )
{
    int row = jd->m_max_mcu_y_size - jd->m_mcu_lines_left;
    uchar* d0 = jd->m_pScan_line_0;
    uchar* d1 = jd->m_pScan_line_1;
    uchar* y;
    uchar* c;

    if( row < 8 )
	y = jd->m_pSample_buf + row * 8;
    else
	y = jd->m_pSample_buf + 64 * 2 + ( row & 7 ) * 8;

    c = jd->m_pSample_buf + 64 * 4 + ( row >> 1 ) * 8;

    for( int i = jd->m_max_mcus_per_row; i > 0; i-- )
    {
	for( int l = 0; l < 2; l++ )
	{
	    for( int j = 0; j < 8; j += 2 )
	    {
		int cb = c[ 0 ];
		int cr = c[ 64 ];

		int rc = jd->m_crr[ cr ];
		int gc = ( ( jd->m_crg[ cr ] + jd->m_cbg[ cb ] ) >> 16 );
		int bc = jd->m_cbb[ cb ];

		int yy = y[ j ];
		d0[ 0 ] = jd_clamp( yy + rc );
		d0[ 1 ] = jd_clamp( yy + gc );
		d0[ 2 ] = jd_clamp( yy + bc );
		d0[ 3 ] = 255;

		yy = y[ j + 1 ];
		d0[ 4 ] = jd_clamp( yy + rc );
		d0[ 5 ] = jd_clamp( yy + gc );
		d0[ 6 ] = jd_clamp( yy + bc );
		d0[ 7 ] = 255;

		yy = y[ j + 8 ];
		d1[ 0 ] = jd_clamp( yy + rc );
		d1[ 1 ] = jd_clamp( yy + gc );
		d1[ 2 ] = jd_clamp( yy + bc );
		d1[ 3 ] = 255;

		yy = y[ j + 8 + 1 ];
		d1[ 4 ] = jd_clamp( yy + rc );
		d1[ 5 ] = jd_clamp( yy + gc );
		d1[ 6 ] = jd_clamp( yy + bc );
		d1[ 7 ] = 255;

		d0 += 8;
		d1 += 8;
                
                c++;
	    }
	    y += 64;
	}

	y += 64*6 - 64*2;
	c += 64*6 - 8;
    }
}

// Y (1 block per MCU) to 8-bit grayscale
static void jd_gray_convert( jpeg_decoder* jd )
{
    int row = jd->m_max_mcu_y_size - jd->m_mcu_lines_left;
    uchar* d = jd->m_pScan_line_0;
    uchar* s = jd->m_pSample_buf + row * 8;

    for( int i = jd->m_max_mcus_per_row; i > 0; i-- )
    {
	*(uint *)d = *(uint *)s;
	*(uint *)( &d[ 4 ] ) = *(uint *)( &s[ 4 ] );

	s += 64;
	d += 8;
    }
}

static void jd_expanded_convert( jpeg_decoder* jd )
{
    int row = jd->m_max_mcu_y_size - jd->m_mcu_lines_left;

    uchar* Py = jd->m_pSample_buf + ( row / 8 ) * 64 * jd->m_comp_h_samp[ 0 ] + ( row & 7 ) * 8;

    uchar* d = jd->m_pScan_line_0;

    for( int i = jd->m_max_mcus_per_row; i > 0; i-- )
    {
	for( int k = 0; k < jd->m_max_mcu_x_size; k += 8 )
	{
    	    const int Y_ofs = k * 8;
    	    const int Cb_ofs = Y_ofs + 64 * jd->m_expanded_blocks_per_component;
    	    const int Cr_ofs = Y_ofs + 64 * jd->m_expanded_blocks_per_component * 2;
    	    for( int j = 0; j < 8; j++ )
    	    {
    		int y = Py[ Y_ofs + j ];
    		int cb = Py[ Cb_ofs + j ];
    		int cr = Py[ Cr_ofs + j ];

    		d[ 0 ] = jd_clamp( y + jd->m_crr[ cr ] );
    		d[ 1 ] = jd_clamp( y + ( ( jd->m_crg[ cr ] + jd->m_cbg[ cb ] ) >> 16 ) );
    		d[ 2 ] = jd_clamp( y + jd->m_cbb[cb]);
    		d[ 3 ] = 255;

    		d += 4;
    	    }
	}

	Py += 64 * jd->m_expanded_blocks_per_mcu;
    }
}

// Find end of image (EOI) marker, so we can return to the user the exact size of the input stream.
static void jd_find_eoi( jpeg_decoder* jd )
{
    if( !jd->m_progressive_flag )
    {
	// Attempt to read the EOI marker.
	//get_bits_no_markers(m_bits_left & 7);

	// Prime the bit buffer
	jd->m_bits_left = 16;
	jd_get_bits( 16, jd );
	jd_get_bits( 16, jd );

	// The next marker _should_ be EOI
	jd_process_markers( jd );
    }

    jd->m_total_bytes_read -= jd->m_in_buf_left;
}

int jd_decode( const void** pScan_line, uint* pScan_line_len, jpeg_decoder* jd )
{
    if( ( jd->m_error_code ) || ( !jd->m_ready_flag ) )
	return JPGD_FAILED;

    if( jd->m_total_lines_left == 0 )
	return JPGD_DONE;

    if( jd->m_mcu_lines_left == 0 )
    {
	if( setjmp( jd->m_jmp_state ) )
	    return JPGD_FAILED;

	if( jd->m_progressive_flag )
    	    jd_load_next_row( jd );
	else
    	    jd_decode_next_row( jd );

	// Find the EOI marker if that was the last row.
	if( jd->m_total_lines_left <= jd->m_max_mcu_y_size )
    	    jd_find_eoi( jd );

	jd->m_mcu_lines_left = jd->m_max_mcu_y_size;
    }

    if( jd->m_freq_domain_chroma_upsample )
    {
	jd_expanded_convert( jd );
	*pScan_line = jd->m_pScan_line_0;
    }
    else
    {
	switch( jd->m_scan_type )
	{
    	    case JPGD_YH2V2:
    		if( ( jd->m_mcu_lines_left & 1 ) == 0 )
		{
        	    jd_H2V2Convert( jd );
		    *pScan_line = jd->m_pScan_line_0;
    		}
    		else
        	    *pScan_line = jd->m_pScan_line_1;
    		break;
    	    case JPGD_YH2V1:
    		jd_H2V1Convert( jd );
    		*pScan_line = jd->m_pScan_line_0;
    		break;
	    case JPGD_YH1V2:
    		if( ( jd->m_mcu_lines_left & 1 ) == 0 )
		{
        	    jd_H1V2Convert( jd );
        	    *pScan_line = jd->m_pScan_line_0;
    		}
    		else
        	    *pScan_line = jd->m_pScan_line_1;
    		break;
    	    case JPGD_YH1V1:
    		jd_H1V1Convert( jd );
		*pScan_line = jd->m_pScan_line_0;
    		break;
	    case JPGD_GRAYSCALE:
    		jd_gray_convert( jd );
    		*pScan_line = jd->m_pScan_line_0;
    		break;
	}
    }

    *pScan_line_len = jd->m_real_dest_bytes_per_scan_line;

    jd->m_mcu_lines_left--;
    jd->m_total_lines_left--;

    return JPGD_SUCCESS;
}

// Creates the tables needed for efficient Huffman decoding.
static void jd_make_huff_table( int index, huff_tables* pH, jpeg_decoder* jd )
{
    int p, i, l, si;
    uchar huffsize[257];
    uint huffcode[257];
    uint code;
    uint subtree;
    int code_size;
    int lastp;
    int nextfreeentry;
    int currententry;

    pH->ac_table = jd->m_huff_ac[ index ] != 0;

    p = 0;

    for( l = 1; l <= 16; l++ )
    {
	for( i = 1; i <= jd->m_huff_num[index][l]; i++ )
        huffsize[p++] = static_cast<uchar>(l);
    }

    huffsize[p] = 0;

    lastp = p;

    code = 0;
    si = huffsize[0];
    p = 0;

    while( huffsize[ p ] )
    {
	while( huffsize[ p ] == si )
	{
    	    huffcode[p++] = code;
	    code++;
	}

	code <<= 1;
	si++;
    }

    memset(pH->look_up, 0, sizeof(pH->look_up));
    memset(pH->look_up2, 0, sizeof(pH->look_up2));
    memset(pH->tree, 0, sizeof(pH->tree));
    memset(pH->code_size, 0, sizeof(pH->code_size));

    nextfreeentry = -1;

    p = 0;

    while( p < lastp )
    {
	i = jd->m_huff_val[index][p];
	code = huffcode[p];
	code_size = huffsize[p];

        pH->code_size[i] = static_cast<uchar>(code_size);

	if( code_size <= 8 )
	{
    	    code <<= (8 - code_size);

    	    for( l = 1 << (8 - code_size); l > 0; l-- )
    	    {
    		JPGD_ASSERT( i < 256 );

    		pH->look_up[code] = i;

    		bool has_extrabits = false;
		int extra_bits = 0;
    		int num_extra_bits = i & 15;

    		int bits_to_fetch = code_size;
    		if( num_extra_bits )
    		{
        	    int total_codesize = code_size + num_extra_bits;
        	    if( total_codesize <= 8 )
        	    {
        		has_extrabits = true;
        		extra_bits = ( (1 << num_extra_bits) - 1) & (code >> (8 - total_codesize) );
        		JPGD_ASSERT( extra_bits <= 0x7FFF );
        		bits_to_fetch += num_extra_bits;
        	    }
    		}

    		if( !has_extrabits )
        	    pH->look_up2[ code ] = i | (bits_to_fetch << 8);
    		else
        	    pH->look_up2[ code ] = i | 0x8000 | (extra_bits << 16) | (bits_to_fetch << 8);

    		code++;
    	    }
	}
	else
	{
    	    subtree = (code >> (code_size - 8)) & 0xFF;

    	    currententry = pH->look_up[subtree];

    	    if( currententry == 0 )
    	    {
    		pH->look_up[subtree] = currententry = nextfreeentry;
    		pH->look_up2[subtree] = currententry = nextfreeentry;

    		nextfreeentry -= 2;
    	    }

    	    code <<= (16 - (code_size - 8));

    	    for( l = code_size; l > 9; l-- )
    	    {
    		if( (code & 0x8000) == 0 )
        	currententry--;

    		if( pH->tree[-currententry - 1] == 0 )
    		{
        	    pH->tree[-currententry - 1] = nextfreeentry;

        	    currententry = nextfreeentry;

        	    nextfreeentry -= 2;
    		}
    		else
        	    currententry = pH->tree[-currententry - 1];

    		code <<= 1;
    	    }

    	    if ((code & 0x8000) == 0)
    		currententry--;

    	    pH->tree[-currententry - 1] = i;
	}

	p++;
    }
}

// Verifies the quantization tables needed for this scan are available.
static void jd_check_quant_tables( jpeg_decoder* jd )
{
    for( int i = 0; i < jd->m_comps_in_scan; i++ )
	if( jd->m_quant[ jd->m_comp_quant[ jd->m_comp_list[ i ] ] ] == NULL )
    	    jd_stop_decoding( JPGD_UNDEFINED_QUANT_TABLE, jd );
}

// Verifies that all the Huffman tables needed for this scan are available.
static void jd_check_huff_tables( jpeg_decoder* jd )
{
    for( int i = 0; i < jd->m_comps_in_scan; i++ )
    {
	if( ( jd->m_spectral_start == 0 ) && ( jd->m_huff_num[ jd->m_comp_dc_tab[ jd->m_comp_list[ i ] ] ] == NULL ) )
    	    jd_stop_decoding( JPGD_UNDEFINED_HUFF_TABLE, jd );

	if( ( jd->m_spectral_end > 0 ) && ( jd->m_huff_num[ jd->m_comp_ac_tab[ jd->m_comp_list[ i ] ] ] == NULL ) )
	    jd_stop_decoding( JPGD_UNDEFINED_HUFF_TABLE, jd );
    }

    for( int i = 0; i < JPGD_MAX_HUFF_TABLES; i++ )
	if( jd->m_huff_num[ i ] )
	{
    	    if( !jd->m_pHuff_tabs[ i ] )
    		jd->m_pHuff_tabs[ i ] = (huff_tables *)jd_alloc( sizeof( huff_tables ), 0, jd );

    	    jd_make_huff_table( i, jd->m_pHuff_tabs[ i ], jd );
	}
}

// Determines the component order inside each MCU.
// Also calcs how many MCU's are on each row, etc.
static void jd_calc_mcu_block_order( jpeg_decoder* jd )
{
    int component_num, component_id;
    int max_h_samp = 0, max_v_samp = 0;

    for( component_id = 0; component_id < jd->m_comps_in_frame; component_id++ )
    {
	if( jd->m_comp_h_samp[ component_id ] > max_h_samp )
    	    max_h_samp = jd->m_comp_h_samp[ component_id ];

	if( jd->m_comp_v_samp[ component_id ] > max_v_samp )
	    max_v_samp = jd->m_comp_v_samp[ component_id ];
    }

    for( component_id = 0; component_id < jd->m_comps_in_frame; component_id++ )
    {
	jd->m_comp_h_blocks[ component_id ] = ((((jd->m_image_x_size * jd->m_comp_h_samp[component_id]) + (max_h_samp - 1)) / max_h_samp) + 7) / 8;
	jd->m_comp_v_blocks[ component_id ] = ((((jd->m_image_y_size * jd->m_comp_v_samp[component_id]) + (max_v_samp - 1)) / max_v_samp) + 7) / 8;
    }

    if( jd->m_comps_in_scan == 1 )
    {
	jd->m_mcus_per_row = jd->m_comp_h_blocks[ jd->m_comp_list[ 0 ] ];
	jd->m_mcus_per_col = jd->m_comp_v_blocks[ jd->m_comp_list[ 0 ] ];
    }
    else
    {
	jd->m_mcus_per_row = (((jd->m_image_x_size + 7) / 8) + (max_h_samp - 1)) / max_h_samp;
	jd->m_mcus_per_col = (((jd->m_image_y_size + 7) / 8) + (max_v_samp - 1)) / max_v_samp;
    }

    if( jd->m_comps_in_scan == 1 )
    {
	jd->m_mcu_org[ 0 ] = jd->m_comp_list[ 0 ];

	jd->m_blocks_per_mcu = 1;
    }
    else
    {
	jd->m_blocks_per_mcu = 0;

	for( component_num = 0; component_num < jd->m_comps_in_scan; component_num++ )
	{
    	    int num_blocks;

    	    component_id = jd->m_comp_list[ component_num ];

    	    num_blocks = jd->m_comp_h_samp[ component_id ] * jd->m_comp_v_samp[ component_id ];

    	    while( num_blocks-- )
    		jd->m_mcu_org[ jd->m_blocks_per_mcu++ ] = component_id;
	}
    }
}

// Starts a new scan.
static int jd_init_scan( jpeg_decoder* jd )
{
    if( !jd_locate_sos_marker( jd ) )
	return JPGD_FALSE;

    jd_calc_mcu_block_order( jd );

    jd_check_huff_tables( jd );

    jd_check_quant_tables( jd );

    memset( jd->m_last_dc_val, 0, jd->m_comps_in_frame * sizeof( uint ) );

    jd->m_eob_run = 0;

    if( jd->m_restart_interval )
    {
	jd->m_restarts_left = jd->m_restart_interval;
	jd->m_next_restart_num = 0;
    }

    jd_fix_in_buffer( jd );

    return JPGD_TRUE;
}

// Starts a frame. Determines if the number of components or sampling factors
// are supported.
static void jd_init_frame( jpeg_decoder* jd )
{
    int i;

    if( jd->m_comps_in_frame == 1 )
    {
	if( ( jd->m_comp_h_samp[ 0 ] != 1 ) || ( jd->m_comp_v_samp[ 0 ] != 1 ) )
	    jd_stop_decoding( JPGD_UNSUPPORTED_SAMP_FACTORS, jd );

	jd->m_scan_type = JPGD_GRAYSCALE;
	jd->m_max_blocks_per_mcu = 1;
	jd->m_max_mcu_x_size = 8;
	jd->m_max_mcu_y_size = 8;
    }
    else if( jd->m_comps_in_frame == 3 )
    {
	if( ( ( jd->m_comp_h_samp[ 1 ] != 1 ) || ( jd->m_comp_v_samp[ 1 ] != 1 ) ) ||
            ( ( jd->m_comp_h_samp[ 2 ] != 1 ) || ( jd->m_comp_v_samp[ 2 ] != 1 ) ) )
    	    jd_stop_decoding( JPGD_UNSUPPORTED_SAMP_FACTORS, jd );

	if( ( jd->m_comp_h_samp[ 0 ] == 1 ) && ( jd->m_comp_v_samp[ 0 ] == 1 ) )
	{
	    jd->m_scan_type = JPGD_YH1V1;

	    jd->m_max_blocks_per_mcu = 3;
	    jd->m_max_mcu_x_size = 8;
	    jd->m_max_mcu_y_size = 8;
	}
	else if( ( jd->m_comp_h_samp[ 0 ] == 2 ) && ( jd->m_comp_v_samp[ 0 ] == 1 ) )
	{
	    jd->m_scan_type = JPGD_YH2V1;
    	    jd->m_max_blocks_per_mcu = 4;
    	    jd->m_max_mcu_x_size = 16;
    	    jd->m_max_mcu_y_size = 8;
	}
	else if( ( jd->m_comp_h_samp[ 0 ] == 1 ) && ( jd->m_comp_v_samp[ 0 ] == 2 ) )
	{
    	    jd->m_scan_type = JPGD_YH1V2;
    	    jd->m_max_blocks_per_mcu = 4;
    	    jd->m_max_mcu_x_size = 8;
    	    jd->m_max_mcu_y_size = 16;
	}
	else if( ( jd->m_comp_h_samp[ 0 ] == 2 ) && ( jd->m_comp_v_samp[ 0 ] == 2 ) )
	{
    	    jd->m_scan_type = JPGD_YH2V2;
    	    jd->m_max_blocks_per_mcu = 6;
    	    jd->m_max_mcu_x_size = 16;
    	    jd->m_max_mcu_y_size = 16;
	}
	else
    	    jd_stop_decoding( JPGD_UNSUPPORTED_SAMP_FACTORS, jd );
    }
    else
	jd_stop_decoding( JPGD_UNSUPPORTED_COLORSPACE, jd );

    jd->m_max_mcus_per_row = ( jd->m_image_x_size + ( jd->m_max_mcu_x_size - 1 ) ) / jd->m_max_mcu_x_size;
    jd->m_max_mcus_per_col = ( jd->m_image_y_size + ( jd->m_max_mcu_y_size - 1 ) ) / jd->m_max_mcu_y_size;

    // These values are for the *destination* pixels: after conversion.
    if( jd->m_scan_type == JPGD_GRAYSCALE )
	jd->m_dest_bytes_per_pixel = 1;
    else
	jd->m_dest_bytes_per_pixel = 4;

    jd->m_dest_bytes_per_scan_line = ( ( jd->m_image_x_size + 15 ) & 0xFFF0 ) * jd->m_dest_bytes_per_pixel;

    jd->m_real_dest_bytes_per_scan_line = ( jd->m_image_x_size * jd->m_dest_bytes_per_pixel );

    // Initialize two scan line buffers.
    jd->m_pScan_line_0 = (uchar *)jd_alloc( jd->m_dest_bytes_per_scan_line, true, jd );
    if( ( jd->m_scan_type == JPGD_YH1V2 ) || ( jd->m_scan_type == JPGD_YH2V2 ) )
	jd->m_pScan_line_1 = (uchar *)jd_alloc( jd->m_dest_bytes_per_scan_line, true, jd );

    jd->m_max_blocks_per_row = jd->m_max_mcus_per_row * jd->m_max_blocks_per_mcu;

    // Should never happen
    if( jd->m_max_blocks_per_row > JPGD_MAX_BLOCKS_PER_ROW )
	jd_stop_decoding( JPGD_ASSERTION_ERROR, jd );

    // Allocate the coefficient buffer, enough for one MCU
    jd->m_pMCU_coefficients = (jpgd_block_t*)jd_alloc( jd->m_max_blocks_per_mcu * 64 * sizeof( jpgd_block_t ), 0, jd );

    for( i = 0; i < jd->m_max_blocks_per_mcu; i++ )
	jd->m_mcu_block_max_zag[ i ] = 64;

    jd->m_expanded_blocks_per_component = jd->m_comp_h_samp[ 0 ] * jd->m_comp_v_samp[ 0 ];
    jd->m_expanded_blocks_per_mcu = jd->m_expanded_blocks_per_component * jd->m_comps_in_frame;
    jd->m_expanded_blocks_per_row = jd->m_max_mcus_per_row * jd->m_expanded_blocks_per_mcu;
    // Freq. domain chroma upsampling is only supported for H2V2 subsampling factor (the most common one I've seen).
    jd->m_freq_domain_chroma_upsample = false;
#if JPGD_SUPPORT_FREQ_DOMAIN_UPSAMPLING
    jd->m_freq_domain_chroma_upsample = ( jd->m_expanded_blocks_per_mcu == 4 * 3 );
#endif

    if( jd->m_freq_domain_chroma_upsample )
	jd->m_pSample_buf = (uchar *)jd_alloc( jd->m_expanded_blocks_per_row * 64, 0, jd );
    else
	jd->m_pSample_buf = (uchar *)jd_alloc( jd->m_max_blocks_per_row * 64, 0, jd );

    jd->m_total_lines_left = jd->m_image_y_size;

    jd->m_mcu_lines_left = 0;

    jd_create_look_ups( jd );
}

// The coeff_buf series of methods originally stored the coefficients
// into a "virtual" file which was located in EMS, XMS, or a disk file. A cache
// was used to make this process more efficient. Now, we can store the entire
// thing in RAM.
static coeff_buf* jd_coeff_buf_open( int block_num_x, int block_num_y, int block_len_x, int block_len_y, jpeg_decoder* jd )
{
    coeff_buf* cb = (coeff_buf*)jd_alloc( sizeof( coeff_buf ), 0, jd );

    cb->block_num_x = block_num_x;
    cb->block_num_y = block_num_y;
    cb->block_len_x = block_len_x;
    cb->block_len_y = block_len_y;
    cb->block_size = ( block_len_x * block_len_y ) * sizeof( jpgd_block_t );
    cb->pData = (uchar *)jd_alloc( cb->block_size * block_num_x * block_num_y, true, jd );
    return cb;
}

inline jpgd_block_t* jd_coeff_buf_getp( coeff_buf* cb, int block_x, int block_y )
{
    JPGD_ASSERT( ( block_x < cb->block_num_x ) && ( block_y < cb->block_num_y ) );
    return (jpgd_block_t *)( cb->pData + block_x * cb->block_size + block_y * ( cb->block_size * cb->block_num_x ) );
}

// The following methods decode the various types of m_blocks encountered
// in progressively encoded images.
static void jd_decode_block_dc_first( jpeg_decoder* pD, int component_id, int block_x, int block_y )
{
    int s, r;
    jpgd_block_t* p = jd_coeff_buf_getp( pD->m_dc_coeffs[component_id], block_x, block_y );

    if( ( s = jd_huff_decode( pD->m_pHuff_tabs[ pD->m_comp_dc_tab[ component_id ] ], pD ) ) != 0)
    {
	r = jd_get_bits_no_markers( s, pD );
	s = JPGD_HUFF_EXTEND(r, s);
    }

    pD->m_last_dc_val[component_id] = (s += pD->m_last_dc_val[component_id]);

    p[0] = static_cast<jpgd_block_t>(s << pD->m_successive_low);
}

static void jd_decode_block_dc_refine( jpeg_decoder* pD, int component_id, int block_x, int block_y )
{
    if( jd_get_bits_no_markers( 1, pD ) )
    {
	jpgd_block_t* p = jd_coeff_buf_getp( pD->m_dc_coeffs[ component_id ], block_x, block_y );

	p[ 0 ] |= ( 1 << pD->m_successive_low );
    }
}

static void jd_decode_block_ac_first( jpeg_decoder* pD, int component_id, int block_x, int block_y )
{
    int k, s, r;

    if( pD->m_eob_run )
    {
	pD->m_eob_run--;
	return;
    }

    jpgd_block_t* p = jd_coeff_buf_getp( pD->m_ac_coeffs[component_id], block_x, block_y );

    for( k = pD->m_spectral_start; k <= pD->m_spectral_end; k++ )
    {
	s = jd_huff_decode( pD->m_pHuff_tabs[pD->m_comp_ac_tab[component_id]], pD );

	r = s >> 4;
	s &= 15;

	if (s)
	{
    	    if ((k += r) > 63)
    		jd_stop_decoding( JPGD_DECODE_ERROR, pD );

    	    r = jd_get_bits_no_markers( s, pD );
    	    s = JPGD_HUFF_EXTEND(r, s);

    	    p[g_ZAG[k]] = static_cast<jpgd_block_t>(s << pD->m_successive_low);
	}
	else
	{
    	    if (r == 15)
    	    {
    		if ((k += 15) > 63)
        	    jd_stop_decoding( JPGD_DECODE_ERROR, pD );
    	    }
    	    else
    	    {
    		pD->m_eob_run = 1 << r;

    		if (r)
        	    pD->m_eob_run += jd_get_bits_no_markers( r, pD );

    		pD->m_eob_run--;

    		break;
    	    }
	}
    }
}

static void jd_decode_block_ac_refine( jpeg_decoder* pD, int component_id, int block_x, int block_y )
{
    int s, k, r;
    int p1 = 1 << pD->m_successive_low;
    int m1 = (-1) << pD->m_successive_low;
    jpgd_block_t* p = jd_coeff_buf_getp( pD->m_ac_coeffs[ component_id ], block_x, block_y );
  
    JPGD_ASSERT(pD->m_spectral_end <= 63);
  
    k = pD->m_spectral_start;
  
    if (pD->m_eob_run == 0)
    {
	for ( ; k <= pD->m_spectral_end; k++)
	{
    	    s = jd_huff_decode( pD->m_pHuff_tabs[pD->m_comp_ac_tab[component_id]], pD );

    	    r = s >> 4;
    	    s &= 15;

    	    if (s)
    	    {
    		if (s != 1)
        	    jd_stop_decoding( JPGD_DECODE_ERROR, pD );

    		if( jd_get_bits_no_markers( 1, pD ) )
        	    s = p1;
    		else
        	    s = m1;
    	    }
    	    else
    	    {
    		if (r != 15)
    		{
        	    pD->m_eob_run = 1 << r;

        	    if (r)
        		pD->m_eob_run += jd_get_bits_no_markers( r, pD );

        	    break;
    		}
    	    }

    	    do
    	    {
    		jpgd_block_t* this_coef = p + g_ZAG[k & 63];

    		if (*this_coef != 0)
    		{
        	    if( jd_get_bits_no_markers( 1, pD ) )
        	    {
        		if ((*this_coef & p1) == 0)
        		{
            		    if (*this_coef >= 0)
            			*this_coef = static_cast<jpgd_block_t>(*this_coef + p1);
            		    else
            			*this_coef = static_cast<jpgd_block_t>(*this_coef + m1);
        		}
        	    }
    		}
    		else
    		{
        	    if (--r < 0)
        		break;
    		}

    		k++;

    	    } while (k <= pD->m_spectral_end);

    	    if ((s) && (k < 64))
    	    {
    		p[g_ZAG[k]] = static_cast<jpgd_block_t>(s);
    	    }
	}
    }

    if (pD->m_eob_run > 0)
    {
	for ( ; k <= pD->m_spectral_end; k++)
	{
    	    jpgd_block_t *this_coef = p + g_ZAG[k & 63]; // logical AND to shut up static code analysis

    	    if (*this_coef != 0)
    	    {
    		if( jd_get_bits_no_markers( 1, pD ) )
    		{
        	    if ((*this_coef & p1) == 0)
        	    {
        		if (*this_coef >= 0)
        		    *this_coef = static_cast<jpgd_block_t>(*this_coef + p1);
        		else
            		    *this_coef = static_cast<jpgd_block_t>(*this_coef + m1);
        	    }
    		}
    	    }
	}

	pD->m_eob_run--;
    }
}

// Decode a scan in a progressively encoded image.
static void jd_decode_scan( pDecode_block_func decode_block_func, jpeg_decoder* jd )
{
    int mcu_row, mcu_col, mcu_block;
    int block_x_mcu[ JPGD_MAX_COMPONENTS ], block_y_mcu[ JPGD_MAX_COMPONENTS ];

    memset( block_y_mcu, 0, sizeof( block_y_mcu ) );

    for( mcu_col = 0; mcu_col < jd->m_mcus_per_col; mcu_col++ )
    {
	int component_num, component_id;

	memset( block_x_mcu, 0, sizeof( block_x_mcu ) );

	for( mcu_row = 0; mcu_row < jd->m_mcus_per_row; mcu_row++ )
	{
    	    int block_x_mcu_ofs = 0, block_y_mcu_ofs = 0;

	    if( ( jd->m_restart_interval ) && ( jd->m_restarts_left == 0 ) )
    		jd_process_restart( jd );

	    for( mcu_block = 0; mcu_block < jd->m_blocks_per_mcu; mcu_block++ )
	    {
    		component_id = jd->m_mcu_org[ mcu_block ];

    		decode_block_func( jd, component_id, block_x_mcu[ component_id ] + block_x_mcu_ofs, block_y_mcu[ component_id ] + block_y_mcu_ofs );

    		if( jd->m_comps_in_scan == 1 )
        	    block_x_mcu[ component_id ]++;
    		else
    		{
        	    if( ++block_x_mcu_ofs == jd->m_comp_h_samp[ component_id ] )
        	    {
        		block_x_mcu_ofs = 0;

        		if( ++block_y_mcu_ofs == jd->m_comp_v_samp[ component_id ] )
        		{
            		    block_y_mcu_ofs = 0;
            		    block_x_mcu[ component_id ] += jd->m_comp_h_samp[ component_id ];
        		}
        	    }
    		}
    	    }

    	    jd->m_restarts_left--;
	}

	if( jd->m_comps_in_scan == 1 )
    	    block_y_mcu[ jd->m_comp_list[ 0 ] ]++;
	else
	{
    	    for( component_num = 0; component_num < jd->m_comps_in_scan; component_num++ )
	    {
    		component_id = jd->m_comp_list[ component_num ];
    		block_y_mcu[ component_id ] += jd->m_comp_v_samp[ component_id ];
    	    }
	}
    }
}

// Decode a progressively encoded image.
static void jd_init_progressive( jpeg_decoder* jd )
{
    int i;

    if( jd->m_comps_in_frame == 4 )
	jd_stop_decoding( JPGD_UNSUPPORTED_COLORSPACE, jd );

    // Allocate the coefficient buffers.
    for( i = 0; i < jd->m_comps_in_frame; i++ )
    {
	jd->m_dc_coeffs[ i ] = jd_coeff_buf_open( jd->m_max_mcus_per_row * jd->m_comp_h_samp[ i ], jd->m_max_mcus_per_col * jd->m_comp_v_samp[ i ], 1, 1, jd );
	jd->m_ac_coeffs[ i ] = jd_coeff_buf_open( jd->m_max_mcus_per_row * jd->m_comp_h_samp[ i ], jd->m_max_mcus_per_col * jd->m_comp_v_samp[ i ], 8, 8, jd );
    }

    for ( ; ; )
    {
	int dc_only_scan, refinement_scan;
	pDecode_block_func decode_block_func;

	if( !jd_init_scan( jd ) )
	    break;

	dc_only_scan = ( jd->m_spectral_start == 0 );
	refinement_scan = ( jd->m_successive_high != 0 );

	if( ( jd->m_spectral_start > jd->m_spectral_end ) || ( jd->m_spectral_end > 63 ) )
    	    jd_stop_decoding( JPGD_BAD_SOS_SPECTRAL, jd );

	if( dc_only_scan )
	{
    	    if( jd->m_spectral_end )
    		jd_stop_decoding( JPGD_BAD_SOS_SPECTRAL, jd );
	}
	else if( jd->m_comps_in_scan != 1 )  /* AC scans can only contain one component */
    	    jd_stop_decoding( JPGD_BAD_SOS_SPECTRAL, jd );

	if( ( refinement_scan ) && ( jd->m_successive_low != jd->m_successive_high - 1 ) )
    	    jd_stop_decoding( JPGD_BAD_SOS_SUCCESSIVE, jd );

	if( dc_only_scan )
	{
    	    if( refinement_scan )
    		decode_block_func = jd_decode_block_dc_refine;
    	    else
    		decode_block_func = jd_decode_block_dc_first;
	}
	else
	{
    	    if( refinement_scan )
    		decode_block_func = jd_decode_block_ac_refine;
    	    else
    		decode_block_func = jd_decode_block_ac_first;
	}

	jd_decode_scan( decode_block_func, jd );

	jd->m_bits_left = 16;
	jd_get_bits( 16, jd );
	jd_get_bits( 16, jd );
    }

    jd->m_comps_in_scan = jd->m_comps_in_frame;

    for( i = 0; i < jd->m_comps_in_frame; i++ )
	jd->m_comp_list[ i ] = i;

    jd_calc_mcu_block_order( jd );
}

static void jd_init_sequential( jpeg_decoder* jd )
{
    if( !jd_init_scan( jd ) )
	jd_stop_decoding( JPGD_UNEXPECTED_MARKER, jd );
}

static void jd_decode_start( jpeg_decoder* jd )
{
    jd_init_frame( jd );

    if( jd->m_progressive_flag )
	jd_init_progressive( jd );
    else
	jd_init_sequential( jd );
}

int jd_begin_decoding( jpeg_decoder* jd )
{
    if( jd->m_ready_flag )
	return JPGD_SUCCESS;

    if( jd->m_error_code )
	return JPGD_FAILED;

    if( setjmp( jd->m_jmp_state ) )
	return JPGD_FAILED;

    jd_decode_start( jd );

    jd->m_ready_flag = true;

    return JPGD_SUCCESS;
}

// Reset everything to default/uninitialized state.
void jd_init( bfs_file pStream, jpeg_decoder* jd )
{
    if( setjmp( jd->m_jmp_state ) )
	return;

    jd->m_pMem_blocks = NULL;
    jd->m_error_code = JPGD_SUCCESS;
    jd->m_ready_flag = false;
    jd->m_image_x_size = jd->m_image_y_size = 0;
    jd->m_pStream = pStream;
    jd->m_progressive_flag = JPGD_FALSE;

    memset( jd->m_huff_ac, 0, sizeof( jd->m_huff_ac ) );
    memset( jd->m_huff_num, 0, sizeof( jd->m_huff_num ) );
    memset( jd->m_huff_val, 0, sizeof( jd->m_huff_val ) );
    memset( jd->m_quant, 0, sizeof( jd->m_quant ) );

    jd->m_scan_type = 0;
    jd->m_comps_in_frame = 0;

    memset( jd->m_comp_h_samp, 0, sizeof( jd->m_comp_h_samp ) );
    memset( jd->m_comp_v_samp, 0, sizeof( jd->m_comp_v_samp ) );
    memset( jd->m_comp_quant, 0, sizeof( jd->m_comp_quant ) );
    memset( jd->m_comp_ident, 0, sizeof( jd->m_comp_ident ) );
    memset( jd->m_comp_h_blocks, 0, sizeof( jd->m_comp_h_blocks ) );
    memset( jd->m_comp_v_blocks, 0, sizeof( jd->m_comp_v_blocks ) );

    jd->m_comps_in_scan = 0;
    memset( jd->m_comp_list, 0, sizeof( jd->m_comp_list ) );
    memset( jd->m_comp_dc_tab, 0, sizeof( jd->m_comp_dc_tab ) );
    memset( jd->m_comp_ac_tab, 0, sizeof( jd->m_comp_ac_tab ) );

    jd->m_spectral_start = 0;
    jd->m_spectral_end = 0;
    jd->m_successive_low = 0;
    jd->m_successive_high = 0;
    jd->m_max_mcu_x_size = 0;
    jd->m_max_mcu_y_size = 0;
    jd->m_blocks_per_mcu = 0;
    jd->m_max_blocks_per_row = 0;
    jd->m_mcus_per_row = 0;
    jd->m_mcus_per_col = 0;
    jd->m_expanded_blocks_per_component = 0;
    jd->m_expanded_blocks_per_mcu = 0;
    jd->m_expanded_blocks_per_row = 0;
    jd->m_freq_domain_chroma_upsample = false;

    memset( jd->m_mcu_org, 0, sizeof( jd->m_mcu_org ) );

    jd->m_total_lines_left = 0;
    jd->m_mcu_lines_left = 0;
    jd->m_real_dest_bytes_per_scan_line = 0;
    jd->m_dest_bytes_per_scan_line = 0;
    jd->m_dest_bytes_per_pixel = 0;

    memset( jd->m_pHuff_tabs, 0, sizeof( jd->m_pHuff_tabs ) );
                                                            
    memset( jd->m_dc_coeffs, 0, sizeof( jd->m_dc_coeffs ) );
    memset( jd->m_ac_coeffs, 0, sizeof( jd->m_ac_coeffs ) );
    memset( jd->m_block_y_mcu, 0, sizeof( jd->m_block_y_mcu ) );

    jd->m_eob_run = 0;

    memset( jd->m_block_y_mcu, 0, sizeof( jd->m_block_y_mcu ) );

    jd->m_pIn_buf_ofs = jd->m_in_buf;
    jd->m_in_buf_left = 0;
    jd->m_eof_flag = false;
    jd->m_tem_flag = 0;

    memset( jd->m_in_buf_pad_start, 0, sizeof( jd->m_in_buf_pad_start ) );
    memset( jd->m_in_buf, 0, sizeof( jd->m_in_buf ) );
    memset( jd->m_in_buf_pad_end, 0, sizeof( jd->m_in_buf_pad_end ) );

    jd->m_restart_interval = 0;
    jd->m_restarts_left    = 0;
    jd->m_next_restart_num = 0;

    jd->m_max_mcus_per_row = 0;
    jd->m_max_blocks_per_mcu = 0;
    jd->m_max_mcus_per_col = 0;

    memset( jd->m_last_dc_val, 0, sizeof( jd->m_last_dc_val ) );
    jd->m_pMCU_coefficients = NULL;
    jd->m_pSample_buf = NULL;

    jd->m_total_bytes_read = 0;

    jd->m_pScan_line_0 = NULL;
    jd->m_pScan_line_1 = NULL;

    // Ready the input buffer.
    jd_prep_in_buffer( jd );

    // Prime the bit buffer.
    jd->m_bits_left = 16;
    jd->m_bit_buf = 0;

    jd_get_bits( 16, jd );
    jd_get_bits( 16, jd );

    for( int i = 0; i < JPGD_MAX_BLOCKS_PER_MCU; i++ )
	jd->m_mcu_block_max_zag[ i ] = 64;

    jd_locate_sof_marker( jd );
}

void jd_deinit( jpeg_decoder* jd )
{
    jd_free_all_blocks( jd );
}
