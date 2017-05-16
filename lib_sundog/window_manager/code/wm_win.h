/*
    wm_win.h. Platform-dependent module : Windows
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

#pragma once

#include "win_res.h" //(IDI_ICON1) Must be in your project

#ifndef WM_TOUCH
    #define WM_TOUCH 0x0240
#endif
#ifndef WM_MOUSEWHEEL
    #define WM_MOUSEWHEEL 0x020A
#endif

const char* className = "SunDogEngine";
const utf8_char* windowName = "SunDogEngine_window";
char windowName_ansi[ 256 ];
HGLRC hGLRC;
WNDCLASS wndClass;
HWND hWnd = 0;
uint win_flags = 0;

int g_mod = 0;

int g_frame_len;
ticks_t g_ticks_per_frame;
ticks_t g_prev_frame_time = 0;

#ifdef OPENGL
    #include "wm_opengl.h"
#endif

void SetupPixelFormat( HDC hDC );
LRESULT APIENTRY WinProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
static int CreateWin( HINSTANCE hCurrentInst, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow, window_manager* wm );
#ifdef DIRECTDRAW
    #include "wm_win_ddraw.h"
#endif

int device_start( const utf8_char* windowname, int xsize, int ysize, window_manager* wm )
{
    int retval = 0;
    
    hWnd = 0;
    win_flags = 0;
    g_mod = 0;
    g_prev_frame_time = 0;
#ifdef FRAMEBUFFER
    wm->fb = 0;
#endif
    
    g_frame_len = 1000 / wm->max_fps;
    g_ticks_per_frame = time_ticks_per_second() / wm->max_fps;

    if( windowname ) windowName = windowname;
    win_flags = wm->flags;
	
    xsize = profile_get_int_value( KEY_SCREENX, xsize, 0 );
    ysize = profile_get_int_value( KEY_SCREENY, ysize, 0 );
    wm->real_window_width = xsize;
    wm->real_window_height = ysize;

#ifdef GDI
    wm->gdi_bitmap_info[ 0 ] = 888;
#endif

    if( wm->screen_angle & 1 )
    {
        wm->screen_xsize = ysize;
        wm->screen_ysize = xsize;
    }
    else
    {
        wm->screen_xsize = xsize;
        wm->screen_ysize = ysize;
    }
    wm->screen_xsize = xsize;
    wm->screen_ysize = ysize;
    
    if( wm->screen_ppi == 0 ) wm->screen_ppi = 110;
    win_zoom_init( wm );
    
    if( CreateWin( wm->sd->hCurrentInst, wm->sd->hPreviousInst, wm->sd->lpszCmdLine, wm->sd->nCmdShow, wm ) ) //create main window
	return 1; //Error
	
    if( profile_get_int_value( KEY_NOCURSOR, -1, 0 ) != -1 )
    {
        //Hide mouse pointer:
        ShowCursor( 0 );
    }

    //Create framebuffer:
#ifdef FRAMEBUFFER
    #ifdef DIRECTDRAW
	wm->fb = 0;
    #else
	wm->fb = (COLORPTR)bmem_new( wm->screen_xsize * wm->screen_ysize * COLORLEN );
	wm->fb_xpitch = 1;
	wm->fb_ypitch = wm->screen_xsize;
    #endif
#else
    #ifdef OPENGL
	#ifdef GLNORETAIN
    	    wm->screen_buffer_preserved = 0;
	#else
    	    wm->screen_buffer_preserved = 1;
	#endif
    #endif
#endif

    return retval;
}

void device_end( window_manager* wm )
{
#ifdef OPENGL
    gl_deinit( wm );
    wglMakeCurrent( wm->hdc, 0 );
    wglDeleteContext( hGLRC );
#endif

#ifdef DIRECTX
    dd_close( wm );
#endif

#ifdef FRAMEBUFFER
#ifndef DIRECTDRAW
    bmem_free( wm->fb );
#endif
#endif

    DestroyWindow( hWnd );
}

void device_event_handler( window_manager* wm )
{
    MSG msg;
    int pause;
    
    if( wm->flags & WIN_INIT_FLAG_FULL_CPU_USAGE )
    {
	pause = 1;
    }
    else 
    {
	pause = g_frame_len;
    }
    
    while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) 
    {
	if( GetMessage( &msg, NULL, 0, 0 ) == 0 ) return; //Exit
	TranslateMessage( &msg );
	DispatchMessage( &msg );
	pause = 0;
    }
    
    if( wm->events_count )
    {
	pause = 0;
    }
    
    if( pause )
    {
	time_sleep( pause );
    }
    
    ticks_t cur_t = time_ticks();
    if( cur_t - g_prev_frame_time >= g_ticks_per_frame )
    {
	wm->frame_event_request = 1;
	g_prev_frame_time = cur_t;
    }
}

#ifdef OPENGL
void SetupPixelFormat( HDC hDC, window_manager* wm )
{
    if( hDC == 0 ) return;

    unsigned int flags = 
	( 0
        | PFD_SUPPORT_OPENGL
        | PFD_DRAW_TO_WINDOW
	| PFD_DOUBLEBUFFER
	);

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),  /* size */
        1,                              /* version */
	flags,
        PFD_TYPE_RGBA,                  /* color type */
        16,                             /* prefered color depth */
        0, 0, 0, 0, 0, 0,               /* color bits (ignored) */
        0,                              /* no alpha buffer */
        0,                              /* alpha bits (ignored) */
        0,                              /* no accumulation buffer */
        0, 0, 0, 0,                     /* accum bits (ignored) */
        16,                             /* depth buffer */
        0,                              /* no stencil buffer */
        0,                              /* no auxiliary buffers */
        PFD_MAIN_PLANE,                 /* main layer */
        0,                              /* reserved */
        0, 0, 0,                        /* no layer, visible, damage masks */
    };
    int pixelFormat;

    pixelFormat = ChoosePixelFormat( hDC, &pfd );
    if( pixelFormat == 0 ) 
    {
        MessageBox( WindowFromDC( hDC ), "ChoosePixelFormat failed.", "Error", MB_ICONERROR | MB_OK );
        exit( 1 );
    }

    if( SetPixelFormat( hDC, pixelFormat, &pfd ) != TRUE ) 
    {
        MessageBox( WindowFromDC( hDC ), "SetPixelFormat failed.", "Error", MB_ICONERROR | MB_OK );
        exit( 1 );
    }
}
#endif

