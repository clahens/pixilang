/*
    webserver.cpp.
    Copyright (C) 2009 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

#include "core/core.h"
#include "webserver.h"

#include <sys/socket.h> //socket definitions
#include <sys/types.h> //socket types
#include <arpa/inet.h> //inet (3) funtions
#include <unistd.h> //misc. UNIX functions
#include <dirent.h> //working with directories
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

//###########################################################
//###########################################################
//## Web Server with File Browser
//###########################################################
//###########################################################

extern void iphone_sundog_get_host_info( void );
extern char* g_iphone_host_addr;
extern char* g_iphone_host_name;

#define MAX_REQ_LINE (1024)

const utf8_char* g_webserver_name = "SunDog webserver " __DATE__;

char g_webserver_root[ MAX_DIR_LEN ] = { '.', 0 };
int g_webserver_port = 8080;
pthread_t g_webserver_pth;
const utf8_char* g_webserver_status = 0;
volatile int g_webserver_finished = 0;
volatile int g_webserver_exit_request = 0;
volatile int g_webserver_pth_work = 0;
volatile bool g_webserver_window_closed = 0;
WINDOWPTR g_webserver = 0;

enum req_method { GET, HEAD, POST, UNSUPPORTED };
enum res_type	{ RES_FILE, RES_DIR };

struct req_info 
{
    enum req_method method;
    char* referer;
    char* useragent;
    char* resource;
    char* resource_name; //without first part of the address
    char* resource_parameters;
    char res_type;
    int post_size;
    char* post_boundary;
    int root;
    int status;
};

enum
{
    STR_WEBSERV_IMPORT_EXPORT,
    STR_WEBSERV_ERROR_CONN,
    STR_WEBSERV_STATUS_STARTING,
    STR_WEBSERV_STATUS_WORKING,
    STR_WEBSERV_HEADER,
    STR_WEBSERV_TITLE,
    STR_WEBSERV_REMOVE,
    STR_WEBSERV_REMOVE2,
    STR_WEBSERV_YES,
    STR_WEBSERV_NO,
    STR_WEBSERV_HOWTO,
    STR_WEBSERV_SEND,
    STR_WEBSERV_CREATE_DIR,
    STR_WEBSERV_HOME,
    STR_WEBSERV_PARENT,
};

const utf8_char* webserv_get_string( int str_id )
{
    const utf8_char* str = 0;
    const utf8_char* lang = blocale_get_lang();
    while( 1 )
    {
        if( bmem_strstr( lang, "ru_" ) )
        {
            switch( str_id )
            {
		case STR_WEBSERV_IMPORT_EXPORT: str = "Wi-Fi Экспорт/Импорт"; break;
		case STR_WEBSERV_ERROR_CONN: str = "ОШИБКА:\nнет подключения к сети"; break;
		case STR_WEBSERV_STATUS_STARTING: str = "Запуск вебсервера ..."; break;
		case STR_WEBSERV_STATUS_WORKING: str = "Активен ..."; break;
		case STR_WEBSERV_HEADER: str = "<html><head><title>Хранилище файлов</title></head><body>\r\n"; break;
    		case STR_WEBSERV_TITLE: str = "<h1>Хранилище файлов программы: "; break;
    		case STR_WEBSERV_REMOVE: str = "<center><h1>Удалить \""; break;
		case STR_WEBSERV_REMOVE2: str = "\"><font color=\"red\">(удалить)</font></a>"; break;
		case STR_WEBSERV_YES: str = "\"><font color=\"RED\">ДА</font></a> "; break;
		case STR_WEBSERV_NO: str = "\"><font color=\"RED\">НЕТ</font></a>"; break;
		case STR_WEBSERV_HOWTO: str = "<p>Укажите один или несколько файлов, которые необходимо загрузить в хранилище:<br>\r\n"; break;
		case STR_WEBSERV_SEND: str = "<input type=\"submit\" value=\"Загрузить\">\r\n"; break;
		case STR_WEBSERV_CREATE_DIR: str = "<input type=\"submit\" value=\"Создать папку\">\r\n"; break;
		case STR_WEBSERV_HOME: str = "<tr><td><a href=\"/\"><b>.. [корневой каталог (основная папка)]</b></a>\r\n"; break;
		case STR_WEBSERV_PARENT: str = "<tr><td><a href=\"../\"><b>.. [предыдущая папка]</b></a>\r\n"; break;
            }
            if( str ) break;
        }
        //Default:
        switch( str_id )
        {
	    case STR_WEBSERV_IMPORT_EXPORT: str = "Wi-Fi Export/Import"; break;
	    case STR_WEBSERV_ERROR_CONN: str = "ERROR:\nno network connection detected"; break;
	    case STR_WEBSERV_STATUS_STARTING: str = "Starting webserver ..."; break;
	    case STR_WEBSERV_STATUS_WORKING: str = "Working ..."; break;
    	    case STR_WEBSERV_HEADER: str = "<html><head><title>File storage</title></head><body>\r\n"; break;
    	    case STR_WEBSERV_TITLE: str = "<h1>File storage: "; break;
	    case STR_WEBSERV_REMOVE: str = "<center><h1>Remove \""; break;
	    case STR_WEBSERV_REMOVE2: str = "\"><font color=\"red\">(remove)</font></a>"; break;
	    case STR_WEBSERV_YES: str = "\"><font color=\"RED\">YES</font></a> "; break;
	    case STR_WEBSERV_NO: str = "\"><font color=\"RED\">NO</font></a>"; break;
	    case STR_WEBSERV_HOWTO: str = "<p>Please specify a file, or a set of files for upload on your device:<br>\r\n"; break;
	    case STR_WEBSERV_SEND: str = "<input type=\"submit\" value=\"Send\">\r\n"; break;
	    case STR_WEBSERV_CREATE_DIR: str = "<input type=\"submit\" value=\"Create directory\">\r\n"; break;
	    case STR_WEBSERV_HOME: str = "<tr><td><a href=\"/\"><b>.. [home]</b></a>\r\n"; break;
	    case STR_WEBSERV_PARENT: str = "<tr><td><a href=\"../\"><b>.. [parent directory]</b></a>\r\n"; break;
        }
        break;
    }
    return str;
}

//-----------------------
//Buffered sockets. Begin
//-----------------------

#define BS_BUF_SIZE (4096)

struct bs_data
{
    int s; //socket
    uchar buf[ BS_BUF_SIZE ]; //buffer
    int64 buf_size;
    int buf_p;
    int p; //global ptr
    int size; //global size
    int eof;
    int err;
};

bs_data* bs_open( int s, int size )
{
    bs_data* bs = (bs_data*)bmem_new( sizeof( bs_data ) );
    bmem_set( bs, sizeof( bs_data ), 0 );
    bs->s = s;
    bs->size = size;
    return bs;
}

void bs_close( bs_data* bs )
{
    if( bs )
	bmem_free( bs );
}

int64 bs_next_buf( bs_data* bs )
{
    int size = BS_BUF_SIZE;
    if( bs->p + size > bs->size )
	size = bs->size - bs->p;
    bs->buf_p = 0;
    bs->buf_size = read( bs->s, bs->buf, size );
    if( bs->buf_size <= 0 )
	bs->err = 1;
    return bs->buf_size;
}

int bs_getc( bs_data* bs )
{
    if( bs->p >= bs->size )
    {
	bs->eof = 1;
	return -1;
    }
    if( bs->buf_p >= bs->buf_size )
    {
	int64 res = bs_next_buf( bs );
	if( res <= 0 )
	{
	    return -1;
	}
    }
    int rv = bs->buf[ bs->buf_p ];
    bs->buf_p++;
    bs->p++;
    return rv;
}

//Read a line from a buffered socket
ssize_t bs_read_line( bs_data* bs, void *vptr, size_t maxlen ) 
{
    ssize_t n;
    int c;
    uchar* buffer;

    buffer = (uchar*)vptr;

    for( n = 1; n < maxlen; n++ ) 
    {
	c = bs_getc( bs );
	if( c >= 0 ) 
	{
	    *buffer++ = (uchar)c;
	    if( c == '\n' )
		break;
	}
	else
	{
	    g_webserver_status = "ERROR: Error in bs_read_line()";
	    printf( "%s\n", g_webserver_status );
	    n = 0;
	    break;
	}
    }

    *buffer = 0;
    return n;
}

//-----------------------
//Buffered sockets. End
//-----------------------

//Read a line from a socket
ssize_t read_line( int sockd, void *vptr, size_t maxlen ) 
{
    ssize_t n, rc;
    char c, *buffer;

    buffer = (char*)vptr;

    for( n = 1; n < maxlen; n++ ) 
    {
	if( ( rc = read( sockd, &c, 1 ) ) == 1 ) 
	{
	    *buffer++ = c;
	    if( c == '\n' )
		break;
	}
	else if( rc == 0 ) 
	{
	    if( n == 1 )
		return 0;
	    else
		break;
	}
	else
	{
	    if( errno == EINTR )
		continue;
	    g_webserver_status = "ERROR: Error in Readline()";
	    printf( "%s\n", g_webserver_status );
	    break;
	}
    }

    *buffer = 0;
    return n;
}

//Write a line to a socket
ssize_t write_line( int sockd, const void *vptr, size_t n ) 
{
    size_t nleft;
    ssize_t nwritten;
    const char *buffer;

    buffer = (const char*)vptr;
    nleft  = n;
    if( nleft == 0 ) nleft = strlen( (const char*)vptr );

    while( nleft > 0 ) 
    {
	if( ( nwritten = write( sockd, buffer, nleft ) ) <= 0 ) 
	{
	    if ( errno == EINTR )
		nwritten = 0;
	    else
	    {
		g_webserver_status = "ERROR: Error in Writeline()";
		printf( "%s\n", g_webserver_status );
		break;
	    }
	}
	nleft -= nwritten;
	buffer += nwritten;
    }

    return n;
}

//Removes trailing whitespace from a string
int trim( char* buffer ) 
{
    int64 n = strlen( buffer ) - 1;

    //while( !isalnum( buffer[ n ] ) && n >= 0 )
    while( n >= 0 && (uchar)buffer[ n ] <= 0x20 )
	buffer[ n-- ] = '\0';

    return 0;
}

//Converts a string to upper-case
int str_upper( char* buffer ) 
{
    while( *buffer ) 
    {
	*buffer = toupper( *buffer );
	++buffer;
    }
    return 0;
}

//Cleans up url-encoded string
void clean_URL( char* buffer ) 
{
    char asciinum[ 3 ] = { 0 };
    int i = 0, c;
    
    while( buffer[ i ] ) 
    {
	if( buffer[ i ] == '+' )
	    buffer[ i ] = ' ';
	else if( buffer[ i ] == '%' ) 
	{
	    asciinum[ 0 ] = buffer[ i + 1 ];
	    asciinum[ 1 ] = buffer[ i + 2 ];
	    buffer[ i ] = strtol( asciinum, NULL, 16 );
	    c = i + 1;
	    do {
		buffer[ c ] = buffer[ c + 2 ];
	    } while( buffer[ 2 + ( c++ ) ] );
	}
	++i;
    }
}

//Initialises a request information structure
void init_req_info( req_info* reqinfo ) 
{
    reqinfo->useragent = NULL;
    reqinfo->referer = NULL;
    reqinfo->resource = NULL;
    reqinfo->resource_name = NULL;
    reqinfo->resource_parameters = NULL;
    reqinfo->res_type = RES_FILE;
    reqinfo->post_size = 0;
    reqinfo->post_boundary = 0;
    reqinfo->method = UNSUPPORTED;
    reqinfo->root = 0;
    reqinfo->status = 200;
}

//Frees memory allocated for a request information structure
void free_req_info( req_info* reqinfo ) 
{
    if( reqinfo->useragent )
	bmem_free( reqinfo->useragent );
    if( reqinfo->referer )
	bmem_free( reqinfo->referer );
    if( reqinfo->resource )
	bmem_free( reqinfo->resource );
    if( reqinfo->resource_name )
	bmem_free( reqinfo->resource_name );
    if( reqinfo->resource_parameters )
	bmem_free( reqinfo->resource_parameters );
    if( reqinfo->post_boundary )
	bmem_free( reqinfo->post_boundary );
}

int parse_HTTP_header( int line_num, char* buffer, req_info* reqinfo ) 
{
    char* endptr;
    int64 len;

    if( line_num == 0 ) 
    {
	//Get the request method, which is case-sensitive. This
	//version of the server only supports the GET and HEAD
	//request methods.

	if( !strncmp( buffer, "GET ", 4 ) ) 
	{
	    reqinfo->method = GET;
	    buffer += 4;
	}
	else if( !strncmp( buffer, "HEAD ", 5 ) ) 
	{
	    reqinfo->method = HEAD;
	    buffer += 5;
	}
	else if( !strncmp( buffer, "POST ", 5 ) ) 
	{
	    reqinfo->method = POST;
	    buffer += 5;
	}
	else 
	{
	    reqinfo->method = UNSUPPORTED;
	    reqinfo->status = 501;
	    return -1;
	}

	//Skip to start of resource:
	while( *buffer && isspace( *buffer ) )
	    buffer++;

	//Get resource name...
	endptr = strchr( buffer, ' ' );
	if( endptr != NULL )
	{
	    *endptr = 0;
	}
	clean_URL( buffer );
	{
	    //check for parameters in the filename:
	    int p = 0;
	    while( 1 )
	    {
		if( buffer[ p ] == '?' )
		{
		    buffer[ p ] = 0;
		    int64 pars_len = strlen( &buffer[ p + 1 ] );
		    if( pars_len > 0 )
		    {
			reqinfo->resource_parameters = (char*)bmem_new( pars_len + 1 );
			bmem_copy( reqinfo->resource_parameters, &buffer[ p + 1 ], pars_len + 1 );
			break;
		    }
		}
		if( buffer[ p ] == 0 ) break;
		p++;
	    }
	}
	len = strlen( buffer );
	if( len == 1 && buffer[ 0 ] == '/' )
	    reqinfo->root = 1;

	//...and store it in the request information structure:
	int64 root_len = strlen( g_webserver_root );
	reqinfo->resource = (char*)bmem_new( root_len + len + 1 );
	reqinfo->resource_name = (char*)bmem_new( len + 1 );
	memcpy( reqinfo->resource, g_webserver_root, root_len + 1 );
	strcat( reqinfo->resource, buffer );
	memcpy( reqinfo->resource_name, buffer, len + 1 );
    }
    else
    {
	if( !strncmp( buffer, "Content-Length: ", 16 ) ) 
	{
	    buffer += 16;
	    reqinfo->post_size = string_to_int( buffer );
	}
	else if( !strncmp( buffer, "Content-Type: ", 14 ) ) 
	{
	    buffer += 14;
	    //Get boundary...
	    char* bp = strstr( buffer, "boundary=" );
	    if( bp != NULL )
	    {
		bp += 9;
		char* bp2 = strchr( bp, ' ' );
		if( bp2 != NULL )
		{
		    *bp2 = 0;
		}
		len = strlen( bp );
		reqinfo->post_boundary = (char*)bmem_new( len + 1 );
		memcpy( reqinfo->post_boundary, bp, len + 1 );
	    }
	}
    }
	
    return 0;
}

/*
    Gets request headers. A CRLF terminates a HTTP header line,
    but if one is never sent we would wait forever. Therefore,
    we use select() to set a maximum length of time we will
    wait for the next complete header. If we timeout before
    this is received, we terminate the connection.
*/
int get_request( int conn, req_info* reqinfo ) 
{
    char buffer[ MAX_REQ_LINE ] = { 0 };
    int rval;
    fd_set fds;
    struct timeval tv;

    //Set timeout to 5 seconds:
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    int line_num = 0;
    while( 1 )
    {
	//Reset file descriptor set:
	FD_ZERO( &fds );
	FD_SET( conn, &fds );

	//Wait until the timeout to see if input is ready:
	rval = select( conn + 1, &fds, NULL, NULL, &tv );

	//Take appropriate action based on return from select():
	if( rval < 0 ) 
	{
	    g_webserver_status = "ERROR: Error calling select() in get_request()";
	    printf( "%s\n", g_webserver_status );
	    return -2;
	}
	else 
	if( rval == 0 ) 
	{
	    //Input not ready after timeout:
	    return -1;
	}
	else 
	{
	    //We have an input line waiting, so retrieve it:
	    read_line( conn, buffer, MAX_REQ_LINE - 1 );
	    trim( buffer );

	    if( buffer[ 0 ] == '\0' )
		break;

	    if( parse_HTTP_header( line_num, buffer, reqinfo ) )
		break;

	    line_num++;
	}
    }
    
    return 0;
}

