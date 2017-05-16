#pragma once

#include <setjmp.h>

#ifdef _MSC_VER
    #define JPGD_NORETURN __declspec(noreturn) 
#elif defined(__GNUC__)
    #define JPGD_NORETURN __attribute__ ((noreturn))
#else
    #define JPGD_NORETURN
#endif

// Success/failure error codes.
enum jpgd_status
{
    JPGD_SUCCESS = 0, JPGD_FAILED = -1, JPGD_DONE = 1,
    JPGD_BAD_DHT_COUNTS = -256, JPGD_BAD_DHT_INDEX, JPGD_BAD_DHT_MARKER, JPGD_BAD_DQT_MARKER, JPGD_BAD_DQT_TABLE, 
    JPGD_BAD_PRECISION, JPGD_BAD_HEIGHT, JPGD_BAD_WIDTH, JPGD_TOO_MANY_COMPONENTS, 
    JPGD_BAD_SOF_LENGTH, JPGD_BAD_VARIABLE_MARKER, JPGD_BAD_DRI_LENGTH, JPGD_BAD_SOS_LENGTH,
    JPGD_BAD_SOS_COMP_ID, JPGD_W_EXTRA_BYTES_BEFORE_MARKER, JPGD_NO_ARITHMITIC_SUPPORT, JPGD_UNEXPECTED_MARKER,
    JPGD_NOT_JPEG, JPGD_UNSUPPORTED_MARKER, JPGD_BAD_DQT_LENGTH, JPGD_TOO_MANY_BLOCKS,
    JPGD_UNDEFINED_QUANT_TABLE, JPGD_UNDEFINED_HUFF_TABLE, JPGD_NOT_SINGLE_SCAN, JPGD_UNSUPPORTED_COLORSPACE,
    JPGD_UNSUPPORTED_SAMP_FACTORS, JPGD_DECODE_ERROR, JPGD_BAD_RESTART_MARKER, JPGD_ASSERTION_ERROR,
    JPGD_BAD_SOS_SPECTRAL, JPGD_BAD_SOS_SUCCESSIVE, JPGD_STREAM_READ, JPGD_NOTENOUGHMEM
};
    
enum 
{ 
    JPGD_IN_BUF_SIZE = 8192, JPGD_MAX_BLOCKS_PER_MCU = 10, JPGD_MAX_HUFF_TABLES = 8, JPGD_MAX_QUANT_TABLES = 4, 
    JPGD_MAX_COMPONENTS = 4, JPGD_MAX_COMPS_IN_SCAN = 4, JPGD_MAX_BLOCKS_PER_ROW = 8192, JPGD_MAX_HEIGHT = 16384, JPGD_MAX_WIDTH = 16384 
};
          
typedef int16 jpgd_quant_t;
typedef int16 jpgd_block_t;

struct huff_tables
{
    bool ac_table;
    uint look_up[ 256 ];
    uint look_up2[ 256 ];
    uchar code_size[ 256 ];
    uint tree[ 512 ];
};

struct coeff_buf
{
    uchar* pData;
    int block_num_x, block_num_y;
    int block_len_x, block_len_y;
    int block_size;
};

struct mem_block
{
    mem_block* m_pNext;
    size_t m_used_count;
    size_t m_size;
    char m_data[ 1 ];
};