uint16 win_key_to_sundog_key( uint16 vk, uint16 par )
{
    switch( vk )
    {
	case VK_BACK: return KEY_BACKSPACE; break;
	case VK_TAB: return KEY_TAB; break;
	case VK_RETURN: return KEY_ENTER; break;
	case VK_ESCAPE: return KEY_ESCAPE; break;
	
	case VK_F1: return KEY_F1; break;
	case VK_F2: return KEY_F2; break;
	case VK_F3: return KEY_F3; break;
	case VK_F4: return KEY_F4; break;
	case VK_F5: return KEY_F5; break;
	case VK_F6: return KEY_F6; break;
	case VK_F7: return KEY_F7; break;
	case VK_F8: return KEY_F8; break;
	case VK_F9: return KEY_F9; break;
	case VK_F10: return KEY_F10; break;
	case VK_F11: return KEY_F11; break;
	case VK_F12: return KEY_F12; break;
	case VK_UP: return KEY_UP; break;
	case VK_DOWN: return KEY_DOWN; break;
	case VK_LEFT: return KEY_LEFT; break;
	case VK_RIGHT: return KEY_RIGHT; break;
	case VK_INSERT: return KEY_INSERT; break;
	case VK_DELETE: return KEY_DELETE; break;
	case VK_HOME: return KEY_HOME; break;
	case VK_END: return KEY_END; break;
	case 33: return KEY_PAGEUP; break;
	case 34: return KEY_PAGEDOWN; break;
	case VK_CAPITAL: return KEY_CAPS; break;
	case VK_SHIFT: return KEY_SHIFT; break;
	case VK_CONTROL: return KEY_CTRL; break;
	case VK_MENU: return KEY_ALT; break;

	case 189: return '-'; break;
	case 187: return '='; break;
	case 219: return '['; break;
	case 221: return ']'; break;
	case 186: return ';'; break;
	case 222: return 0x27; break; // '
	case 188: return ','; break;
	case 190: return '.'; break;
	case 191: return '/'; break;
	case 220: return 0x5C; break; // |
	case 192: return '`'; break;

	case VK_NUMPAD0: return '0'; break;
	case VK_NUMPAD1: return '1'; break;
	case VK_NUMPAD2: return '2'; break;
	case VK_NUMPAD3: return '3'; break;
	case VK_NUMPAD4: return '4'; break;
	case VK_NUMPAD5: return '5'; break;
	case VK_NUMPAD6: return '6'; break;
	case VK_NUMPAD7: return '7'; break;
	case VK_NUMPAD8: return '8'; break;
	case VK_NUMPAD9: return '9'; break;
	case VK_MULTIPLY: return '*'; break;
	case VK_ADD: return '+'; break;
	case VK_SEPARATOR: return '/'; break;
	case VK_SUBTRACT: return '-'; break;
	case VK_DECIMAL: return '.'; break;
	case VK_DIVIDE: return '/'; break;
    }
    
    if( vk == 0x20 ) return vk;
    if( vk >= 0x30 && vk <= 0x39 ) return vk; //Numbers
    if( vk >= 0x41 && vk <= 0x5A ) return vk + 0x20; //Letters (lowercase)
    
    return KEY_UNKNOWN + vk;
}


