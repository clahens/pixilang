/*
    wm_opengl.h. Platform-dependent module : OpenGL
    This file is part of the SunDog engine.
    Copyright (C) 2002 - 2016 Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
*/

#pragma once

#ifdef WIN
    //Framebuffer:
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = 0;
    PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = 0;
    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = 0;
    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = 0;
    //Shaders:
    PFNGLATTACHSHADERPROC glAttachShader = 0;
    PFNGLCOMPILESHADERPROC glCompileShader = 0;
    PFNGLCREATEPROGRAMPROC glCreateProgram = 0;
    PFNGLCREATESHADERPROC glCreateShader = 0;
    PFNGLDELETEPROGRAMPROC glDeleteProgram = 0;
    PFNGLDELETESHADERPROC glDeleteShader = 0;
    PFNGLDETACHSHADERPROC glDetachShader = 0;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = 0;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = 0;
    PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = 0;
    PFNGLGETPROGRAMIVPROC glGetProgramiv = 0;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = 0;
    PFNGLGETSHADERIVPROC glGetShaderiv = 0;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = 0;
    PFNGLLINKPROGRAMPROC glLinkProgram = 0;
    PFNGLSHADERSOURCEPROC glShaderSource = 0;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = 0;
    PFNGLUSEPROGRAMPROC glUseProgram = 0;
    PFNGLUNIFORM1FPROC glUniform1f = 0;
    PFNGLUNIFORM2FPROC glUniform2f = 0;
    PFNGLUNIFORM3FPROC glUniform3f = 0;
    PFNGLUNIFORM4FPROC glUniform4f = 0;
    PFNGLUNIFORM1IPROC glUniform1i = 0;
    PFNGLUNIFORM2IPROC glUniform2i = 0;
    PFNGLUNIFORM3IPROC glUniform3i = 0;
    PFNGLUNIFORM4IPROC glUniform4i = 0;
    PFNGLUNIFORM1FVPROC glUniform1fv = 0;
    PFNGLUNIFORM2FVPROC glUniform2fv = 0;
    PFNGLUNIFORM3FVPROC glUniform3fv = 0;
    PFNGLUNIFORM4FVPROC glUniform4fv = 0;
    PFNGLUNIFORM1IVPROC glUniform1iv = 0;
    PFNGLUNIFORM2IVPROC glUniform2iv = 0;
    PFNGLUNIFORM3IVPROC glUniform3iv = 0;
    PFNGLUNIFORM4IVPROC glUniform4iv = 0;
    PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv = 0;
    PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv = 0;
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = 0;
    PFNGLVALIDATEPROGRAMPROC glValidateProgram = 0;
    PFNGLVERTEXATTRIB1DPROC glVertexAttrib1d = 0;
    PFNGLVERTEXATTRIB1DVPROC glVertexAttrib1dv = 0;
    PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f = 0;
    PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv = 0;
    PFNGLVERTEXATTRIB1SPROC glVertexAttrib1s = 0;
    PFNGLVERTEXATTRIB1SVPROC glVertexAttrib1sv = 0;
    PFNGLVERTEXATTRIB2DPROC glVertexAttrib2d = 0;
    PFNGLVERTEXATTRIB2DVPROC glVertexAttrib2dv = 0;
    PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f = 0;
    PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv = 0;
    PFNGLVERTEXATTRIB2SPROC glVertexAttrib2s = 0;
    PFNGLVERTEXATTRIB2SVPROC glVertexAttrib2sv = 0;
    PFNGLVERTEXATTRIB3DPROC glVertexAttrib3d = 0;
    PFNGLVERTEXATTRIB3DVPROC glVertexAttrib3dv = 0;
    PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f = 0;
    PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv = 0;
    PFNGLVERTEXATTRIB3SPROC glVertexAttrib3s = 0;
    PFNGLVERTEXATTRIB3SVPROC glVertexAttrib3sv = 0;
    PFNGLVERTEXATTRIB4NBVPROC glVertexAttrib4Nbv = 0;
    PFNGLVERTEXATTRIB4NIVPROC glVertexAttrib4Niv = 0;
    PFNGLVERTEXATTRIB4NSVPROC glVertexAttrib4Nsv = 0;
    PFNGLVERTEXATTRIB4NUBPROC glVertexAttrib4Nub = 0;
    PFNGLVERTEXATTRIB4NUBVPROC glVertexAttrib4Nubv = 0;
    PFNGLVERTEXATTRIB4NUIVPROC glVertexAttrib4Nuiv = 0;
    PFNGLVERTEXATTRIB4NUSVPROC glVertexAttrib4Nusv = 0;
    PFNGLVERTEXATTRIB4BVPROC glVertexAttrib4bv = 0;
    PFNGLVERTEXATTRIB4DPROC glVertexAttrib4d = 0;
    PFNGLVERTEXATTRIB4DVPROC glVertexAttrib4dv = 0;
    PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f = 0;
    PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv = 0;
    PFNGLVERTEXATTRIB4IVPROC glVertexAttrib4iv = 0;
    PFNGLVERTEXATTRIB4SPROC glVertexAttrib4s = 0;
    PFNGLVERTEXATTRIB4SVPROC glVertexAttrib4sv = 0;
    PFNGLVERTEXATTRIB4UBVPROC glVertexAttrib4ubv = 0;
    PFNGLVERTEXATTRIB4UIVPROC glVertexAttrib4uiv = 0;
    PFNGLVERTEXATTRIB4USVPROC glVertexAttrib4usv = 0;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = 0;
    PFNGLACTIVETEXTUREPROC glActiveTexture = 0;
#endif

#ifdef ANDROID
    bool g_gl_no_shader_detach = false;
#endif

//
// Init/Deinit/...
//

int gl_default_shaders_init( window_manager* wm );
void gl_default_shaders_deinit( window_manager* wm );