//Returns a resource
int return_resource( int conn, int resource, req_info* reqinfo ) 
{
    int rv = 0;
    void* buf = bmem_new( 32000 );

    while( 1 )
    {
	int64 size = read( resource, buf, 32000 );
	if( size < 0 )
	{
	    g_webserver_status = "ERROR: Error reading from file.";
	    printf( "%s\n", g_webserver_status );
	    rv = -1;
	    break;
	}
	if( size == 0 ) break;
	if( write( conn, buf, size ) < size )
	{
	    g_webserver_status = "ERROR: Error sending file.";
	    printf( "%s\n", g_webserver_status );
	    rv = -2;
	    break;
	}	
    }
    
    bmem_free( buf );
    return rv;
}

//Returns an error message
int return_error_msg( int conn, req_info* reqinfo ) 
{
    char buffer[ 100 ];

    sprintf( 
	buffer, 
	"<HTML>\n<HEAD>\n<TITLE>Server Error %d</TITLE>\n"
	"</HEAD>\n\n", reqinfo->status 
    );
    write_line( conn, buffer, strlen( buffer ) );

    sprintf(
	buffer,
	"<BODY>\n<H1>Server Error %d</H1>\n", 
	reqinfo->status
    );
    write_line( conn, buffer, strlen( buffer ) );

    sprintf( 
	buffer, 
	"<P>The request could not be completed.</P>\n"
        "</BODY>\n</HTML>\n"
    );
    write_line( conn, buffer, strlen( buffer ) );

    return 0;
}