#ifdef MULTITOUCH
LRESULT OnTouch( HWND hWnd, WPARAM wParam, LPARAM lParam, window_manager* wm )
{
    BOOL bHandled = FALSE;
    UINT cInputs = LOWORD( wParam );
    PTOUCHINPUT pInputs = (PTOUCHINPUT)bmem_new( sizeof( TOUCHINPUT ) * cInputs );
    if( pInputs )
    {
	if( GetTouchInputInfo( (HTOUCHINPUT)lParam, cInputs, pInputs, sizeof(TOUCHINPUT)) )
	{
    	    for( UINT i = 0; i < cInputs; i++ )
    	    {
        	TOUCHINPUT* ti = &pInputs[ i ]; //x, y, dwID
        	int x = ti->x / 100;
        	int y = ti->y / 100;
		POINT point;
		point.x = x;
		point.y = y;
		ScreenToClient( hWnd, &point );
		x = point.x;
		y = point.y;
        	if( ti->dwFlags & TOUCHEVENTF_DOWN )
        	{
        	    collect_touch_events( 0, TOUCH_FLAG_REALWINDOW_XY, g_mod, x, y, 1024, ti->dwID, wm );
        	    if( ti->dwFlags & TOUCHEVENTF_PRIMARY )
        		wm->ignore_mouse_evts = true;
        	}
        	if( ti->dwFlags & TOUCHEVENTF_MOVE )
        	{
        	    uint evt_flags = g_mod;
        	    if( i < cInputs - 1 )
        		evt_flags |= EVT_FLAG_DONTDRAW;
        	    collect_touch_events( 1, TOUCH_FLAG_REALWINDOW_XY, evt_flags, x, y, 1024, ti->dwID, wm );
        	    if( ti->dwFlags & TOUCHEVENTF_PRIMARY )
        		wm->ignore_mouse_evts = true;
        	}
        	if( ti->dwFlags & TOUCHEVENTF_UP )
        	{
        	    collect_touch_events( 2, TOUCH_FLAG_REALWINDOW_XY, g_mod, x, y, 1024, ti->dwID, wm );
        	    if( ti->dwFlags & TOUCHEVENTF_PRIMARY )
        	    {
        		if( wm->ignore_mouse_evts )
        		{
        		    wm->ignore_mouse_evts = false;
        		    wm->ignore_mouse_evts_before = time_ticks() + time_ticks_per_second();
        		}
        	    }
        	}
            }
            send_touch_events( wm );
            bHandled = TRUE;
        }
        else
        {
    	    /* handle the error here */
        }
        bmem_free( pInputs );
    }
    else
    {
        /* handle the error here, probably out of memory */
    }
    if( bHandled )
    {
        // if you handled the message, close the touch input handle and return
        CloseTouchInputHandle( (HTOUCHINPUT)lParam );
        return 0;
    }
    else
    {
        // if you didn't handle the message, let DefWindowProc handle it
        return DefWindowProc( hWnd, WM_TOUCH, wParam, lParam );
    }
}
#endif

#define GET_XY \
    x = lParam & 0xFFFF;\
    y = lParam >> 16;