void gl_error( const utf8_char* fn_name )
{
    blog( "OpenGL error %d in %s\n", glGetError(), fn_name );
}

//Use gl_lock/gl_unlock outside of the WM lock/unlock block only!

int gl_lock( window_manager* wm )
{
#ifdef OSX
    int err = CGLLockContext( (CGLContextObj)g_gl_context_obj );
    if( err != 0 )
    {
	blog( "CGLLockContext() error %d\n", err );
    }
    return 0;
#endif
#ifdef IPHONE
    iphone_sundog_gl_lock();
    return 0;
#endif
    if( wm == 0 ) return -1;
    bmutex_lock( &wm->gl_mutex );
    return 0;
}

void gl_unlock( window_manager* wm )
{
#ifdef OSX
    int err = CGLUnlockContext( (CGLContextObj)g_gl_context_obj );
    if( err != 0 )
    {
	blog( "CGLUnlockContext() error %d\n", err );
    }
    return;
#endif
#ifdef IPHONE
    iphone_sundog_gl_unlock();
    return;
#endif
    if( wm ) bmutex_unlock( &wm->gl_mutex );
}

#define WGL_GET_PROC_ADDRESS( type, name ) \
    name = (type)wglGetProcAddress( #name ); \
    if( name == 0 ) \
    { \
	blog( "OpenGL %s() not found\n", #name ); \
	utf8_char* ts = (utf8_char*)bmem_new( 4096 ); \
	sprintf( ts, "Function %s() not found. Please make sure that you have installed the latest video card drivers.", #name ); \
	MessageBox( hWnd, ts, "OpenGL Error", MB_ICONERROR | MB_OK ); \
	bmem_free( ts ); \
        break; \
    }

int gl_init( window_manager* wm )
{
    int rv = -1;
    gl_lock( wm );

    while( 1 )
    {

	//Get OpenGL information:
    
	const utf8_char* s1;
	const utf8_char* s2;
	s1 = (const utf8_char*)glGetString( GL_VERSION ); if( s1 == 0 ) s1 = "Unknown";
	s2 = (const utf8_char*)glGetString( GL_RENDERER ); if( s2 == 0 ) s2 = "Unknown";
#ifndef OPENGLES
	const utf8_char* s3;
	s3 = (const utf8_char*)glGetString( 0x8B8C ); if( s3 == 0 ) s3 = "Unknown"; //GL_SHADING_LANGUAGE_VERSION
	blog( "OpenGL version: %s. Renderer: %s. GLSL: %s.\n", s1, s2, s3 );
#else
	blog( "OpenGL version: %s. Renderer: %s.\n", s1, s2 );
#endif
#ifdef ANDROID
	if( bmem_strstr( s2, "Adreno" ) 
	    && ( bmem_strstr( s2, "330" ) || bmem_strstr( s2, "320" ) ) )
	{
	    if( ( g_android_version_nums[ 0 ] == 5 && g_android_version_nums[ 1 ] >= 1 ) || g_android_version_nums[ 0 ] > 5 )
	    {
		//Bug on Android devices with Adreno 330 / 320
		wm->gl_no_points = true;
		blog( "GL_POINTS disabled.\n" );
	    }
	}
	if( bmem_strstr( s2, "Tegra" ) )
	{
	    //6.11.2015:
	    //Bug on Android devices with NVIDIA Tegra + NativeActivity:
	    //alpha blending will not work properly, if you detach the shaders here
	    g_gl_no_shader_detach = true;
	    blog( "Shaders detaching disabled.\n" );
	}
#endif
    
	//Get some additional functions:
    
#ifdef WIN
        wm->wglSwapIntervalEXT = 0;
	const char* ext = (const char*)glGetString( GL_EXTENSIONS );
        if( ext )
	{
	    if( bmem_strstr( ext, "WGL_EXT_swap_control" ) )
		wm->wglSwapIntervalEXT = (BOOL(WINAPI*)(int))wglGetProcAddress( "wglSwapIntervalEXT" );
    	    //Framebuffer:
    	    WGL_GET_PROC_ADDRESS( PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer );
	    WGL_GET_PROC_ADDRESS( PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers );
    	    WGL_GET_PROC_ADDRESS( PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers );
	    WGL_GET_PROC_ADDRESS( PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D );
	    //Shaders:
	    WGL_GET_PROC_ADDRESS( PFNGLATTACHSHADERPROC, glAttachShader );
    	    WGL_GET_PROC_ADDRESS( PFNGLCOMPILESHADERPROC, glCompileShader );
    	    WGL_GET_PROC_ADDRESS( PFNGLCREATEPROGRAMPROC, glCreateProgram );
    	    WGL_GET_PROC_ADDRESS( PFNGLCREATESHADERPROC, glCreateShader );
            WGL_GET_PROC_ADDRESS( PFNGLDELETEPROGRAMPROC, glDeleteProgram );
	    WGL_GET_PROC_ADDRESS( PFNGLDELETESHADERPROC, glDeleteShader );
            WGL_GET_PROC_ADDRESS( PFNGLDETACHSHADERPROC, glDetachShader );
    	    WGL_GET_PROC_ADDRESS( PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray );
    	    WGL_GET_PROC_ADDRESS( PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray );
    	    WGL_GET_PROC_ADDRESS( PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation );
    	    WGL_GET_PROC_ADDRESS( PFNGLGETPROGRAMIVPROC, glGetProgramiv );
    	    WGL_GET_PROC_ADDRESS( PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog );
    	    WGL_GET_PROC_ADDRESS( PFNGLGETSHADERIVPROC, glGetShaderiv );
    	    WGL_GET_PROC_ADDRESS( PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog );
    	    WGL_GET_PROC_ADDRESS( PFNGLLINKPROGRAMPROC, glLinkProgram );
    	    WGL_GET_PROC_ADDRESS( PFNGLSHADERSOURCEPROC, glShaderSource );
    	    WGL_GET_PROC_ADDRESS( PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation );
    	    WGL_GET_PROC_ADDRESS( PFNGLUSEPROGRAMPROC, glUseProgram );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM1FPROC, glUniform1f );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM2FPROC, glUniform2f );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM3FPROC, glUniform3f );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM4FPROC, glUniform4f );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM1IPROC, glUniform1i );
	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM2IPROC, glUniform2i );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM3IPROC, glUniform3i );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM4IPROC, glUniform4i );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM1FVPROC, glUniform1fv );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM2FVPROC, glUniform2fv );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM3FVPROC, glUniform3fv );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM4FVPROC, glUniform4fv );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM1IVPROC, glUniform1iv );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM2IVPROC, glUniform2iv );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM3IVPROC, glUniform3iv );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORM4IVPROC, glUniform4iv );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORMMATRIX2FVPROC, glUniformMatrix2fv );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORMMATRIX3FVPROC, glUniformMatrix3fv );
    	    WGL_GET_PROC_ADDRESS( PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVALIDATEPROGRAMPROC, glValidateProgram );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB1DPROC, glVertexAttrib1d );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB1DVPROC, glVertexAttrib1dv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB1FPROC, glVertexAttrib1f );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB1FVPROC, glVertexAttrib1fv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB1SPROC, glVertexAttrib1s );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB1SVPROC, glVertexAttrib1sv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB2DPROC, glVertexAttrib2d );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB2DVPROC, glVertexAttrib2dv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB2FPROC, glVertexAttrib2f );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB2FVPROC, glVertexAttrib2fv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB2SPROC, glVertexAttrib2s );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB2SVPROC, glVertexAttrib2sv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB3DPROC, glVertexAttrib3d ); 
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB3DVPROC, glVertexAttrib3dv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB3FPROC, glVertexAttrib3f );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB3FVPROC, glVertexAttrib3fv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB3SPROC, glVertexAttrib3s );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB3SVPROC, glVertexAttrib3sv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4NBVPROC, glVertexAttrib4Nbv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4NIVPROC, glVertexAttrib4Niv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4NSVPROC, glVertexAttrib4Nsv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4NUBPROC, glVertexAttrib4Nub );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4NUBVPROC, glVertexAttrib4Nubv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4NUIVPROC, glVertexAttrib4Nuiv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4NUSVPROC, glVertexAttrib4Nusv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4BVPROC, glVertexAttrib4bv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4DPROC, glVertexAttrib4d );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4DVPROC, glVertexAttrib4dv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4FPROC, glVertexAttrib4f );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4FVPROC, glVertexAttrib4fv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4IVPROC, glVertexAttrib4iv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4SPROC, glVertexAttrib4s );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4SVPROC, glVertexAttrib4sv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4UBVPROC, glVertexAttrib4ubv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4UIVPROC, glVertexAttrib4uiv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIB4USVPROC, glVertexAttrib4usv );
    	    WGL_GET_PROC_ADDRESS( PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer );
    	    WGL_GET_PROC_ADDRESS( PFNGLACTIVETEXTUREPROC, glActiveTexture );
	}
