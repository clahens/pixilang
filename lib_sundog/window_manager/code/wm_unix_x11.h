/*
    wm_unix_x11.h. Platform-dependent module : X11 + OpenGL
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

#pragma once

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>   	//for getenv()
#include <sched.h>	//for sched_yield()

XSetWindowAttributes g_swa;
Colormap g_cmap;
int g_depth;
int g_xscreen;
int g_auto_repeat = 0;
Atom g_wm_delete_message;

int g_mod = 0;

int g_frame_len;
ticks_t g_ticks_per_frame;
ticks_t g_prev_frame_time = 0;

#ifdef OPENGL
#ifdef OPENGLES
#ifdef RASPBERRY_PI
    #include "bcm_host.h"
#endif
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
#else
    GLXContext gl_context;
    static int snglBuf[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, None };
    static int dblBuf[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
    XVisualInfo* vi;
#endif
    #include "wm_opengl.h"
#else
    Visual* vi;
#endif

uint local_colors[ 32 * 32 * 32 ];
uchar* temp_bitmap = 0;
size_t temp_bitmap_size = 0;
XImage* last_ximg = 0;

uint g_flags = 0;

int g_mod_state = 0;

int device_start( const utf8_char* windowname, int xsize, int ysize, window_manager* wm )
{
    int retval = 0;
    
    XInitThreads();

    g_depth = 0;
    g_auto_repeat = 0;
    g_mod = 0;
    g_prev_frame_time = 0;
    temp_bitmap = 0;
    temp_bitmap_size = 0;
    last_ximg = 0;
    g_flags = 0;
    g_mod_state = 0;
    
    g_frame_len = 1000 / wm->max_fps;
    g_ticks_per_frame = time_ticks_per_second() / wm->max_fps;

    for( int lc = 0; lc < 32 * 32 * 32; lc++ ) local_colors[ lc ] = 0xFFFFFFFF;

    g_flags = wm->flags;
    
#if defined(OPENGL)
    xsize = profile_get_int_value( KEY_SCREENX, xsize, 0 );
    ysize = profile_get_int_value( KEY_SCREENY, ysize, 0 );
#endif
#ifdef X11
    xsize = profile_get_int_value( KEY_SCREENX, xsize, 0 );
    ysize = profile_get_int_value( KEY_SCREENY, ysize, 0 );
#endif
    wm->real_window_width = xsize;
    wm->real_window_height = ysize;

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

    if( wm->screen_ppi == 0 ) wm->screen_ppi = 110;
    win_zoom_init( wm );

    bmem_set( &g_swa, sizeof( XSetWindowAttributes ), 0 );
    g_swa.event_mask = 
	( 0
	| ExposureMask 
	| ButtonPressMask 
	| ButtonReleaseMask 
	| PointerMotionMask 
	| StructureNotifyMask 
	| KeyPressMask 
	| KeyReleaseMask 
	| FocusChangeMask 
	| StructureNotifyMask
	);
    
    //Open a connection to the X server:
    const char* name;
    if( ( name = getenv( "DISPLAY" ) ) == NULL )
	name = ":0";
    wm->dpy = XOpenDisplay( name );
    if( wm->dpy == NULL ) 
    {
    	blog( "could not open display\n" );
	return 1;
    }
#ifndef OPENGL
    //Simple X11 init (not GLX) :
    g_xscreen = XDefaultScreen( wm->dpy );
    vi = XDefaultVisual( wm->dpy, g_xscreen ); wm->win_visual = vi; if( !vi ) blog( "XDefaultVisual error\n" );
    g_depth = XDefaultDepth( wm->dpy, g_xscreen ); wm->win_depth = g_depth; if( !g_depth ) blog( "XDefaultDepth error\n" );
    blog( "depth = %d\n", g_depth );
    wm->cmap = g_cmap = XDefaultColormap( wm->dpy, g_xscreen );
    g_swa.colormap = g_cmap;
    wm->win = XCreateWindow( wm->dpy, XDefaultRootWindow( wm->dpy ), 0, 0, xsize, ysize, 0, CopyFromParent, InputOutput, vi, CWBorderPixel | CWColormap | CWEventMask, &g_swa );
    XSetStandardProperties( wm->dpy, wm->win, windowname, windowname, None, wm->sd->argv, wm->sd->argc, NULL );
    XMapWindow( wm->dpy, wm->win ); //Request the X window to be displayed on the screen
    wm->win_gc = XDefaultGC( wm->dpy, g_xscreen ); if( !wm->win_gc ) blog( "XDefaultGC error\n" );
#endif //!OPENGL

#ifdef OPENGL
#ifdef OPENGLES
    //OpenGLES:

#ifdef RASPBERRY_PI
    bcm_host_init();
#endif

    wm->win = XCreateWindow( wm->dpy, XDefaultRootWindow( wm->dpy ), 0, 0, xsize, ysize, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &g_swa );
    
    XSetWindowAttributes xattr;
    bmem_set( &xattr, sizeof( xattr ), 0 );
    Atom atom;
    int one = 1;
    
    xattr.override_redirect = 0;
    XChangeWindowAttributes( wm->dpy, wm->win, CWOverrideRedirect, &xattr );

#if defined(MAEMO)
    atom = XInternAtom( wm->dpy, "_NET_WM_STATE_FULLSCREEN", 1 );
    XChangeProperty(
	wm->dpy, wm->win,
	XInternAtom( wm->dpy, "_NET_WM_STATE", 1 ),
	XA_ATOM, 32, PropModeReplace,
	(unsigned char*)&atom, 1 );
    
    XChangeProperty(
	wm->dpy, wm->win,
	XInternAtom( wm->dpy, "_HILDON_NON_COMPOSITED_WINDOW", 1 ),
	XA_INTEGER, 32, PropModeReplace,
	(unsigned char*)&one, 1 );
#endif
    
    XMapWindow( wm->dpy, wm->win ); //Make the window visible on the screen
    XStoreName( wm->dpy, wm->win, windowname ); //Give the window a name
    
#if defined(MAEMO)
    //Get identifiers for the provided atom name strings:
    Atom wm_state   = XInternAtom( wm->dpy, "_NET_WM_STATE", 0 );
    Atom fullscreen = XInternAtom( wm->dpy, "_NET_WM_STATE_FULLSCREEN", 0 );
    
    XEvent xev;
    memset( &xev, 0, sizeof( xev ) );
    
    xev.type                 = ClientMessage;
    xev.xclient.window       = wm->win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format       = 32;
    xev.xclient.data.l[ 0 ]  = 1;
    xev.xclient.data.l[ 1 ]  = fullscreen;
    XSendEvent(
	wm->dpy,
	DefaultRootWindow( wm->dpy ),
	0,
	SubstructureNotifyMask,
	&xev );
#endif

    //Init OpenGLES:

#ifdef RASPBERRY_PI
    egl_display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
#else
    egl_display = eglGetDisplay( (EGLNativeDisplayType)wm->dpy );
#endif
    if( egl_display == EGL_NO_DISPLAY ) 
    {
	blog( "Got no EGL display.\n" );
	return 1;
    }
    
    if( !eglInitialize( egl_display, NULL, NULL ) ) 
    {
	blog( "Unable to initialize EGL\n" );
	return 1;
    }
    
    //Some attributes to set up our egl-interface:
    EGLint attr[] = 
    {
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_NONE
    };
    
    EGLConfig ecfg;
    EGLint num_config;
    if( !eglChooseConfig( egl_display, attr, &ecfg, 1, &num_config ) ) 
    {
	blog( "Failed to choose config (eglError: %d)\n", eglGetError() );
	return 1;
    }
    
    if( num_config != 1 ) 
    {
	blog( "Didn't get exactly one config, but %d\n", num_config );
	return 1;
    }

#ifdef RASPBERRY_PI
    static EGL_DISPMANX_WINDOW_T nativewindow;
    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;
    uint32_t screen_width;
    uint32_t screen_height;
    EGLint gles2_attrib[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    egl_context = eglCreateContext( egl_display, ecfg, EGL_NO_CONTEXT, gles2_attrib );
    if( egl_context == EGL_NO_CONTEXT ) 
    {
	blog( "Unable to create EGL context (eglError: %d)\n", eglGetError() );
	return 1;
    }
    if( graphics_get_display_size( 0 /* LCD */, &screen_width, &screen_height ) )
    {
	blog( "graphics_get_display_size() error\n" );
	return 1;
    }
    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = screen_width;
    dst_rect.height = screen_height;
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = screen_width << 16;
    src_rect.height = screen_height << 16;
    dispman_display = vc_dispmanx_display_open( 0 /* LCD */ );
    dispman_update = vc_dispmanx_update_start( 0 );
    dispman_element = vc_dispmanx_element_add( dispman_update, dispman_display,
	0 /*layer*/, &dst_rect, 0 /*src*/,
	&src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0 /*clamp*/, (DISPMANX_TRANSFORM_T)0 /*transform*/ );
    nativewindow.element = dispman_element;
    nativewindow.width = screen_width;
    nativewindow.height = screen_height;
    vc_dispmanx_update_submit_sync( dispman_update );
    egl_surface = eglCreateWindowSurface( egl_display, ecfg, &nativewindow, NULL );
    if( egl_surface == EGL_NO_SURFACE ) 
    {
	blog( "Unable to create EGL surface (eglError: %d)\n", eglGetError() );
	return 1;
    }
