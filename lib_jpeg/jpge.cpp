// JPEG encoder
// SunDog port by Alexander Zolotov (2011 - 2016)

// based on:
// jpge.cpp - C++ class for JPEG compression.
// Public domain, Rich Geldreich <richgel99@gmail.com>
// v1.01, Dec. 18, 2010 - Initial release
// v1.02, Apr. 6, 2011 - Removed 2x2 ordered dither in H2V1 chroma subsampling method load_block_16_8_8(). (The rounding factor was 2, when it should have been 1. Either way, it wasn't helping.)
// v1.03, Apr. 16, 2011 - Added support for optimized Huffman code tables, optimized dynamic memory allocation down to only 1 alloc.
//                        Also from Alex Evans: Added RGBA support, linear memory allocator (no longer needed in v1.03).
// v1.04, May. 19, 2012: Forgot to set m_pFile ptr to NULL in cfile_stream::close(). Thanks to Owen Kaluza for reporting this bug.
//                       Code tweaks to fix VS2008 static code analysis warnings (all looked harmless).
//                       Code review revealed method load_block_16_8_8() (used for the non-default H2V1 sampling mode to downsample chroma) somehow didn't get the rounding factor fix from v1.02.

//Modularity: 100%

#include "core/core.h"
#include "jpge.h"

#define JPGE_MAX( a,b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#define JPGE_MIN( a,b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )

#define clear_obj( obj ) { bmem_set( obj, sizeof( obj ), 0 ); }

// Various JPEG enums and tables.
enum { M_SOF0 = 0xC0, M_DHT = 0xC4, M_SOI = 0xD8, M_EOI = 0xD9, M_SOS = 0xDA, M_DQT = 0xDB, M_APP0 = 0xE0 };
enum { DC_LUM_CODES = 12, AC_LUM_CODES = 256, DC_CHROMA_CODES = 12, AC_CHROMA_CODES = 256, MAX_HUFF_SYMBOLS = 257, MAX_HUFF_CODESIZE = 32 };