#endif
#if defined(LINUX) && !defined(OPENGLES)
        wm->glXSwapIntervalEXT = 0;
	wm->glXSwapIntervalSGI = 0;
        wm->glXSwapIntervalMESA = 0;
	const char* ext = glXQueryExtensionsString( wm->dpy, 0 );
	if( ext )
	{
	    if( bmem_strstr( ext, "GLX_EXT_swap_control" ) )
		wm->glXSwapIntervalEXT = (int(*)(Display*,GLXDrawable,int))glXGetProcAddress( (const GLubyte*)"glXSwapIntervalEXT" );
	    if( bmem_strstr( ext, "GLX_SGI_swap_control" ) )
		wm->glXSwapIntervalSGI = (void(*)(GLint))glXGetProcAddress( (const GLubyte*)"glXSwapIntervalSGI" );
	    if( bmem_strstr( ext, "GLX_MESA_swap_control" ) )
		wm->glXSwapIntervalMESA = (void(*)(GLint))glXGetProcAddress( (const GLubyte*)"glXSwapIntervalMESA" );
	}
#endif

	//Set OpenGL defaults:

	glClearColor( 0, 0, 0, 1 );
#ifdef OPENGLES
	glClearDepthf( 1 );
#else
	glClearDepth( 1 );
#endif
	glClearStencil( 0 );
	glDepthFunc( GL_LESS );
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); //Rows in the client textures are 1-byte aligned
	
	if( gl_default_shaders_init( wm ) ) break;

	//Set the final screen size:
    
	wm->screen_xsize /= wm->screen_zoom;
	wm->screen_ysize /= wm->screen_zoom;
    
	rv = 0;
	break;
    }
    
    gl_unlock( wm );
    
    return rv;
}

void gl_deinit( window_manager* wm )
{
    gl_lock( wm );

    gl_default_shaders_deinit( wm );
    bmem_free( wm->gl_points_array ); wm->gl_points_array = 0;

    gl_unlock( wm );
}