//Outputs HTTP response headers
int output_HTTP_headers( int conn, req_info* reqinfo ) 
{
    char buffer[ 100 ];

    sprintf( buffer, "HTTP/1.0 %d OK\r\n", reqinfo->status );
    write_line( conn, buffer, strlen( buffer ) );

    utf8_char ts[ 256 ];
    sprintf( ts, "Server: %s\r\n", g_webserver_name );
    write_line( conn, ts, 0 );
    int html = 1;
    int64 len = strlen( reqinfo->resource );
    if( len > 5 &&
	reqinfo->resource[ len - 1 ] == 'm' &&
	reqinfo->resource[ len - 2 ] == 't' &&
	reqinfo->resource[ len - 3 ] == 'h' )
    {
	html = 1;
    }
    else
    {
	if( reqinfo->res_type == RES_FILE )
	    html = 0;
    }
    if( html )
	write_line( conn, "Content-Type: text/html; charset=utf-8\r\n", 40 );
    else
	write_line( conn, "Content-Type: application/octet-stream\r\n", 40 );
    write_line( conn, "\r\n", 2 );

    return 0;
}

//Service an HTTP request
int service_request( int conn ) 
{
    int rv = 0;
    req_info reqinfo;
    int res = -1;
    DIR* res_dir = 0;

    init_req_info( &reqinfo );
    
    //Get HTTP request:
    if( get_request( conn, &reqinfo ) < 0 )
    {
	rv = -1;
	goto service_end;
    }

    if( reqinfo.method == POST )
    {
	bs_data* bs;
	bs = bs_open( conn, reqinfo.post_size );
	if( bs )
	{
	    //0 - data;
	    //1 - boundary.
	    int status = 0;
	    
	    int64 bound_size = bmem_strlen( reqinfo.post_boundary ) + 4;
	    uchar* bound = (uchar*)bmem_new( bound_size );
	    bound[ 0 ] = '\r';
	    bound[ 1 ] = '\n';
	    bound[ 2 ] = '-';
	    bound[ 3 ] = '-';
	    bmem_copy( bound + 4, reqinfo.post_boundary, bound_size - 4 );
	    int bound_p = 2;
	    
	    uchar* temp_buf = (uchar*)bmem_new( bound_size );
	    char* temp_str_buf = (char*)bmem_new( MAX_REQ_LINE );
	    
	    char* filename = 0;
	    bfs_file fd = 0;
	    
	    while( bs->eof == 0 && bs->err == 0 )
	    {
		int c = bs_getc( bs );
		if( c < 0 ) break;
		//if( status < 2 )
		{
		    //data or boundary
		    if( c == (uchar)bound[ bound_p ] )
		    {
			status = 1;
			temp_buf[ bound_p ] = (uchar)c;
			bound_p++;
			if( bound_p == bound_size )
			{
			    //Boundary found.

			    //Skip CRLF:
			    int c1 = bs_getc( bs ); 
			    int c2 = bs_getc( bs );
			    if( c1 == '-' && c2 == '-' ) break;

			    //Close previous files:
			    if( fd ) bfs_close( fd );
			    fd = 0;
			
			    //Get fields:
			    char* temp_str = temp_str_buf;
			    temp_str[ 0 ] = 0;
			    while( 1 )
			    {
				bs_read_line( bs, temp_str, MAX_REQ_LINE - 1 );
				trim( temp_str );
				
				if( temp_str[ 0 ] == '\0' )
				    break;
				    
				if( !strncmp( temp_str, "Content-Disposition: ", 21 ) ) 
				{
				    temp_str += 21;
				    char* name_ptr = strstr( temp_str, "name=\"" );
				    if( name_ptr != NULL )
				    {
					name_ptr += 6;
					char* np = name_ptr;
					while( 1 ) 
					{
					    if( *np == '"' )
					    {
						temp_str = np + 2;
						*np = 0;
					    }
					    if( *np == 0 ) break;
					    np++;
					}
				    }
				    char* filename_ptr = strstr( temp_str, "filename=\"" );
				    if( filename_ptr != NULL )
				    {
					filename_ptr += 10;
					char* np = filename_ptr;
					while( 1 ) 
					{
					    if( *np == '"' )
					    {
						*np = 0;
					    }
					    if( *np == 0 ) break;
					    np++;
					}
					int64 root_len = strlen( reqinfo.resource );
					int64 filename_len = strlen( filename_ptr );
					if( filename_len > 2 )
					{
					    //Clean filename:
					    for( int64 i = filename_len - 1; i >= 0; i-- )
					    {
						if( filename_ptr[ i ] == 0x5C )
						{
						    filename_ptr = filename_ptr + i + 1;
						    filename_len = strlen( filename_ptr );
						    break;
						}
					    }
					}
					if( filename ) bmem_free( filename );
					filename = (char*)bmem_new( root_len + filename_len + 1 );
					filename[ 0 ] = 0;
					bmem_strcat_resize( filename, reqinfo.resource );
					bmem_strcat_resize( filename, filename_ptr );
					fd = bfs_open( filename, "wb" );
				    }
				}
			    }
			    
			    status = 0;
			    bound_p = 0;
			}
		    }
		    else 
		    {
			if( status == 1 )
			{
			    //No boundary.
			    //We need to get last parsed bytes from the temp_buf[ 0 ... ( bound_p - 1 ) ].
			    if( fd )
			    {
				bfs_write( temp_buf, 1, bound_p, fd );
			    }
			    status = 0;
			    bound_p = 0;
			}
			if( fd )
			{
			    bfs_write( &c, 1, 1, fd );
			}
		    }
		}
	    }
	    
	    if( bound ) bmem_free( bound );
	    if( temp_buf ) bmem_free( temp_buf );
	    if( temp_str_buf ) bmem_free( temp_str_buf );
	    if( filename ) bmem_free( filename );
	    if( fd ) bfs_close( fd );
	    
	    bs_close( bs );
	}
    }    

    if( reqinfo.status == 200 )
    {
        res_dir = opendir( reqinfo.resource );
        if( res_dir == 0 )
        {
    	    if( errno == EACCES )
		reqinfo.status = 401;
	    res = open( reqinfo.resource, O_RDONLY );
	    if( res < 0 )
	    {
	        if( errno == EACCES )
	            reqinfo.status = 401;
	    }
	}
    }
    if( reqinfo.status == 200 )
    {
        if( res == -1 && res_dir == 0 )
        {
    	    reqinfo.status = 404;
	}
	else
	{
	    if( res >= 0 ) reqinfo.res_type = RES_FILE;
	    if( res_dir ) reqinfo.res_type = RES_DIR;
	}
    }
    
    //Service the HTTP request:
    if( reqinfo.status == 200 ) 
    {
	output_HTTP_headers( conn, &reqinfo );

	int answer = 0;

	if( reqinfo.resource_parameters )
	{
	    if( reqinfo.resource_parameters[ 0 ] == 'r' )
	    {
		//Remove?
		write_line( conn, webserv_get_string( STR_WEBSERV_HEADER ), 0 );
		write_line( conn, webserv_get_string( STR_WEBSERV_REMOVE ), 0 );
		write_line( conn, reqinfo.resource_parameters + 1, 0 );
		write_line( conn, "\"?<br>", 0 );
		write_line( conn, "<a href=\"", 0 );
		write_line( conn, reqinfo.resource_name, 0 );
		write_line( conn, "?", 0 );
		reqinfo.resource_parameters[ 0 ] = 'R';
		write_line( conn, reqinfo.resource_parameters, 0 );
		write_line( conn, webserv_get_string( STR_WEBSERV_YES ), 0 );
		write_line( conn, "<a href=\"", 0 );
		write_line( conn, reqinfo.resource_name, 0 );
		write_line( conn, webserv_get_string( STR_WEBSERV_NO ), 0 );
		write_line( conn, "</h1></center>", 0 );
		write_line( conn, "</body></html>\r\n", 0 );
		answer = 1;
	    }
            else
	    if( reqinfo.resource_parameters[ 0 ] == 'R' )
	    {
		//Remove :(
		int64 root_len = bmem_strlen( reqinfo.resource );
		int64 rem_len = bmem_strlen( reqinfo.resource_parameters + 1 );
		char* rem_name = (char*)bmem_new( root_len + rem_len + 1 );
		bmem_copy( rem_name, reqinfo.resource, root_len + 1 );
		bmem_strcat_resize( rem_name, reqinfo.resource_parameters + 1 );
		bfs_remove( rem_name );
		bmem_free( rem_name );
	    }
            if( reqinfo.resource_parameters[ 0 ] == 'd' )
	    {
                //Create directory:
                int64 root_len = bmem_strlen( reqinfo.resource );
		int64 dir_len = bmem_strlen( reqinfo.resource_parameters + 2 );
		char* dir_name = (char*)bmem_new( root_len + dir_len + 2 );
		bmem_copy( dir_name, reqinfo.resource, root_len + 2 );
		bmem_strcat_resize( dir_name, reqinfo.resource_parameters + 2 );
		bfs_mkdir( dir_name, S_IRWXU | S_IRWXG | S_IRWXO );
		bmem_free( dir_name );
            }
	}
	
	if( answer )
	    goto service_end;
	
	if( res >= 0 )
	{
	    if( return_resource( conn, res, &reqinfo ) )
	    {
		g_webserver_status = "ERROR: Something wrong returning resource.";
		printf( "%s\n", g_webserver_status );
		rv = -1;
		goto service_end;
	    }
	}
	if( res_dir )
	{
	    list_data ld;
	    list_init( &ld );

	    bfs_find_struct fs;
	    fs.start_dir = reqinfo.resource;
	    fs.mask = 0;
	    int fs_res = bfs_find_first( &fs );
	    while( fs_res )
	    {
		int ignore = 0;
		int64 len = strlen( fs.name );
		if( len == 1 && fs.name[ 0 ] == '.' ) ignore = 1;
		if( len == 2 && fs.name[ 0 ] == '.' && fs.name[ 1 ] == '.' ) ignore = 1;
		if( ignore == 0 )
		    list_add_item( fs.name, fs.type, &ld );
		fs_res = bfs_find_next( &fs );
	    }
	    bfs_find_close( &fs );
	    
	    list_sort( &ld );

	    write_line( conn, webserv_get_string( STR_WEBSERV_HEADER ), 0 );
	    write_line( conn, webserv_get_string( STR_WEBSERV_TITLE ), 0 );
	    write_line( conn, reqinfo.resource_name, 0 );
	    write_line( conn, "</h1>\r\n", 0 );
	    write_line( conn, "<form action=\"", 0 );
	    write_line( conn, reqinfo.resource_name, 0 );
	    write_line( conn, "\" enctype=\"multipart/form-data\" method=\"post\" accept-charset=\"utf-8\"> \r\n", 0 );
	    write_line( conn, webserv_get_string( STR_WEBSERV_HOWTO ), 0 );
	    write_line( conn, "<input type=\"file\" name=\"f1\" size=\"40\"><br>\r\n", 0 );
	    write_line( conn, "<input type=\"file\" name=\"f2\" size=\"40\"><br>\r\n", 0 );
	    write_line( conn, "<input type=\"file\" name=\"f3\" size=\"40\"><br>\r\n", 0 );
	    write_line( conn, "<input type=\"file\" name=\"f4\" size=\"40\"><br>\r\n", 0 );
	    write_line( conn, webserv_get_string( STR_WEBSERV_SEND ), 0 );
	    write_line( conn, "</p>\r\n", 0 );
	    write_line( conn, "</form>\r\n", 0 );
            
            write_line( conn, "<form action=\"", 0 );
	    write_line( conn, reqinfo.resource_name, 0 );
	    write_line( conn, "\" method=\"get\" accept-charset=\"utf-8\"> \r\n", 0 );
            write_line( conn, "<input type=\"text\" name=\"d\" size=\"40\"> \r\n", 0 );
            write_line( conn, webserv_get_string( STR_WEBSERV_CREATE_DIR ), 0 );
	    write_line( conn, "</form>\r\n", 0 );
            
	    write_line( conn, "<table border=\"0\">\r\n", 0 );

	    if( reqinfo.root == 0 )
	    {
		write_line( conn, webserv_get_string( STR_WEBSERV_HOME ), 0 );
		write_line( conn, webserv_get_string( STR_WEBSERV_PARENT ), 0 );
	    }
	    for( int i = 0; i < ld.items_num; i++ )
	    {
		write_line( conn, "<tr><td>", 0 );
		
		utf8_char* name = list_get_item( i, &ld );
		int type = list_get_attr( i, &ld );
		write_line( conn, "<a href=\"", 0 );
		write_line( conn, name, 0 );
		if( type == 1 ) write_line( conn, "/", 0 );
		write_line( conn, "\">", 0 );
		if( type == 1 ) write_line( conn, "<b>", 0 );
		write_line( conn, name, 0 );
		if( type == 1 ) write_line( conn, "/</b>", 0 );
		
		//if( type == 0 )
		{
		    write_line( conn, "<td>", 0 );
		    write_line( conn, "</a> ", 0 );
		    write_line( conn, "<a href=\"?r", 0 );
		    write_line( conn, name, 0 );
		    write_line( conn, webserv_get_string( STR_WEBSERV_REMOVE2 ), 0 );
		}
	    }

	    write_line( conn, "</table>\r\n", 0 );

	    write_line( conn, "</body></html>\r\n", 0 );

	    list_close( &ld );	    
	}
    }
    else
    {
	return_error_msg( conn, &reqinfo );
    }

service_end:

    if( res >= 0 )
    {
	if( close( res ) < 0 )
	{
	    g_webserver_status = "ERROR: Error closing resource.";
	    printf( "%s\n", g_webserver_status );
	}
    }
    if( res_dir )
    {
	if( closedir( res_dir ) < 0 )
	{
	    g_webserver_status = "ERROR: Error closing resource (dir).";
	    printf( "%s\n", g_webserver_status );
	}
    }

    free_req_info( &reqinfo );

    return rv;
}

