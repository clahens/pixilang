extern int android_sundog_copy_resources( void );

enum
{
    STR_RES_UNPACKING,
    STR_RES_LICENSE_ERROR,
    STR_RES_UNKNOWN,
    STR_RES_LOCAL_RES,
    STR_RES_MEM_CARD,
    STR_RES_UNPACK_ERROR,
};

const utf8_char* res_get_string( int str_id )
{
    const utf8_char* str = 0;
    const utf8_char* lang = blocale_get_lang();
    while( 1 )
    {
        if( bmem_strstr( lang, "ru_" ) )
        {
            switch( str_id )
            {
                case STR_RES_UNPACKING: str = "Распаковка и проверка ..."; break;
                case STR_RES_LICENSE_ERROR: str = "!Ошибка проверки лицензии %d.\nУбедитесь, что Интернет подключен\nи попробуйте еще раз.\nЛибо свяжитесь с разработчиком:\n<nightradio@gmail.com>"; break;
                case STR_RES_UNKNOWN: str = "неизвестная ошибка"; break;
                case STR_RES_LOCAL_RES: str = "нет доступа к лок. ресурсам"; break;
                case STR_RES_MEM_CARD: str = "нет доступа к карте памяти,\nили на ней кончилось место"; break;
                case STR_RES_UNPACK_ERROR: str = "Ошибка распаковки"; break;
            }
            if( str ) break;
        }
        //Default:
        switch( str_id )
        {
            case STR_RES_UNPACKING: str = "Unpacking / Verifying ..."; break;
            case STR_RES_LICENSE_ERROR: str = "!License Check Error %d.\nPlease check Internet connection\nand try again later.\nOr contact developer directly:\n<nightradio@gmail.com>"; break;
            case STR_RES_UNKNOWN: str = "unknown"; break;
            case STR_RES_LOCAL_RES: str = "can't open local resource"; break;
            case STR_RES_MEM_CARD: str = "memory card is locked by someone\nor it has no free space"; break;
            case STR_RES_UNPACK_ERROR: str = "Unpacking error"; break;
        }
        break;
    }
    return str;
}

void copy_resources( window_manager* wm )
{
    show_status_message( res_get_string( STR_RES_UNPACKING ), 0, wm );
    wm->device_redraw_framebuffer( wm );
    int rv = android_sundog_copy_resources();
    utf8_char ts[ 1024 ];
    switch( rv )
    {
	case 0:
	    break;
        default:
    	    {
    		const utf8_char* err = res_get_string( STR_RES_UNKNOWN );
    		switch( rv )
    		{
    		    case -1: err = res_get_string( STR_RES_LOCAL_RES ); break;
    		    case -2: err = res_get_string( STR_RES_MEM_CARD ); break;
    		}
    		sprintf( ts, "!%s %d:\n%s", res_get_string( STR_RES_UNPACK_ERROR ), rv, err );
    		dialog( ts, "OK", wm );
    	    }
            wm->exit_request = 1;
            break;
    }
}

void check_resources( void )
{
}