void gl_resize( window_manager* wm )
{
    //Set viewport to cover the window:
    
    gl_lock( wm );

    gl_set_default_viewport( wm );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
    
    if( wm->screen_zoom > 1 )
    {
        wm->gl_xscale = wm->screen_zoom;
        wm->gl_yscale = wm->screen_zoom;
    }
    else
    {
        wm->gl_xscale = 1;
        wm->gl_yscale = 1;
    }

    matrix_4x4_reset( wm->gl_projection_matrix );
    matrix_4x4_ortho( 0, wm->real_window_width, wm->real_window_height, 0, -wm->real_window_width * 10, wm->real_window_width * 10, wm->gl_projection_matrix );
    switch( wm->screen_angle )
    {
        case 0:
            break;
        case 1:
    	    matrix_4x4_translate( 0, wm->real_window_height, 0, wm->gl_projection_matrix );
            matrix_4x4_rotate( -90, 0, 0, 1, wm->gl_projection_matrix );
            break;
        case 2:
            matrix_4x4_translate( wm->real_window_width, wm->real_window_height, 0, wm->gl_projection_matrix );
            matrix_4x4_rotate( -90 * 2, 0, 0, 1, wm->gl_projection_matrix );
            break;
        case 3:
            matrix_4x4_translate( wm->real_window_width, 0, 0, wm->gl_projection_matrix );
            matrix_4x4_rotate( -90 * 3, 0, 0, 1, wm->gl_projection_matrix );
            break;
    }
    matrix_4x4_translate( 0.375, 0.375, 0, wm->gl_projection_matrix );
    if( wm->gl_xscale != 1 && wm->gl_yscale != 1 )
    {
        float xs, ys;
        xs = wm->gl_xscale;
        ys = wm->gl_yscale;
        matrix_4x4_scale( xs, ys, 1, wm->gl_projection_matrix );
    }
    gl_program_reset( wm );
    
    gl_unlock( wm );
}

void gl_bind_framebuffer( GLuint fb, window_manager* wm )
{
#ifdef IPHONE
    if( fb == 0 ) fb = g_view_framebuffer;
#endif
    glBindFramebuffer( GL_FRAMEBUFFER, fb );
}

void gl_set_default_viewport( window_manager* wm )
{
    glViewport( 0, 0, wm->real_window_width, wm->real_window_height );    
}

//
// Shaders
//

static const utf8_char* g_gl_vshader_solid = "\
uniform mat4 g_projection; // shader projection matrix uniform \n\
IN vec4 position; // vertex position attribute \n\
void main() \n\
{ \n\
    gl_Position = g_projection * position; \n\
    gl_PointSize = 1.0; \n\
} \n\
";

static const utf8_char* g_gl_vshader_gradient = "\
uniform mat4 g_projection; // shader projection matrix uniform \n\
IN vec4 position; // vertex position attribute \n\
IN vec4 color; \n\
OUT vec4 color_var; \n\
void main() \n\
{ \n\
    gl_Position = g_projection * position; \n\
    gl_PointSize = 1.0; \n\
    color_var = color; \n\
} \n\
";

static const utf8_char* g_gl_vshader_tex = "\
uniform mat4 g_projection; // shader projection matrix uniform \n\
IN vec4 position; // vertex position attribute \n\
IN vec2 tex_coord; // vertex texture coordinate attribute \n\
OUT vec2 tex_coord_var; // vertex texture coordinate varying \n\
void main() \n\
{ \n\
    gl_Position = g_projection * position; \n\
    gl_PointSize = 1.0; \n\
    tex_coord_var = tex_coord; \n\
} \n\
";

static const utf8_char* g_gl_fshader_solid = "\
PRECISION( LOWP, float ) \n\
uniform vec4 g_color; \n\
void main() \n\
{ \n\
    gl_FragColor = g_color; \n\
} \n\
";

static const utf8_char* g_gl_fshader_gradient = "\
PRECISION( LOWP, float ) \n\
IN vec4 color_var; \n\
void main() \n\
{ \n\
    gl_FragColor = color_var; \n\
} \n\
";

static const utf8_char* g_gl_fshader_tex_alpha = "\
PRECISION( LOWP, float ) \n\
uniform sampler2D g_texture; \n\
uniform vec4 g_color; \n\
IN vec2 tex_coord_var; \n\
void main() \n\
{ \n\
    gl_FragColor = vec4( 1, 1, 1, texture2D( g_texture, tex_coord_var ).a ) * g_color; \n\
} \n\
";

static const utf8_char* g_gl_fshader_tex_rgb = "\
PRECISION( LOWP, float ) \n\
uniform sampler2D g_texture; \n\
uniform vec4 g_color; \n\
IN vec2 tex_coord_var; \n\
void main() \n\
{ \n\
    gl_FragColor = texture2D( g_texture, tex_coord_var ) * g_color; \n\
} \n\
";

void gl_print_shader_info_log( GLuint shader )
{
    GLint length; 
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &length );
    if( length ) 
    {
	utf8_char* buffer = (utf8_char*)bmem_new( length );
        glGetShaderInfoLog( shader, length, NULL, buffer );
        blog( "Shader %d log:\n%s\n", shader, buffer );
        bmem_free( buffer );
    }
}

void gl_print_program_info_log( GLuint program )
{
    GLint length; 
    glGetProgramiv( program, GL_INFO_LOG_LENGTH, &length );
    if( length ) 
    {
	utf8_char* buffer = (utf8_char*)bmem_new( length );
        glGetProgramInfoLog( program, length, NULL, buffer );
        blog( "Program %d log:\n%s\n", program, buffer );
        bmem_free( buffer );
    }
}