//Main webserver thread
void* webserver_thread( void *arg )
{
    int listener = -1;
    int conn = -1;

    struct sockaddr_in servaddr;
    int tval;

    g_webserver_pth_work = 1;
    
    g_webserver_status = webserv_get_string( STR_WEBSERV_STATUS_STARTING );

    //Create socket:
    if( ( listener = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
	g_webserver_status = "ERROR: Couldn't create listening socket.";
	printf( "%s\n", g_webserver_status );
	goto tend;
    }

    tval = 1;
    if( setsockopt( listener, SOL_SOCKET, SO_REUSEADDR, &tval, sizeof( int ) ) != 0 )
    {
	g_webserver_status = "ERROR: Error calling setcoskopt()";
	printf( "%s\n", g_webserver_status );
	goto tend;
    }

    //Populate socket address structure:
    memset( &servaddr, 0, sizeof( servaddr ) );
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl( INADDR_ANY );
    servaddr.sin_port = htons( g_webserver_port );

    //Assign socket address to socket:
    if( bind( listener, (struct sockaddr *) &servaddr, sizeof( servaddr ) ) < 0 )
    {
	g_webserver_status = "ERROR: Couldn't bind listening socket.";
	printf( "%s %s\n", g_webserver_status, strerror( errno ) );
	goto tend;
    }

    //Make socket a listening socket:
    if( listen( listener, 4 ) < 0 )
    {
	g_webserver_status = "ERROR: Call to listen failed.";
	printf( "%s\n", g_webserver_status );
	goto tend;
    }

    //Loop infinitely to accept and service connections:
    while( g_webserver_exit_request == 0 ) 
    {
	//Wait for connection:
	
	g_webserver_status = webserv_get_string( STR_WEBSERV_STATUS_WORKING );

	fd_set socks;
	FD_ZERO( &socks );
	FD_SET( listener, &socks );
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	int select_result = select( listener + 1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout );
	if( select_result == 0 )
	{
	    //Select timeout:
	    continue;
	}
	if( select_result < 0 )
	{
	    g_webserver_status = "ERROR: Error calling select()";
	    printf( "%s\n", g_webserver_status );
	    goto tend;
	}

	if( ( conn = accept( listener, NULL, NULL ) ) < 0 )
	{
	    g_webserver_status = "ERROR: Error calling accept()";
	    printf( "%s\n", g_webserver_status );
	    goto tend;
	}

	service_request( conn );

        //Close connected socket:
	shutdown( conn, SHUT_RDWR );
	if( close( conn ) < 0 )
	{
	    g_webserver_status = "ERROR: Error closing connection socket.";
	    printf( "%s\n", g_webserver_status );
	    goto tend;
	}
    }

tend:

    if( listener >= 0 )
    {
	shutdown( listener, SHUT_RDWR );
	if( close( listener ) < 0 )
	{
	    g_webserver_status = "ERROR: Error closing listener socket.";
	    printf( "%s\n", g_webserver_status );
	}
    }

    g_webserver_finished = 1;
    
    pthread_exit( NULL );
}

struct webserver_data
{
    int timer;
    WINDOWPTR close;
};

int webserver_close_button_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    remove_window( g_webserver, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    return 0;
}

void webserver_timer( void* user_data, sundog_timer* t, window_manager* wm )
{
    if( g_webserver )
    {
	draw_window( g_webserver, wm );
    }
}

int webserver_handler( sundog_event* evt, window_manager* wm )
{
    int retval = 0;
    WINDOWPTR win = evt->win;
    webserver_data *data = (webserver_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( webserver_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		g_webserver = win;
		
		iphone_sundog_get_host_info();
		
		g_webserver_root[ 0 ] = 0;
		bmem_strcat( g_webserver_root, sizeof( g_webserver_root ), bfs_get_work_path() );
                
		data->close = new_window( wm_get_string( STR_WM_CLOSE ), 0, 0, 1, 1, wm->button_color, win, button_handler, wm );
        	set_handler( data->close, webserver_close_button_handler, data, wm );
        	set_window_controller( data->close, 0, wm, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->close, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->close, 2, wm, (WCMD)wm->interelement_space + wm->button_xsize, CEND );
		set_window_controller( data->close, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->button_ysize + wm->interelement_space, CEND );
		
		//Create the timer:
		data->timer = add_timer( webserver_timer, data, time_ticks_per_second(), wm );
	    
		//Start webserver thread:
                g_webserver_window_closed = 0;
		g_webserver_finished = 0;
		g_webserver_exit_request = 0;
		g_webserver_pth_work = 0;
		int err = pthread_create( &g_webserver_pth, 0, &webserver_thread, (void*)0 );
		if( err == 0 )
		{
    		    //The pthread_detach() function marks the thread identified by thread as
    		    //detached. When a detached thread terminates, its resources are
    		    //automatically released back to the system.
		    err = pthread_detach( g_webserver_pth );
    		    if( err != 0 )
    		    {
        		g_webserver_status = "ERROR: (webserver init) pthread_detach failed";
			printf( "%s\n", g_webserver_status );
    		    }
		}
		else
		{
		    g_webserver_status = "ERROR: (webserver init) pthread_create failed";
		    printf( "%s\n", g_webserver_status );
		}
	    }
	    retval = 1;
	    break;
	case EVT_DRAW:
	    {
		wbd_lock( win );
                
                int ychar = char_y_size( wm );
		
		draw_frect( 0, 0, win->xsize, win->ysize, win->color, wm );

		wm->cur_font_color = wm->color2;
                
                const utf8_char* str1 = "To transfer the files, launch a web browser on your computer.";
                const utf8_char* str2 = "It must be on the same Wi-Fi network as this device.";
                const utf8_char* str3 = "Then enter the following address:";
                const utf8_char* status_name = "Status:";
                if( bmem_strstr( blocale_get_lang(), "ru_" ) )
                {
            	    str1 = "Для передачи файлов откройте веб-браузер на другом компьютере.";
            	    str2 = "Компьютер должен быть в одной Wi-Fi сети с вашим iPad/iPhone.";
            	    str3 = "В браузере введите следующий адрес:";
        	    status_name = "Статус:";
		}
                
                int scale = 512;
                if( string_x_size( str1, wm ) > win->xsize - wm->interelement_space * 2 )
                {
                    str1 = "Launch a web browser";
                    str2 = "on the same Wi-Fi network.";
            	    if( bmem_strstr( blocale_get_lang(), "ru_" ) )
            	    {
                	str1 = "Откройте веб-браузер на компе";
                	str2 = "в одной WiFi сети с iPad/iPhone.";
            	    }
                    scale = 256;
                }
		
		if( g_iphone_host_addr && bmem_strcmp( g_iphone_host_addr, "127.0.0.1" ) == 0 )
		{
		    wm->cur_font_scale = scale;
		    draw_string( 
			webserv_get_string( STR_WEBSERV_ERROR_CONN ),
			wm->interelement_space, 
			wm->interelement_space,
			wm );
		    goto no_connection;
		}
		
                draw_string( str1, wm->interelement_space, wm->interelement_space + ychar * 0, wm );
                draw_string( str2, wm->interelement_space, wm->interelement_space + ychar * 1, wm );
                draw_string( str3, wm->interelement_space, wm->interelement_space + ychar * 2, wm );
		
		//Draw address:
		if( g_iphone_host_addr )
		{
		    int yy = wm->interelement_space + ychar * 3;
		    wm->cur_font_scale = scale;
		    wm->cur_font_color = wm->red;
		    utf8_char addr[ 256 ];
		    addr[ 0 ] = 0;
		    bmem_strcat( addr, sizeof( addr ), "http://" );
		    bmem_strcat( addr, sizeof( addr ), g_iphone_host_addr );
		    bmem_strcat( addr, sizeof( addr ), ":8080/" );
		    draw_string( addr, wm->interelement_space, yy, wm );
		    wm->cur_font_scale = 256;
		}
		
		//Draw the status:
		if( g_webserver_status )
		{
		    wm->cur_font_color = wm->red;
		    draw_string( status_name, wm->interelement_space, wm->interelement_space + ychar * 6, wm );
		    draw_string( g_webserver_status, wm->interelement_space, wm->interelement_space + ychar * 7, wm );
		}
		
no_connection:		
		wbd_draw( wm );
		wbd_unlock( wm );
	    }
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    {
		//Close the thread:
		if( g_webserver_pth_work )
		{
		    //Stop the thread:
		    g_webserver_exit_request = 1;
		    int timeout_counter = 1000 * 5; //Timeout in milliseconds
		    while( timeout_counter > 0 )
		    {
    			struct timespec delay;
    			delay.tv_sec = 0;
    			delay.tv_nsec = 1000000 * 20; //20 milliseconds
    			if( g_webserver_finished ) break;
    			nanosleep( &delay, NULL ); //Sleep for delay time
    			timeout_counter -= 20;
		    }
		    if( timeout_counter <= 0 )
		    {
    			pthread_cancel( g_webserver_pth );
		    }
		}
	
		//Close the timer:	
		if( data->timer >= 0 )
    		{
        	    remove_timer( data->timer, wm );
        	    data->timer = -1;
    		}
		
		g_webserver = 0;
                g_webserver_window_closed = 1;
	    }
	    retval = 1;
	    break;
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
    }
    return retval;
}

void webserver_open( window_manager* wm )
{
    if( g_webserver == 0 )
    {
	uint flags = DECOR_FLAG_CENTERED | DECOR_FLAG_CHECK_SIZE | DECOR_FLAG_STATIC | DECOR_FLAG_NOBORDER | DECOR_FLAG_FULLSCREEN;
	WINDOWPTR w = new_window_with_decorator(
	    webserv_get_string( STR_WEBSERV_IMPORT_EXPORT ),
	    0, 0,
	    320, 200,
	    wm->dialog_color,
	    wm->root_win,
	    webserver_handler,
	    flags,
	    wm );
	show_window( w, wm );
	bring_to_front( w, wm );
	recalc_regions( wm );
	draw_window( wm->root_win, wm );
    }
}

void webserver_wait_for_close( window_manager* wm )
{
    while( g_webserver_window_closed == 0 )
    {
        sundog_event evt;
        EVENT_LOOP_BEGIN( &evt, wm );
        if( EVENT_LOOP_END( wm ) ) break;
    }
    g_webserver_window_closed = 0;
}