bool mouse_evt_ignore( window_manager* wm )
{
    if( wm->ignore_mouse_evts ) return true;
    if( wm->ignore_mouse_evts_before )
    {
	if( (unsigned)( wm->ignore_mouse_evts_before - time_ticks() ) < time_ticks_per_second() * 10 )	
	    return true;
	else
	{
	    wm->ignore_mouse_evts_before = 0;
	    wm->ignore_mouse_evts = false;
	}
    }
    return false;
}

LRESULT APIENTRY
WinProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    int x, y, d;
    short d2;
    int key;
    POINT point;
    
    window_manager* wm = (window_manager*)GetWindowLongPtr( hWnd, GWLP_USERDATA );

    switch( message )
    {
	case WM_CREATE:
	    break;

	case WM_CLOSE:
	    if( wm )
	    {
		send_event( 0, EVT_QUIT, 0, 0, 0, 0, 0, 1024, 0, wm );
	    }
	    break;

	case WM_SIZE:
	    if( wm && LOWORD(lParam) != 0 && HIWORD(lParam) != 0 )
	    {
		int xsize = (int)LOWORD( lParam );
		int ysize = (int)HIWORD( lParam );
		if( xsize <= 64 && ysize <= 64 ) break; //Wine fix
#if defined(OPENGL)
		// track window size changes
		if( hGLRC ) 
		{
		    wm->real_window_width = xsize;
		    wm->real_window_height = ysize;
		    wm->screen_xsize = xsize / wm->screen_zoom;
		    wm->screen_ysize = ysize / wm->screen_zoom;
		    send_event( wm->root_win, EVT_SCREENRESIZE, 0, 0, 0, 0, 0, 1024, 0, wm );
            	    gl_resize( wm );
		    return 0;
		}
#endif
#ifdef GDI
		wm->screen_xsize = xsize / wm->screen_zoom;
		wm->screen_ysize = ysize / wm->screen_zoom;
		send_event( wm->root_win, EVT_SCREENRESIZE, 0, 0, 0, 0, 0, 1024, 0, wm );
#endif
	    }
	    break;

	case WM_PAINT:
	    if( wm )
	    {
		PAINTSTRUCT gdi_ps;
		BeginPaint( hWnd, &gdi_ps );
		EndPaint( hWnd, &gdi_ps );
		send_event( wm->root_win, EVT_DRAW, 0, 0, 0, 0, 0, 1024, 0, wm );
		return 0;
	    }
	    break;
	    
	case WM_MOUSEWHEEL:
	    if( wm )
	    {
		if( mouse_evt_ignore( wm ) ) break;
		GET_XY;
		point.x = x;
		point.y = y;
		ScreenToClient( hWnd, &point );
		x = point.x;
		y = point.y;
		convert_real_window_xy( x, y, wm );
		key = 0;
		d = (unsigned int)wParam >> 16;
		d2 = (short)d;
		if( d2 < 0 ) key = MOUSE_BUTTON_SCROLLDOWN;
		if( d2 > 0 ) key = MOUSE_BUTTON_SCROLLUP;
		send_event( 0, EVT_MOUSEBUTTONDOWN, g_mod, x, y, key, 0, 1024, 0, wm );
	    }
	    break;	    
	case WM_LBUTTONDOWN:
	    if( wm )
	    {
		if( mouse_evt_ignore( wm ) ) break;
		GET_XY;
		convert_real_window_xy( x, y, wm );
		send_event( 0, EVT_MOUSEBUTTONDOWN, g_mod, x, y, MOUSE_BUTTON_LEFT, 0, 1024, 0, wm );
	    }
	    break;
	case WM_LBUTTONUP:
	    if( wm )
	    {
		if( mouse_evt_ignore( wm ) ) break;
		GET_XY;
		convert_real_window_xy( x, y, wm );
		send_event( 0, EVT_MOUSEBUTTONUP, g_mod, x, y, MOUSE_BUTTON_LEFT, 0, 1024, 0, wm );
	    }
	    break;
	case WM_MBUTTONDOWN:
	    if( wm )
	    {
		if( mouse_evt_ignore( wm ) ) break;
		GET_XY;
		convert_real_window_xy( x, y, wm );
		send_event( 0, EVT_MOUSEBUTTONDOWN, g_mod, x, y, MOUSE_BUTTON_MIDDLE, 0, 1024, 0, wm );
	    }
	    break;
	case WM_MBUTTONUP:
	    if( wm )
	    {
		if( mouse_evt_ignore( wm ) ) break;
		GET_XY;
		convert_real_window_xy( x, y, wm );
		send_event( 0, EVT_MOUSEBUTTONUP, g_mod, x, y, MOUSE_BUTTON_MIDDLE, 0, 1024, 0, wm );
	    }
	    break;
	case WM_RBUTTONDOWN:
	    if( wm )
	    {
		if( mouse_evt_ignore( wm ) ) break;
		GET_XY;
		convert_real_window_xy( x, y, wm );
		send_event( 0, EVT_MOUSEBUTTONDOWN, g_mod, x, y, MOUSE_BUTTON_RIGHT, 0, 1024, 0, wm );
	    }
	    break;
	case WM_RBUTTONUP:
	    if( wm )
	    {
		if( mouse_evt_ignore( wm ) ) break;
		GET_XY;
		convert_real_window_xy( x, y, wm );
		send_event( 0, EVT_MOUSEBUTTONUP, g_mod, x, y, MOUSE_BUTTON_RIGHT, 0, 1024, 0, wm );
	    }
	    break;
	case WM_MOUSEMOVE:
	    if( wm )
	    {
		if( mouse_evt_ignore( wm ) ) break;
		GET_XY;
		convert_real_window_xy( x, y, wm );
		key = 0;
		if( wParam & MK_LBUTTON ) key |= MOUSE_BUTTON_LEFT;
		if( wParam & MK_MBUTTON ) key |= MOUSE_BUTTON_MIDDLE;
		if( wParam & MK_RBUTTON ) key |= MOUSE_BUTTON_RIGHT;
		send_event( 0, EVT_MOUSEMOVE, g_mod, x, y, key, 0, 1024, 0, wm );
	    }
	    break;
	    
#ifdef MULTITOUCH
	case WM_TOUCH:
	    if( wm )
	    {
		OnTouch( hWnd, wParam, lParam, wm );
	    }
	    break;
#endif
	    
	case WM_KEYDOWN:
	    if( wm )
	    {
		if( wParam == VK_SHIFT )
		    g_mod |= EVT_FLAG_SHIFT;
		if( wParam == VK_CONTROL )
		    g_mod |= EVT_FLAG_CTRL;
		if( wParam == VK_MENU )
		    g_mod |= EVT_FLAG_ALT;
		key = win_key_to_sundog_key( wParam, lParam );
		if( key ) 
		{
		    send_event( 0, EVT_BUTTONDOWN, g_mod, 0, 0, key, ( lParam >> 16 ) & 511, 1024, 0, wm );
		}
	    }
	    break;
	case WM_KEYUP:
	    if( wm )
	    {
		if( wParam == VK_SHIFT )
		    g_mod &= ~EVT_FLAG_SHIFT;
		if( wParam == VK_CONTROL )
		    g_mod &= ~EVT_FLAG_CTRL;
		if( wParam == VK_MENU )
		    g_mod &= ~EVT_FLAG_ALT;
		key = win_key_to_sundog_key( wParam, lParam );
		if( key ) 
		{
		    send_event( 0, EVT_BUTTONUP, g_mod, 0, 0, key, ( lParam >> 16 ) & 511, 1024, 0, wm );
		}
	    }
	    break;
	    
	case WM_KILLFOCUS:
	    if( wm )
	    {
		g_mod = 0;
	    }
	    break;

	default:
	    return DefWindowProc( hWnd, message, wParam, lParam );
	    break;
    }

    return 0;
}

