#pragma once

typedef struct 
{
    uchar bits[ 16 ];
    uchar hval[ 256 ];
    uchar size[ 256 ];
    uint16 code[ 256 ];
} jpeg_huffman_table_t;

typedef struct 
{
    uchar* ptr;
    bfs_file file;
    const utf8_char* file_name;
    int width;
    int height;
    uchar* data; //ycbcr
    int data_precision;
    int num_components;
    int restart_interval;
    struct 
    {
        int id;
        int h;
        int v;
        int t;
        int td;
        int ta;
    } component_info[ 3 ];
    jpeg_huffman_table_t hac[ 4 ];
    jpeg_huffman_table_t hdc[ 4 ];
    int qtable[ 4 ][ 64 ];
    struct 
    {
        int ss, se;
        int ah, al;
    } scan;
    int dc[ 3 ];
    int curbit;
    uchar curbyte;
} jpeg_file_desc;

int jpeg_readmarkers( jpeg_file_desc* jp );
void jpeg_decompress( jpeg_file_desc* jp );
void jpeg_ycbcr2color( COLORPTR dest, uchar* src, int width, int height );
void jpeg_ycbcr2rgb( uchar* dest, uchar* src, int width, int height );
void jpeg_ycbcr2grayscale( uchar* dest, uchar* src, int width, int height );