static uchar s_zag[ 64 ] = { 0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63 };
static int16 s_std_lum_quant[ 64 ] = { 16,11,12,14,12,10,16,14,13,14,18,17,16,19,24,40,26,24,22,22,24,49,35,37,29,40,58,51,61,60,57,51,56,55,64,72,92,78,64,68,87,69,55,56,80,109,81,87,95,98,103,104,103,62,77,113,121,112,100,120,92,101,103,99 };
static int16 s_std_croma_quant[ 64 ] = { 17,18,18,24,21,24,47,26,26,47,99,66,56,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99 };
static uchar s_dc_lum_bits[ 17 ] = { 0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
static uchar s_dc_lum_val[ DC_LUM_CODES ] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
static uchar s_ac_lum_bits[ 17 ] = { 0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };
static uchar s_ac_lum_val[ AC_LUM_CODES ] =
{
  0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,
  0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
  0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
  0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,
  0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
  0xf9,0xfa
};
static uchar s_dc_chroma_bits[ 17 ] = { 0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };
static uchar s_dc_chroma_val[ DC_CHROMA_CODES ]  = { 0,1,2,3,4,5,6,7,8,9,10,11 };
static uchar s_ac_chroma_bits[ 17 ] = { 0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77 };
static uchar s_ac_chroma_val[ AC_CHROMA_CODES ] =
{
  0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,
  0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,
  0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
  0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,
  0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
  0xf9,0xfa
};

// Forward DCT - DCT derived from jfdctint.
enum { CONST_BITS = 13, ROW_BITS = 2 };
#define DCT_DESCALE( x, n ) ( ((x) + (((int)1) << ((n) - 1))) >> (n) )
#define DCT_MUL( var, c ) ( (int16)(var) * (int)(c) )
#define DCT1D( s0, s1, s2, s3, s4, s5, s6, s7 ) \
    int t0 = s0 + s7, t7 = s0 - s7, t1 = s1 + s6, t6 = s1 - s6, t2 = s2 + s5, t5 = s2 - s5, t3 = s3 + s4, t4 = s3 - s4; \
    int t10 = t0 + t3, t13 = t0 - t3, t11 = t1 + t2, t12 = t1 - t2; \
    int u1 = DCT_MUL( t12 + t13, 4433 ); \
    s2 = u1 + DCT_MUL( t13, 6270 ); \
    s6 = u1 + DCT_MUL( t12, -15137 ); \
    u1 = t4 + t7; \
    int u2 = t5 + t6, u3 = t4 + t6, u4 = t5 + t7; \
    int z5 = DCT_MUL( u3 + u4, 9633 ); \
    t4 = DCT_MUL( t4, 2446 ); t5 = DCT_MUL( t5, 16819 ); \
    t6 = DCT_MUL( t6, 25172 ); t7 = DCT_MUL( t7, 12299 ); \
    u1 = DCT_MUL( u1, -7373 ); u2 = DCT_MUL( u2, -20995 ); \
    u3 = DCT_MUL( u3, -16069 ); u4 = DCT_MUL( u4, -3196 ); \
    u3 += z5; u4 += z5; \
    s0 = t10 + t11; s1 = t7 + u1 + u4; s3 = t6 + u2 + u3; s4 = t10 - t11; s5 = t5 + u2 + u4; s7 = t4 + u1 + u3;

static void DCT2D( int* p )
{
    int c, *q = p;
    for( c = 7; c >= 0; c--, q += 8 )
    {
	int s0 = q[ 0 ], s1 = q[ 1 ], s2 = q[ 2 ], s3 = q[ 3 ], s4 = q[ 4 ], s5 = q[ 5 ], s6 = q[ 6 ], s7 = q[ 7 ];
	DCT1D( s0, s1, s2, s3, s4, s5, s6, s7 );
	q[ 0 ] = s0 << ROW_BITS; q[ 1 ] = DCT_DESCALE( s1, CONST_BITS - ROW_BITS ); q[ 2 ] = DCT_DESCALE( s2, CONST_BITS - ROW_BITS ); q[ 3 ] = DCT_DESCALE( s3, CONST_BITS - ROW_BITS );
	q[ 4 ] = s4 << ROW_BITS; q[ 5 ] = DCT_DESCALE( s5, CONST_BITS - ROW_BITS ); q[ 6 ] = DCT_DESCALE( s6, CONST_BITS - ROW_BITS ); q[ 7 ] = DCT_DESCALE( s7, CONST_BITS - ROW_BITS );
    }
    for( q = p, c = 7; c >= 0; c--, q++ )
    {
	int s0 = q[ 0 * 8 ], s1 = q[ 1 * 8 ], s2 = q[ 2 * 8 ], s3 = q[ 3 * 8 ], s4 = q[ 4 * 8 ], s5 = q[ 5 * 8 ], s6 = q[ 6 * 8 ], s7 = q[ 7 * 8 ];
	DCT1D( s0, s1, s2, s3, s4, s5, s6, s7 );
	q[ 0 * 8 ] = DCT_DESCALE( s0, ROW_BITS + 3 ); q[ 1 * 8 ] = DCT_DESCALE( s1, CONST_BITS + ROW_BITS + 3 ); q[ 2 * 8 ] = DCT_DESCALE( s2, CONST_BITS + ROW_BITS + 3 ); q[ 3 * 8 ] = DCT_DESCALE( s3, CONST_BITS + ROW_BITS + 3 );
	q[ 4 * 8 ] = DCT_DESCALE( s4, ROW_BITS + 3 ); q[ 5 * 8 ] = DCT_DESCALE( s5, CONST_BITS + ROW_BITS + 3 ); q[ 6 * 8 ] = DCT_DESCALE( s6, CONST_BITS + ROW_BITS + 3 ); q[ 7 * 8 ] = DCT_DESCALE( s7, CONST_BITS + ROW_BITS + 3 );
    }
}

struct sym_freq { uint m_key, m_sym_index; };

// Radix sorts sym_freq[] array by 32-bit key m_key. Returns ptr to sorted values.
inline sym_freq* radix_sort_syms( uint num_syms, sym_freq* pSyms0, sym_freq* pSyms1 )
{
    const uint cMaxPasses = 4;
    uint hist[ 256 * cMaxPasses ]; 
    clear_obj( hist );
    for( uint i = 0; i < num_syms; i++ ) 
    { 
	uint freq = pSyms0[ i ].m_key; 
	hist[ freq & 0xFF ]++; 
	hist[ 256 + ( ( freq >> 8 ) & 0xFF ) ]++; 
	hist[ 256 * 2 + ( ( freq >> 16 ) & 0xFF ) ]++; 
	hist[ 256 * 3 + ( ( freq >> 24 ) & 0xFF ) ]++; 
    }
    sym_freq* pCur_syms = pSyms0, *pNew_syms = pSyms1;
    uint total_passes = cMaxPasses; 
    while( ( total_passes > 1 ) && ( num_syms == hist[ ( total_passes - 1 ) * 256 ] ) ) 
    {
	total_passes--;
    }
    for( uint pass_shift = 0, pass = 0; pass < total_passes; pass++, pass_shift += 8 )
    {
	const uint* pHist = &hist[ pass << 8 ];
	uint offsets[ 256 ], cur_ofs = 0;
	for( uint i = 0; i < 256; i++ ) { offsets[ i ] = cur_ofs; cur_ofs += pHist[ i ]; }
	for( uint i = 0; i < num_syms; i++ )
	    pNew_syms[ offsets[ ( pCur_syms[ i ].m_key >> pass_shift ) & 0xFF ]++ ] = pCur_syms[ i ];
	sym_freq* t = pCur_syms; pCur_syms = pNew_syms; pNew_syms = t;
    }
    return pCur_syms;
}

// calculate_minimum_redundancy() originally written by: Alistair Moffat, alistair@cs.mu.oz.au, Jyrki Katajainen, jyrki@diku.dk, November 1996.
static void calculate_minimum_redundancy( sym_freq* A, int n )
{
    int root, leaf, next, avbl, used, dpth;
    if( n == 0 ) return; else if( n == 1 ) { A[ 0 ].m_key = 1; return; }
    A[ 0 ].m_key += A[ 1 ].m_key; root = 0; leaf = 2;
    for( next = 1; next < n - 1; next++ )
    {
	if( leaf >= n || A[ root ].m_key < A[ leaf ].m_key ) { A[ next ].m_key = A[ root ].m_key; A[ root++ ].m_key = next; } else A[ next ].m_key = A[ leaf++ ].m_key;
	if( leaf >= n || ( root < next && A[ root ].m_key < A[ leaf ].m_key ) ) { A[ next ].m_key += A[ root ].m_key; A[ root++ ].m_key = next; } else A[ next ].m_key += A[ leaf++ ].m_key;
    }
    A[ n - 2 ].m_key = 0;
    for( next = n - 3; next >= 0; next-- ) A[ next ].m_key = A[ A[ next ].m_key ].m_key + 1;
    avbl = 1; used = dpth = 0; root = n - 2; next = n - 1;
    while( avbl > 0 )
    {
	while( root >= 0 && (int)A[ root ].m_key == dpth ) { used++; root--; }
	while( avbl > used ) { A[ next-- ].m_key = dpth; avbl--; }
	avbl = 2 * used; dpth++; used = 0;
    }
}

// Limits canonical Huffman code table's max code size to max_code_size.
static void huffman_enforce_max_code_size( int* pNum_codes, int code_list_len, int max_code_size )
{
    if( code_list_len <= 1 ) return;

    for( int i = max_code_size + 1; i <= MAX_HUFF_CODESIZE; i++ ) pNum_codes[ max_code_size ] += pNum_codes[ i ];

    uint total = 0;
    for( int i = max_code_size; i > 0; i-- )
    total += ( ( (uint)pNum_codes[ i ] ) << ( max_code_size - i ) );
    
    while( total != ( 1UL << max_code_size ) )
    {
	pNum_codes[ max_code_size ]--;
	for( int i = max_code_size - 1; i > 0; i-- )
	{
	    if( pNum_codes[ i ] ) { pNum_codes[ i ]--; pNum_codes[ i + 1 ] += 2; break; }
	}
	total--;
    }
}

// Generates an optimized offman table.
static void je_optimize_huffman_table( int table_num, int table_len, jpeg_encoder* je )
{
    sym_freq syms0[ MAX_HUFF_SYMBOLS ], syms1[ MAX_HUFF_SYMBOLS ];
    syms0[ 0 ].m_key = 1; syms0[ 0 ].m_sym_index = 0;  // dummy symbol, assures that no valid code contains all 1's
    int num_used_syms = 1;
    const uint* pSym_count = &je->m_huff_count[ table_num ][ 0 ];
    for( int i = 0; i < table_len; i++ )
	if( pSym_count[ i ] ) { syms0[ num_used_syms ].m_key = pSym_count[i]; syms0[ num_used_syms++ ].m_sym_index = i + 1; }
    sym_freq* pSyms = radix_sort_syms( num_used_syms, syms0, syms1 );
    calculate_minimum_redundancy( pSyms, num_used_syms );

    // Count the # of symbols of each code size.
    int num_codes[ 1 + MAX_HUFF_CODESIZE ]; 
    clear_obj( num_codes );
    for( int i = 0; i < num_used_syms; i++ )
	num_codes[ pSyms[ i ].m_key ]++;

    const uint JPGE_CODE_SIZE_LIMIT = 16; // the maximum possible size of a JPEG Huffman code (valid range is [9,16] - 9 vs. 8 because of the dummy symbol)
    huffman_enforce_max_code_size( num_codes, num_used_syms, JPGE_CODE_SIZE_LIMIT );

    // Compute m_huff_bits array, which contains the # of symbols per code size.
    clear_obj( je->m_huff_bits[ table_num ] );
    for( int i = 1; i <= (int)JPGE_CODE_SIZE_LIMIT; i++ )
	je->m_huff_bits[ table_num ][ i ] = (uchar)( num_codes[ i ] );

    // Remove the dummy symbol added above, which must be in largest bucket.
    for( int i = JPGE_CODE_SIZE_LIMIT; i >= 1; i-- )
    {
	if( je->m_huff_bits[ table_num ][ i ] ) { je->m_huff_bits[ table_num ][ i ]--; break; }
    }

    // Compute the m_huff_val array, which contains the symbol indices sorted by code size (smallest to largest).
    for( int i = num_used_syms - 1; i >= 1; i-- )
	je->m_huff_val[ table_num ][ num_used_syms - 1 - i ] = (uchar)( pSyms[ i ].m_sym_index - 1 );
}

// JPEG marker generation.
static void je_emit_byte( uchar i, jpeg_encoder* je )
{
    je->m_all_stream_writes_succeeded = je->m_all_stream_writes_succeeded && ( bfs_putc( i, je->f ) != -1 );
}

static void je_emit_word( uint i, jpeg_encoder* je )
{
    je_emit_byte( (uchar)( i >> 8 ), je ); 
    je_emit_byte( (uchar)( i & 0xFF ), je );
}

static void je_emit_marker( int marker, jpeg_encoder* je )
{
    je_emit_byte( 0xFF, je ); 
    je_emit_byte( (uchar)marker, je );
}

// Emit JFIF marker
static void je_emit_jfif_app0( jpeg_encoder* je )
{
    je_emit_marker( M_APP0, je );
    je_emit_word( 2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1, je );
    je_emit_byte( 0x4A, je ); je_emit_byte( 0x46, je ); je_emit_byte( 0x49, je ); je_emit_byte( 0x46, je ); // Identifier: ASCII "JFIF"
    je_emit_byte( 0, je );
    je_emit_byte( 1, je ); // Major version
    je_emit_byte( 1, je ); // Minor version
    je_emit_byte( 0, je ); // Density unit
    je_emit_word( 1, je );
    je_emit_word( 1, je );
    je_emit_byte( 0, je ); // No thumbnail image
    je_emit_byte( 0, je );
}

// Emit quantization tables
static void je_emit_dqt( jpeg_encoder* je )
{
    for( int i = 0; i < ( ( je->m_num_components == 3 ) ? 2 : 1 ); i++ )
    {
	je_emit_marker( M_DQT, je );
	je_emit_word( 64 + 1 + 2, je );
	je_emit_byte( (uchar)i, je );
	for( int j = 0; j < 64; j++ )
	    je_emit_byte( (uchar)( je->m_quantization_tables[ i ][ j ] ), je );
    }
}

// Emit start of frame marker
static void je_emit_sof( jpeg_encoder* je )
{
    je_emit_marker( M_SOF0, je ); // baseline
    je_emit_word( 3 * je->m_num_components + 2 + 5 + 1, je );
    je_emit_byte( 8, je ); // precision
    je_emit_word( je->m_image_y, je );
    je_emit_word( je->m_image_x, je );
    je_emit_byte( je->m_num_components, je );
    for( int i = 0; i < je->m_num_components; i++ )
    {
	je_emit_byte( (uchar)( i + 1 ), je ); // component ID
	je_emit_byte( ( je->m_comp_h_samp[ i ] << 4 ) + je->m_comp_v_samp[ i ], je ); // h and v sampling
	je_emit_byte( i > 0, je ); // quant. table num
    }
}

// Emit Huffman table.
static void je_emit_dht( uchar* bits, uchar* val, int index, bool ac_flag, jpeg_encoder* je )
{
    je_emit_marker( M_DHT, je );

    int length = 0;
    for( int i = 1; i <= 16; i++ )
	length += bits[ i ];

    je_emit_word( length + 2 + 1 + 16, je );
    je_emit_byte( (uchar)( index + ( ac_flag << 4 ) ), je );

    for( int i = 1; i <= 16; i++ )
	je_emit_byte( bits[ i ], je );

    for( int i = 0; i < length; i++ )
	je_emit_byte( val[ i ], je );
}

// Emit all Huffman tables.
static void je_emit_dhts( jpeg_encoder* je )
{
    je_emit_dht( je->m_huff_bits[ 0 + 0 ], je->m_huff_val[ 0 + 0 ], 0, 0, je );
    je_emit_dht( je->m_huff_bits[ 2 + 0 ], je->m_huff_val[ 2 + 0 ], 0, 1, je );
    if( je->m_num_components == 3 )
    {
	je_emit_dht( je->m_huff_bits[ 0 + 1 ], je->m_huff_val[ 0 + 1 ], 1, 0, je );
	je_emit_dht( je->m_huff_bits[ 2 + 1 ], je->m_huff_val[ 2 + 1 ], 1, 1, je );
    }
}

// emit start of scan
static void je_emit_sos( jpeg_encoder* je )
{
    je_emit_marker( M_SOS, je );
    je_emit_word( 2 * je->m_num_components + 2 + 1 + 3, je );
    je_emit_byte( je->m_num_components, je );
    for( int i = 0; i < je->m_num_components; i++ )
    {
	je_emit_byte( (uchar)( i + 1 ), je );
	if( i == 0 )
	    je_emit_byte( ( 0 << 4 ) + 0, je );
	else
	    je_emit_byte( ( 1 << 4 ) + 1, je );
    }
    je_emit_byte( 0, je ); // spectral selection
    je_emit_byte( 63, je );
    je_emit_byte( 0, je );
}

// Emit all markers at beginning of image file.
static void je_emit_markers( jpeg_encoder* je )
{
    je_emit_marker( M_SOI, je );
    je_emit_jfif_app0( je );
    je_emit_dqt( je );
    je_emit_sof( je );
    je_emit_dhts( je );
    je_emit_sos( je );
}

// Compute the actual canonical Huffman codes/code sizes given the JPEG huff bits and val arrays.
static void je_compute_huffman_table( uint* codes, uchar* code_sizes, uchar* bits, uchar* val, jpeg_encoder* je )
{
    int i, l, last_p, si;
    uchar huff_size[ 257 ];
    uint huff_code[ 257 ];
    uint code;

    int p = 0;
    for( l = 1; l <= 16; l++ )
	for( i = 1; i <= bits[ l ]; i++ )
	    huff_size[ p++ ] = (uchar)l;

    huff_size[ p ] = 0; last_p = p; // write sentinel

    code = 0; si = huff_size[ 0 ]; p = 0;

    while( huff_size[ p ] )
    {
	while( huff_size[ p ] == si )
	huff_code[ p++ ] = code++;
	code <<= 1;
	si++;
    }

    bmem_set( codes, sizeof( codes[ 0 ] ) * 256, 0 );
    bmem_set( code_sizes, sizeof( code_sizes[ 0 ] ) * 256, 0 );
    for( p = 0; p < last_p; p++ )
    {
	codes[ val[ p ] ] = huff_code[ p ];
	code_sizes[ val[ p ] ] = huff_size[ p ];
    }
}

// Quantization table generation.
static void je_compute_quant_table( int* pDst, int16* pSrc, jpeg_encoder* je )
{
    int q;
    if( je->m_params.m_quality < 50 )
	q = 5000 / je->m_params.m_quality;
    else
	q = 200 - je->m_params.m_quality * 2;
    for( int i = 0; i < 64; i++ )
    {
	int j = *pSrc++; j = ( j * q + 50L ) / 100L;
	*pDst++ = JPGE_MIN( JPGE_MAX( j, 1 ), 255 );
    }
}

// Higher-level methods.
static void je_first_pass_init( jpeg_encoder* je )
{
    je->m_bit_buffer = 0; je->m_bits_in = 0;
    bmem_set( je->m_last_dc_val, 3 * sizeof( je->m_last_dc_val[ 0 ] ), 0 );
    je->m_mcu_y_ofs = 0;
    je->m_pass_num = 1;
}

static bool je_second_pass_init( jpeg_encoder* je )
{
    je_compute_huffman_table( &je->m_huff_codes[ 0 + 0 ][ 0 ], &je->m_huff_code_sizes[ 0 + 0 ][ 0 ], je->m_huff_bits[ 0 + 0 ], je->m_huff_val[ 0 + 0 ], je );
    je_compute_huffman_table( &je->m_huff_codes[ 2 + 0 ][ 0 ], &je->m_huff_code_sizes[ 2 + 0 ][ 0 ], je->m_huff_bits[ 2 + 0 ], je->m_huff_val[ 2 + 0 ], je );
    if( je->m_num_components > 1 )
    {
	je_compute_huffman_table( &je->m_huff_codes[ 0 + 1 ][ 0 ], &je->m_huff_code_sizes[ 0 + 1 ][ 0 ], je->m_huff_bits[ 0 + 1 ], je->m_huff_val[ 0 + 1 ], je );
	je_compute_huffman_table( &je->m_huff_codes[ 2 + 1 ][ 0 ], &je->m_huff_code_sizes[ 2 + 1 ][ 0 ], je->m_huff_bits[ 2 + 1 ], je->m_huff_val[ 2 + 1 ], je );
    }
    je_first_pass_init( je );
    je_emit_markers( je );
    je->m_pass_num = 2;
    return 1;
}

static bool je_jpg_open( int p_x_res, int p_y_res, int src_channels, jpeg_encoder* je )
{
    je->m_num_components = 3;
    switch( je->m_params.m_subsampling )
    {
	case Y_ONLY:
	    je->m_num_components   = 1;
	    je->m_comp_h_samp[ 0 ] = 1; je->m_comp_v_samp[ 0 ] = 1;
	    je->m_mcu_x            = 8; je->m_mcu_y            = 8;
	    break;
	case H1V1:
	    je->m_comp_h_samp[ 0 ] = 1; je->m_comp_v_samp[ 0 ] = 1;
	    je->m_comp_h_samp[ 1 ] = 1; je->m_comp_v_samp[ 1 ] = 1;
	    je->m_comp_h_samp[ 2 ] = 1; je->m_comp_v_samp[ 2 ] = 1;
	    je->m_mcu_x            = 8; je->m_mcu_y            = 8;
	    break;
	case H2V1:
	    je->m_comp_h_samp[ 0 ] = 2; je->m_comp_v_samp[ 0 ] = 1;
	    je->m_comp_h_samp[ 1 ] = 1; je->m_comp_v_samp[ 1 ] = 1;
	    je->m_comp_h_samp[ 2 ] = 1; je->m_comp_v_samp[ 2 ] = 1;
	    je->m_mcu_x            = 16; je->m_mcu_y           = 8;
	    break;
	case H2V2:
	    je->m_comp_h_samp[ 0 ] = 2; je->m_comp_v_samp[ 0 ] = 2;
	    je->m_comp_h_samp[ 1 ] = 1; je->m_comp_v_samp[ 1 ] = 1;
	    je->m_comp_h_samp[ 2 ] = 1; je->m_comp_v_samp[ 2 ] = 1;
	    je->m_mcu_x            = 16; je->m_mcu_y           = 16;
    }

    je->m_image_x        = p_x_res; je->m_image_y = p_y_res;
    je->m_image_bpl      = je->m_image_x * src_channels;
    je->m_image_x_mcu    = ( je->m_image_x + je->m_mcu_x - 1 ) & ( ~( je->m_mcu_x - 1 ) );
    je->m_image_y_mcu    = ( je->m_image_y + je->m_mcu_y - 1 ) & ( ~( je->m_mcu_y - 1 ) );
    je->m_image_bpl_xlt  = je->m_image_x * je->m_num_components;
    je->m_image_bpl_mcu  = je->m_image_x_mcu * je->m_num_components;
    je->m_mcus_per_row   = je->m_image_x_mcu / je->m_mcu_x;

    if( ( je->m_mcu_lines[ 0 ] = (uchar*)bmem_new( je->m_image_bpl_mcu * je->m_mcu_y ) ) == NULL ) return 0;
    for( int i = 1; i < je->m_mcu_y; i++ )
	je->m_mcu_lines[ i ] = je->m_mcu_lines[ i - 1 ] + je->m_image_bpl_mcu;

    je_compute_quant_table( je->m_quantization_tables[ 0 ], s_std_lum_quant, je );
    je_compute_quant_table( je->m_quantization_tables[ 1 ], je->m_params.m_no_chroma_discrim_flag ? s_std_lum_quant : s_std_croma_quant, je );

    je->m_out_buf_left = JPGE_OUT_BUF_SIZE;
    je->m_pOut_buf = je->m_out_buf;

    if( je->m_params.m_two_pass_flag )
    {
	clear_obj( je->m_huff_count );
	je_first_pass_init( je );
    }
    else
    {
	bmem_copy( je->m_huff_bits[ 0 + 0 ], s_dc_lum_bits, 17 );    bmem_copy( je->m_huff_val[ 0 + 0 ], s_dc_lum_val, DC_LUM_CODES );
	bmem_copy( je->m_huff_bits[ 2 + 0 ], s_ac_lum_bits, 17 );    bmem_copy( je->m_huff_val[ 2 + 0 ], s_ac_lum_val, AC_LUM_CODES );
	bmem_copy( je->m_huff_bits[ 0 + 1 ], s_dc_chroma_bits, 17 ); bmem_copy( je->m_huff_val[ 0 + 1 ], s_dc_chroma_val, DC_CHROMA_CODES );
	bmem_copy( je->m_huff_bits[ 2 + 1 ], s_ac_chroma_bits, 17 ); bmem_copy( je->m_huff_val[ 2 + 1 ], s_ac_chroma_val, AC_CHROMA_CODES );
	if( !je_second_pass_init( je ) ) return 0; // in effect, skip over the first pass
    }
    return je->m_all_stream_writes_succeeded;
}

static void je_load_block_8_8_grey( int x, jpeg_encoder* je )
{
    uchar* pSrc;
    sample_array_t* pDst = je->m_sample_array;
    x <<= 3;
    for( int i = 0; i < 8; i++, pDst += 8 )
    {
	pSrc = je->m_mcu_lines[ i ] + x;
	pDst[ 0 ] = pSrc[ 0 ] - 128; pDst[ 1 ] = pSrc[ 1 ] - 128; pDst[ 2 ] = pSrc[ 2 ] - 128; pDst[ 3 ] = pSrc[ 3 ] - 128;
	pDst[ 4 ] = pSrc[ 4 ] - 128; pDst[ 5 ] = pSrc[ 5 ] - 128; pDst[ 6 ] = pSrc[ 6 ] - 128; pDst[ 7 ] = pSrc[ 7 ] - 128;
    }
}

static void je_load_block_8_8( int x, int y, int c, jpeg_encoder* je )
{
    uchar* pSrc;
    sample_array_t* pDst = je->m_sample_array;
    x = ( x * ( 8 * 3 ) ) + c;
    y <<= 3;
    for( int i = 0; i < 8; i++, pDst += 8 )
    {
	pSrc = je->m_mcu_lines[ y + i ] + x;
	pDst[ 0 ] = pSrc[ 0 * 3 ] - 128; pDst[ 1 ] = pSrc[ 1 * 3 ] - 128; pDst[ 2 ] = pSrc[ 2 * 3 ] - 128; pDst[ 3 ] = pSrc[ 3 * 3 ] - 128;
	pDst[ 4 ] = pSrc[ 4 * 3 ] - 128; pDst[ 5 ] = pSrc[ 5 * 3 ] - 128; pDst[ 6 ] = pSrc[ 6 * 3 ] - 128; pDst[ 7 ] = pSrc[ 7 * 3 ] - 128;
    }
}

static void je_load_block_16_8( int x, int c, jpeg_encoder* je )
{
    uchar* pSrc1;
    uchar* pSrc2;
    sample_array_t* pDst = je->m_sample_array;
    x = ( x * ( 16 * 3 ) ) + c;
    int a = 0, b = 2;
    for( int i = 0; i < 16; i += 2, pDst += 8 )
    {
	pSrc1 = je->m_mcu_lines[ i + 0 ] + x;
	pSrc2 = je->m_mcu_lines[ i + 1 ] + x;
	pDst[ 0 ] = ( ( pSrc1[ 0 * 3 ] + pSrc1[ 1 * 3 ] + pSrc2[ 0 * 3 ] + pSrc2[ 1 * 3 ] + a ) >> 2 ) - 128; pDst[ 1 ] = ( ( pSrc1[ 2 * 3 ] + pSrc1[ 3 * 3 ] + pSrc2[ 2 * 3 ] + pSrc2[ 3 * 3 ] + b ) >> 2 ) - 128;
	pDst[ 2 ] = ( ( pSrc1[ 4 * 3 ] + pSrc1[ 5 * 3 ] + pSrc2[ 4 * 3 ] + pSrc2[ 5 * 3 ] + a ) >> 2 ) - 128; pDst[ 3 ] = ( ( pSrc1[ 6 * 3 ] + pSrc1[ 7 * 3 ] + pSrc2[ 6 * 3 ] + pSrc2[ 7 * 3 ] + b ) >> 2 ) - 128;
	pDst[ 4 ] = ( ( pSrc1[ 8 * 3 ] + pSrc1[ 9 * 3 ] + pSrc2[ 8 * 3 ] + pSrc2[ 9 * 3 ] + a ) >> 2 ) - 128; pDst[ 5 ] = ( ( pSrc1[ 10 * 3 ] + pSrc1[ 11 * 3 ] + pSrc2[ 10 * 3 ] + pSrc2[ 11 * 3 ] + b ) >> 2 ) - 128;
	pDst[ 6 ] = ( ( pSrc1[ 12 * 3 ] + pSrc1[ 13 * 3 ] + pSrc2[ 12 * 3 ] + pSrc2[ 13 * 3 ] + a ) >> 2 ) - 128; pDst[ 7 ] = ( ( pSrc1[ 14 * 3 ] + pSrc1[ 15 * 3 ] + pSrc2[ 14 * 3 ] + pSrc2[ 15 * 3 ] + b ) >> 2 ) - 128;
	int temp = a; a = b; b = temp;
    }
}

static void je_load_block_16_8_8( int x, int c, jpeg_encoder* je )
{
    uchar* pSrc1;
    sample_array_t* pDst = je->m_sample_array;
    x = ( x * ( 16 * 3 ) ) + c;
    for( int i = 0; i < 8; i++, pDst += 8 )
    {
	pSrc1 = je->m_mcu_lines[ i + 0 ] + x;
	pDst[ 0 ] = ( ( pSrc1[ 0 * 3 ] + pSrc1[ 1 * 3 ] ) >> 1 ) - 128; pDst[ 1 ] = ( ( pSrc1[ 2 * 3 ] + pSrc1[ 3 * 3 ] ) >> 1 ) - 128;
	pDst[ 2 ] = ( ( pSrc1[ 4 * 3 ] + pSrc1[ 5 * 3 ] ) >> 1 ) - 128; pDst[ 3 ] = ( ( pSrc1[ 6 * 3 ] + pSrc1[ 7 * 3 ] ) >> 1 ) - 128;
	pDst[ 4 ] = ( ( pSrc1[ 8 * 3 ] + pSrc1[ 9 * 3 ] ) >> 1 ) - 128; pDst[ 5 ] = ( ( pSrc1[ 10 * 3 ] + pSrc1[ 11 * 3 ] ) >> 1 ) - 128;
	pDst[ 6 ] = ( ( pSrc1[ 12 * 3 ] + pSrc1[ 13 * 3 ] ) >> 1 ) - 128; pDst[ 7 ] = ( ( pSrc1[ 14 * 3 ] + pSrc1[ 15 * 3 ] ) >> 1 ) - 128;
    }
}

static void je_load_quantized_coefficients( int component_num, jpeg_encoder* je )
{
    int* q = je->m_quantization_tables[ component_num > 0 ];
    int16* pDst = je->m_coefficient_array;
    for( int i = 0; i < 64; i++ )
    {
	sample_array_t j = je->m_sample_array[ s_zag[ i ] ];
	if( j < 0 )
	{
	    if( ( j = -j + ( *q >> 1 ) ) < *q )
		*pDst++ = 0;
	    else
		*pDst++ = (int16)( -( j / *q ) );
	}
	else
	{
	    if( ( j = j + ( *q >> 1 ) ) < *q )
		*pDst++ = 0;
	    else
		*pDst++ = (int16)( j / *q );
	}
	q++;
    }
}

static void je_flush_output_buffer( jpeg_encoder* je )
{
    if( je->m_out_buf_left != JPGE_OUT_BUF_SIZE )
	je->m_all_stream_writes_succeeded = je->m_all_stream_writes_succeeded && ( bfs_write( je->m_out_buf, 1, JPGE_OUT_BUF_SIZE - je->m_out_buf_left, je->f ) > 0 );
    je->m_pOut_buf = je->m_out_buf;
    je->m_out_buf_left = JPGE_OUT_BUF_SIZE;
}

#define JPGE_PUT_BYTE( c ) { *je->m_pOut_buf++ = (c); if( --je->m_out_buf_left == 0 ) je_flush_output_buffer( je ); }

static void je_put_bits( uint bits, uint len, jpeg_encoder* je )
{
    je->m_bit_buffer |= ( (uint)bits << ( 24 - ( je->m_bits_in += len ) ) );
    while( je->m_bits_in >= 8 )
    {
	uchar c;
	JPGE_PUT_BYTE( c = (uchar)( ( je->m_bit_buffer >> 16 ) & 0xFF ) );
	if( c == 0xFF ) JPGE_PUT_BYTE( 0 );
	je->m_bit_buffer <<= 8;
	je->m_bits_in -= 8;
    }
}

static void je_code_coefficients_pass_one( int component_num, jpeg_encoder* je )
{
    if( component_num >= 3 ) return; // just to shut up static analysis

    int i, run_len, nbits, temp1;
    int16* src = je->m_coefficient_array;
    uint* dc_count = component_num ? je->m_huff_count[ 0 + 1 ] : je->m_huff_count[ 0 + 0 ];
    uint* ac_count = component_num ? je->m_huff_count[ 2 + 1 ] : je->m_huff_count[ 2 + 0 ];

    temp1 = src[ 0 ] - je->m_last_dc_val[ component_num ];
    je->m_last_dc_val[ component_num ] = src[ 0 ];
    if( temp1 < 0 ) temp1 = -temp1;

    nbits = 0;
    while( temp1 )
    {
	nbits++; temp1 >>= 1;
    }

    dc_count[ nbits ]++;
    for( run_len = 0, i = 1; i < 64; i++ )
    {
	if( ( temp1 = je->m_coefficient_array[ i ] ) == 0 )
	{
	    run_len++;
	}
	else
	{
	    while( run_len >= 16 )
	    {
		ac_count[ 0xF0 ]++;
		run_len -= 16;
	    }
	    if( temp1 < 0 ) temp1 = -temp1;
	    nbits = 1;
	    while( temp1 >>= 1 ) nbits++;
	    ac_count[ ( run_len << 4 ) + nbits ]++;
	    run_len = 0;
	}
    }
    if( run_len ) ac_count[ 0 ]++;
}

static void je_code_coefficients_pass_two( int component_num, jpeg_encoder* je )
{
    int i, j, run_len, nbits, temp1, temp2;
    int16* pSrc = je->m_coefficient_array;
    uint* codes[ 2 ];
    uchar* code_sizes[ 2 ];

    if( component_num == 0 )
    {
	codes[ 0 ] = je->m_huff_codes[ 0 + 0 ]; codes[ 1 ] = je->m_huff_codes[ 2 + 0 ];
	code_sizes[ 0 ] = je->m_huff_code_sizes[ 0 + 0 ]; code_sizes[ 1 ] = je->m_huff_code_sizes[ 2 + 0 ];
    }
    else
    {
	codes[ 0 ] = je->m_huff_codes[ 0 + 1 ]; codes[ 1 ] = je->m_huff_codes[ 2 + 1 ];
	code_sizes[ 0 ] = je->m_huff_code_sizes[ 0 + 1 ]; code_sizes[ 1 ] = je->m_huff_code_sizes[ 2 + 1 ];
    }

    temp1 = temp2 = pSrc[ 0 ] - je->m_last_dc_val[ component_num ];
    je->m_last_dc_val[ component_num ] = pSrc[ 0 ];

    if( temp1 < 0 )
    {
	temp1 = -temp1; temp2--;
    }

    nbits = 0;
    while( temp1 )
    {
	nbits++; temp1 >>= 1;
    }

    je_put_bits( codes[ 0 ][ nbits ], code_sizes[ 0 ][ nbits ], je );
    if( nbits ) je_put_bits( temp2 & ( ( 1 << nbits ) - 1 ), nbits, je );

    for( run_len = 0, i = 1; i < 64; i++ )
    {
	if( ( temp1 = je->m_coefficient_array[ i ] ) == 0 )
	{
	    run_len++;
	}
	else
	{
	    while( run_len >= 16 )
	    {
		je_put_bits( codes[ 1 ][ 0xF0 ], code_sizes[ 1 ][ 0xF0 ], je );
		run_len -= 16;
	    }
	    if( ( temp2 = temp1 ) < 0 )
	    {
		temp1 = -temp1;
		temp2--;
	    }
	    nbits = 1;
	    while( temp1 >>= 1	)
		nbits++;
	    j = ( run_len << 4 ) + nbits;
	    je_put_bits( codes[ 1 ][ j ], code_sizes[ 1 ][ j ], je );
	    je_put_bits( temp2 & ( ( 1 << nbits ) - 1 ), nbits, je );
	    run_len = 0;
	}
    }
    if( run_len )
	je_put_bits( codes[ 1 ][ 0 ], code_sizes[ 1 ][ 0 ], je );
}

static void je_code_block( int component_num, jpeg_encoder* je )
{
    DCT2D( je->m_sample_array );
    je_load_quantized_coefficients( component_num, je);
    if( je->m_pass_num == 1 )
	je_code_coefficients_pass_one( component_num, je );
    else
	je_code_coefficients_pass_two( component_num, je );
}

static void je_process_mcu_row( jpeg_encoder* je )
{
    if( je->m_num_components == 1 )
    {
	for( int i = 0; i < je->m_mcus_per_row; i++ )
	{
	    je_load_block_8_8_grey( i, je ); je_code_block( 0, je );
	}
    }
    else if( ( je->m_comp_h_samp[ 0 ] == 1 ) && ( je->m_comp_v_samp[ 0 ] == 1 ) )
    {
	for( int i = 0; i < je->m_mcus_per_row; i++ )
	{
	    je_load_block_8_8( i, 0, 0, je ); je_code_block( 0, je ); je_load_block_8_8( i, 0, 1, je ); je_code_block( 1, je ); je_load_block_8_8( i, 0, 2, je ); je_code_block( 2, je );
	}
    }
    else if( ( je->m_comp_h_samp[ 0 ] == 2 ) && ( je->m_comp_v_samp[ 0 ] == 1 ) )
    {
	for( int i = 0; i < je->m_mcus_per_row; i++ )
	{
	    je_load_block_8_8( i * 2 + 0, 0, 0, je ); je_code_block( 0, je ); je_load_block_8_8( i * 2 + 1, 0, 0, je ); je_code_block( 0, je );
	    je_load_block_16_8_8( i, 1, je ); je_code_block( 1, je ); je_load_block_16_8_8( i, 2, je ); je_code_block( 2, je );
	}
    }
    else if( ( je->m_comp_h_samp[ 0 ] == 2 ) && ( je->m_comp_v_samp[ 0 ] == 2 ) )
    {
	for( int i = 0; i < je->m_mcus_per_row; i++ )
	{
	    je_load_block_8_8( i * 2 + 0, 0, 0, je ); je_code_block( 0, je ); je_load_block_8_8( i * 2 + 1, 0, 0, je ); je_code_block( 0, je );
	    je_load_block_8_8( i * 2 + 0, 1, 0, je ); je_code_block( 0, je ); je_load_block_8_8( i * 2 + 1, 1, 0, je ); je_code_block( 0, je );
	    je_load_block_16_8( i, 1, je ); je_code_block( 1, je ); je_load_block_16_8( i, 2, je ); je_code_block( 2, je );
	}
    }
}

static bool je_terminate_pass_one( jpeg_encoder* je )
{
    je_optimize_huffman_table( 0 + 0, DC_LUM_CODES, je ); je_optimize_huffman_table( 2 + 0, AC_LUM_CODES, je );
    if( je->m_num_components > 1 )
    {
	je_optimize_huffman_table( 0 + 1, DC_CHROMA_CODES, je ); je_optimize_huffman_table( 2 + 1, AC_CHROMA_CODES, je );
    }
    return je_second_pass_init( je );
}

static bool je_terminate_pass_two( jpeg_encoder* je )
{
    je_put_bits( 0x7F, 7, je );
    je_flush_output_buffer( je );
    je_emit_marker( M_EOI, je );
    je->m_pass_num++; // purposely bump up m_pass_num, for debugging
    return 1;
}

static bool je_process_end_of_image( jpeg_encoder* je )
{
    if( je->m_mcu_y_ofs )
    {
	if( je->m_mcu_y_ofs < 16 ) // check here just to shut up static analysis
	{
	    for( int i = je->m_mcu_y_ofs; i < je->m_mcu_y; i++ )
		bmem_copy( je->m_mcu_lines[ i ], je->m_mcu_lines[ je->m_mcu_y_ofs - 1 ], je->m_image_bpl_mcu );
	}

	je_process_mcu_row( je );
    }

    if( je->m_pass_num == 1 )
	return je_terminate_pass_one( je );
    else
	return je_terminate_pass_two( je );
}

static void je_load_mcu( const void* pSrc, jpeg_encoder* je )
{
    const uchar* Psrc = (const uchar*)pSrc;

    uchar* pDst = je->m_mcu_lines[ je->m_mcu_y_ofs ]; // OK to write up to m_image_bpl_xlt bytes to pDst

    if( je->m_num_components == 1 )
    {
	je->m_params.convert_to_Y( pDst, Psrc, je->m_image_x );
    }
    else
    {
	je->m_params.convert_to_YCC( pDst, Psrc, je->m_image_x );
    }

    // Possibly duplicate pixels at end of scanline if not a multiple of 8 or 16
    if( je->m_num_components == 1 )
    {
	bmem_set( je->m_mcu_lines[ je->m_mcu_y_ofs ] + je->m_image_bpl_xlt, je->m_image_x_mcu - je->m_image_x, pDst[ je->m_image_bpl_xlt - 1 ] );
    }
    else
    {
	const uchar y = pDst[ je->m_image_bpl_xlt - 3 + 0 ], cb = pDst[ je->m_image_bpl_xlt - 3 + 1 ], cr = pDst[ je->m_image_bpl_xlt - 3 + 2 ];
	uchar* q = je->m_mcu_lines[ je->m_mcu_y_ofs ] + je->m_image_bpl_xlt;
	for( int i = je->m_image_x; i < je->m_image_x_mcu; i++ )
	{
	    *q++ = y; *q++ = cb; *q++ = cr;
	}
    }

    if( ++je->m_mcu_y_ofs == je->m_mcu_y )
    {
	je_process_mcu_row( je );
	je->m_mcu_y_ofs = 0;
    }
}

static void je_clear( jpeg_encoder* je )
{
    je->m_mcu_lines[ 0 ] = NULL;
    je->m_pass_num = 0;
    je->m_all_stream_writes_succeeded = 1;
}

void je_init( jpeg_encoder* je )
{
    je_clear( je );
}

void je_deinit( jpeg_encoder* je )
{
    bmem_free( je->m_mcu_lines[ 0 ] );
    je_clear( je );
}

bool je_set_params( bfs_file f, int width, int height, int src_channels, je_comp_params* comp_params, jpeg_encoder* je )
{
    je_deinit( je );
    if( f == 0 ) return 0;
    if( width < 1 ) return 0;
    if( height < 1 ) return 0;
    if( ( src_channels != 1 ) && ( src_channels != 3 ) && ( src_channels != 4 ) ) return 0;
    if( !check_je_comp_params( comp_params ) ) return 0;
    je->f = f;
    je->m_params = *comp_params;
    return je_jpg_open( width, height, src_channels, je );
}

bool je_process_scanline( const void* pScanline, jpeg_encoder* je )
{
    if( ( je->m_pass_num < 1 ) || ( je->m_pass_num > 2 ) ) return 0;
    if( je->m_all_stream_writes_succeeded )
    {
	if( !pScanline )
	{
	    if( !je_process_end_of_image( je ) ) return 0;
	}
	else
	{
	    je_load_mcu( pScanline, je );
	}
    }
    return je->m_all_stream_writes_succeeded;
}

//
//
//

void init_je_comp_params( je_comp_params* jp )
{
    jp->m_quality = 85;
    jp->m_subsampling = H2V2;
    jp->m_no_chroma_discrim_flag = 0;
    jp->m_two_pass_flag = 0;
}

bool check_je_comp_params( je_comp_params* jp )
{
    if( ( jp->m_quality < 1 ) || ( jp->m_quality > 100 ) ) return 0;
    if( (uint)jp->m_subsampling > (uint)H2V2 ) return 0;
    return 1;
}