#else
    egl_surface = eglCreateWindowSurface( egl_display, ecfg, wm->win, NULL );
    if( egl_surface == EGL_NO_SURFACE ) 
    {
	blog( "Unable to create EGL surface (eglError: %d)\n", eglGetError() );
	return 1;
    }
    //egl-contexts collect all state descriptions needed required for operation:
    eglBindAPI( EGL_OPENGL_ES_API );
    EGLint ctxattr[] =
    {
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
    };
    egl_context = eglCreateContext( egl_display, ecfg, EGL_NO_CONTEXT, ctxattr );
    if( egl_context == EGL_NO_CONTEXT ) 
    {
	blog( "Unable to create EGL context (eglError: %d)\n", eglGetError() );
	return 1;
    }
#endif

    //Associate the egl-context with the egl-surface:
    eglMakeCurrent( egl_display, egl_surface, egl_surface, egl_context );

#else
    //OpenGL:

    //Make sure OpenGL's GLX extension supported:
    int dummy;
    if( !glXQueryExtension( wm->dpy, &dummy, &dummy ) ) 
    {
    	blog( "X server has no OpenGL GLX extension\n" );
	return 1;
    }

    //Find an appropriate visual:
    //Find an OpenGL-capable RGB visual with depth buffer:
    vi = glXChooseVisual( wm->dpy, DefaultScreen( wm->dpy ), dblBuf );
    if( vi == 0 ) 
    {
    	vi = glXChooseVisual( wm->dpy, DefaultScreen( wm->dpy ), snglBuf );
	if( vi == NULL ) 
	{
	    blog( "No RGB visual with depth buffer.\n" );
	}
    }

    //Create an OpenGL rendering context:
    gl_context = glXCreateContext( wm->dpy, vi, /* no sharing of display lists */ None, /* direct rendering if possible */ GL_TRUE );
    if( gl_context == NULL ) 
    {
	blog( "Could not create rendering context.\n" );
	return 1;
    }

    //Create an X window with the selected visual:
    //Create an X colormap since probably not using default visual:
    g_cmap = XCreateColormap( wm->dpy, RootWindow( wm->dpy, vi->screen ), vi->visual, AllocNone );
    g_swa.colormap = g_cmap;
    wm->win = XCreateWindow( wm->dpy, RootWindow( wm->dpy, vi->screen ), 0, 0, xsize, ysize, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &g_swa );

    XSetStandardProperties( wm->dpy, wm->win, windowname, windowname, None, wm->sd->argv, wm->sd->argc, NULL );
    XMapWindow( wm->dpy, wm->win ); //Request the X window to be displayed on the screen
    
    //Bind the rendering context to the window:
    if( glXMakeCurrent( wm->dpy, wm->win, gl_context ) == 0 )
	blog( "glXMakeCurrent() ERROR %d\n", glGetError() );