GLuint gl_make_shader( const utf8_char* shader_source, GLenum type )
{
    GLuint shader = 0;
    
    while( 1 )
    {
        static const utf8_char* glsl_defaults_v = "\
#ifdef GL_ES \n\
    #define LOWP lowp \n\
    #define MEDIUMP mediump \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
	#define HIGHP highp \n\
    #else \n\
	#define HIGHP mediump\n\
    #endif \n\
    #define PRECISION( P, T ) precision P T \n\
    #if __VERSION__ < 300 \n\
        #define IN attribute \n\
        #define OUT varying \n\
    #else \n\
        #define IN in \n\
        #define OUT out \n\
    #endif \n\
#else \n\
    #define LOWP \n\
    #define MEDIUMP \n\
    #define HIGHP \n\
    #define PRECISION( P, T ) \n\
    #if __VERSION__ < 130 \n\
	#define IN attribute \n\
        #define OUT varying \n\
    #else \n\
        #define IN in \n\
        #define OUT out \n\
    #endif \n\
#endif \n\
";
        static const utf8_char* glsl_defaults_f = "\
#ifdef GL_ES \n\
    #define LOWP lowp \n\
    #define MEDIUMP mediump \n\
    #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
	#define HIGHP highp \n\
    #else \n\
	#define HIGHP mediump\n\
    #endif \n\
    #define PRECISION( P, T ) precision P T; \n\
    #if __VERSION__ < 300 \n\
        #define IN varying \n\
    #else \n\
        #define IN in \n\
    #endif \n\
#else \n\
    #define LOWP \n\
    #define MEDIUMP \n\
    #define HIGHP \n\
    #define PRECISION( P, T ) \n\
    #if __VERSION__ < 130 \n\
        #define IN varying \n\
    #else \n\
        #define IN in \n\
    #endif \n\
#endif \n\
";
	const utf8_char* glsl_defaults;
	if( type == GL_VERTEX_SHADER )
	    glsl_defaults = glsl_defaults_v;
	else
	    glsl_defaults = glsl_defaults_f;
    
	utf8_char* shader_source2 = (utf8_char*)bmem_new( bmem_strlen( glsl_defaults ) + bmem_strlen( shader_source ) + 2 );
	sprintf( shader_source2, "%s%s", glsl_defaults, shader_source );
	
	shader = glCreateShader( type );
	if( shader == 0 ) 
	{
	    gl_error( "glCreateShader()" );
	    break;
	}
 
	glShaderSource( shader, 1, (const utf8_char**)&shader_source2, NULL );
        glCompileShader( shader );
        GLint success = 0;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &success );
	if( success == GL_FALSE )
	{
	    gl_error( "glCompileShader()" );
	    gl_print_shader_info_log( shader );
	    if( type == GL_VERTEX_SHADER )
		blog( "Vertex shader %d source:\n%s", shader, shader_source2 );
	    else
		blog( "Fragment shader %d source:\n%s", shader, shader_source2 );
	    glDeleteShader( shader );
	    shader = 0;
	    break;
	}
	
	//gl_print_shader_info_log( shader );
	bmem_free( shader_source2 );
	
	break;
    }
    
    return shader;
}

GLuint gl_make_program( GLuint vertex_shader, GLuint fragment_shader )
{
    GLuint program = 0;
    
    //Vertex and fragment shaders are successfully compiled.
    //Now time to link them together into a program.
    
    while( 1 )
    {
	//Get a program object:
	program = glCreateProgram();
	if( program == 0 )
        {
            gl_error( "glCreateProgram()" );
            break;
        }
    
	//Attach our shaders to our program:
	glAttachShader( program, vertex_shader );
	glAttachShader( program, fragment_shader );
	
	//Link our program:
	glLinkProgram( program );
        GLint success = 0;
	glGetProgramiv( program, GL_LINK_STATUS, &success );
	if( success == GL_FALSE )
	{
	    gl_error( "glLinkProgram()" );
	    gl_print_program_info_log( program );
	    glDeleteProgram( program );
	    program = 0;
	    break;
	}
	
	//Always detach shaders after a successful link.
#ifdef ANDROID
	if( g_gl_no_shader_detach == false )
#endif
	{
	    glDetachShader( program, vertex_shader );
	    glDetachShader( program, fragment_shader );
	}
		
	break;
    }
    
    return program;
}

gl_program_struct* gl_program_new( GLuint vertex_shader, GLuint fragment_shader )
{
    gl_program_struct* rv = 0;
    
    while( 1 )
    {
	rv = (gl_program_struct*)bmem_new( sizeof( gl_program_struct ) );
	if( rv == 0 ) break;
	bmem_zero( rv );
	for( int i = 0; i < GL_PROG_ATT_MAX; i++ )
	    rv->attributes[ i ] = -1;
	for( int i = 0; i < GL_PROG_UNI_MAX; i++ )
	    rv->uniforms[ i ] = -1;
	
	rv->program = gl_make_program( vertex_shader, fragment_shader );
	if( rv->program == 0 ) 
	{
	    bmem_free( rv );
	    rv = 0;
	    break;
	}	

	break;
    }
    
    return rv;
}

void gl_program_remove( gl_program_struct* p )
{
    if( p == 0 ) return;
    glDeleteProgram( p->program );
    bmem_free( p );
}

void gl_init_uniform( gl_program_struct* prog, int n, const utf8_char* name )
{
    prog->uniforms[ n ] = glGetUniformLocation( prog->program, name );
    if( prog->uniforms[ n ] == -1 )
	blog( "OpenGL prog %d -> uniform (global var) \"%s\" not found (or removed as unused).\n", prog->program, name );
}

void gl_init_attribute( gl_program_struct* prog, int n, const utf8_char* name )
{
    prog->attributes[ n ] = glGetAttribLocation( prog->program, name );
    if( prog->attributes[ n ] == -1 )
	blog( "OpenGL prog %d -> attribute (v.shader IN) \"%s\" not found (or removed as unused).\n", prog->program, name );
}

void gl_enable_attributes( gl_program_struct* prog, uint attr )
{
    uint b = 1;
    for( int i = 0; i < GL_PROG_ATT_MAX; i++ )
    {
	if( attr & b )
	{
	    if( ( prog->attributes_enabled & b ) == 0 )
		glEnableVertexAttribArray( prog->attributes[ i ] );
	}
	else
	{
	    if( prog->attributes_enabled & b )
		glDisableVertexAttribArray( prog->attributes[ i ] );
	}
	b <<= 1;
    }
    prog->attributes_enabled = attr;
}

