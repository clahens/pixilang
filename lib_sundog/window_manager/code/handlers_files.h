/*
    handlers_files.h.
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

//Modularity: 100%

struct files_data
{
    WINDOWPTR win;
    WINDOWPTR go_up_button;
    WINDOWPTR go_home_button;
    WINDOWPTR disk_buttons[ 32 ];
    WINDOWPTR path;
    WINDOWPTR name;
    WINDOWPTR files;
    WINDOWPTR ok_button;
    WINDOWPTR cancel_button;
    WINDOWPTR edit_button;
    WINDOWPTR preview_button;
    int (*preview_handler)( files_preview_data*, window_manager* );
    files_preview_data preview_data;
    const utf8_char* mask;
    const utf8_char* props_file; //File with dialog properties
    const utf8_char* preview;
    utf8_char* prop_path[ 32 ];
    int prop_cur_file[ 32 ];
    int	prop_cur_disk;
    int first_time;
    bool files_preview_flag;
};

int files_ok_button_handler( void* user_data, WINDOWPTR win, window_manager* wm );
int files_cancel_button_handler( void* user_data, WINDOWPTR win, window_manager* wm );
int files_name_handler( void* user_data, WINDOWPTR name_win, window_manager* wm );
static void files_refresh_list( WINDOWPTR win, window_manager* wm );
static void files_save_props( WINDOWPTR win, window_manager* wm );
utf8_char* files_name_fix( utf8_char* in_name, const utf8_char* mask );

#ifdef IPHONE
    #define ONLY_WORKING_DIR
#endif

static int files_get_disk_count( void )
{
#ifdef ONLY_WORKING_DIR
    return 1;
#else
    return bfs_get_disk_count();
#endif
}

static void files_refresh_disks( void )
{
    bfs_refresh_disks();
}

static const utf8_char* files_get_disk_name( uint n )
{
#ifdef ONLY_WORKING_DIR
    return "1:/";
#else
    return bfs_get_disk_name( n );
#endif
}

static uint files_get_current_disk( void )
{
#ifdef ONLY_WORKING_DIR
    return 0;
#else
    return bfs_get_current_disk();
#endif
}

static const utf8_char* files_get_work_path( void )
{
#ifdef ONLY_WORKING_DIR
    return "";
#else
    return bfs_get_work_path();
#endif
}

static utf8_char* files_make_resulted_filename( files_data* data, window_manager* wm )
{
    utf8_char* rv = 0;
    if( wm->fdialog_filename )
    {
	utf8_char* fname = text_get_text( data->name, wm );
	utf8_char* res = wm->fdialog_filename;
	res[ 0 ] = 0;
	if( fname )
	{
	    bmem_strcat_resize( res, files_get_disk_name( data->prop_cur_disk ) );
	    if( data->prop_path[ data->prop_cur_disk ] )
	    {
		bmem_strcat_resize( res, data->prop_path[ data->prop_cur_disk ] );
	    }
	    bmem_strcat_resize( res, fname );
	    rv = res;
	}
    }
    return rv;
}

static utf8_char* files_fix_list_item( utf8_char* item_name )
{
    if( item_name == 0 ) return 0;
    size_t len = bmem_strlen( item_name );
    utf8_char* new_item = (utf8_char*)bmem_new( len + 1 );
    if( new_item )
    {
	bmem_copy( new_item, item_name, len + 1 );
	for( size_t i = 0; i < len; i++ )
	{
	    if( new_item[ i ] == '\t' )
	    {
		new_item[ i ] = 0;
		break;
	    }
        }
    }
    return new_item;
}

int files_list_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    files_data* data = (files_data*)user_data;
    list_data* ldata = list_get_data( data->files, wm );
    int last_action = list_get_last_action( win, wm );
    if( ldata && last_action )
    {
	if( last_action == LIST_ACTION_ESCAPE )
	{
	    //ESCAPE KEY:
	    files_cancel_button_handler( data, data->ok_button, wm );
	    return 0;
	}
	utf8_char* item = files_fix_list_item( list_get_item( list_get_selected_num( ldata ), ldata ) );
	char attr = list_get_attr( list_get_selected_num( ldata ), ldata );
	int subdir = 0;
	bool preview_name = 0;
	if( item )
	{
	    if( last_action == LIST_ACTION_UPDOWN )
	    {
		//UP/DOWN KEY:
		if( attr == 0 )
		{
		    preview_name = 1;
		    text_set_text( data->name, item, wm );
		}
	    }
	    if( last_action == LIST_ACTION_ENTER ||
		last_action == LIST_ACTION_DOUBLECLICK )
	    {
		//ENTER/SPACE KEY:
		if( list_get_attr( list_get_selected_num( ldata ), ldata ) == 1 )
		{
		    //It's a dir:
		    subdir = 1;
		}
		else
		{
		    //Select file:
		    if( last_action == LIST_ACTION_DOUBLECLICK )
			text_set_text( data->name, item, wm );
		    files_ok_button_handler( data, data->ok_button, wm );
		    bmem_free( item );
		    return 0;
		}
	    }
	    if( last_action == LIST_ACTION_CLICK )
	    {
		//MOUSE CLICK:
		if( list_get_attr( list_get_selected_num( ldata ), ldata ) == 1 )
		{
		    //It's a dir:
		    subdir = 1;
		}
		else
		{
		    preview_name = 1;
		    text_set_text( data->name, item, wm );
		}
	    }
	    if( subdir )
	    {
		//Go to the subdir:
		utf8_char* path = data->prop_path[ data->prop_cur_disk ];
		if( path )
		    path = (utf8_char*)bmem_resize( path, bmem_strlen( path ) + bmem_strlen( item ) + 2 );
		else
		{
		    path = (utf8_char*)bmem_new( bmem_strlen( item ) + 2 );
		    path[ 0 ] = 0;
		}
		bmem_strcat_resize( path, item );
		bmem_strcat_resize( path, "/" );
		data->prop_path[ data->prop_cur_disk ] = path;
		data->prop_cur_file[ data->prop_cur_disk ] = -1;
		text_set_text( data->path, path, wm );
		files_refresh_list( data->win, wm );
		draw_window( data->win, wm );
	    }
	    bmem_free( item );
	}
	if( preview_name && data->preview && data->files_preview_flag )
	{
	    data->preview_data.name = files_make_resulted_filename( data, wm );
	    if( data->preview_data.name )
	    {
		data->preview_data.status = FPREVIEW_FILE_SELECTED;
		data->preview_handler( &data->preview_data, wm );
		data->preview_data.name[ 0 ] = 0;
	    }
	}
	data->prop_cur_file[ data->prop_cur_disk ] = ldata->selected_item;
    }
    return 0;
}

static void files_remove_props( WINDOWPTR win, window_manager* wm )
{
    if( win == 0 ) return;
    files_data* data = (files_data*)win->data;
    for( int a = 0; a < 32; a++ )
    {
	bmem_free( data->prop_path[ a ] );
        data->prop_path[ a ] = 0;
        data->prop_cur_file[ a ] = 0;
    }
}

static void files_go_home( WINDOWPTR win, window_manager* wm )
{
    if( win == 0 ) return;
    files_data* data = (files_data*)win->data;

    //Get starting directory:
    const utf8_char* cdir = files_get_work_path();
    if( cdir && cdir[ 0 ] )
    {
    	size_t s = bmem_strlen( cdir );
    	if( s > 5 ) s = 5;
    	for( size_t i = 0; i < s; i++ )
    	{
    	    if( cdir[ i ] == ':' )
    	    {
    	        cdir += i + 1;
    	        break;
    	    }
        }
        if( bmem_strlen( cdir ) > 0 )
        {
    	    if( cdir[ 0 ] == '/' )
    	    {
        	cdir++;
            }
        }
    }
    else
    {
        cdir = (utf8_char*)"";
    }

    //Set current disk and directory:
    data->prop_cur_disk = files_get_current_disk();		
    if( data->prop_path[ data->prop_cur_disk ] )
	bmem_free( data->prop_path[ data->prop_cur_disk ] );
    data->prop_path[ data->prop_cur_disk ] = bmem_strdup( cdir );

    //Refresh list:
    files_refresh_list( win, wm );
}

int files_disk_button_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    files_data* data = (files_data*)user_data;

    if( win == data->go_up_button )
    {
        //Go to parent dir:
        utf8_char* path = data->prop_path[ data->prop_cur_disk ];
        if( path )
        {
    	    int p;
	    for( p = bmem_strlen( path ) - 2; p >= 0; p-- )
	    {
	        if( path[ p ] == '/' ) { path[ p + 1 ] = 0; break; }
	    }
	    if( p < 0 ) path[ 0 ] = 0;
	}
	data->prop_cur_file[ data->prop_cur_disk ] = -1;
    }

    if( win == data->go_home_button )
    {
        //Go to home dir:
        files_go_home( data->win, wm );
    }

    for( int a = 0; a < files_get_disk_count(); a++ )
    {
        if( data->disk_buttons[ a ] )
        {
    	    data->disk_buttons[ a ]->color = wm->button_color;
	    if( data->disk_buttons[ a ] == win )
	        data->prop_cur_disk = a;
	}
    }

    //Show selected disk:
    if( data->disk_buttons[ data->prop_cur_disk ] )
    {
        data->disk_buttons[ data->prop_cur_disk ]->color = SELECTED_BUTTON_COLOR;
    }

    //Set new filelist:
    files_refresh_list( data->win, wm );
    //Set new path:
    if( data->prop_path[ data->prop_cur_disk ] )
	text_set_text( data->path, data->prop_path[ data->prop_cur_disk ], wm );
    else
	text_set_text( data->path, "", wm );

    //Save props:
    files_save_props( data->win, wm );
    draw_window( data->win, wm );

    return 0;
}

int files_preview_button_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    files_data* data = (files_data*)user_data;
    
    if( data->files_preview_flag )
	data->files_preview_flag = false;
    else
	data->files_preview_flag = true;
    if( data->files_preview_flag )
        profile_set_int_value( KEY_FILE_PREVIEW, 1, 0 );
    else
        profile_remove_key( KEY_FILE_PREVIEW, 0 );
    profile_save( 0 );

    if( data->files_preview_flag )
    {
	data->preview_button->color = SELECTED_BUTTON_COLOR;
    }
    else 
    {
	data->preview_button->color = wm->button_color;
    }
    
    if( data->files_preview_flag )
	data->preview_data.status = FPREVIEW_OPEN;
    else 
	data->preview_data.status = FPREVIEW_CLOSE;
    data->preview_handler( &data->preview_data, wm );
    
    draw_window( wm->root_win, wm );
    
    return 0;
}

int files_ok_button_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    files_data* data = (files_data*)user_data;
    
    utf8_char* name = text_get_text( data->name, wm );
    
    if( win != data->edit_button )
    {
	//Just OK:
    
	if( name == 0 || name[ 0 ] == 0 )
	{
    	    dialog( wm_get_string( STR_WM_FILE_MSG_NONAME ), wm_get_string( STR_WM_OK ), wm );
    	    return 0;
    	}

	files_name_handler( data, data->name, wm ); //Add file extension
	files_make_resulted_filename( data, wm );
	name = 0; //name is not valid here

	remove_window( data->win, wm );
	recalc_regions( wm );

    }
    else
    {
	//Edit:
	utf8_char* res = 0;
	switch( win->action_result )
	{	
	    case 0:
		//Delete file:
	    case 1:
		//Rename file:
	    case 2:
		//Cut file:
	    case 3:
		//Copy file:
		{
		    if( name == 0 || name[ 0 ] == 0 )
		    {
	        	dialog( wm_get_string( STR_WM_FILE_MSG_NONAME ), wm_get_string( STR_WM_OK ), wm );
	        	return 0;
	    	    }		

		    files_name_handler( data, data->name, wm ); //Add file extension
		    res = files_make_resulted_filename( data, wm );

		    name = text_get_text( data->name, wm );
		}
		break;
	}
	switch( win->action_result )
	{	
	    case 0:
		//Delete file:
		if( res )
		{		    
		    utf8_char* ts = (utf8_char*)bmem_new( bmem_strlen( name ) + 256 );
	    	    ts[ 0 ] = 0;
		    bmem_strcat_resize( ts, "!" );
		    bmem_strcat_resize( ts, wm_get_string( STR_WM_DELETE ) );
		    bmem_strcat_resize( ts, " " );		    
		    bmem_strcat_resize( ts, name );
		    bmem_strcat_resize( ts, "?" );
		    if( res[ 0 ] != 0 )
		    {
			if( dialog( ts, wm_get_string( STR_WM_YESNO ), wm ) == 0 )
			{
		    	    blog( "Deleting file: %s\n", res );
		    	    blog( "Ret.val = %d\n", bfs_remove( res ) );
		    	    files_refresh_list( data->win, wm );
			}
		    }
		    bmem_free( ts );
		}
		break;
	    case 1:
		//Rename file:
		if( res )
		{
		    utf8_char* ts = (utf8_char*)bmem_new( bmem_strlen( name ) + 1 );
		    bmem_copy( ts, name, bmem_strlen( name ) + 1 );
		    dialog_item di[ 2 ];
		    bmem_set( &di, sizeof( di ), 0 );
                    di[ 0 ].type = DIALOG_ITEM_TEXT;
                    di[ 0 ].str_val = ts;
                    di[ 1 ].type = DIALOG_ITEM_NONE;
                    wm->opt_dialog_items = di;
                    int d = dialog( wm_get_string( STR_WM_RENAME_FILE ), wm_get_string( STR_WM_OKCANCEL ), wm );
                    if( d == 0 && di[ 0 ].str_val )
                    {
                	if( bmem_strcmp( di[ 0 ].str_val, name ) != 0 )
                	{
                	    if( di[ 0 ].str_val[ 0 ] != 0 )
                	    {
				utf8_char* fixed_name = files_name_fix( di[ 0 ].str_val, data->mask );
                		utf8_char* new_name = (utf8_char*)bmem_new( bmem_strlen( fixed_name ) + 8192 );
                		new_name[ 0 ] = 0;
                		bmem_strcat_resize( new_name, files_get_disk_name( data->prop_cur_disk ) );
		                if( data->prop_path[ data->prop_cur_disk ] )
		                {
		                    bmem_strcat_resize( new_name, data->prop_path[ data->prop_cur_disk ] );
		                }
		                bmem_strcat_resize( new_name, fixed_name );
		                if( bfs_get_file_size( new_name ) )
		                {
		            	    //Already exists:
		            	    if( dialog_overwrite( fixed_name, wm ) == 0 )
                			bfs_rename( res, new_name );
		                }
		                else
		                {
                		    bfs_rename( res, new_name );
                		}
                		bmem_free( new_name );
                		bmem_free( fixed_name );
				files_refresh_list( data->win, wm );
                	    }
                	}
		    }
		    bmem_free( di[ 0 ].str_val );
		}
		break;
	    case 2:
		//Cut file:
	    case 3:
		//Copy file:
		if( res )
		{
		    bmem_free( wm->fdialog_copy_file_name );
		    bmem_free( wm->fdialog_copy_file_name2 );
		    wm->fdialog_copy_file_name = (utf8_char*)bmem_strdup( res );
		    wm->fdialog_copy_file_name2 = (utf8_char*)bmem_strdup( name );
		    if( win->action_result == 2 )
			wm->fdialog_cut_file_flag = true;
		    else
			wm->fdialog_cut_file_flag = false;
		}
		break;
	    case 4:
		//Paste file:
		if( wm->fdialog_copy_file_name )
		{
            	    utf8_char* new_name = (utf8_char*)bmem_new( bmem_strlen( wm->fdialog_copy_file_name ) + 8192 );
        	    new_name[ 0 ] = 0;
            	    bmem_strcat_resize( new_name, files_get_disk_name( data->prop_cur_disk ) );
                    if( data->prop_path[ data->prop_cur_disk ] )
                    {
                        bmem_strcat_resize( new_name, data->prop_path[ data->prop_cur_disk ] );
	            }
	            bmem_strcat_resize( new_name, wm->fdialog_copy_file_name2 );
	            while( bfs_get_file_size( new_name ) )
	            {
	        	//Already exists:
	        	int i = (int)bmem_strlen( new_name );
	        	bool dot_found = 0;
	        	for( ; i >= 0; i-- )
	        	{
	        	    if( new_name[ i ] == '.' ) { dot_found = 1; break; }
	        	    if( new_name[ i ] == '/' ) break;
	        	}
	        	if( dot_found )
	        	{
	        	    int i2 = (int)bmem_strlen( new_name ) + 1;
	        	    for( ; i2 > i; i2-- )
	        	    {
	        		new_name[ i2 ] = new_name[ i2 - 1 ];
	        	    }
	        	    new_name[ i ] = '_';
	        	}
	        	else
	        	{
	        	    bmem_strcat_resize( new_name, "_" );
	        	}
	            }
    		    bfs_copy_file( new_name, wm->fdialog_copy_file_name );
		    if( wm->fdialog_cut_file_flag ) bfs_remove( wm->fdialog_copy_file_name );
            	    bmem_free( new_name );
		    
		    files_refresh_list( data->win, wm );
		    
		    bmem_free( wm->fdialog_copy_file_name );
		    bmem_free( wm->fdialog_copy_file_name2 );
		    wm->fdialog_copy_file_name = 0;
		    wm->fdialog_copy_file_name2 = 0;
		}
		break;
	    case 5:
		//Create directory:
		{
		    dialog_item di[ 2 ];
		    bmem_set( &di, sizeof( di ), 0 );
                    di[ 0 ].type = DIALOG_ITEM_TEXT;
                    di[ 0 ].str_val = 0;
                    di[ 1 ].type = DIALOG_ITEM_NONE;
                    wm->opt_dialog_items = di;
                    int d = dialog( wm_get_string( STR_WM_CREATE_DIR ), wm_get_string( STR_WM_OKCANCEL ), wm );
                    if( d == 0 && di[ 0 ].str_val )
                    {
                	if( di[ 0 ].str_val[ 0 ] != 0 )
                	{
                	    utf8_char* new_name = (utf8_char*)bmem_new( bmem_strlen( di[ 0 ].str_val ) + 8192 );
                	    new_name[ 0 ] = 0;
                	    bmem_strcat_resize( new_name, files_get_disk_name( data->prop_cur_disk ) );
		    	    if( data->prop_path[ data->prop_cur_disk ] )
		            {
		                bmem_strcat_resize( new_name, data->prop_path[ data->prop_cur_disk ] );
		            }
		            bmem_strcat_resize( new_name, di[ 0 ].str_val );
#ifdef UNIX		            
		            bfs_mkdir( new_name, S_IRWXU | S_IRWXG | S_IRWXO );
#else
		            bfs_mkdir( new_name, 0 );
#endif
                	    bmem_free( new_name );
			    files_refresh_list( data->win, wm );
                	}
		    }
		    bmem_free( di[ 0 ].str_val );
		}
		break;
	    case 6:
		//Delete directory:
		{
		    utf8_char* path = data->prop_path[ data->prop_cur_disk ];
		    if( path )
		    {
			if( path[ 0 ] != 0 )
			{
			    size_t size = bmem_strlen( path );
			    utf8_char* path2 = (utf8_char*)bmem_new( size + 1 );
			    bmem_copy( path2, path, size + 1 );
			    path = path2;
			    
			    if( path[ size - 1 ] == '/' ) { path[ size - 1 ] = 0; size--; }
			    if( size > 0 )
			    {			    
				int i = (int)size - 2;
				for( ; i > 0; i-- )
				{
				    if( path[ i ] == '/' ) { i++; break; }
				}
				utf8_char* dir_name = path + i;
				
				utf8_char* ts = (utf8_char*)bmem_new( bmem_strlen( dir_name ) + 256 );
	    	    		ts[ 0 ] = 0;
				bmem_strcat_resize( ts, "!" );
				bmem_strcat_resize( ts, wm_get_string( STR_WM_DELETE_DIR ) );
				bmem_strcat_resize( ts, " " );
				bmem_strcat_resize( ts, dir_name );
				bmem_strcat_resize( ts, "\n" );
				bmem_strcat_resize( ts, wm_get_string( STR_WM_RECURS ) );
				bmem_strcat_resize( ts, "?" );
				if( dialog( ts, wm_get_string( STR_WM_YESNO ), wm ) == 0 )
				{
				    utf8_char* full_path = (utf8_char*)bmem_new( size + 1024 );
				    full_path[ 0 ] = 0;
				    bmem_strcat_resize( full_path, files_get_disk_name( data->prop_cur_disk ) );
				    bmem_strcat_resize( full_path, path );
		    		    blog( "Deleting directory: %s\n", full_path );
		    		    int rv = bfs_remove( full_path );
		    		    blog( "Ret.val = %d\n", rv );
		    		    if( rv == 0 )
		    		    {
		    			files_disk_button_handler( data, data->go_up_button, wm );
		    		    }
		    		    files_refresh_list( data->win, wm );
		    		    bmem_free( full_path );
				}
				bmem_free( ts );
			    }
			    
			    bmem_free( path );
			}
		    }
		}
		break;
	}
	wm->fdialog_filename[ 0 ] = 0;
    }

    draw_window( wm->root_win, wm );
    return 0;
}

int files_cancel_button_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    files_data* data = (files_data*)user_data;
    if( wm->fdialog_filename )
    {
	utf8_char* res = wm->fdialog_filename;
	res[ 0 ] = 0;
    }
    remove_window( data->win, wm );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );
    return 0;
}

int files_wifi_button_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
#ifdef WEBSERVER
    webserver_open( wm );
#endif
    return 0;
}

utf8_char* files_name_fix( utf8_char* in_name, const utf8_char* mask )
{
    utf8_char* out_name = 0;
    int mask_extensions = 0;
    int mask_cur_ext = 0;
    if( mask )
    {
	mask_extensions = 1;
	for( size_t i = 0; i < bmem_strlen( mask ); i++ )
	{
	    if( mask[ i ] == '/' ) mask_extensions++;
	}
    }
    if( in_name && mask )
    {
	int a;
	bool ext_eq = 0;
	for( a = bmem_strlen( in_name ) - 1; a >= 0; a-- )
	    if( in_name[ a ] == '.' ) break;
	if( a >= 0 )
	{
	    //Dot found:
	    while( 1 )
	    {
		size_t p;
		int extnum = 0;
		for( p = 0; p < bmem_strlen( mask ); p++ )
		{
		    if( extnum == mask_cur_ext ) break;
		    if( mask[ p ] == '/' ) 
		    {
			extnum++;
		    }
		}
		ext_eq = 1;
		for( int i = a + 1 ; ; i++, p++ )
		{
		    int c = in_name[ i ];
		    if( c >= 65 && c <= 90 ) c += 0x20; //Make small leters
		    int m = mask[ p ];
		    if( m == '/' ) m = 0;
		    if( c != m )
		    {
			ext_eq = 0;
			break;
		    }
		    if( c == 0 ) break;
		    if( m == 0 ) break;
		}
		if( ext_eq == 0 )
		{
		    //File extension is not equal to extension from mask:
		    if( mask_cur_ext < mask_extensions - 1 )
		    {
			//But there are another extensions in mask. Lets try them too:
			mask_cur_ext++;
			continue;
		    }
		}
		break;
	    }
	}
	if( ext_eq == 0 ) 
	{
	    //Add extension:
	    out_name = (utf8_char*)bmem_new( bmem_strlen( in_name ) + 32 );
	    out_name[ 0 ] = 0;
	    bmem_strcat_resize( out_name, in_name );
	    a = bmem_strlen( in_name );
	    out_name[ a ] = '.';
	    a++;
	    int p = 0;
	    for(;;)
	    {
		out_name[ a ] = mask[ p ];
		if( out_name[ a ] == '/' ||
		    out_name[ a ] == 0 )
		{
		    out_name[ a ] = 0;
		    break;
		}
		p++;
		a++;
	    }
	}
    }
    if( out_name == 0 )
	out_name = bmem_strdup( in_name );
    return out_name;
}

int files_name_handler( void* user_data, WINDOWPTR name_win, window_manager* wm )
{
    files_data* data = (files_data*)user_data;
    utf8_char* new_name = files_name_fix( text_get_text( name_win, wm ), data->mask );
    if( new_name )
    {
	text_set_text( name_win, new_name, wm );
	bmem_free( new_name );
	draw_window( name_win, wm );
    }
    return 0;
}

static void files_refresh_list( WINDOWPTR win, window_manager* wm )
{
    if( win == 0 ) return;
    files_data* data = (files_data*)win->data;
    if( data->files == 0 ) return;
    list_data* ldata = list_get_data( data->files, wm );
    list_close( ldata );
    list_init( ldata );
    bfs_find_struct fs;
    const utf8_char* disk_name = files_get_disk_name( data->prop_cur_disk );
    utf8_char* path = data->prop_path[ data->prop_cur_disk ];
    utf8_char* res = (utf8_char*)bmem_new( bmem_strlen( disk_name ) + bmem_strlen( path ) + 1 );
    res[ 0 ] = 0;
    bmem_strcat_resize( res, disk_name );
    bmem_strcat_resize( res, path );
    fs.start_dir = res;
    fs.mask = data->mask;
    if( bfs_find_first( &fs ) )
    {
	size_t path_len = bmem_strlen( fs.start_dir );
	utf8_char* temp_path = (utf8_char*)bmem_new( path_len + MAX_DIR_LEN );
	utf8_char* temp_name = (utf8_char*)bmem_new( MAX_DIR_LEN );	
	bmem_copy( temp_path, fs.start_dir, path_len + 1 );
	if( temp_path[ path_len - 1 ] != '/' )
	{
	    temp_path[ path_len - 1 ] = '/';
	    path_len++;
	}
	while( 1 )
	{
	    if( fs.name[ 0 ] != '.' )
	    {
		if( fs.type == BFS_FILE )
		{
		    size_t name_len = bmem_strlen( fs.name );
		    bmem_copy( temp_path + path_len, fs.name, name_len + 1 );
		    size_t file_size = bfs_get_file_size( temp_path );
		    
		    while( 1 )
		    {
			if( file_size > 1024 * 1024 * 1024 )
			{
			    sprintf( temp_name, "%s\t%dG", fs.name, (int)( file_size / ( 1024 * 1024 * 1024 ) ) );
			    break;
			}
			if( file_size > 1024 * 1024 )
			{
			    sprintf( temp_name, "%s\t%dM", fs.name, (int)( file_size / ( 1024 * 1024 ) ) );
			    break;
			}
			if( file_size > 1024 )
			{
			    sprintf( temp_name, "%s\t%dK", fs.name, (int)( file_size / 1024 ) );
			    break;
			}
			sprintf( temp_name, "%s\t%dB", fs.name, (int)file_size );
			break;
		    }		    
		    list_add_item( temp_name, fs.type, ldata );
		}
		else
		{
		    list_add_item( fs.name, fs.type, ldata );
		}
	    }
	    if( bfs_find_next( &fs ) == 0 ) break;
	}
	bmem_free( temp_path );
	bmem_free( temp_name );
    }
    bfs_find_close( &fs );
    bmem_free( res );
    list_sort( ldata );

    //Set file num:
    list_select_item( data->files, data->prop_cur_file[ data->prop_cur_disk ], true );
}

void files_load_string( bfs_file f, utf8_char *dest )
{
    for( int p = 0; p < MAX_DIR_LEN; p++ )
    {
	if( bfs_eof( f ) ) { dest[ p ] = 0; break; }
	dest[ p ] = bfs_getc( f );
	if( dest[ p ] == 0 ) break;
    }
}

static int files_get_disk_num( const utf8_char* name )
{
    for( int a = 0; a < files_get_disk_count(); a++ )
    {
	if( bmem_strcmp( name, files_get_disk_name( a ) ) == 0 ) return a;
    }
    return -1;
}

static void files_load_props( WINDOWPTR win, window_manager* wm )
{
    if( win == 0 ) return;
    files_data* data = (files_data*)win->data;

    files_remove_props( win, wm );
    files_go_home( win, wm );
    
    if( data->props_file == 0 ) return;

    bfs_file f = bfs_open( data->props_file, "rb" );
    if( f == 0 ) return;

    utf8_char* temp_str = (utf8_char*)bmem_new( 4096 );

    //Current disk:
    files_load_string( f, temp_str );
    int d = files_get_disk_num( temp_str );
    if( d != -1 ) 
        data->prop_cur_disk = d;

    //Disks info:
    for( int a = 0; a < 32; a++ )
    {
        if( bfs_eof( f ) ) break;
        files_load_string( f, temp_str ); //disk name
        d = files_get_disk_num( temp_str ); //disk num
	files_load_string( f, temp_str ); //path on this disk
	int file_num; bfs_read( &file_num, 4, 1, f ); //file number on this disk
	if( d != -1 )
	{
	    if( data->prop_path[ d ] )
	    {
	        //Remove old path:
	        bmem_free( data->prop_path[ d ] );
	        data->prop_path[ d ] = 0;
	    }
	    if( temp_str[ 0 ] )
	    {
	        //Set new path:
	        data->prop_path[ d ] = bmem_strdup( temp_str );
	    }
	    data->prop_cur_file[ d ] = file_num;
	}
    }
    	
    bmem_free( temp_str );
    bfs_close( f );

    //Refresh list:
    files_refresh_list( win, wm );
}

static void files_save_props( WINDOWPTR win, window_manager* wm )
{
    if( win == 0 ) return;
    files_data* data = (files_data*)win->data;
    if( data->props_file == 0 ) return;
    
    bfs_file f = bfs_open( data->props_file, "wb" );
    if( f == 0 ) return;

    //Save current disk:
    utf8_char* path;
    const utf8_char* disk_name = files_get_disk_name( data->prop_cur_disk );
    bfs_write( disk_name, 1, bmem_strlen( disk_name ) + 1, f );

    //Save other properties:
    for( int a = 0; a < files_get_disk_count(); a++ )
    {
        //Disk name:
        disk_name = files_get_disk_name( a );
        bfs_write( disk_name, 1, bmem_strlen( disk_name ) + 1, f );
        //Current path:
        if( data->prop_path[ a ] == 0 )
	    bfs_putc( 0, f );
	else
	{
	    path = data->prop_path[ a ];
	    bfs_write( path, 1, bmem_strlen( path ) + 1, f );
	}
	//Selected file num:
	bfs_write( &data->prop_cur_file[ a ], 4, 1, f );
    }

    bfs_close( f );
}

static int get_disk_button_xsize( const utf8_char* name, WINDOWPTR win )
{
    window_manager* wm = win->wm;
    int xsize = font_string_x_size( name, win->font, wm ) + wm->interelement_space * 2;
    if( xsize < wm->scrollbar_size ) 
	xsize = wm->scrollbar_size;
    return xsize;
}

int files_handler( sundog_event* evt, window_manager* wm )
{
    if( window_handler_check_data( evt, wm ) ) return 0;
    int retval = 0;
    WINDOWPTR win = evt->win;
    files_data* data = (files_data*)win->data;
    int a, b;
    int disks_x;
    int disks_buttons;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( files_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		const utf8_char* bname;
		int but_xsize = wm->button_xsize;
		int but_ysize = wm->button_ysize;
		
		//DATA INIT:
		data->go_up_button = 0;
		data->go_home_button = 0;
		data->path = 0;
		data->name = 0;
		data->files = 0;
		data->ok_button = 0;
		data->cancel_button = 0;
		data->edit_button = 0;
		data->preview_button = 0;

		files_refresh_disks();
		for( a = 0; a < 32; a ++ )
		{
		    data->prop_path[ a ] = 0;
		    data->prop_cur_file[ a ] = 0;
		    data->disk_buttons[ a ] = 0;
    	        }
		data->prop_cur_disk = files_get_current_disk();
		if( wm->opt_files_props )
		{
		    data->props_file = (utf8_char*)bmem_strdup( wm->opt_files_props );
    	        }
		else
		{
		    data->props_file = 0;
		}
		data->mask = wm->opt_files_mask;
		data->preview = wm->opt_files_preview;
		data->preview_handler = wm->opt_files_preview_handler;
		data->preview_data.user_data = wm->opt_files_preview_user_data;
		data->win = win;
		data->first_time = 1;
		data->files_preview_flag = false;

		data->preview_data.win = new_window( "Preview win", 0, wm->interelement_space, 0, 0, win->color, win, null_handler, wm );
		set_window_controller( data->preview_data.win, 0, wm, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->preview_data.win, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
	    
		//DISKS:
		int disk_button_xsize;
		for( a = 0; a < 32; a++ ) data->disk_buttons[ a ] = 0;
		disks_x = wm->interelement_space;
		disks_buttons = 1;
		disk_button_xsize = get_disk_button_xsize( "..", win );
		data->go_up_button = new_window( 
		    "..", 
		    disks_x, wm->interelement_space, disk_button_xsize, wm->scrollbar_size, 
		    wm->button_color,
		    win,
		    button_handler, 
		    wm );
		disks_x += disk_button_xsize + wm->interelement_space2;
		set_handler( data->go_up_button, files_disk_button_handler, data, wm );
		data->go_home_button = new_window( 
		    "\x18\x19", 
		    disks_x, wm->interelement_space, disk_button_xsize, wm->scrollbar_size, 
		    wm->button_color,
		    win,
		    button_handler, 
		    wm );
		disks_x += disk_button_xsize + wm->interelement_space2;
		set_handler( data->go_home_button, files_disk_button_handler, data, wm );
		set_window_controller( data->go_up_button, 1, wm, CWIN, (WCMD)data->preview_data.win, CY2, CEND );
		set_window_controller( data->go_up_button, 3, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)wm->scrollbar_size, CEND );
		set_window_controller( data->go_home_button, 1, wm, CWIN, (WCMD)data->preview_data.win, CY2, CEND );
		set_window_controller( data->go_home_button, 3, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)wm->scrollbar_size, CEND );
		if( files_get_disk_count() > 1 || ( files_get_disk_name( 0 )[ 0 ] != '/' && files_get_disk_name( 0 )[ 0 ] != '1' ) )
		{
		    for( a = 0; a < files_get_disk_count(); a++ )
		    {
			disk_button_xsize = get_disk_button_xsize( files_get_disk_name( a ), win );
			data->disk_buttons[ a ] = new_window( 
			    files_get_disk_name( a ), 
			    disks_x, 
			    wm->interelement_space, 
			    disk_button_xsize, 
			    wm->scrollbar_size, 
			    wm->button_color,
			    win,
			    button_handler, 
			    wm );
			set_handler( data->disk_buttons[ a ], files_disk_button_handler, data, wm );
			set_window_controller( data->disk_buttons[ a ], 1, wm, CWIN, (WCMD)data->preview_data.win, CY2, CEND );
			set_window_controller( data->disk_buttons[ a ], 3, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)wm->scrollbar_size, CEND );
			disks_x += disk_button_xsize + wm->interelement_space2;
			disks_buttons++;
		    }
		}
		bool path_on_new_line = 0;
		if( disks_x > win->xsize / 2 )
		    path_on_new_line = 1;

		//DIRECTORY:
		a = font_string_x_size( wm_get_string( STR_WM_FILE_PATH ), win->font, wm ) + font_string_x_size( " ", win->font, wm );
		b = wm->scrollbar_size + wm->interelement_space;
#ifdef IPHONE
        	wm->opt_text_ro = true;
		data->path = new_window( "pathn", 5000, 5000, 1, 1, wm->text_background, win, text_handler, wm );
#else
		if( path_on_new_line )
		{
		    WINDOWPTR tw = new_window( wm_get_string( STR_WM_FILE_PATH ), wm->interelement_space, b, a, wm->text_ysize, 0, win, label_handler, wm );
		    set_window_controller( tw, 1, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)b, CEND );
		    set_window_controller( tw, 3, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)b + wm->text_ysize, CEND );
		}
		wm->opt_text_ro = true;
		data->path = new_window( "pathn", 0, 0, 1, 1, wm->text_background, win, text_handler, wm );
		if( path_on_new_line )
		{
		    set_window_controller( data->path, 0, wm, (WCMD)wm->interelement_space + a, CEND );
		    set_window_controller( data->path, 1, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)b, CEND );
		    set_window_controller( data->path, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		    set_window_controller( data->path, 3, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)b + wm->text_ysize, CEND );
        	    b += wm->text_ysize + wm->interelement_space;
    		}
    		else
    		{
		    set_window_controller( data->path, 0, wm, (WCMD)disks_x + wm->interelement_space, CEND );
		    set_window_controller( data->path, 1, wm, CWIN, (WCMD)data->preview_data.win, CY2, CEND );
		    set_window_controller( data->path, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		    set_window_controller( data->path, 3, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)wm->scrollbar_size, CEND );
    		}
#endif
            
		//FILENAME:
		a = font_string_x_size( wm_get_string( STR_WM_FILE_NAME ), win->font, wm ) + font_string_x_size( " ", win->font, wm );
		WINDOWPTR tw = new_window( wm_get_string( STR_WM_FILE_NAME ), wm->interelement_space, b, font_string_x_size( wm_get_string( STR_WM_FILE_NAME ), win->font, wm ) + 4, wm->text_ysize, 0, win, label_handler, wm );
		set_window_controller( tw, 1, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)b, CEND );
		set_window_controller( tw, 3, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)b + wm->text_ysize, CEND );
		data->name = new_window( "namen", 0, 0, 1, 1, wm->text_background, win, text_handler, wm );
		set_handler( data->name, files_name_handler, data, wm );
	        set_window_controller( data->name, 0, wm, (WCMD)wm->interelement_space + a, CEND );
		set_window_controller( data->name, 1, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)b, CEND );
		set_window_controller( data->name, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->name, 3, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)b + wm->text_ysize, CEND );

		//FILES:
		b += wm->text_ysize + wm->interelement_space;
		data->files = new_window( "files", 0, b, 100, 100, wm->list_background, win, list_handler, wm );
		set_handler( data->files, files_list_handler, data, wm );
		set_window_controller( data->files, 0, wm, CPERC, (WCMD)0, CADD, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->files, 1, wm, CWIN, (WCMD)data->preview_data.win, CY2, CADD, (WCMD)b, CEND );
		set_window_controller( data->files, 2, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->files, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)but_ysize + wm->interelement_space * 2, CEND );

		//BUTTONS:

		int x = wm->interelement_space;

		bname = wm_get_string( STR_WM_OK ); but_xsize = button_get_optimal_xsize( bname, win->font, false, wm );
		data->ok_button = new_window( bname, x, 0, but_xsize, 1, wm->button_color, win, button_handler, wm );
		set_handler( data->ok_button, files_ok_button_handler, data, wm );
		set_window_controller( data->ok_button, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->ok_button, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)but_ysize + wm->interelement_space, CEND );
		x += but_xsize + wm->interelement_space2;
    
		bname = wm_get_string( STR_WM_CANCEL ); but_xsize = button_get_optimal_xsize( bname, win->font, false, wm );
		data->cancel_button = new_window( bname, x, 0, but_xsize, 1, wm->button_color, win, button_handler, wm );
		set_handler( data->cancel_button, files_cancel_button_handler, data, wm );
		set_window_controller( data->cancel_button, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->cancel_button, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)but_ysize + wm->interelement_space, CEND );
		x += but_xsize + wm->interelement_space2;

		bname = wm_get_string( STR_WM_EDIT ); but_xsize = button_get_optimal_xsize( bname, win->font, false, wm );
		data->edit_button = new_window( bname, x, 0, but_xsize, 1, wm->button_color, win, button_handler, wm );
		utf8_char* ts = (utf8_char*)bmem_new( 4096 );
		sprintf( ts, "%s\n%s\n%s\n%s\n%s\n%s\n%s", 
		    wm_get_string( STR_WM_DELETE ),
		    wm_get_string( STR_WM_RENAME ),
		    wm_get_string( STR_WM_CUT ),
		    wm_get_string( STR_WM_COPY ),
		    wm_get_string( STR_WM_PASTE ),
		    wm_get_string( STR_WM_CREATE_DIR ),
		    wm_get_string( STR_WM_DELETE_DIR ) );
		button_set_menu( data->edit_button, ts, wm );
		bmem_free( ts );
		set_handler( data->edit_button, files_ok_button_handler, data, wm );
		set_window_controller( data->edit_button, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		set_window_controller( data->edit_button, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)but_ysize + wm->interelement_space, CEND );
		x += but_xsize + wm->interelement_space2;

		if( data->preview )
		{
		    //13 july 2016: we now need to store all the settings in the SunDog profile:
		    data->files_preview_flag = profile_get_int_value( KEY_FILE_PREVIEW, 0, 0 );
		    //Load previous data from the file and remove this file:
		    bfs_file f = bfs_open( "2:/.sundog_files_preview", "rb" );
		    if( f )
		    {
		        if( bfs_getc( f ) == 1 )
		    	    data->files_preview_flag = true;
			else 
			    data->files_preview_flag = false;
			bfs_close( f );
			bfs_remove( "2:/.sundog_files_preview" );
			if( data->files_preview_flag )
			    profile_set_int_value( KEY_FILE_PREVIEW, 1, 0 );
			else
			    profile_remove_key( KEY_FILE_PREVIEW, 0 );
			profile_save( 0 );
		    }
		    COLOR pcolor;
		    if( data->files_preview_flag )
		    {
			pcolor = SELECTED_BUTTON_COLOR;
			data->preview_data.status = FPREVIEW_OPEN;
			data->preview_handler( &data->preview_data, wm );
		    }
		    else
		    {
			pcolor = wm->button_color;
			data->preview_data.status = FPREVIEW_NONE;
		    }
		    but_xsize = button_get_optimal_xsize( data->preview, win->font, false, wm );
		    data->preview_button = new_window( data->preview, x, 0, but_xsize, 1, pcolor, win, button_handler, wm );
		    set_handler( data->preview_button, files_preview_button_handler, data, wm );
		    set_window_controller( data->preview_button, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		    set_window_controller( data->preview_button, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)but_ysize + wm->interelement_space, CEND );
		    x += but_xsize + wm->interelement_space2;
		}
		else 
		{
		    data->preview_button = 0;
		}
#ifdef WEBSERVER
		bname = "Wi-Fi"; but_xsize = button_get_optimal_xsize( bname, win->font, false, wm );
		WINDOWPTR wifi_button = new_window( bname, x, 0, but_xsize, 1, wm->button_color, win, button_handler, wm );
		set_handler( wifi_button, files_wifi_button_handler, data, wm );
		set_window_controller( wifi_button, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
		set_window_controller( wifi_button, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)but_ysize + wm->interelement_space, CEND );
		x += but_xsize + wm->interelement_space2;
#endif
		for( int i = 0; i < 4; i++ )
		{
		    if( wm->opt_files_user_button[ i ] )
		    {
			but_xsize = button_get_optimal_xsize( wm->opt_files_user_button[ i ], win->font, false, wm );
			WINDOWPTR user_button = new_window( wm->opt_files_user_button[ i ], x, 0, but_xsize, 1, wm->button_color, win, button_handler, wm );
			set_handler( user_button, wm->opt_files_user_button_handler[ i ], wm->opt_files_user_button_data[ i ], wm );
			set_window_controller( user_button, 1, wm, CPERC, (WCMD)100, CSUB, (WCMD)wm->interelement_space, CEND );
			set_window_controller( user_button, 3, wm, CPERC, (WCMD)100, CSUB, (WCMD)but_ysize + wm->interelement_space, CEND );
			x += but_xsize + wm->interelement_space2;
		    }
		}

		//LOAD LAST PROPERTIES:
		recalc_regions( wm ); //for getting calculated size of file's list
    		files_load_props( win, wm );
		if( data->prop_path[ data->prop_cur_disk ] )
		    text_set_text( data->path, data->prop_path[ data->prop_cur_disk ], wm );
		if( data->prop_cur_file[ data->prop_cur_disk ] >= 0 )
		{
		    list_data* ldata = list_get_data( data->files, wm );
		    int snum = list_get_selected_num( ldata );
		    utf8_char* item = files_fix_list_item( list_get_item( snum, ldata ) );
		    //Set file num:
		    list_select_item( data->files, data->prop_cur_file[ data->prop_cur_disk ], true );
		    if( wm->opt_files_def_filename )
		    {
			text_set_text( data->name, wm->opt_files_def_filename, wm );
		    }
		    else
		    {
			if( item )
			{
			    if( list_get_attr( snum, ldata ) == 0 )
				text_set_text( data->name, item, wm );
			}
		    }
		    bmem_free( item );
		}

		//SHOW CURRENT DISK:
		if( data->disk_buttons[ data->prop_cur_disk ] )
		{
		    data->disk_buttons[ data->prop_cur_disk ]->color = SELECTED_BUTTON_COLOR;
		}

		//SET FOCUS:
		set_focus_win( data->files, wm );

		wm->opt_files_props = 0;
		wm->opt_files_mask = 0;
		wm->opt_files_preview = 0;
		wm->opt_files_preview_handler = 0;
		wm->opt_files_preview_user_data = 0;
		for( int i = 0; i < 4; i++ )
		{
		    wm->opt_files_user_button[ i ] = 0;
		    wm->opt_files_user_button_handler[ i ] = 0;
		    wm->opt_files_user_button_data[ i ] = 0;
		}
		wm->opt_files_def_filename = 0;
	    }
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    files_save_props( win, wm );
	    bmem_free( (void*)data->props_file );
	    files_remove_props( win, wm );
	    if( data->preview )
	    {
		data->preview_data.status = FPREVIEW_CLOSE;
		data->preview_handler( &data->preview_data, wm );
	    }
	    break;
	case EVT_DRAW:
	    win_draw_frect( win, 0, 0, win->xsize, win->ysize, win->color, wm );
	    if( data->first_time && data->files )
	    {
		//Set file num:
		list_select_item( data->files, data->prop_cur_file[ data->prop_cur_disk ], true );
		data->first_time = 0;
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	case EVT_MOUSEMOVE:
	case EVT_MOUSEBUTTONUP:
	case EVT_TOUCHBEGIN:
	case EVT_TOUCHEND:
	case EVT_TOUCHMOVE:
	    retval = 1;
	    break;
    }
    return retval;
}