#endif
#endif //OPENGL

    if( wm->flags & WIN_INIT_FLAG_FULLSCREEN )
    {
	XWindowAttributes xwa;
	XGetWindowAttributes( wm->dpy, wm->win, &xwa );
	XEvent e = { 0 };
	e.xclient.type = ClientMessage;
	e.xclient.message_type = XInternAtom( wm->dpy, "_NET_WM_STATE", false );
	e.xclient.display = wm->dpy;
	e.xclient.window = wm->win;
	e.xclient.format = 32;
	e.xclient.data.l[ 0 ] = 1;
	e.xclient.data.l[ 1 ] = XInternAtom( wm->dpy, "_NET_WM_STATE_FULLSCREEN", false );
	XSendEvent( wm->dpy, xwa.root, false, SubstructureNotifyMask | SubstructureRedirectMask, &e );
    }
    
    //Register interest in the delete window message:
    g_wm_delete_message = XInternAtom( wm->dpy, "WM_DELETE_WINDOW", false );
    XSetWMProtocols( wm->dpy, wm->win, &g_wm_delete_message, 1 );
    
    if( profile_get_int_value( KEY_NOCURSOR, -1, 0 ) != -1 )
    {
	//Hide mouse pointer:
	Cursor cursor;
	Pixmap blank;
	XColor dummy;
	static char data[] = { 0 };
	blank = XCreateBitmapFromData( wm->dpy, wm->win, data, 1, 1 );
	cursor = XCreatePixmapCursor( wm->dpy, blank, blank, &dummy, &dummy, 0, 0 );
	XDefineCursor( wm->dpy, wm->win, cursor );
	XFreePixmap( wm->dpy, blank );
    }

    XkbSetDetectableAutoRepeat( wm->dpy, 1, &g_auto_repeat );
    
    //XGrabKeyboard( wm->dpy, wm->win, 1, GrabModeAsync, GrabModeAsync, CurrentTime );
    //XGrabKey( wm->dpy, 0, ControlMask, wm->win, 1, 1, GrabModeSync );
    