int gl_default_shaders_init( window_manager* wm )
{
    int rv = -1;
    gl_program_struct* p;

    while( 1 )
    {
	wm->gl_transform_counter = 0;
	wm->gl_current_prog = 0;
	
	GLuint vshader_solid;
	GLuint vshader_gradient;
	GLuint vshader_tex;
        GLuint fshader_solid;
        GLuint fshader_gradient;
        GLuint fshader_tex_alpha;
        GLuint fshader_tex_rgb;
	
	vshader_solid = gl_make_shader( g_gl_vshader_solid, GL_VERTEX_SHADER );
	if( vshader_solid == 0 ) break;

	vshader_gradient = gl_make_shader( g_gl_vshader_gradient, GL_VERTEX_SHADER );
	if( vshader_gradient == 0 ) break;

	vshader_tex = gl_make_shader( g_gl_vshader_tex, GL_VERTEX_SHADER );
	if( vshader_tex == 0 ) break;

	fshader_solid = gl_make_shader( g_gl_fshader_solid, GL_FRAGMENT_SHADER );
	if( fshader_solid == 0 ) break;

	fshader_gradient = gl_make_shader( g_gl_fshader_gradient, GL_FRAGMENT_SHADER );
	if( fshader_gradient == 0 ) break;

	fshader_tex_alpha = gl_make_shader( g_gl_fshader_tex_alpha, GL_FRAGMENT_SHADER );
	if( fshader_tex_alpha == 0 ) break;

	fshader_tex_rgb = gl_make_shader( g_gl_fshader_tex_rgb, GL_FRAGMENT_SHADER );
	if( fshader_tex_rgb == 0 ) break;

	wm->gl_prog_solid = gl_program_new( vshader_solid, fshader_solid );
	p = wm->gl_prog_solid;
	if( p == 0 ) break;
	gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_projection" );
	gl_init_uniform( p, GL_PROG_UNI_COLOR, "g_color" );
	gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );

	wm->gl_prog_gradient = gl_program_new( vshader_gradient, fshader_gradient );
	p = wm->gl_prog_gradient;
	if( p == 0 ) break;
	gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_projection" );
	gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
	gl_init_attribute( p, GL_PROG_ATT_COLOR, "color" );

	wm->gl_prog_tex_alpha = gl_program_new( vshader_tex, fshader_tex_alpha );
	p = wm->gl_prog_tex_alpha;
	if( p == 0 ) break;
	gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_projection" );
	gl_init_uniform( p, GL_PROG_UNI_COLOR, "g_color" );
	gl_init_uniform( p, GL_PROG_UNI_TEXTURE, "g_texture" );
	gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
	gl_init_attribute( p, GL_PROG_ATT_TEX_COORD, "tex_coord" );

	wm->gl_prog_tex_rgb = gl_program_new( vshader_tex, fshader_tex_rgb );
	p = wm->gl_prog_tex_rgb;
	if( p == 0 ) break;
	gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_projection" );
	gl_init_uniform( p, GL_PROG_UNI_COLOR, "g_color" );
	gl_init_uniform( p, GL_PROG_UNI_TEXTURE, "g_texture" );
	gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
	gl_init_attribute( p, GL_PROG_ATT_TEX_COORD, "tex_coord" );

	glDeleteShader( vshader_solid );
        glDeleteShader( vshader_gradient );
	glDeleteShader( vshader_tex );
        glDeleteShader( fshader_solid );
	glDeleteShader( fshader_gradient );
        glDeleteShader( fshader_tex_alpha );
	glDeleteShader( fshader_tex_rgb );
		
	rv = 0;
	break;
    }

    return rv;
}

void gl_default_shaders_deinit( window_manager* wm )
{
    gl_program_remove( wm->gl_prog_solid );
    gl_program_remove( wm->gl_prog_gradient );
    gl_program_remove( wm->gl_prog_tex_alpha );
    gl_program_remove( wm->gl_prog_tex_rgb );
}

void gl_program_reset( window_manager* wm )
{
    if( wm->gl_current_prog ) gl_enable_attributes( wm->gl_current_prog, 0 );
    wm->gl_current_prog = 0;
    wm->gl_transform_counter++;
}

//
// Drawing primitives
//

#define USE_PROG( pp ) \
    if( wm->gl_current_prog ) gl_enable_attributes( wm->gl_current_prog, 0 ); \
    glUseProgram( pp->program ); \
    wm->gl_current_prog = pp; \
    if( pp->transform_counter != wm->gl_transform_counter ) \
    { \
        pp->transform_counter = wm->gl_transform_counter; \
	glUniformMatrix4fv( pp->uniforms[ GL_PROG_UNI_TRANSFORM1 ], 1, 0, wm->gl_projection_matrix ); \
    } \

void gl_draw_points( int16* coord2d, COLOR color, int count, window_manager* wm )
{
    uchar opacity = wm->cur_opacity;
    gl_program_struct* p = wm->gl_prog_solid;
    if( wm->gl_current_prog != p )
    {
	USE_PROG( p );
	gl_enable_attributes( p, 1 << GL_PROG_ATT_POSITION );
    } 
    glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( color ) / 255, (float)green( color ) / 255, (float)blue( color ) / 255, (float)opacity / 255 ); 
    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 2, GL_SHORT, false, 0, coord2d );
    glDrawArrays( GL_POINTS, 0, count );    
    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 2, GL_SHORT, false, 0, wm->gl_array_s ); //because all other gl_prog_solid users use the gl_array_s
}

void gl_draw_triangles( int16* coord2d, COLOR color, int count, window_manager* wm )
{
    uchar opacity = wm->cur_opacity;
    gl_program_struct* p = wm->gl_prog_solid;
    if( wm->gl_current_prog != p )
    {
	USE_PROG( p );
	gl_enable_attributes( p, 1 << GL_PROG_ATT_POSITION );
    } 
    glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( color ) / 255, (float)green( color ) / 255, (float)blue( color ) / 255, (float)opacity / 255 ); 
    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 2, GL_SHORT, false, 0, coord2d );
    glDrawArrays( GL_TRIANGLES, 0, count * 3 );
    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 2, GL_SHORT, false, 0, wm->gl_array_s ); //because all other gl_prog_solid users use the gl_array_s
}