static int CreateWin( HINSTANCE hCurrentInst, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow, window_manager* wm )
{
    utf16_char windowName_utf16[ 256 ];
    utf8_to_utf16( windowName_utf16, 256, windowName );
    wcstombs( windowName_ansi, (const wchar_t*)windowName_utf16, 256 );

    //Register window class:
#ifdef DIRECTDRAW
    wndClass.style = CS_BYTEALIGNCLIENT;
#else
    wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
#endif
    wndClass.lpfnWndProc = WinProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hCurrentInst;
    wndClass.hIcon = LoadIcon( hCurrentInst, (LPCTSTR)IDI_ICON1 );
    wndClass.hCursor = LoadCursor( NULL, IDC_ARROW );
    wndClass.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = className;
    RegisterClass( &wndClass );

    //Create window:
    RECT Rect;
    Rect.top = 0;
    Rect.bottom = wm->screen_ysize;
    Rect.left = 0;
    Rect.right = wm->screen_xsize;
    uint WS = 0;
    uint WS_EX = 0;
    if( win_flags & WIN_INIT_FLAG_SCALABLE )
	WS = WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    else
	WS = WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
    if( win_flags & WIN_INIT_FLAG_NOBORDER )
	WS = WS_POPUP;
#ifdef DIRECTDRAW
    WS = WS_POPUP;
    WS_EX = WS_EX_TOPMOST;
#else
    if( wm->flags & WIN_INIT_FLAG_FULLSCREEN )
    {
	WS_EX = WS_EX_APPWINDOW;
        WS = WS_POPUP;
    	Rect.right = GetSystemMetrics( SM_CXSCREEN );
    	Rect.bottom = GetSystemMetrics( SM_CYSCREEN );
    	wm->screen_xsize = Rect.right;
    	wm->screen_ysize = Rect.bottom;
    	wm->real_window_width = wm->screen_xsize;
    	wm->real_window_height = wm->screen_ysize;
    }
#endif
    AdjustWindowRectEx( &Rect, WS, 0, WS_EX );
    hWnd = CreateWindowEx(
	WS_EX,
        className, windowName_ansi,
        WS,
        ( GetSystemMetrics( SM_CXSCREEN ) - ( Rect.right - Rect.left ) ) / 2, 
        ( GetSystemMetrics( SM_CYSCREEN ) - ( Rect.bottom - Rect.top ) ) / 2, 
        Rect.right - Rect.left, Rect.bottom - Rect.top,
        NULL, NULL, hCurrentInst, NULL
    );

    //Init WM pointer:
    wm->hwnd = hWnd;
    wm->hdc = GetDC( hWnd );
    SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)wm );
    