struct jpeg_decoder
{
    jmp_buf m_jmp_state;
    mem_block* m_pMem_blocks;
    int m_image_x_size;
    int m_image_y_size;
    bfs_file m_pStream;
    int m_progressive_flag;
    uchar m_huff_ac[JPGD_MAX_HUFF_TABLES];
    uchar* m_huff_num[JPGD_MAX_HUFF_TABLES];      // pointer to number of Huffman codes per bit size
    uchar* m_huff_val[JPGD_MAX_HUFF_TABLES];      // pointer to Huffman codes per bit size
    jpgd_quant_t* m_quant[JPGD_MAX_QUANT_TABLES]; // pointer to quantization tables
    int m_scan_type;                              // Gray, Yh1v1, Yh1v2, Yh2v1, Yh2v2 (CMYK111, CMYK4114 no longer supported)
    int m_comps_in_frame;                         // # of components in frame
    int m_comp_h_samp[JPGD_MAX_COMPONENTS];       // component's horizontal sampling factor
    int m_comp_v_samp[JPGD_MAX_COMPONENTS];       // component's vertical sampling factor
    int m_comp_quant[JPGD_MAX_COMPONENTS];        // component's quantization table selector
    int m_comp_ident[JPGD_MAX_COMPONENTS];        // component's ID
    int m_comp_h_blocks[JPGD_MAX_COMPONENTS];
    int m_comp_v_blocks[JPGD_MAX_COMPONENTS];
    int m_comps_in_scan;                          // # of components in scan
    int m_comp_list[JPGD_MAX_COMPS_IN_SCAN];      // components in this scan
    int m_comp_dc_tab[JPGD_MAX_COMPONENTS];       // component's DC Huffman coding table selector
    int m_comp_ac_tab[JPGD_MAX_COMPONENTS];       // component's AC Huffman coding table selector
    int m_spectral_start;                         // spectral selection start
    int m_spectral_end;                           // spectral selection end
    int m_successive_low;                         // successive approximation low
    int m_successive_high;                        // successive approximation high
    int m_max_mcu_x_size;                         // MCU's max. X size in pixels
    int m_max_mcu_y_size;                         // MCU's max. Y size in pixels
    int m_blocks_per_mcu;
    int m_max_blocks_per_row;
    int m_mcus_per_row, m_mcus_per_col;
    int m_mcu_org[JPGD_MAX_BLOCKS_PER_MCU];
    int m_total_lines_left;                       // total # lines left in image
    int m_mcu_lines_left;                         // total # lines left in this MCU
    int m_real_dest_bytes_per_scan_line;
    int m_dest_bytes_per_scan_line;               // rounded up
    int m_dest_bytes_per_pixel;                   // 4 (RGB) or 1 (Y)
    huff_tables* m_pHuff_tabs[JPGD_MAX_HUFF_TABLES];
    coeff_buf* m_dc_coeffs[JPGD_MAX_COMPONENTS];
    coeff_buf* m_ac_coeffs[JPGD_MAX_COMPONENTS];
    int m_eob_run;
    int m_block_y_mcu[JPGD_MAX_COMPONENTS];
    uchar* m_pIn_buf_ofs;
    int m_in_buf_left;
    int m_tem_flag;
    bool m_eof_flag;
    uchar m_in_buf_pad_start[128];
    uchar m_in_buf[JPGD_IN_BUF_SIZE + 128];
    uchar m_in_buf_pad_end[128];
    int m_bits_left;
    uint m_bit_buf;
    int m_restart_interval;
    int m_restarts_left;
    int m_next_restart_num;
    int m_max_mcus_per_row;
    int m_max_blocks_per_mcu;
    int m_expanded_blocks_per_mcu;
    int m_expanded_blocks_per_row;
    int m_expanded_blocks_per_component;
    bool  m_freq_domain_chroma_upsample;
    int m_max_mcus_per_col;
    uint m_last_dc_val[JPGD_MAX_COMPONENTS];
    jpgd_block_t* m_pMCU_coefficients;
    int m_mcu_block_max_zag[JPGD_MAX_BLOCKS_PER_MCU];
    uchar* m_pSample_buf;
    int m_crr[256];
    int m_cbb[256];
    int m_crg[256];
    int m_cbg[256];
    uchar* m_pScan_line_0;
    uchar* m_pScan_line_1;
    jpgd_status m_error_code;
    bool m_ready_flag;
    int m_total_bytes_read;
};

typedef void (*pDecode_block_func)(jpeg_decoder *, int, int, int);

void jd_init( bfs_file pStream, jpeg_decoder* jd );
void jd_deinit( jpeg_decoder* jd );

// Call this method after constructing the object to begin decompression.
// If JPGD_SUCCESS is returned you may then call decode() on each scanline.
int jd_begin_decoding( jpeg_decoder* jd );

// Returns the next scan line.
// For grayscale images, pScan_line will point to a buffer containing 8-bit pixels (get_bytes_per_pixel() will return 1). 
// Otherwise, it will always point to a buffer containing 32-bit RGBA pixels (A will always be 255, and get_bytes_per_pixel() will return 4).
// Returns JPGD_SUCCESS if a scan line has been returned.
// Returns JPGD_DONE if all scan lines have been returned.
// Returns JPGD_FAILED if an error occurred. Call get_error_code() for a more info.
int jd_decode( const void** pScan_line, uint* pScan_line_len, jpeg_decoder* jd );