void gl_draw_polygon( sundog_polygon* poly, window_manager* wm )
{
    int16* v = wm->gl_array_s;
    uchar* c = wm->gl_array_c;

    gl_program_struct* p = wm->gl_prog_gradient;
    if( wm->gl_current_prog != p )
    {
	USE_PROG( p );
	gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) | ( 1 << GL_PROG_ATT_COLOR ) );
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 2, GL_SHORT, false, 0, v );
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_COLOR ], 4, GL_UNSIGNED_BYTE, true, 0, c );
    }

    bool one_color = ( wm->cur_flags & WBD_FLAG_ONE_COLOR ) != 0;
    bool one_opacity = ( wm->cur_flags & WBD_FLAG_ONE_OPACITY ) != 0;

    v[ 0 ] = poly->v[ 0 ].x;
    v[ 1 ] = poly->v[ 0 ].y;
    COLOR color = poly->v[ 0 ].c;
    c[ 0 ] = red( color );
    c[ 1 ] = green( color );
    c[ 2 ] = blue( color );
    c[ 3 ] = poly->v[ 0 ].t;
    for( int i = 2; i < poly->vnum; i++ )
    {
	v[ 2 ] = poly->v[ i - 1 ].x;
	v[ 3 ] = poly->v[ i - 1 ].y;
	v[ 4 ] = poly->v[ i ].x;
	v[ 5 ] = poly->v[ i ].y;
	if( one_color )
	{
	    c[ 4 ] = c[ 0 ];
	    c[ 5 ] = c[ 1 ];
	    c[ 6 ] = c[ 2 ];
	    c[ 8 ] = c[ 0 ];
	    c[ 9 ] = c[ 1 ];
	    c[ 10 ] = c[ 2 ];
	}
	else 
	{
	    color = poly->v[ i - 1 ].c;
	    c[ 4 ] = red( color );
	    c[ 5 ] = green( color );
	    c[ 6 ] = blue( color );
	    color = poly->v[ i ].c;
	    c[ 8 ] = red( color );
	    c[ 9 ] = green( color );
	    c[ 10 ] = blue( color );
	}
	if( one_opacity )
	{
	    c[ 7 ] = c[ 3 ];
	    c[ 11 ] = c[ 3 ];
	}
	else 
	{
	    c[ 7 ] = poly->v[ i - 1 ].t;
	    c[ 11 ] = poly->v[ i ].t;
	}
	glDrawArrays( GL_TRIANGLES, 0, 3 );
    }
}

void gl_draw_image_scaled( 
    int dest_x, int dest_y, 
    int dest_xs, int dest_ys,
    float src_x, float src_y,
    float src_xs, float src_ys,
    sundog_image* img,
    window_manager* wm )
{
    uchar opacity = img->opacity;

    int16* v = wm->gl_array_s;
    v[ 0 ] = (int16)dest_x; v[ 1 ] = (int16)dest_y;
    v[ 2 ] = (int16)dest_x + (int16)dest_xs; v[ 3 ] = (int16)dest_y;
    v[ 4 ] = (int16)dest_x; v[ 5 ] = (int16)dest_y + (int16)dest_ys;
    v[ 6 ] = (int16)dest_x + (int16)dest_xs; v[ 7 ] = (int16)dest_y + (int16)dest_ys;

    float* t = wm->gl_array_f;
    float tx = (float)src_x / (float)img->int_xsize;
    float ty = (float)src_y / (float)img->int_ysize;
    float tx2 = (float)( src_x + src_xs ) / (float)img->int_xsize;
    float ty2 = (float)( src_y + src_ys ) / (float)img->int_ysize;
    t[ 0 ] = tx; t[ 1 ] = ty;
    t[ 2 ] = tx2; t[ 3 ] = ty;
    t[ 4 ] = tx; t[ 5 ] = ty2;
    t[ 6 ] = tx2; t[ 7 ] = ty2;

    gl_program_struct* p;
    if( img->flags & IMAGE_ALPHA8 )
	p = wm->gl_prog_tex_alpha;
    else
	p = wm->gl_prog_tex_rgb;
    bool first_time = 0;
    if( wm->gl_current_prog != p )
    {
	first_time = 1;
	USE_PROG( p );
	gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) | ( 1 << GL_PROG_ATT_TEX_COORD ) );
        glActiveTexture( GL_TEXTURE0 );
	glUniform1i( p->uniforms[ GL_PROG_UNI_TEXTURE ], 0 ); 
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 2, GL_SHORT, false, 0, v );
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_TEX_COORD ], 2, GL_FLOAT, false, 0, t );
    }
    glBindTexture( GL_TEXTURE_2D, img->gl_texture_id );
    glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( img->color ) / 255, (float)green( img->color ) / 255, (float)blue( img->color ) / 255, (float)opacity / 255 ); 
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
}

void gl_draw_line( int x1, int y1, int x2, int y2, COLOR color, window_manager* wm )
{
    uchar opacity = wm->cur_opacity;
    int16* v = wm->gl_array_s;
    v[ 0 ] = (int16)x1; v[ 1 ] = (int16)y1;
    v[ 2 ] = (int16)x2; v[ 3 ] = (int16)y2;
    gl_program_struct* p = wm->gl_prog_solid;
    if( wm->gl_current_prog != p )
    {
	USE_PROG( p );
	gl_enable_attributes( p, 1 << GL_PROG_ATT_POSITION );
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 2, GL_SHORT, false, 0, v );
    }
    glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( color ) / 255, (float)green( color ) / 255, (float)blue( color ) / 255, (float)opacity / 255 ); 
    glDrawArrays( GL_LINES, 0, 2 );
    if( !wm->gl_no_points )
	glDrawArrays( GL_POINTS, 1, 1 );
}