#ifdef MULTITOUCH
    //Init touch device:
    int v = GetSystemMetrics( SM_DIGITIZER );
    if( v & NID_READY )
    {
	RegisterTouchWindow( hWnd, 0 );
    }
#endif
    
    //Display window:
    ShowWindow( hWnd, nCmdShow );
    UpdateWindow( hWnd );

#ifdef OPENGL
    SetupPixelFormat( wm->hdc, wm );
    hGLRC = wglCreateContext( wm->hdc );
    wglMakeCurrent( wm->hdc, hGLRC );
    if( gl_init( wm ) )
	return 1; //error
    gl_resize( wm );
#endif
#ifdef DIRECTDRAW
    if( dd_init( fullscreen, wm ) ) 
	return 1; //error
#endif

    return 0;
}

void device_screen_unlock( WINDOWPTR win, window_manager* wm )
{
    if( wm->screen_lock_counter == 1 )
    {
#ifdef DIRECTDRAW
	if( g_primary_locked )
	{
	    lpDDSPrimary->Unlock( g_sd.lpSurface );
	    g_primary_locked = 0;
	    wm->fb = 0;
	}
#endif //DIRECTDRAW
	gl_unlock( wm );
    }

    if( wm->screen_lock_counter > 0 )
    {
	wm->screen_lock_counter--;
    }

    if( wm->screen_lock_counter > 0 ) 
	wm->screen_is_active = true;
    else
	wm->screen_is_active = false;
}

void device_screen_lock( WINDOWPTR win, window_manager* wm )
{
    if( wm->screen_lock_counter == 0 )
    {
#ifdef DIRECTDRAW
	if( g_primary_locked == 0 )
	{
	    if( lpDDSPrimary )
	    {
		lpDDSPrimary->Lock( 0, &g_sd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, 0 );
		wm->fb_ypitch = g_sd.lPitch / COLORLEN;
		wm->fb = (COLORPTR)g_sd.lpSurface;
		g_primary_locked = 1;
	    }
	}
#endif //DIRECTDRAW
	gl_lock( wm );
    }
    wm->screen_lock_counter++;
    
    if( wm->screen_lock_counter > 0 ) 
	wm->screen_is_active = true;
    else
	wm->screen_is_active = false;
}