#ifdef MULTITOUCH
    if( XQueryExtension( wm->dpy, INAME, &wm->xi_opcode, &wm->xi_event, &wm->xi_error ) ) 
    {
	wm->xi = true;
	XIGrabModifiers modifiers;
	XIEventMask mask;
	mask.deviceid = XIAllMasterDevices;
	mask.mask_len = XIMaskLen( XI_LASTEVENT );
	mask.mask = (unsigned char*)calloc( mask.mask_len, 1 );
	XISetMask( mask.mask, XI_TouchBegin );
        XISetMask( mask.mask, XI_TouchUpdate );
	XISetMask( mask.mask, XI_TouchEnd );
	XISelectEvents( wm->dpy, wm->win, &mask, 1 );
    }
#endif
    
#ifdef OPENGL
    #ifdef GLNORETAIN
	wm->screen_buffer_preserved = 0;
    #else
	wm->screen_buffer_preserved = 1;
    #endif
    if( gl_init( wm ) ) return 1;
    gl_resize( wm );
#endif
    
#ifdef FRAMEBUFFER
    //Create framebuffer:
    wm->fb = (COLORPTR)bmem_new( wm->screen_xsize * wm->screen_ysize * COLORLEN );
    wm->fb_xpitch = 1;
    wm->fb_ypitch = wm->screen_xsize;
#endif

    return retval;
}

