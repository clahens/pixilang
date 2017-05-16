#pragma once

typedef int sample_array_t;

#define JPGE_OUT_BUF_SIZE 2048

typedef void (*tconvert_to_YCC)( uchar* dst, const uchar* src, int num_pixels );
typedef void (*tconvert_to_Y)( uchar* dst, const uchar* src, int num_pixels );

// JPEG chroma subsampling factors. Y_ONLY (grayscale images) and H2V2 (color images) are the most common.
enum subsampling_t 
{
    Y_ONLY = 0, 
    H1V1,
    H2V1,
    H2V2
};

// JPEG compression parameters structure.
struct je_comp_params
{
    // Quality: 1-100, higher is better. Typical values are around 50-95.
    int m_quality;

    // m_subsampling:
    // 0 = Y (grayscale) only
    // 1 = YCbCr, no subsampling (H1V1, YCbCr 1x1x1, 3 blocks per MCU)
    // 2 = YCbCr, H2V1 subsampling (YCbCr 2x1x1, 4 blocks per MCU)
    // 3 = YCbCr, H2V2 subsampling (YCbCr 4x1x1, 6 blocks per MCU-- very common)
    subsampling_t m_subsampling;

    // Disables CbCr discrimination - only intended for testing.
    // If true, the Y quantization table is also used for the CbCr channels.
    bool m_no_chroma_discrim_flag;

    bool m_two_pass_flag;
    
    // Line handlers:
    tconvert_to_YCC convert_to_YCC;
    tconvert_to_Y convert_to_Y;
};

void init_je_comp_params( je_comp_params* jp );
bool check_je_comp_params( je_comp_params* jp );

struct jpeg_encoder
{
    je_comp_params m_params;
    uchar m_num_components;
    uchar m_comp_h_samp[ 3 ], m_comp_v_samp[ 3 ];
    int m_image_x, m_image_y, m_image_bpl;
    int m_image_x_mcu, m_image_y_mcu;
    int m_image_bpl_xlt, m_image_bpl_mcu;
    int m_mcus_per_row;
    int m_mcu_x, m_mcu_y;
    uchar* m_mcu_lines[ 16 ];
    uchar m_mcu_y_ofs;
    sample_array_t m_sample_array[ 64 ];
    int16 m_coefficient_array[ 64 ];
    int m_quantization_tables[ 2 ][ 64 ];
    uint m_huff_codes[ 4 ][ 256 ];
    uchar m_huff_code_sizes[ 4 ][ 256 ];
    uchar m_huff_bits[ 4 ][ 17 ];
    uchar m_huff_val[ 4 ][ 256 ];
    uint m_huff_count[ 4 ][ 256 ];
    int m_last_dc_val[ 3 ];
    uchar m_out_buf[ JPGE_OUT_BUF_SIZE ];
    uchar* m_pOut_buf;
    uint m_out_buf_left;
    uint m_bit_buffer;
    uint m_bits_in;
    uchar m_pass_num;
    bool m_all_stream_writes_succeeded;  
    bfs_file f;
};

// Initializes the compressor.
void je_init( jpeg_encoder* je );

// Set compression parameters.
// f: The stream object to use for writing compressed data.
// params - Compression parameters structure, defined above.
// width, height  - Image dimensions.
// channels - May be 1, or 3. 1 indicates grayscale, 3 indicates RGB source data.
// Returns false on out of memory or if a stream write fails.
bool je_set_params( bfs_file f, int width, int height, int src_channels, je_comp_params* comp_params, jpeg_encoder* je );

// Deinitializes the compressor, freeing any allocated memory. May be called at any time.
void je_deinit( jpeg_encoder* je );

// Call this method with each source scanline.
// width * src_channels bytes per scanline is expected (RGB or Y format).
// You must call with NULL after all scanlines are processed to finish compression.
// Returns false on out of memory or if a stream write fails.
bool je_process_scanline( const void* pScanline, jpeg_encoder* je );
