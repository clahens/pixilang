uchar g_sign_jpeg1[] = { 'I', 'F', '.' };
uchar g_sign_jpeg2[] = { 'i', 'f', '.' };
uchar g_sign_jpeg3[] = { 'I', 'F', 'F', '.' };
uchar g_sign_jpeg4[] = { 0xFF, 0xD8, 0xFF };
uchar g_sign_gif1[] = { 0x47, 0x49, 0x46, 0x38, 0x37, 0x61 };
uchar g_sign_gif2[] = { 0x47, 0x49, 0x46, 0x38, 0x39, 0x61 };
uchar g_sign_png[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
uchar g_sign_riff[] = { 'R', 'I', 'F', 'F' };
uchar g_sign_wave[] = { 'W', 'A', 'V', 'E' };
uchar g_sign_avi[] = { 'A', 'V', 'I', ' ' };
uchar g_sign_mp41[] = { 0x00, 0x00, 0x00, 0x00, 0x66, 0x74, 0x79, 0x70 };
uchar g_sign_mp42[] = { 0x33, 0x67, 0x70 }; //3gp
uchar g_sign_mp43[] = { 0x69, 0x73, 0x6F, 0x6D }; //ISO Base Media file (MPEG-4) v1
uchar g_sign_mp44[] = { 0x33, 0x67, 0x70, 0x35 }; //3gp5
uchar g_sign_mp45[] = { 0x6D, 0x70, 0x34, 0x32 }; //MPEG-4 video/QuickTime file
uchar g_sign_mp46[] = { 0x4D, 0x53, 0x4E, 0x56 }; //MSNV
uchar g_sign_aiff[] = { 'F', 'O', 'R', 'M' };
uchar g_sign_aiff_type0[] = { 'A', 'I', 'F', 'F' };
uchar g_sign_aiff_type1[] = { 'A', 'I', 'F', 'C' };
uchar g_sign_mp31[] = { 0xFF, 0xFB };
uchar g_sign_mp32[] = { 0x49, 0x44, 0x33 };
uchar g_sign_flac[] = { 0x66, 0x4C, 0x61, 0x43 };
uchar g_sign_ogg[] = { 0x4F, 0x67, 0x67, 0x53 };
uchar g_sign_midi[] = { 0x4D, 0x54, 0x68, 0x64 };
uchar g_sign_zip1[] = { 0x50, 0x4B, 0x03, 0x04, 0x50, 0x4B, 0x05, 0x06 };
uchar g_sign_zip2[] = { 0x50, 0x4B, 0x07, 0x08 };
uchar g_sign_pixicont[] = { 'p', 'i', 'x', 'i', 'C', 'O', 'N', 'T' };

bfs_file_type bfs_get_file_type( const utf8_char* filename, bfs_file f )
{
    bfs_file_type type = BFS_FILE_TYPE_UNKNOWN;
    bfs_file_type uncertain_type = BFS_FILE_TYPE_UNKNOWN;
    
    while( 1 )
    {
	if( filename && f == 0 ) f = bfs_open( filename, "rb" );
	if( f == 0 ) break;
	
	bfs_rewind( f );
	uchar sign[ 32 ];
	uchar temp;
	bmem_set( sign, sizeof( sign ), 0 );
	bfs_read( sign, 1, 32, f );
	bfs_rewind( f );
	while( 1 )
	{
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_jpeg1, sizeof( g_sign_jpeg1 ) ) == 0 ) { type = BFS_FILE_TYPE_JPEG; break; }
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_jpeg2, sizeof( g_sign_jpeg2 ) ) == 0 ) { type = BFS_FILE_TYPE_JPEG; break; }
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_jpeg3, sizeof( g_sign_jpeg3 ) ) == 0 ) { type = BFS_FILE_TYPE_JPEG; break; }
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_jpeg4, sizeof( g_sign_jpeg4 ) ) == 0 ) { type = BFS_FILE_TYPE_JPEG; break; }
	    
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_gif1, sizeof( g_sign_gif1 ) ) == 0 ) { type = BFS_FILE_TYPE_GIF; break; }
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_gif2, sizeof( g_sign_gif2 ) ) == 0 ) { type = BFS_FILE_TYPE_GIF; break; }
	    
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_png, sizeof( g_sign_png ) ) == 0 ) { type = BFS_FILE_TYPE_PNG; break; }
	    
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_riff, sizeof( g_sign_riff ) ) == 0 )
	    {
		if( bmem_cmp( (const char*)&sign[ 8 ], (const char*)g_sign_wave, sizeof( g_sign_wave ) ) == 0 ) { type = BFS_FILE_TYPE_WAVE; break; }
		if( bmem_cmp( (const char*)&sign[ 8 ], (const char*)g_sign_avi, sizeof( g_sign_avi ) ) == 0 ) { type = BFS_FILE_TYPE_AVI; break; }
	    }

	    temp = sign[ 3 ]; sign[ 3 ] = 0;
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_mp41, sizeof( g_sign_mp41 ) ) == 0 )
	    {
		if( bmem_cmp( (const char*)&sign[ 8 ], (const char*)g_sign_mp42, sizeof( g_sign_mp42 ) ) == 0 ) { type = BFS_FILE_TYPE_MP4; break; }
		if( bmem_cmp( (const char*)&sign[ 8 ], (const char*)g_sign_mp43, sizeof( g_sign_mp43 ) ) == 0 ) { type = BFS_FILE_TYPE_MP4; break; }
		if( bmem_cmp( (const char*)&sign[ 8 ], (const char*)g_sign_mp44, sizeof( g_sign_mp44 ) ) == 0 ) { type = BFS_FILE_TYPE_MP4; break; }
		if( bmem_cmp( (const char*)&sign[ 8 ], (const char*)g_sign_mp45, sizeof( g_sign_mp45 ) ) == 0 ) { type = BFS_FILE_TYPE_MP4; break; }
		if( bmem_cmp( (const char*)&sign[ 8 ], (const char*)g_sign_mp46, sizeof( g_sign_mp46 ) ) == 0 ) { type = BFS_FILE_TYPE_MP4; break; }
		uncertain_type = BFS_FILE_TYPE_MP4;
	    }
	    sign[ 3 ] = temp;
	    
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_aiff, sizeof( g_sign_aiff ) ) == 0 &&
		( bmem_cmp( (const char*)&sign[ 8 ], (const char*)g_sign_aiff_type0, sizeof( g_sign_aiff_type0 ) ) == 0 ||
		  bmem_cmp( (const char*)&sign[ 8 ], (const char*)g_sign_aiff_type1, sizeof( g_sign_aiff_type1 ) ) == 0 ) ) { type = BFS_FILE_TYPE_AIFF; break; }
		
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_mp31, sizeof( g_sign_mp31 ) ) == 0 ) { type = BFS_FILE_TYPE_MP3; break; }
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_mp32, sizeof( g_sign_mp32 ) ) == 0 ) { type = BFS_FILE_TYPE_MP3; break; }
		
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_flac, sizeof( g_sign_flac ) ) == 0 ) { type = BFS_FILE_TYPE_FLAC; break; }
		
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_ogg, sizeof( g_sign_ogg ) ) == 0 ) { type = BFS_FILE_TYPE_OGG; break; }
		
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_midi, sizeof( g_sign_midi ) ) == 0 ) { type = BFS_FILE_TYPE_MIDI; break; }
		
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_zip1, sizeof( g_sign_zip1 ) ) == 0 ) { type = BFS_FILE_TYPE_ZIP; break; }
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_zip2, sizeof( g_sign_zip2 ) ) == 0 ) { type = BFS_FILE_TYPE_ZIP; break; }
		
	    if( bmem_cmp( (const char*)sign, (const char*)g_sign_pixicont, sizeof( g_sign_pixicont ) ) == 0 ) { type = BFS_FILE_TYPE_PIXICONTAINER; break; }
		
	    break;
	}
	
	if( filename && f )
	{
	    bfs_close( f );
	}   
	
	break;
    }
    
    if( type == BFS_FILE_TYPE_UNKNOWN && uncertain_type != BFS_FILE_TYPE_UNKNOWN )
	type = uncertain_type;
    
    return type;
}

const utf8_char* bfs_get_mime_type( bfs_file_type type )
{
    const utf8_char* rv = 0;
    switch( type )
    {
	case BFS_FILE_TYPE_WAVE: rv = "audio/vnd.wave"; break;
	case BFS_FILE_TYPE_AIFF: rv = "audio/x-aiff"; break;
	case BFS_FILE_TYPE_OGG: rv = "audio/ogg"; break;
	case BFS_FILE_TYPE_MP3: rv = "audio/mpeg"; break;
	case BFS_FILE_TYPE_FLAC: rv = "audio/ogg"; break;
	case BFS_FILE_TYPE_MIDI: rv = "audio/midi"; break;
	case BFS_FILE_TYPE_JPEG: rv = "image/jpeg"; break;
	case BFS_FILE_TYPE_PNG: rv = "image/png"; break;
	case BFS_FILE_TYPE_GIF: rv = "image/gif"; break;
	case BFS_FILE_TYPE_AVI: rv = "video/avi"; break;
	case BFS_FILE_TYPE_MP4: rv = "video/mp4"; break;
	case BFS_FILE_TYPE_ZIP: rv = "application/zip"; break;
	default: rv = "application/octet-stream"; break;
    }
    return rv;
}