void gl_draw_frect( int x, int y, int xsize, int ysize, COLOR color, window_manager* wm )
{
    uchar opacity = wm->cur_opacity; 
    int16* v = wm->gl_array_s;
    v[ 0 ] = (int16)x; v[ 1 ] = (int16)y;
    v[ 2 ] = (int16)x + (int16)xsize; v[ 3 ] = (int16)y;
    v[ 4 ] = (int16)x; v[ 5 ] = (int16)y + (int16)ysize;
    v[ 6 ] = (int16)x + (int16)xsize; v[ 7 ] = (int16)y + (int16)ysize;
    gl_program_struct* p = wm->gl_prog_solid;
    if( wm->gl_current_prog != p )
    {
	USE_PROG( p );
	gl_enable_attributes( p, 1 << GL_PROG_ATT_POSITION );
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 2, GL_SHORT, false, 0, v );
    }
    glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( color ) / 255, (float)green( color ) / 255, (float)blue( color ) / 255, (float)opacity / 255 ); 
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
}

void gl_draw_image( 
    int dest_x, int dest_y, 
    int dest_xs, int dest_ys,
    int src_x, int src_y,
    sundog_image* img,
    window_manager* wm )
{
    uchar opacity = img->opacity;

    int16* v = wm->gl_array_s;
    v[ 0 ] = (int16)dest_x; v[ 1 ] = (int16)dest_y;
    v[ 2 ] = (int16)dest_x + (int16)dest_xs; v[ 3 ] = (int16)dest_y;
    v[ 4 ] = (int16)dest_x; v[ 5 ] = (int16)dest_y + (int16)dest_ys;
    v[ 6 ] = (int16)dest_x + (int16)dest_xs; v[ 7 ] = (int16)dest_y + (int16)dest_ys;

    float* t = wm->gl_array_f;
    float tx = (float)src_x / (float)img->int_xsize;
    float ty = (float)src_y / (float)img->int_ysize;
    float tx2 = (float)( src_x + dest_xs ) / (float)img->int_xsize;
    float ty2 = (float)( src_y + dest_ys ) / (float)img->int_ysize;
    t[ 0 ] = tx; t[ 1 ] = ty;
    t[ 2 ] = tx2; t[ 3 ] = ty;
    t[ 4 ] = tx; t[ 5 ] = ty2;
    t[ 6 ] = tx2; t[ 7 ] = ty2;

    gl_program_struct* p;
    if( img->flags & IMAGE_ALPHA8 )
	p = wm->gl_prog_tex_alpha;
    else
	p = wm->gl_prog_tex_rgb;
    if( wm->gl_current_prog != p )
    {
	USE_PROG( p );
	gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) | ( 1 << GL_PROG_ATT_TEX_COORD ) );
        glActiveTexture( GL_TEXTURE0 );
	glUniform1i( p->uniforms[ GL_PROG_UNI_TEXTURE ], 0 ); 
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 2, GL_SHORT, false, 0, v );
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_TEX_COORD ], 2, GL_FLOAT, false, 0, t );
    }
    glBindTexture( GL_TEXTURE_2D, img->gl_texture_id );
    glUniform4f( p->uniforms[ GL_PROG_UNI_COLOR ], (float)red( img->color ) / 255, (float)green( img->color ) / 255, (float)blue( img->color ) / 255, (float)opacity / 255 ); 
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
}

void device_vsync( bool vsync, window_manager* wm )
{
#ifdef WIN
    if( wm->wglSwapIntervalEXT )
	wm->wglSwapIntervalEXT( vsync );
#endif
#ifdef X11
#ifndef OPENGLES
    while( 1 )
    {
	if( wm->glXSwapIntervalEXT )
	{
	    wm->glXSwapIntervalEXT( wm->dpy, wm->win, vsync );
	    break;
	}
	if( wm->glXSwapIntervalSGI )
	{
	    wm->glXSwapIntervalSGI( vsync );
	    break;
	}
	if( wm->glXSwapIntervalMESA )
	{
	    wm->glXSwapIntervalMESA( vsync );
	    break;
	}
	break;
    }
#endif
#endif
#ifdef OSX
    GLint p = vsync;
    CGLSetParameter( CGLGetCurrentContext(), kCGLCPSwapInterval, &p );
#endif
}

//static int g_fps = 0;
//static ticks_t g_fps_t = 0;

void device_redraw_framebuffer( window_manager* wm ) 
{
    if( wm->screen_changed == 0 ) return;
    wm->screen_changed = 0;
    
    /*if( g_fps_t == 0 )
	g_fps_t = time_ticks();
    g_fps++;
    if( time_ticks() - g_fps_t > time_ticks_per_second() )
    {
	g_fps_t = time_ticks();
	blog( "%d fps\n", g_fps );
	g_fps = 0;
    }*/
    
#ifdef WIN
    glFlush();
    SwapBuffers( wm->hdc );
#endif

#ifdef X11
#ifdef OPENGLES
    glFlush();
    eglSwapBuffers( egl_display, egl_surface );
#else
    XSync( wm->dpy, 0 );
    glFlush();
    glXSwapBuffers( wm->dpy, wm->win );
#endif
#endif

#ifdef IPHONE
    iphone_sundog_screen_redraw();
#endif

#ifdef OSX
    static bool gl_first_redraw = 1;
    if( gl_first_redraw )
    {
        glFlush();
        gl_first_redraw = 0;
    }
    osx_sundog_screen_redraw();
#endif

#ifdef ANDROID
    android_sundog_screen_redraw();
#endif
}	