void device_end( window_manager* wm )
{
    int temp;

    //blog( "device_end(): stage 1\n" );
    XkbSetDetectableAutoRepeat( wm->dpy, g_auto_repeat, &temp );
#ifdef OPENGL
    //blog( "device_end(): stage 2\n" );
    gl_deinit( wm );
#ifdef OPENGLES
    eglMakeCurrent( egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroyContext( egl_display, egl_context );
    eglDestroySurface( egl_display, egl_surface );
    eglTerminate( egl_display );
#ifdef RASPBERRY_PI
    bcm_host_deinit();
#endif
#endif
#else
    //blog( "device_end(): stage 3\n" );
    if( last_ximg ) 
    {
	XDestroyImage( last_ximg );
	last_ximg = 0;
    }
    if( temp_bitmap )
    {
	temp_bitmap = 0;
    }
#endif
    //blog( "device_end(): stage 4\n" );
    XDestroyWindow( wm->dpy, wm->win );
    //blog( "device_end(): stage 5\n" );
    XCloseDisplay( wm->dpy );

#ifdef FRAMEBUFFER
    bmem_free( wm->fb );
#endif
}

void device_event_handler( window_manager* wm )
{
    int old_screen_x, old_screen_y;
    int old_char_x, old_char_y;
    int x, y, window_num;
    int key = 0;
    XEvent evt;
    KeySym sym;
    XSizeHints size_hints;
    int pend = XPending( wm->dpy );
    if( pend )
    {
	XNextEvent( wm->dpy, &evt );
        switch( evt.type ) 
	{
	    case FocusIn:
		//XGrabKeyboard( wm->dpy, wm->win, 1, GrabModeAsync, GrabModeAsync, CurrentTime ); 
		break;

	    case FocusOut:
		//XUngrabKeyboard( wm->dpy, CurrentTime );
		if( g_mod & EVT_FLAG_SHIFT ) send_event( 0, EVT_BUTTONUP, 0, 0, 0, KEY_SHIFT, 0, 1024, 0, wm );
		if( g_mod & EVT_FLAG_CTRL ) send_event( 0, EVT_BUTTONUP, 0, 0, 0, KEY_CTRL, 0, 1024, 0, wm );
		if( g_mod & EVT_FLAG_ALT ) send_event( 0, EVT_BUTTONUP, 0, 0, 0, KEY_ALT, 0, 1024, 0, wm );
		if( g_mod & EVT_FLAG_CMD ) send_event( 0, EVT_BUTTONUP, 0, 0, 0, KEY_CMD, 0, 1024, 0, wm );
		g_mod = 0;
		break;
		
	    case MapNotify:
	    case Expose:
		send_event( wm->root_win, EVT_DRAW, 0, 0, 0, 0, 0, 1024, 0, wm );
		break;
	
	    case DestroyNotify:
		send_event( 0, EVT_QUIT, 0, 0, 0, 0, 0, 1024, 0, wm );
		break;
		
	    case ClientMessage:
		if( evt.xclient.data.l[ 0 ] == g_wm_delete_message )
		{
		    send_event( 0, EVT_QUIT, 0, 0, 0, 0, 0, 1024, 0, wm );
		}
		break;
		
	    case ConfigureNotify:
#ifndef OPENGL
		if( !( g_flags & WIN_INIT_FLAG_SCALABLE ) )
		{
		    size_hints.flags = PMinSize;
		    int xsize = wm->screen_xsize;
		    int ysize = wm->screen_ysize;
    	    	    if( wm->screen_angle & 1 )
		    {
			int temp = xsize;
			xsize = ysize;
			ysize = temp;
		    }
		    size_hints.min_width = xsize;
		    size_hints.min_height = ysize;
		    size_hints.max_width = xsize;
		    size_hints.max_height = ysize;
		    XSetNormalHints( wm->dpy, wm->win, &size_hints );
		}
#endif
		if( g_flags & WIN_INIT_FLAG_SCALABLE )
		{
		    wm->real_window_width = evt.xconfigure.width;
	    	    wm->real_window_height = evt.xconfigure.height;
		    wm->screen_xsize = wm->real_window_width / wm->screen_zoom;
    	    	    wm->screen_ysize = wm->real_window_height / wm->screen_zoom;
    	    	    if( wm->screen_angle & 1 )
		    {
			int temp = wm->screen_xsize;
		        wm->screen_xsize = wm->screen_ysize;
    			wm->screen_ysize = temp;
		    }
		    send_event( wm->root_win, EVT_SCREENRESIZE, 0, 0, 0, 0, 0, 1024, 0, wm );
		}
#ifdef OPENGL
		gl_resize( wm );
#endif
		break;
		
	    case MotionNotify:
		if( evt.xmotion.window != wm->win ) break;
		x = evt.xmotion.x;
		y = evt.xmotion.y;
		convert_real_window_xy( x, y, wm );
		key = 0;
		if( evt.xmotion.state & Button1Mask )
                    key |= MOUSE_BUTTON_LEFT;
                if( evt.xmotion.state & Button2Mask )
                    key |= MOUSE_BUTTON_MIDDLE;
                if( evt.xmotion.state & Button3Mask )
                    key |= MOUSE_BUTTON_RIGHT;
		send_event( 0, EVT_MOUSEMOVE, g_mod, x, y, key, 0, 1024, 0, wm );
		break;
		
	    case ButtonPress:
	    case ButtonRelease:
		if( evt.xmotion.window != wm->win ) break;
		x = evt.xbutton.x;
		y = evt.xbutton.y;
		convert_real_window_xy( x, y, wm );
		/* Get pressed button */
	    	if( evt.xbutton.button == 1 )
		    key = MOUSE_BUTTON_LEFT;
		else if( evt.xbutton.button == 2 )
		    key = MOUSE_BUTTON_MIDDLE;
		else if( evt.xbutton.button == 3 )
		    key = MOUSE_BUTTON_RIGHT;
		else if( ( evt.xbutton.button == 4 || evt.xbutton.button == 5 ) && evt.type == ButtonPress )
		{
		    if( evt.xbutton.button == 4 ) 
			key = MOUSE_BUTTON_SCROLLUP;
		    else
			key = MOUSE_BUTTON_SCROLLDOWN;
		}
		if( key )
		{
		    if( evt.type == ButtonPress )
			send_event( 0, EVT_MOUSEBUTTONDOWN, g_mod, x, y, key, 0, 1024, 0, wm );
		    else
			send_event( 0, EVT_MOUSEBUTTONUP, g_mod, x, y, key, 0, 1024, 0, wm );
		}
		break;
		
#ifdef MULTITOUCH
	    case GenericEvent:
		{
		    XGenericEventCookie* cookie = &evt.xcookie;
		    if( XGetEventData( wm->dpy, cookie ) )
		    {
			if( cookie->type == GenericEvent && cookie->extension == wm->xi_opcode )
			{
			    XIDeviceEvent* xevt = (XIDeviceEvent*)cookie->data;
			    x = xevt->event_x;
			    y = xevt->event_y;
			    switch( xevt->evtype )
			    {
    				case XI_TouchBegin:
    				    collect_touch_events( 0, TOUCH_FLAG_REALWINDOW_XY, g_mod, x, y, 1024, xevt->detail, wm );
    				    break;
			        case XI_TouchUpdate: 
    				    collect_touch_events( 1, TOUCH_FLAG_REALWINDOW_XY, g_mod, x, y, 1024, xevt->detail, wm );
			    	    break;
			        case XI_TouchEnd:
    				    collect_touch_events( 2, TOUCH_FLAG_REALWINDOW_XY, g_mod, x, y, 1024, xevt->detail, wm );
			    	    break;
			    }
			    send_touch_events( wm );
			}
			XFreeEventData( wm->dpy, cookie );
		    }
		}
		break;
#endif
		
	    case KeyPress:
	    case KeyRelease:
		sym = XKeycodeToKeysym( wm->dpy, evt.xkey.keycode, 0 );
		key = 0;
		if( sym == NoSymbol || sym == 0 ) break;
		switch( sym )
		{
		    case XK_F1: key = KEY_F1; break;
		    case XK_F2: key = KEY_F2; break;
		    case XK_F3: key = KEY_F3; break;
		    case XK_F4: key = KEY_F4; break;
		    case XK_F5: key = KEY_F5; break;
		    case XK_F6: key = KEY_F6; break;
		    case XK_F7: key = KEY_F7; break;
		    case XK_F8: key = KEY_F8; break;
		    case XK_F9: key = KEY_F9; break;
		    case XK_F10: key = KEY_F10; break;
		    case XK_F11: key = KEY_F11; break;
		    case XK_F12: key = KEY_F12; break;
		    case XK_BackSpace: key = KEY_BACKSPACE; break;
		    case XK_Tab: key = KEY_TAB; break;
		    case XK_Return: key = KEY_ENTER; break;
		    case XK_Escape: key = KEY_ESCAPE; break;
		    case XK_Left: key = KEY_LEFT; break;
		    case XK_Right: key = KEY_RIGHT; break;
		    case XK_Up: key = KEY_UP; break;
		    case XK_Down: key = KEY_DOWN; break;
		    case XK_Home: key = KEY_HOME; break;
		    case XK_End: key = KEY_END; break;
		    case XK_Page_Up: key = KEY_PAGEUP; break;
		    case XK_Page_Down: key = KEY_PAGEDOWN; break;
		    case XK_Delete: key = KEY_DELETE; break;
		    case XK_Insert: key = KEY_INSERT; break;
		    case XK_Caps_Lock: key = KEY_CAPS; break;
		    case XK_Shift_L: 
		    case XK_Shift_R:
			key = KEY_SHIFT;
			if( evt.type == KeyPress )
                            g_mod |= EVT_FLAG_SHIFT;
                        else
                            g_mod &= ~EVT_FLAG_SHIFT;
			break;
		    case XK_Control_L: 
		    case XK_Control_R:
			key = KEY_CTRL;
			if( evt.type == KeyPress )
                            g_mod |= EVT_FLAG_CTRL;
                        else
                            g_mod &= ~EVT_FLAG_CTRL;
			break;
		    case XK_Alt_L: 
		    case XK_Alt_R:
			key = KEY_ALT;
			if( evt.type == KeyPress )
                            g_mod |= EVT_FLAG_ALT;
                        else
                            g_mod &= ~EVT_FLAG_ALT;
			break;
		    case XK_Mode_switch: 
			if( evt.type == KeyPress )
                            g_mod |= EVT_FLAG_MODE;
                        else
                            g_mod &= ~EVT_FLAG_MODE;
			break;
		    default: 
			if( sym >= 0x20 && sym <= 0x7E ) 
			    key = sym;
			else
			    key = KEY_UNKNOWN + sym;
			break;
		}
		if( key )
		{
		    if( evt.type == KeyPress )
		    {
			send_event( 0, EVT_BUTTONDOWN, g_mod, 0, 0, key, 0, 1024, 0, wm );
		    }
		    else
		    {
			send_event( 0, EVT_BUTTONUP, g_mod, 0, 0, key, 0, 1024, 0, wm );
		    }
		}
		break;
	}
    } 
    else
    {
	//There are no X11 events
	if( wm->events_count == 0 ) 
	{
	    //And no WM events
	    if( wm->flags & WIN_INIT_FLAG_FULL_CPU_USAGE )
	    {
		sched_yield();
	    }
	    else
	    {
		time_sleep( g_frame_len );
	    }
	}
    }
    
    ticks_t cur_t = time_ticks();
    if( cur_t - g_prev_frame_time >= g_ticks_per_frame )
    {
	wm->frame_event_request = 1;
	g_prev_frame_time = cur_t;
    }
}

static void device_find_color( XColor* col, COLOR color, window_manager* wm )
{
    int r = red( color );
    int g = green( color );
    int b = blue( color );
    int p = ( r >> 3 ) | ( ( g >> 3 ) << 5 ) | ( ( b >> 3 ) << 10 );
    if( local_colors[ p ] == 0xFFFFFFFF )
    {
	col->red = r << 8;
	col->green = g << 8;
	col->blue = b << 8;
	XAllocColor( wm->dpy, wm->cmap, col );
	local_colors[ p ] = col->pixel;
    }
    else col->pixel = local_colors[ p ];
}

void device_screen_lock( WINDOWPTR win, window_manager* wm )
{
    if( wm->screen_lock_counter == 0 )
    {
        gl_lock( wm );
    }
    wm->screen_lock_counter++;

    if( wm->screen_lock_counter > 0 )
        wm->screen_is_active = true;
    else
        wm->screen_is_active = false;
}

void device_screen_unlock( WINDOWPTR win, window_manager* wm )
{
    if( wm->screen_lock_counter == 1 )
    {
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

#ifndef OPENGL

void device_vsync( bool vsync, window_manager* wm )
{
}

void device_redraw_framebuffer( window_manager* wm ) 
{
}

void device_draw_image( 
    int dest_x, int dest_y, 
    int dest_xs, int dest_ys,
    int src_x, int src_y,
    sundog_image* img,
    window_manager* wm )
{
    int src_xs = img->xsize;
    int src_ys = img->ysize;
    COLORPTR data = (COLORPTR)img->data;

    if( temp_bitmap == 0 )
    {
	temp_bitmap_size = 64 * 1024;
	temp_bitmap = (uchar*)malloc( temp_bitmap_size );
    }
    
    XImage* ximg;
    if( last_ximg )
    {
	ximg = last_ximg;
	ximg->width = src_xs;
	ximg->height = src_ys;
	ximg->bytes_per_line = ( ximg->bits_per_pixel / 8 ) * src_xs;
    }
    else
    {
	ximg = XCreateImage( wm->dpy, wm->win_visual, wm->win_depth, ZPixmap, 0, (char*)temp_bitmap, src_xs, src_ys, 8, 0 );
	last_ximg = ximg;
    }
    
    size_t new_temp_bitmap_size = ( ximg->bits_per_pixel / 8 ) * src_xs * src_ys;
    if( new_temp_bitmap_size > temp_bitmap_size )
    {
	new_temp_bitmap_size += 32 * 1024;
	temp_bitmap_size = new_temp_bitmap_size;
	temp_bitmap = (uchar*)realloc( temp_bitmap, temp_bitmap_size );
	ximg->data = (char*)temp_bitmap;
    }
    
    if( ximg->bits_per_pixel == COLORLEN )
    {
	bmem_copy( temp_bitmap, data, src_xs * src_ys * COLORLEN );
    }
    else
    {
	int pixel_len = ximg->bits_per_pixel / 8;
	uchar* bitmap = temp_bitmap + ( src_y * src_xs + src_x ) * pixel_len;
	int a = src_y * src_xs + src_x;
	for( int y = 0; y < dest_ys; y++ )
	{
	    for( int x = 0; x < dest_xs; x++ )
	    {
		COLOR c = data[ a ];
		int r = red( c );
		int g = green( c );
		int b = blue( c );
		int res;
		switch( pixel_len )
		{
		    case 1:
			*(bitmap++) = (uchar)( (r>>5) + ((g>>5)<<3) + (b>>6)<<6 ); 
			break;
		    case 2:
			res = (b>>3) + ((g>>2)<<5) + ((r>>3)<<11);
			*(bitmap++) = (uchar)(res & 255); *(bitmap++) = (uchar)(res >> 8); 
			break;
		    case 3:
			*(bitmap++) = (uchar)b; *(bitmap++) = (uchar)g; *(bitmap++) = (uchar)r; 
			break;
		    case 4:
			*(bitmap++) = (uchar)b; *(bitmap++) = (uchar)g; *(bitmap++) = (uchar)r; *(bitmap++) = 0;
			break;
		}
		a++;
	    }
	    int ystep = src_xs - dest_xs;
	    bitmap += ystep * pixel_len;
	    a += ystep;
	}
    }
    switch( XPutImage( wm->dpy, wm->win, wm->win_gc, ximg, src_x, src_y, dest_x, dest_y, dest_xs, dest_ys ) )
    {
	case BadDrawable: blog( "BadDrawable\n" ); break;
	case BadGC: blog( "BadGC\n" ); break;
	case BadMatch: blog( "BadMatch\n" ); break;
	case BadValue: blog( "BadValue\n" ); break;
    }
}

void device_draw_line( int x1, int y1, int x2, int y2, COLOR color, window_manager* wm )
{
    XColor col;
    device_find_color( &col, color, wm );
    XSetForeground( wm->dpy, wm->win_gc, col.pixel );
    XDrawLine( wm->dpy, wm->win, wm->win_gc, x1, y1, x2, y2 );
}

void device_draw_frect( int x, int y, int xsize, int ysize, COLOR color, window_manager* wm )
{
    XColor col;
    device_find_color( &col, color, wm );
    XSetForeground( wm->dpy, wm->win_gc, col.pixel );
    XFillRectangle( wm->dpy, wm->win, wm->win_gc, x, y, xsize, ysize );
}

#endif //!OPENGL