#ifndef OPENGL

void device_vsync( bool vsync, window_manager* wm )
{
}

void device_redraw_framebuffer( window_manager* wm ) 
{
}

void device_draw_line( int x1, int y1, int x2, int y2, COLOR color, window_manager* wm )
{
    if( wm->hdc == 0 ) return;
    HPEN pen;
    pen = CreatePen( PS_SOLID, 1, RGB( red( color ), green( color ), blue( color ) ) );
    SelectObject( wm->hdc, pen );
    MoveToEx( wm->hdc, x1, y1, 0 );
    LineTo( wm->hdc, x2, y2 );
    SetPixel( wm->hdc, x2, y2, RGB( red( color ), green( color ), blue( color ) ) );
    DeleteObject( pen );
}

void device_draw_frect( int x, int y, int xsize, int ysize, COLOR color, window_manager* wm )
{
    if( wm->hdc == 0 ) return;
    if( xsize == 1 && ysize == 1 )
    {
	SetPixel( wm->hdc, x, y, RGB( red( color ), green( color ), blue( color ) ) );
    }
    else
    {
	HPEN pen = CreatePen( PS_SOLID, 1, RGB( red( color ), green( color ), blue( color ) ) );
	HBRUSH brush = CreateSolidBrush( RGB( red( color ), green( color ), blue( color ) ) );
	SelectObject( wm->hdc, pen );
	SelectObject( wm->hdc, brush );
	Rectangle( wm->hdc, x, y, x + xsize, y + ysize );
	DeleteObject( brush );
	DeleteObject( pen );
    }
}

void device_draw_image( 
    int dest_x, int dest_y, 
    int dest_xs, int dest_ys,
    int src_x, int src_y,
    sundog_image* img,
    window_manager* wm )
{
    if( wm->hdc == 0 ) return;
    BITMAPINFO* bi = (BITMAPINFO*)wm->gdi_bitmap_info;
    if( wm->gdi_bitmap_info[ 0 ] == 888 )
    {
	memset( bi, 0, sizeof( wm->gdi_bitmap_info ) );
	//Set 256 colors palette:
	int a;
#ifdef GRAYSCALE
	for( a = 0; a < 256; a++ ) 
	{ 
    	    bi->bmiColors[ a ].rgbRed = a; 
    	    bi->bmiColors[ a ].rgbGreen = a; 
    	    bi->bmiColors[ a ].rgbBlue = a; 
	}
#else
	for( a = 0; a < 256; a++ ) 
	{ 
    	    bi->bmiColors[ a ].rgbRed = (a<<5)&224; 
	    if( bi->bmiColors[ a ].rgbRed ) 
		bi->bmiColors[ a ].rgbRed |= 0x1F; 
	    bi->bmiColors[ a ].rgbReserved = 0;
	}
	for( a = 0; a < 256; a++ )
	{
	    bi->bmiColors[ a ].rgbGreen = (a<<2)&224; 
	    if( bi->bmiColors[ a ].rgbGreen ) 
		bi->bmiColors[ a ].rgbGreen |= 0x1F; 
	}
	for( a = 0; a < 256; a++ ) 
	{ 
	    bi->bmiColors[ a ].rgbBlue = (a&192);
	    if( bi->bmiColors[ a ].rgbBlue ) 
		bi->bmiColors[ a ].rgbBlue |= 0x3F; 
	}
#endif
    }
    int src_xs = img->xsize;
    int src_ys = img->ysize;
    COLORPTR data = (COLORPTR)img->data;
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = src_xs;
    bi->bmiHeader.biHeight = -src_ys;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = COLORBITS;
    bi->bmiHeader.biCompression = BI_RGB;
    SetDIBitsToDevice(  
	wm->hdc,
	dest_x, // Destination top left hand corner X Position
	dest_y, // Destination top left hand corner Y Position
	dest_xs, // Destinations Width
	dest_ys, // Desitnations height
	src_x, // Source low left hand corner's X Position
	src_ys - ( src_y + dest_ys ), // Source low left hand corner's Y Position
	0,
	src_ys,
	data, // Source's data
	(BITMAPINFO*)wm->gdi_bitmap_info, // Bitmap Info
	DIB_RGB_COLORS );
}

#endif //!OPENGL
