/*
    pixilang_vm_opengl.cpp
    This file is part of the Pixilang programming language.
    
    [ MIT license ]

    Copyright (c) 2015 - 2016, Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to 
    deal in the Software without restriction, including without limitation the 
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is 
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in 
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

//Modularity: 100%

#include "core/core.h"
#include "pixilang.h"

#ifdef OPENGL

static const utf8_char* g_gl_vshader_solid = "\
uniform mat4 g_wm_transform; \n\
uniform mat4 g_pixi_transform; \n\
IN vec4 position; // vertex position attribute \n\
void main() \n\
{ \n\
    gl_Position = g_wm_transform * g_pixi_transform * position; \n\
    gl_PointSize = 1.0; \n\
} \n\
";

static const utf8_char* g_gl_vshader_gradient = "\
uniform mat4 g_wm_transform; \n\
uniform mat4 g_pixi_transform; \n\
IN vec4 position; // vertex position attribute \n\
IN vec4 color; \n\
OUT vec4 color_var; \n\
void main() \n\
{ \n\
    gl_Position = g_wm_transform * g_pixi_transform * position; \n\
    gl_PointSize = 1.0; \n\
    color_var = color; \n\
} \n\
";

static const utf8_char* g_gl_vshader_tex_solid = "\
uniform mat4 g_wm_transform; \n\
uniform mat4 g_pixi_transform; \n\
IN vec4 position; // vertex position attribute \n\
IN vec2 tex_coord; // vertex texture coordinate attribute \n\
OUT vec2 tex_coord_var; // vertex texture coordinate varying \n\
void main() \n\
{ \n\
    gl_Position = g_wm_transform * g_pixi_transform * position; \n\
    gl_PointSize = 1.0; \n\
    tex_coord_var = tex_coord; // assign the texture coordinate attribute to its varying \n\
} \n\
";

static const utf8_char* g_gl_vshader_tex_gradient = "\
uniform mat4 g_wm_transform; \n\
uniform mat4 g_pixi_transform; \n\
IN vec4 position; // vertex position attribute \n\
IN vec2 tex_coord; // vertex texture coordinate attribute \n\
IN vec4 color; \n\
OUT vec2 tex_coord_var; // vertex texture coordinate varying \n\
OUT vec4 color_var; \n\
void main() \n\
{ \n\
    gl_Position = g_wm_transform * g_pixi_transform * position; \n\
    gl_PointSize = 1.0; \n\
    tex_coord_var = tex_coord; // assign the texture coordinate attribute to its varying \n\
    color_var = color; \n\
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

static const utf8_char* g_gl_fshader_tex_alpha_solid = "\
PRECISION( MEDIUMP, float ) \n\
uniform sampler2D g_texture; \n\
uniform vec4 g_color; \n\
IN vec2 tex_coord_var; \n\
void main() \n\
{ \n\
    gl_FragColor = vec4( 1, 1, 1, texture2D( g_texture, tex_coord_var ).a ) * g_color; \n\
} \n\
";

static const utf8_char* g_gl_fshader_tex_alpha_gradient = "\
PRECISION( MEDIUMP, float ) \n\
uniform sampler2D g_texture; \n\
IN vec2 tex_coord_var; \n\
IN vec4 color_var; \n\
void main() \n\
{ \n\
    gl_FragColor = vec4( 1, 1, 1, texture2D( g_texture, tex_coord_var ).a ) * color_var; \n\
} \n\
";

static const utf8_char* g_gl_fshader_tex_rgb_solid = "\
PRECISION( MEDIUMP, float ) \n\
uniform sampler2D g_texture; \n\
uniform vec4 g_color; \n\
IN vec2 tex_coord_var; \n\
void main() \n\
{ \n\
    gl_FragColor = texture2D( g_texture, tex_coord_var ) * g_color; \n\
} \n\
";

static const utf8_char* g_gl_fshader_tex_rgb_gradient = "\
PRECISION( MEDIUMP, float ) \n\
uniform sampler2D g_texture; \n\
IN vec2 tex_coord_var; \n\
IN vec4 color_var; \n\
void main() \n\
{ \n\
    gl_FragColor = texture2D( g_texture, tex_coord_var ) * color_var; \n\
} \n\
";

int pix_vm_gl_init( pix_vm* vm )
{
    int rv = -1;
    gl_lock( vm->wm );
    while( 1 )
    {
        bmutex_init( &vm->gl_mutex, 0 );

        gl_program_struct* p;

        vm->gl_transform_counter = 0;
        vm->gl_current_prog = 0;

        vm->gl_vshader_solid = gl_make_shader( g_gl_vshader_solid, GL_VERTEX_SHADER );
        if( vm->gl_vshader_solid == 0 ) break;

        vm->gl_vshader_gradient = gl_make_shader( g_gl_vshader_gradient, GL_VERTEX_SHADER );
        if( vm->gl_vshader_gradient == 0 ) break;

        vm->gl_vshader_tex_solid = gl_make_shader( g_gl_vshader_tex_solid, GL_VERTEX_SHADER );
        if( vm->gl_vshader_tex_solid == 0 ) break;

        vm->gl_vshader_tex_gradient = gl_make_shader( g_gl_vshader_tex_gradient, GL_VERTEX_SHADER );
        if( vm->gl_vshader_tex_gradient == 0 ) break;
        
        vm->gl_fshader_solid = gl_make_shader( g_gl_fshader_solid, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_solid == 0 ) break;

        vm->gl_fshader_gradient = gl_make_shader( g_gl_fshader_gradient, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_gradient == 0 ) break;
        
        vm->gl_fshader_tex_alpha_solid = gl_make_shader( g_gl_fshader_tex_alpha_solid, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_tex_alpha_solid == 0 ) break;
        
        vm->gl_fshader_tex_alpha_gradient = gl_make_shader( g_gl_fshader_tex_alpha_gradient, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_tex_alpha_gradient == 0 ) break;
        
        vm->gl_fshader_tex_rgb_solid = gl_make_shader( g_gl_fshader_tex_rgb_solid, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_tex_rgb_solid == 0 ) break;
     
        vm->gl_fshader_tex_rgb_gradient = gl_make_shader( g_gl_fshader_tex_rgb_gradient, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_tex_rgb_gradient == 0 ) break;

        vm->gl_prog_solid = gl_program_new( vm->gl_vshader_solid, vm->gl_fshader_solid );
        p = vm->gl_prog_solid; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_uniform( p, GL_PROG_UNI_COLOR, "g_color" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );

        vm->gl_prog_gradient = gl_program_new( vm->gl_vshader_gradient, vm->gl_fshader_gradient );
        p = vm->gl_prog_gradient; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
        gl_init_attribute( p, GL_PROG_ATT_COLOR, "color" );

        vm->gl_prog_tex_alpha_solid = gl_program_new( vm->gl_vshader_tex_solid, vm->gl_fshader_tex_alpha_solid );
        p = vm->gl_prog_tex_alpha_solid; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_uniform( p, GL_PROG_UNI_COLOR, "g_color" );
        gl_init_uniform( p, GL_PROG_UNI_TEXTURE, "g_texture" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
        gl_init_attribute( p, GL_PROG_ATT_TEX_COORD, "tex_coord" );

        vm->gl_prog_tex_alpha_gradient = gl_program_new( vm->gl_vshader_tex_gradient, vm->gl_fshader_tex_alpha_gradient );
        p = vm->gl_prog_tex_alpha_gradient; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TEXTURE, "g_texture" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
        gl_init_attribute( p, GL_PROG_ATT_TEX_COORD, "tex_coord" );
        gl_init_attribute( p, GL_PROG_ATT_COLOR, "color" );

        vm->gl_prog_tex_rgb_solid = gl_program_new( vm->gl_vshader_tex_solid, vm->gl_fshader_tex_rgb_solid );
        p = vm->gl_prog_tex_rgb_solid; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_uniform( p, GL_PROG_UNI_COLOR, "g_color" );
        gl_init_uniform( p, GL_PROG_UNI_TEXTURE, "g_texture" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
        gl_init_attribute( p, GL_PROG_ATT_TEX_COORD, "tex_coord" );

        vm->gl_prog_tex_rgb_gradient = gl_program_new( vm->gl_vshader_tex_gradient, vm->gl_fshader_tex_rgb_gradient );
        p = vm->gl_prog_tex_rgb_gradient; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TEXTURE, "g_texture" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
        gl_init_attribute( p, GL_PROG_ATT_TEX_COORD, "tex_coord" );
        gl_init_attribute( p, GL_PROG_ATT_COLOR, "color" );        

        rv = 0;
        break;
    }    
    gl_unlock( vm->wm );
    return rv;
}

void pix_vm_gl_deinit( pix_vm* vm )
{
    gl_lock( vm->wm );

    pix_vm_empty_gl_trash( vm );
    glDeleteShader( vm->gl_vshader_solid );
    glDeleteShader( vm->gl_vshader_gradient );
    glDeleteShader( vm->gl_vshader_tex_solid );
    glDeleteShader( vm->gl_vshader_tex_gradient );
    glDeleteShader( vm->gl_fshader_solid );
    glDeleteShader( vm->gl_fshader_gradient );
    glDeleteShader( vm->gl_fshader_tex_alpha_solid );
    glDeleteShader( vm->gl_fshader_tex_alpha_gradient );
    glDeleteShader( vm->gl_fshader_tex_rgb_solid );
    glDeleteShader( vm->gl_fshader_tex_rgb_gradient );
    gl_program_remove( vm->gl_prog_solid );
    gl_program_remove( vm->gl_prog_gradient );
    gl_program_remove( vm->gl_prog_tex_alpha_solid );
    gl_program_remove( vm->gl_prog_tex_alpha_gradient );
    gl_program_remove( vm->gl_prog_tex_rgb_solid );
    gl_program_remove( vm->gl_prog_tex_rgb_gradient );

    gl_unlock( vm->wm );

    bmem_free( vm->gl_unused_textures );
    bmem_free( vm->gl_unused_framebuffers );
    bmem_free( vm->gl_unused_progs );
    bmutex_destroy( &vm->gl_mutex );
}

void pix_vm_gl_matrix_set( pix_vm* vm )
{
    if( vm->screen == PIX_GL_SCREEN )
    {
        pix_vm_gl_program_reset( vm );
    }
}

void pix_vm_gl_program_reset( pix_vm* vm )
{
    if( vm->gl_current_prog ) 
    {
	gl_enable_attributes( vm->gl_current_prog, 0 );
	vm->gl_current_prog = 0;
    }
    vm->gl_transform_counter++;
}

void pix_vm_gl_use_prog( gl_program_struct* p, pix_vm* vm )
{
    if( vm->gl_current_prog ) gl_enable_attributes( vm->gl_current_prog, 0 ); //Disable attributes of the previous program
    glUseProgram( p->program );
    vm->gl_current_prog = p;
    if( p->transform_counter != vm->gl_transform_counter )
    {
        p->transform_counter = vm->gl_transform_counter;
        glUniformMatrix4fv( p->uniforms[ GL_PROG_UNI_TRANSFORM1 ], 1, 0, vm->gl_wm_transform );
        float mm[ 4 * 4 ];
        float* m;
        if( vm->t_enabled )
        {
            PIX_FLOAT* src = vm->t_matrix + ( vm->t_matrix_sp * 16 );
            if( sizeof( PIX_FLOAT ) == sizeof( float ) )
            {
                m = src;
            }
            else
            {
                for( int i = 0; i < 4 * 4; i++ ) mm[ i ] = src[ i ];
                m = mm;
            }
        }
        else
        {
            matrix_4x4_reset( mm );
            m = mm;
        }
        glUniformMatrix4fv( p->uniforms[ GL_PROG_UNI_TRANSFORM2 ], 1, 0, m );
    }
}

pix_vm_container_gl_data* pix_vm_get_container_gl_data( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
	if( c && c->opt_data && c->opt_data->gl )
	{
	    return c->opt_data->gl;
	}
    }
    
    return 0;
}

static void clear_fbox( uchar* img, int img_xsize, int pixel_size, int x, int y, int xsize, int ysize )
{
    img_xsize *= pixel_size;
    x *= pixel_size;
    xsize *= pixel_size;
    uchar* ptr = img + y * img_xsize + x;
    for( int y = 0; y < ysize; y++ )
    {
	bmem_set( ptr, xsize, 0 );
	ptr += img_xsize;
    }
}

static void pix_vm_get_gl_shader_uniforms( PIX_CID cnum, GLuint prog, const utf8_char* src, pix_vm* vm )
{
    utf8_char ts[ 1024 ];
    const utf8_char* next;
    while( 1 )
    {
	next = bmem_strstr( src, "uniform " );
	if( next == 0 ) break;
	const utf8_char* name_end = bmem_strstr( next, ";" );
	if( name_end == 0 ) break;
	src = name_end;
	const utf8_char* name_begin = name_end;
	while( 1 )
	{
	    if( *name_begin == ' ' ) break;
	    name_begin--;
	}
	name_begin++;
	bmem_copy( ts, name_begin, name_end - name_begin );
	ts[ name_end - name_begin ] = 0;
	PIX_VAL v;
	v.i = glGetUniformLocation( prog, ts ) + 1;
	pix_vm_set_container_property( cnum, ts, -1, 0, v, vm );
    }
}

pix_vm_container_gl_data* pix_vm_create_container_gl_data( PIX_CID cnum, pix_vm* vm )
{
    pix_vm_container_gl_data* rv = 0;

    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
	    if( c->opt_data == 0 )
	    {
		c->opt_data = (pix_vm_container_opt_data*)bmem_new( sizeof( pix_vm_container_opt_data ) );
		bmem_zero( c->opt_data );
	    }
	    if( c->opt_data )
	    {
		if( c->opt_data->gl ) return c->opt_data->gl; //Already created
		c->opt_data->gl = (pix_vm_container_gl_data*)bmem_new( sizeof( pix_vm_container_gl_data ) );
		pix_vm_empty_gl_trash( vm );
		pix_vm_container_gl_data* gl = c->opt_data->gl;
		while( gl )
		{		    
		    if( c->flags & PIX_CONTAINER_FLAG_GL_PROG )
		    {
			//GLSL program:
			GLuint vshader = 0;
			GLuint fshader = 0;
			bool vshader_default = false;
			bool fshader_default = false;
			while( 1 )
			{
			    if( c->size < 4 )
			    {
				blog( "GLSL program error: wrong container data size %d x %d. Must be at least 4\n", (int)c->xsize, (int)c->ysize );
				break;
			    }
			    
			    const utf8_char* vshader_str = (const utf8_char*)c->data;
			    const utf8_char* fshader_str = (const utf8_char*)c->data + bmem_strlen( vshader_str ) + 1;
			    
			    if( vshader_str[ 0 ] >= '0' && vshader_str[ 0 ] <= '9' )
			    {
				//Default shader:
				vshader_default = true;
				switch( vshader_str[ 0 ] - '0' )
				{
				    case GL_SHADER_SOLID: vshader_str = g_gl_vshader_solid; break;
				    case GL_SHADER_GRAD: vshader_str = g_gl_vshader_gradient; break;
				    case GL_SHADER_TEX_ALPHA_SOLID: vshader_str = g_gl_vshader_tex_solid; break;
				    case GL_SHADER_TEX_ALPHA_GRAD: vshader_str = g_gl_vshader_tex_gradient; break;
				    case GL_SHADER_TEX_RGB_SOLID: vshader_str = g_gl_vshader_tex_solid; break;
				    case GL_SHADER_TEX_RGB_GRAD: vshader_str = g_gl_vshader_tex_gradient; break;				    
				}
			    }
			    vshader = gl_make_shader( vshader_str, GL_VERTEX_SHADER );				
		            if( vshader == 0 ) break;

			    if( fshader_str[ 0 ] >= '0' && fshader_str[ 0 ] <= '9' )
			    {
				//Default shader:
				fshader_default = true;
				switch( fshader_str[ 0 ] - '0' )
				{
				    case GL_SHADER_SOLID: fshader_str = g_gl_fshader_solid; break;
				    case GL_SHADER_GRAD: fshader_str = g_gl_fshader_gradient; break;
				    case GL_SHADER_TEX_ALPHA_SOLID: fshader_str = g_gl_fshader_tex_alpha_solid; break;
				    case GL_SHADER_TEX_ALPHA_GRAD: fshader_str = g_gl_fshader_tex_alpha_gradient; break;
				    case GL_SHADER_TEX_RGB_SOLID: fshader_str = g_gl_fshader_tex_alpha_solid; break;
				    case GL_SHADER_TEX_RGB_GRAD: fshader_str = g_gl_fshader_tex_alpha_gradient; break;				    
				}
			    }
			    fshader = gl_make_shader( fshader_str, GL_FRAGMENT_SHADER );
		            if( fshader == 0 ) break;
		            
		            gl->prog = gl_program_new( vshader, fshader );
    			    if( gl->prog == 0 ) break;
		            gl_init_uniform( gl->prog, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
		            gl_init_uniform( gl->prog, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
		            gl_init_uniform( gl->prog, GL_PROG_UNI_TEXTURE, "g_texture" );
    			    gl_init_uniform( gl->prog, GL_PROG_UNI_COLOR, "g_color" );
		            gl_init_attribute( gl->prog, GL_PROG_ATT_POSITION, "position" );
			    gl_init_attribute( gl->prog, GL_PROG_ATT_TEX_COORD, "tex_coord" );
		            gl_init_attribute( gl->prog, GL_PROG_ATT_COLOR, "color" );
		            
		    	    pix_vm_get_gl_shader_uniforms( cnum, gl->prog->program, vshader_str, vm );
		    	    pix_vm_get_gl_shader_uniforms( cnum, gl->prog->program, fshader_str, vm );
			
			    rv = gl;
			    break;
			}
			glDeleteShader( vshader );
			glDeleteShader( fshader );
			break;
		    }
		    
		    //Texture:
		    
		    int size_changed = 0;
		    gl->xsize = round_to_power_of_two( c->xsize );
		    gl->ysize = round_to_power_of_two( c->ysize );
		    if( gl->xsize != c->xsize )
			size_changed |= 1;
		    if( gl->ysize != c->ysize )
			size_changed |= 2;

		    uchar* temp_pixels = 0;
		    uchar* pixels = (uchar*)c->data;
		    int base_pixel_size = g_pix_container_type_sizes[ c->type ];
		    int pixel_size = base_pixel_size;
		    bool alpha_ch = 0;
		    uchar* alpha_data = 0;
		    if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) || pix_vm_get_container_alpha( cnum, vm ) >= 0 )
		    {
			alpha_ch = 1;
			alpha_data = (uchar*)pix_vm_get_container_alpha_data( cnum, vm );
			pixel_size = 4;
			if( base_pixel_size == 2 )
			{
			    if( !( c->flags & PIX_CONTAINER_FLAG_GL_NICEST ) )
				pixel_size = 2;
			}
		    }

		    if( alpha_ch || size_changed )
		    {
			temp_pixels = (uchar*)bmem_new( gl->xsize * gl->ysize * pixel_size );
			if( size_changed )
			{
			    //Make empty border:
			    if( size_changed & 1 )
			    {
				clear_fbox( temp_pixels, gl->xsize, pixel_size, c->xsize, 0, 1, c->ysize );
				if( c->xsize + 1 < gl->xsize )
				    clear_fbox( temp_pixels, gl->xsize, pixel_size, gl->xsize - 1, 0, 1, gl->ysize );
			    }
			    if( size_changed & 2 )
			    {
				clear_fbox( temp_pixels, gl->xsize, pixel_size, 0, c->ysize, c->xsize, 1 );
				if( c->ysize + 1 < gl->ysize )
				    clear_fbox( temp_pixels, gl->xsize, pixel_size, 0, gl->ysize - 1, gl->xsize, 1 );
			    }
			    if( size_changed == 3 )
				clear_fbox( temp_pixels, gl->xsize, pixel_size, c->xsize, c->ysize, 1, 1 );
			}
			for( int y = 0; y < c->ysize; y++ )
			{
			    switch( base_pixel_size )
			    {
				case 1:
				    bmem_copy( temp_pixels + y * gl->xsize, pixels + y * c->xsize, c->xsize );
				    break;
				case 2:
				    {
					uint16* src = (uint16*)( pixels + y * c->xsize * 2 );
					uchar* alpha_src = 0;
					if( alpha_data )
					    alpha_src = alpha_data + y * c->xsize;
					switch( pixel_size )
					{
					    case 2:
						//From 16bit to 16bit:
						if( alpha_ch )
						{
						    uint16* dest = (uint16*)( temp_pixels + y * gl->xsize * 2 );
						    if( alpha_src )
							for( int x = 0; x < c->xsize; x++ )
							{
							    //to RGBA 4444
							    uint16 p = *src;
							    uchar r = ( ( p >> 11 ) << 3 ) & 0xF8; r >>= 4;
							    uchar g = ( ( p >> 5 ) << 2 ) & 0xFC; g >>= 4;
							    uchar b = ( p << 3 ) & 0xF8; b >>= 4;
							    if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
								*dest = ( r << 12 ) | ( g << 8 ) | ( b << 4 );
							    else
							    {
							        *dest = ( r << 12 ) | ( g << 8 ) | ( b << 4 ) | ( *alpha_src >> 4 );
							    }
							    alpha_src++;
							    src++;
							    dest++;
							}
						    else
							for( int x = 0; x < c->xsize; x++ )
							{
							    //to RGBA 5551
							    uint16 p = *src;
							    uchar r = ( ( p >> 11 ) << 3 ) & 0xF8; r >>= 3;
							    uchar g = ( ( p >> 5 ) << 2 ) & 0xFC; g >>= 3;
							    uchar b = ( p << 3 ) & 0xF8; b >>= 3;
							    if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
								*dest = ( r << 11 ) | ( g << 6 ) | ( b << 1 );
							    else
							    {
							        *dest = ( r << 11 ) | ( g << 6 ) | ( b << 1 ) | 1;
							    }
							    alpha_src++;
							    src++;
							    dest++;
							}
						}
						else
						{
						    bmem_copy( temp_pixels + y * gl->xsize * 2, src, c->xsize * 2 );
						}
						break;
					    case 4:
						//From 16bit to 32bit:
						{
						    uint* dest = (uint*)( temp_pixels + y * gl->xsize * 4 );
						    if( alpha_src )
							for( int x = 0; x < c->xsize; x++ )
							{
							    uint16 p = *src;
							    uchar r = ( ( p >> 11 ) << 3 ) & 0xF8; if( r ) r |= 7;
							    uchar g = ( ( p >> 5 ) << 2 ) & 0xFC; if( g ) g |= 3;
							    uchar b = ( p << 3 ) & 0xF8; if( b ) b |= 7;
							    if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
								*dest = r | ( g << 8 ) | ( b << 16 );
							    else
							    {
							        *dest = r | ( g << 8 ) | ( b << 16 ) | ( *alpha_src << 24 );
							    }
							    alpha_src++;
							    src++;
							    dest++;
							}
						    else
							for( int x = 0; x < c->xsize; x++ )
							{
							    uint16 p = *src;
							    uchar r = ( ( p >> 11 ) << 3 ) & 0xF8; if( r ) r |= 7;
							    uchar g = ( ( p >> 5 ) << 2 ) & 0xFC; if( g ) g |= 3;
							    uchar b = ( p << 3 ) & 0xF8; if( b ) b |= 7;
							    if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
								*dest = r | ( g << 8 ) | ( b << 16 );
							    else
							        *dest = r | ( g << 8 ) | ( b << 16 ) | ( 255 << 24 );
							    src++;
							    dest++;
							}
						}
						break;
					    default:
						break;
					}
				    }
				    break;
				case 4:
				    {
					uint* src = (uint*)( pixels + y * c->xsize * 4 );
					switch( pixel_size )
					{
					    case 4:
						//From 32bit to 32bit:
						if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) || alpha_ch )
						{
						    uchar* alpha_src = 0;
						    if( alpha_data )
							alpha_src = alpha_data + y * c->xsize;
						    uint* dest = (uint*)( temp_pixels + y * gl->xsize * 4 );
						    if( alpha_src )
							for( int x = 0; x < c->xsize; x++ )
							{
							    uint p = *src;
							    if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
								*dest = p & 0x00FFFFFF;
							    else
							    {
							        *dest = ( p & 0x00FFFFFF ) | ( *alpha_src << 24 );
							    }
							    alpha_src++;
							    src++;
							    dest++;
							}
						    else
							for( int x = 0; x < c->xsize; x++ )
							{
							    uint p = *src;
							    if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
								*dest = p & 0x00FFFFFF;
							    else
							        *dest = ( p & 0x00FFFFFF ) | ( 255 << 24 );
							    src++;
							    dest++;
							}
						}
						else
						{
						    bmem_copy( temp_pixels + y * gl->xsize * 4, src, c->xsize * 4 );
						}
						break;
					    default:
						break;
					}
				    }
				    break;
				default:
				    break;
			    }
			}
			pixels = temp_pixels;
		    }
		    
		    glGenTextures( 1, &gl->texture_id );
    		    glBindTexture( GL_TEXTURE_2D, gl->texture_id );

		    int internal_format;
		    int format;
		    int type;
		    bool no_texture = 0;
		    switch( pixel_size )
		    {
			case 1:
			    if( c->flags & PIX_CONTAINER_FLAG_GL_NO_ALPHA )
			    {
				internal_format = GL_LUMINANCE;
				format = GL_LUMINANCE;
			    }
			    else
			    {			    
				internal_format = GL_ALPHA;
				format = GL_ALPHA;
			    }
			    type = GL_UNSIGNED_BYTE;
			    break;
			case 2:
			    internal_format = GL_RGB;
			    format = GL_RGB;
			    type = GL_UNSIGNED_SHORT_5_6_5;
			    if( alpha_ch )
			    {
				internal_format = GL_RGBA;
				format = GL_RGBA;
				if( alpha_data )
				    type = GL_UNSIGNED_SHORT_4_4_4_4;
				else
				    type = GL_UNSIGNED_SHORT_5_5_5_1;
			    }
			    else
			    {
				internal_format = GL_RGB;
				format = GL_RGB;
				type = GL_UNSIGNED_SHORT_5_6_5;
			    }
			    break;
			case 4:
			    if( alpha_ch )
				internal_format = GL_RGBA;
			    else
				internal_format = GL_RGB;			    
			    format = GL_RGBA;
			    type = GL_UNSIGNED_BYTE;
			    break;
			default:
			    no_texture = 1;
			    break;
		    }
		    if( no_texture == 0 )
		    {
			if( c->flags & PIX_CONTAINER_FLAG_GL_FRAMEBUFFER )
			{
			    if( c->flags & PIX_CONTAINER_FLAG_GL_NO_ALPHA )
			    {
				internal_format = GL_RGB;
				format = GL_RGB;
			    }
			    else
			    {
				internal_format = GL_RGBA;
				format = GL_RGBA;
			    }
			    pixels = 0;
			}
#ifdef OPENGLES
			else
			{
			    if( internal_format == GL_RGB && format == GL_RGBA )
			    {
				internal_format = GL_RGBA;
				uint* p = (uint*)pixels;
				for( size_t i = 0; i < gl->xsize * gl->ysize; i++ ) { (*p) |= 0xFF000000; p++; }
			    }
			}
#endif
			gl->texture_format = format;
			glTexImage2D(
            		    GL_TEXTURE_2D,
            		    0,
		    	    internal_format,
            		    gl->xsize, gl->ysize,
		    	    0,
            		    format,
		            type,
            		    pixels );
            		GLenum glerr = glGetError();
            		if( glerr != GL_NO_ERROR )
            		{
            		    if( internal_format == GL_RGB && format == GL_RGBA )
            		    {
				internal_format = GL_RGBA;
				uint* p = (uint*)pixels;
				for( size_t i = 0; i < gl->xsize * gl->ysize; i++ ) { (*p) |= 0xFF000000; p++; }
				glTexImage2D(
            			    GL_TEXTURE_2D,
            			    0,
		    		    internal_format,
            			    gl->xsize, gl->ysize,
		    		    0,
            			    format,
		        	    type,
            			    pixels );
            		    }
            		    else
            		    {
            			blog( "glTexImage2D( GL_TEXTURE_2D, 0, %d, %d, %d, 0, %d, %d, pixels ) error %d\n", internal_format, gl->xsize, gl->ysize, format, type, glerr );
            		    }
            		}
            		rv = gl;

    			if( c->flags & PIX_CONTAINER_FLAG_GL_MIN_LINEAR )
    			    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    			else
    			    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    			if( c->flags & PIX_CONTAINER_FLAG_GL_MAG_LINEAR )
        		    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        		else
        		    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );    
    			if( c->flags & PIX_CONTAINER_FLAG_GL_NO_XREPEAT )
        		    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    			if( c->flags & PIX_CONTAINER_FLAG_GL_NO_YREPEAT )
        		    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
            	    }
		    
		    if( c->flags & PIX_CONTAINER_FLAG_GL_FRAMEBUFFER )
		    {
			//Create framebuffer:
        		glGenFramebuffers( 1, &gl->framebuffer_id );
        		glBindFramebuffer( GL_FRAMEBUFFER, gl->framebuffer_id );
        		//Attach renderbuffer:
        		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl->texture_id, 0 );
        		//Unbind framebuffer:
        		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		    }

		    bmem_free( temp_pixels );
		    
		    break;
		} //while( gl )
	    }
	}
    }
    
    return rv;
}

void pix_vm_remove_container_gl_data( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c && c->opt_data && c->opt_data->gl )
        {
	    bmutex_lock( &vm->gl_mutex );

	    if( c->flags & PIX_CONTAINER_FLAG_GL_PROG )
	    {
		//GLSL program:
		if( c->opt_data->gl->prog )
		{
		    if( vm->gl_unused_progs == 0 )
		    {
			vm->gl_unused_progs = (gl_program_struct**)bmem_new( 32 * sizeof( void* ) );
			vm->gl_unused_progs_count = 0;
		    }
		    vm->gl_unused_progs_count++;
		    if( vm->gl_unused_progs_count > bmem_get_size( vm->gl_unused_progs ) / sizeof( void* ) )
		    {
			vm->gl_unused_progs = (gl_program_struct**)bmem_resize( vm->gl_unused_progs, bmem_get_size( vm->gl_unused_progs ) + 32 * sizeof( void* ) );
		    }
		    vm->gl_unused_progs[ vm->gl_unused_progs_count - 1 ] = c->opt_data->gl->prog;
		}
	    }
	    else
	    {
		if( vm->gl_unused_textures == 0 )
		{
		    vm->gl_unused_textures = (uint*)bmem_new( 32 * sizeof( uint ) );
		    vm->gl_unused_textures_count = 0;
		}
		vm->gl_unused_textures_count++;
		if( vm->gl_unused_textures_count > bmem_get_size( vm->gl_unused_textures ) / sizeof( uint ) )
		{
		    vm->gl_unused_textures = (uint*)bmem_resize( vm->gl_unused_textures, bmem_get_size( vm->gl_unused_textures ) + 32 * sizeof( uint ) );
		}
		vm->gl_unused_textures[ vm->gl_unused_textures_count - 1 ] = c->opt_data->gl->texture_id;
	    
		if( c->flags & PIX_CONTAINER_FLAG_GL_FRAMEBUFFER )
		{
		    if( vm->gl_unused_framebuffers == 0 )
		    {
			vm->gl_unused_framebuffers = (uint*)bmem_new( 32 * sizeof( uint ) );
			vm->gl_unused_framebuffers_count = 0;
		    }
		    vm->gl_unused_framebuffers_count++;
		    if( vm->gl_unused_framebuffers_count > bmem_get_size( vm->gl_unused_framebuffers ) / sizeof( uint ) )
		    {
			vm->gl_unused_framebuffers = (uint*)bmem_resize( vm->gl_unused_framebuffers, bmem_get_size( vm->gl_unused_framebuffers ) + 32 * sizeof( uint ) );
		    }
		    vm->gl_unused_framebuffers[ vm->gl_unused_framebuffers_count - 1 ] = c->opt_data->gl->framebuffer_id;
		}
	    }
	    
	    bmutex_unlock( &vm->gl_mutex );
	    
            bmem_free( c->opt_data->gl );
            c->opt_data->gl = 0;
        }
    }
}

//Call this function only in the thread with OpenGL context!
void pix_vm_empty_gl_trash( pix_vm* vm )
{
    if( vm->gl_unused_textures_count > 0 || vm->gl_unused_framebuffers_count > 0 || vm->gl_unused_progs_count > 0 )
    {
	bmutex_lock( &vm->gl_mutex );
	if( vm->gl_unused_textures_count > 0 )
	{
	    if( vm->gl_unused_textures )
	    {
		glDeleteTextures( vm->gl_unused_textures_count, vm->gl_unused_textures );	    
		vm->gl_unused_textures_count = 0;
	    }
	}
	if( vm->gl_unused_framebuffers_count > 0 )
	{
	    if( vm->gl_unused_framebuffers )
	    {
		glDeleteFramebuffers( vm->gl_unused_framebuffers_count, vm->gl_unused_framebuffers );
		vm->gl_unused_framebuffers_count = 0;
	    }
	}
	if( vm->gl_unused_progs_count > 0 )
	{
	    if( vm->gl_unused_progs )
	    {
		for( int i = 0; i < vm->gl_unused_progs_count; i++ )
		{
		    gl_program_remove( vm->gl_unused_progs[ i ] );
		}
		vm->gl_unused_progs_count = 0;
	    }
	}
	bmutex_unlock( &vm->gl_mutex );
    }
}

#endif //OPENGL
