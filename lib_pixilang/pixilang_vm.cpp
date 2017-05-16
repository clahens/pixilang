/*
    pixilang_vm.cpp
    This file is part of the Pixilang programming language.
    
    [ MIT license ]

    Copyright (c) 2006 - 2016, Alexander Zolotov <nightradio@gmail.com>
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

#include <errno.h>

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
    #define DPRINT( fmt, ARGS... ) blog( fmt, ## ARGS )
#else
    #define DPRINT( fmt, ARGS... ) {}
#endif

//#define VM_DEBUGGER

#define FIRST_CODE_PAGE	    1024
#define NEXT_CODE_PAGE	    1024

const utf8_char* g_pix_container_type_names[] =
{
    "INT8",
    "INT16",
    "INT32",
    "INT64",
    "FLOAT32",
    "FLOAT64"
};

const int g_pix_container_type_sizes[] =
{
    1,
    2,
    4,
    8,
    4,
    8,
    sizeof( PIX_VAL )
};

int pix_vm_init( pix_vm* vm, window_manager* wm )
{
    int rv = 1;
    while( 1 )
    {    
	bmem_set( vm, sizeof( pix_vm ), 0 );
	
	vm->wm = wm;
    
	//Containers:
	size_t c_num = 8192;
#ifdef PIXI_CONTAINERS_NUM
	c_num = PIXI_CONTAINERS_NUM;
#endif
	for( int i = 0; user_options[ i ] != 0; i++ )
	{
	    if( bmem_strcmp( user_options[ i ], "pixi_containers_num" ) == 0 )
	    {
		c_num = user_options_val[ i ];
		break;
	    }
	}
	vm->c_num = profile_get_int_value( "pixi_containers_num", c_num, 0 );
	vm->c = (pix_vm_container**)bmem_new( sizeof( pix_vm_container* ) * vm->c_num );
	if( vm->c == 0 )
	{
	    blog( "Memory allocation error (containers)\n" );
	    break;
        }
	bmem_zero( vm->c );
        bmutex_init( &vm->c_mutex, 0 );
	vm->c_counter = 0;
    
        //Events:
	bmutex_init( &vm->events_mutex, 0 );
    
        //Threads:
	bmutex_init( &vm->th_mutex, 0 );
    
        //Log:
	vm->log_buffer = (utf8_char*)bmem_new( 4 * 1024 );
        bmem_zero( vm->log_buffer );
	vm->log_prev_msg = (utf8_char*)bmem_new( 4 );
        vm->log_prev_msg[ 0 ] = 0;
	bmutex_init( &vm->log_mutex, 0 );

        //Some initial values:
        vm->zbuf = -1;
        vm->pixel_size = 1;
        vm->transp = 255;
	pix_vm_gfx_matrix_reset( vm );
	vm->gl_callback = -1;
	vm->audio_callback = -1;
	vm->quit_action = 1;
	vm->audio_channels_cont = -1;
        vm->audio_input_channels_cont = -1;
	for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
        {
    	    vm->audio_buffers_conts[ i ] = -1;
	    vm->audio_input_buffers_conts[ i ] = -1;
        }
	vm->event = -1;
	vm->current_path = -1;
        vm->user_path = -1;
	vm->temp_path = -1;
        vm->os_name = -1;
	vm->arch_name = -1;
        vm->lang_name = -1;

#ifdef OPENGL
	if( pix_vm_gl_init( vm ) )
	{
	    blog( "OpenGL init error\n" );
	    break;
	}
#endif
        
    	COMPILER_MEMORY_BARRIER();
        vm->ready = 1;
        rv = 0;
        break;
    }
    
    return rv;
}

int pix_vm_deinit( pix_vm* vm )
{
    int rv = 0;

    if( vm == 0 ) return 2;
    
    vm->ready = 0;
    
    //Stop separate threads created by Pixilang:
    int sleep_counter = 0;
    int sleep_counter_deadline = 2000;
    for( int i = 0; i < PIX_VM_THREADS; i++ )
    {
	bmutex_lock( &vm->th_mutex );
	if( vm->th[ i ] )
	{
	    if( vm->th[ i ]->thread_open )
	    {
		int step_ms = 25;
		bool closed = 0;
		while( sleep_counter < sleep_counter_deadline )
		{
		    int r = bthread_destroy( &vm->th[ i ]->th, -step_ms );
		    bmutex_unlock( &vm->th_mutex );
		    time_sleep( 10 );
		    bmutex_lock( &vm->th_mutex );
		    if( r == 0 )
		    {
			closed = 1;
			break;
		    }
		    sleep_counter += step_ms;
		}
		if( closed == 0 )
		{
		    vm->th[ i ]->active = 0;
		    if( bthread_destroy( &vm->th[ i ]->th, 100 ) )
		    {
			PIX_VM_LOG( "Thread %d is not responding. Forcibly removed.\n", i );
		    }
		    else 
		    {
			PIX_VM_LOG( "Thread %d is not responding. Forcibly removed (via halt).\n", i );
		    }
		}
	    }
	}
	bmutex_unlock( &vm->th_mutex );
    }
    
    //Close audio stream:
    PIX_VAL u;
    u.i = 0;
    pix_vm_set_audio_callback( -1, u, 0, 0, PIX_CONTAINER_TYPE_INT16, 0, 0, vm );
    
    //Remove all threads data:
    for( int i = 0; i < PIX_VM_THREADS; i++ )
    {
	bmutex_lock( &vm->th_mutex );
	if( vm->th[ i ] )
	{
	    bmem_free( vm->th[ i ] );
	    vm->th[ i ] = 0;
	}
	bmutex_unlock( &vm->th_mutex );
    }
    bmutex_destroy( &vm->th_mutex );

    //Log:
    bmutex_lock( &vm->log_mutex );
    bmem_free( vm->log_buffer );
    bmem_free( vm->log_prev_msg );
    vm->log_buffer = 0;
    vm->log_prev_msg = 0;
    bmutex_unlock( &vm->log_mutex );

    //Events:
    bmutex_destroy( &vm->events_mutex );

    //Variables:
    if( vm->var_names )
    {
	for( size_t i = 0; i < vm->vars_num; i++ )
	{
	    bmem_free( vm->var_names[ i ] );
	}
    }
    bmem_free( vm->vars );
    bmem_free( vm->var_types );
    bmem_free( vm->var_names );
    vm->vars = 0;
    vm->var_types = 0;
    vm->var_names = 0;
    vm->vars_num = 0;
    
    //Containers:
    vm->c_ignore_mutex = 1;
    if( vm->c )
    {
	for( int i = 0; i < bmem_get_size( vm->c ) / sizeof( pix_vm_container* ); i++ )
	{
	    pix_vm_remove_container( i, vm );
	}
	bmem_free( vm->c );
	vm->c = 0;
    }
    bmutex_destroy( &vm->c_mutex );
    
    //Text:
    bmem_free( vm->text );
    bmem_free( vm->text_lines );
    vm->text = 0;
    vm->text_lines = 0;
    
    //Effector:
    bmem_free( vm->effector_colors_r );
    bmem_free( vm->effector_colors_g );
    bmem_free( vm->effector_colors_b );
    
    //Code:
    bmem_free( vm->code );
    vm->code = 0;
    vm->code_ptr = 0;
    vm->code_size = 0;

    //Virtual disk0:
    bfs_close( vm->virt_disk0 );
    
    //Base path:
    bmem_free( vm->base_path );
    vm->base_path = 0;

    //Final log deinit:
    bmutex_destroy( &vm->log_mutex );
    
#ifdef OPENGL    
    pix_vm_gl_deinit( vm );
#endif    

    return rv;
}

void pix_vm_log( utf8_char* message, pix_vm* vm )
{
    if( vm->log_buffer == 0 )
    {
	blog( "%s", message );
	return;
    }
    bmutex_lock( &vm->log_mutex );
    size_t log_size = bmem_get_size( vm->log_buffer );
    size_t msg_size = bmem_strlen( message );
    if( bmem_strcmp( message, vm->log_prev_msg ) != 0 )
    {
	utf8_char date[ 10 ];
	sprintf( date, "%02d:%02d:%02d ", time_hours(), time_minutes(), time_seconds() );
	blog( "%s", message );
	if( bmem_get_size( vm->log_prev_msg ) < msg_size + 1 )
	    vm->log_prev_msg = (utf8_char*)bmem_resize( vm->log_prev_msg, msg_size + 1 );
	if( vm->log_prev_msg )
	    bmem_copy( vm->log_prev_msg, message, msg_size + 1 );
	for( size_t i = 0; i < 9; i++ )
	{
    	    vm->log_buffer[ vm->log_ptr ] = date[ i ];
    	    vm->log_ptr++;
    	    if( vm->log_ptr >= log_size ) vm->log_ptr = 0;
	}
	vm->log_filled += 9;
	for( size_t i = 0; i < msg_size; i++ )
	{
    	    vm->log_buffer[ vm->log_ptr ] = message[ i ];
    	    vm->log_ptr++;
    	    if( vm->log_ptr >= log_size ) vm->log_ptr = 0;
	}
	vm->log_filled += msg_size;
	if( vm->log_filled > log_size ) vm->log_filled = log_size;
    }
    bmutex_unlock( &vm->log_mutex );
}

void pix_vm_put_opcode( PIX_OPCODE opcode, pix_vm* vm )
{
    if( vm->code == 0 )
    {
	vm->code = (PIX_OPCODE*)bmem_new( FIRST_CODE_PAGE * sizeof( PIX_OPCODE ) );
	vm->code_size = 1;
    }
    
    if( vm->code_ptr >= vm->code_size )
    {
	vm->code_size = vm->code_ptr + 1;
	if( vm->code_size > bmem_get_size( vm->code ) / sizeof( PIX_OPCODE ) )
	{
	    vm->code = (PIX_OPCODE*)bmem_resize( vm->code, ( vm->code_size + NEXT_CODE_PAGE ) * sizeof( PIX_OPCODE ) );
	}
    }
    
    vm->code[ vm->code_ptr++ ] = opcode;
}

void pix_vm_put_int( PIX_INT v, pix_vm* vm )
{
    if( sizeof( PIX_OPCODE ) >= sizeof( PIX_INT ) )
    {
	pix_vm_put_opcode( (PIX_OPCODE)v, vm );
    }
    else 
    {
	for( int i = 0; i < sizeof( PIX_INT ); i += sizeof( PIX_OPCODE ) )
	{
	    PIX_OPCODE opcode;
	    int size = sizeof( PIX_INT ) - i;
	    if( size > sizeof( PIX_OPCODE ) ) size = sizeof( PIX_OPCODE );
	    bmem_copy( &opcode, (char*)&v + i, size );
	    pix_vm_put_opcode( opcode, vm );
	}
    }
}

void pix_vm_put_float( PIX_FLOAT v, pix_vm* vm )
{
    if( sizeof( PIX_OPCODE ) >= sizeof( PIX_FLOAT ) )
    {
	volatile PIX_OPCODE opcode;
	volatile PIX_FLOAT* ptr = (PIX_FLOAT*)&opcode;
	*ptr = v;
	pix_vm_put_opcode( opcode, vm );
    }
    else 
    {
	for( int i = 0; i < sizeof( PIX_FLOAT ); i += sizeof( PIX_OPCODE ) )
	{
	    PIX_OPCODE opcode;
	    int size = sizeof( PIX_FLOAT ) - i;
	    if( size > sizeof( PIX_OPCODE ) ) size = sizeof( PIX_OPCODE );
	    bmem_copy( &opcode, (char*)&v + i, size );
	    pix_vm_put_opcode( opcode, vm );
	}
    }
}

utf8_char* pix_vm_get_variable_name( pix_vm* vm, size_t vnum )
{
    utf8_char* name = 0;
    if( vm->var_names && vnum < vm->vars_num )
    {
	name = vm->var_names[ vnum ];
	if( name == 0 )
	{
	    if( vnum < 128 )
	    {
		vm->var_names[ vnum ] = (utf8_char*)bmem_new( 2 );
		name = vm->var_names[ vnum ];
		name[ 0 ] = (utf8_char)vnum;
		name[ 1 ] = 0;
	    }
	}
    }
    return name;
}

void pix_vm_resize_variables( pix_vm* vm )
{
    if( vm->vars == 0 )
    {
	vm->vars = (PIX_VAL*)bmem_new( vm->vars_num * sizeof( PIX_VAL ) );
	vm->var_types = (char*)bmem_new( vm->vars_num * sizeof( char ) );
	vm->var_names = (utf8_char**)bmem_new( vm->vars_num * sizeof( utf8_char* ) );
	bmem_zero( vm->vars );
	bmem_zero( vm->var_types );
	bmem_zero( vm->var_names );
    }
    else
    {
	size_t prev_size = bmem_get_size( vm->vars ) / sizeof( PIX_VAL );
	if( vm->vars_num > prev_size ) 
	{
	    size_t new_size = vm->vars_num + 64;
	    vm->vars = (PIX_VAL*)bmem_resize( vm->vars, new_size * sizeof( PIX_VAL ) );
	    vm->var_types = (char*)bmem_resize( vm->var_types, new_size * sizeof( char ) );
	    vm->var_names = (utf8_char**)bmem_resize( vm->var_names, new_size * sizeof( utf8_char* ) );
	    bmem_set( vm->vars + prev_size, ( new_size - prev_size ) * sizeof( PIX_VAL ), 0 );
	    bmem_set( vm->var_types + prev_size, ( new_size - prev_size ) * sizeof( char ), 0 );
	    bmem_set( vm->var_names + prev_size, ( new_size - prev_size ) * sizeof( utf8_char* ), 0 );
	}
    }
}

int pix_vm_send_event(
    int16 type,
    int16 flags,
    int16 x,
    int16 y,
    int16 key,
    int16 scancode,
    int16 pressure,
    utf32_char unicode,
    pix_vm* vm )
{
    int rv = 0;
    
    if( vm == 0 ) return 2;
    
    bmutex_lock( &vm->events_mutex );
    
    if( vm->events_count + 1 <= PIX_VM_EVENTS )
    {
	//Get pointer to a new event:
	int new_ptr = ( vm->current_event_num + vm->events_count ) & ( PIX_VM_EVENTS - 1 );
	
	//Save new event to FIFO buffer:
	vm->events[ new_ptr ].type = (uint16)type;
	vm->events[ new_ptr ].time = time_ticks();
	vm->events[ new_ptr ].flags = (uint16)flags;
	vm->events[ new_ptr ].x = (int16)x / vm->pixel_size;
	vm->events[ new_ptr ].y = (int16)y / vm->pixel_size;
	vm->events[ new_ptr ].key = (uint16)key;
	vm->events[ new_ptr ].scancode = (uint16)scancode;
	vm->events[ new_ptr ].pressure = (uint16)pressure;
	vm->events[ new_ptr ].unicode = unicode;
	
	//Increment number of unhandled events:
	vm->events_count = vm->events_count + 1;
    }
    else
    {
	rv = 1;
    }
    
    bmutex_unlock( &vm->events_mutex );
    
    return rv;
}

int pix_vm_get_event( pix_vm* vm )
{
    if( vm->events_count )
    {
	bmutex_lock( &vm->events_mutex );
	
	//There are unhandled events:
	//Copy current event (prepare it for handling):
	pix_vm_event* evt = &vm->events[ vm->current_event_num ];
	int current_event = vm->event;
	if( (unsigned)current_event < (unsigned)vm->c_num )
	{
	    pix_vm_container* c = vm->c[ current_event ];
	    if( c && c->data && c->type == PIX_CONTAINER_TYPE_INT32 && c->size >= 16 )
	    {
		int* fields = (int*)c->data;
		fields[ 0 ] = evt->type;
		fields[ 1 ] = evt->flags;
		fields[ 2 ] = evt->time;
		fields[ 3 ] = evt->x;
		fields[ 4 ] = evt->y;
		fields[ 5 ] = evt->key;
		fields[ 6 ] = evt->scancode;
		fields[ 7 ] = evt->pressure;
		fields[ 8 ] = evt->unicode;
	    }
	}
	//This event will be handled. So decrement count of events:
	vm->events_count--;
	//And increment FIFO pointer:
	vm->current_event_num = ( vm->current_event_num + 1 ) & ( PIX_VM_EVENTS - 1 );
	
	bmutex_unlock( &vm->events_mutex );
	
	return 1;
    }
    else 
    {
	return 0;
    }
}

#define CONTROL_PC { }
#define LOAD_OPCODE( v ) { v = code[ pc++ ]; CONTROL_PC; }
#define LOAD_INT( v ) \
{ \
    if( sizeof( PIX_OPCODE ) >= sizeof( PIX_INT ) ) \
    { \
	v = (PIX_INT)code[ pc++ ]; CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 2 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 4 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 2 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 3 ] = code[ pc++ ]; CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 8 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 2 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 3 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 4 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 5 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 6 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 7 ] = code[ pc++ ]; CONTROL_PC; \
    } \
}
#define LOAD_FLOAT( v ) \
{ \
    if( sizeof( PIX_OPCODE ) >= sizeof( PIX_INT ) ) \
    { \
	v = *( (PIX_FLOAT*)&code[ pc++ ] ); CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 2 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 4 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 2 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 3 ] = code[ pc++ ]; CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 8 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 2 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 3 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 4 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 5 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 6 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 7 ] = code[ pc++ ]; CONTROL_PC; \
    } \
}

void* pix_vm_thread_body( void* user_data )
{
    pix_vm_thread* th = (pix_vm_thread*)user_data;
    int thread_num = th->thread_num;
    
    DPRINT( "Thread %d begin.\n", thread_num );
    
    pix_vm_run( th->thread_num, 0, 0, PIX_VM_CONTINUE, th->vm );

    DPRINT( "Thread %d end.\n", thread_num );
    
    return 0;
}

int pix_vm_create_active_thread( int thread_num, pix_vm* vm )
{
    bmutex_lock( &vm->th_mutex );
    if( (unsigned)thread_num >= PIX_VM_THREADS )
    {
	for( int i = 0; i < PIX_VM_THREADS - PIX_VM_SYSTEM_THREADS; i++ )
	{
	    if( vm->th[ i ] )
	    {
		if( vm->th[ i ]->active == 0 && ( vm->th[ i ]->flags & PIX_THREAD_FLAG_AUTO_DESTROY ) )
		{
		    pix_vm_destroy_thread( i, 200, vm );
		}
	    }
	    if( vm->th[ i ] == 0 )
	    {
		thread_num = i;
		break;
	    }
	}
	if( (unsigned)thread_num >= PIX_VM_THREADS )
	{
	    //No thread
	    PIX_VM_LOG( "Can't create a new thread. All %d slots busy.\n", thread_num );
	    bmutex_unlock( &vm->th_mutex );
	    return -1;
	}
    }
    if( vm->th[ thread_num ] == 0 )
    {
	vm->th[ thread_num ] = (pix_vm_thread*)bmem_new( sizeof( pix_vm_thread ) );
	bmem_zero( vm->th[ thread_num ] );
    }
    vm->th[ thread_num ]->active = 1;
    bmutex_unlock( &vm->th_mutex );
    return thread_num;
}

int pix_vm_destroy_thread( int thread_num, PIX_INT timeout, pix_vm* vm )
{
    int rv = -1;
    if( (unsigned)thread_num < (unsigned)PIX_VM_THREADS )
    {
        if( timeout == PIX_INT_MAX_POSITIVE ) timeout = 0x7FFFFFFF;
        pix_vm_thread* t = vm->th[ thread_num ];
        if( t && t->thread_open )
        {
            rv = bthread_destroy( &t->th, (int)timeout );
            if( rv == 1 && timeout < 0 )
            {
                //Don't touch
            }
            else
            {
                bmutex_lock( &vm->th_mutex );
                bmem_free( t );
                vm->th[ thread_num ] = 0;
                bmutex_unlock( &vm->th_mutex );
            }
        }
    }
    return rv;
}

int pix_vm_get_thread_retval( int thread_num, pix_vm* vm, PIX_VAL* retval, char* retval_type )
{
    if( (unsigned)thread_num < PIX_VM_THREADS )
    {
	pix_vm_thread* th = vm->th[ thread_num ];
	if( th->sp < PIX_VM_STACK_SIZE )
        {
            *retval = th->stack[ th->sp ];
            *retval_type = th->stack_types[ th->sp ];
            return 0;
        }
    }
    retval[ 0 ].i = 0;
    *retval_type = 0;
    return -1;
}

int pix_vm_run(
    int thread_num, 
    bool open_new_thread, 
    pix_vm_function* fun, 
    pix_vm_run_mode mode, 
    pix_vm* vm )
{
    pix_vm_thread* th;
    
    thread_num = pix_vm_create_active_thread( thread_num, vm );
    if( thread_num < 0 ) return thread_num;
    th = vm->th[ thread_num ];
    
    if( mode == PIX_VM_CONTINUE )
    {
    }
    else
    {
	//Reset thread:
	th->sp = PIX_VM_STACK_SIZE;
	//Push the parameters:
	if( fun && fun->p_num )
	{
	    for( int pnum = fun->p_num - 1; pnum >= 0; pnum-- )
	    {
		th->sp--;
		th->stack[ th->sp ] = fun->p[ pnum ];
		th->stack_types[ th->sp ] = fun->p_types[ pnum ];
	    }
	    th->sp--;
	    th->fp = th->sp;
	    th->stack[ th->sp ].i = fun->p_num;
	    th->stack_types[ th->sp ] = 0;
	}
	else 
	{
	    //No parameters:
	    th->sp--;
	    th->fp = th->sp;
	    th->stack[ th->sp ].i = 0;
	    th->stack_types[ th->sp ] = 0;
	}
	//Push previous FP:
	th->sp--;
	th->stack[ th->sp ].i = PIX_VM_STACK_SIZE - 1;
	//Push previous PC:
	th->sp--;
	th->stack[ th->sp ].i = (PIX_INT)vm->halt_addr;
	//Set function pointer:
	switch( mode )
	{
	    case PIX_VM_CALL_FUNCTION: th->pc = fun->addr; break;
	    case PIX_VM_CALL_MAIN: th->pc = 1; break;
	}
    }
    
    if( open_new_thread )
    {
	//Open new thread for code execution:
	th->thread_num = thread_num;
	th->vm = vm;
	th->thread_open = 1;
	bthread_create( &th->th, &pix_vm_thread_body, (void*)th, 0 );
	return thread_num;
    }
    else
    {
	//Execute code in the current thread (blocking mode):
	size_t pc = th->pc;
	size_t sp = th->sp;
	size_t fp = th->fp;
	PIX_OPCODE* code = vm->code;
	size_t code_size = vm->code_size;
	PIX_VAL* vars = vm->vars;
	char* var_types = vm->var_types;
	PIX_VAL* stack = th->stack;
	char* stack_types = th->stack_types;
	PIX_OPCODE val;
	PIX_INT val_i;
	PIX_FLOAT val_f;
	size_t sp2;
	if( pc >= code_size )
	{
	    PIX_VM_LOG( "Error. Incorrect PC (program counter) value %u\n", (unsigned int)pc );
	}
	else
	while( th->active )
	{
#ifdef VM_DEBUGGER
	    PIX_VM_LOG( "PC:%d; SP:%d; FP:%d. q - exit.\n", (int)pc, (int)sp, (int)fp );
	    int key = getchar();
	    if( key == 'q' ) { th->active = 0; break; }
#endif
	    PIX_OPCODE c = code[ pc ];
	    pc++;
	    switch( (pix_vm_opcode)( c & PIX_OPCODE_MASK ) )
	    {
		case OPCODE_NOP:
		    break;
		
		case OPCODE_HALT:
		    th->active = 0;
		    break;
		
		case OPCODE_PUSH_I:
		    LOAD_INT( val_i );
		    sp--;
		    stack_types[ sp ] = 0;
		    stack[ sp ].i = val_i;
		    break;
		case OPCODE_PUSH_i:
		    sp--;
		    stack_types[ sp ] = 0;
		    stack[ sp ].i = (signed)c >> PIX_OPCODE_BITS;
		    break;
                case OPCODE_PUSH_F:
		    LOAD_FLOAT( val_f );
		    sp--;
		    stack_types[ sp ] = 1;
		    stack[ sp ].f = val_f;
		    break;
		case OPCODE_PUSH_v:
		    val = c >> PIX_OPCODE_BITS;
		    sp--;
		    stack_types[ sp ] = var_types[ val ];
		    stack[ sp ] = vars[ val ];
		    break;
		    
		case OPCODE_GO:
		    {
			size_t addr;
			if( stack_types[ sp ] == 0 )
			    addr = (size_t)stack[ sp ].i;
			else
			    addr = (size_t)stack[ sp ].f;
			if( IS_ADDRESS_CORRECT( addr ) )
			    pc = addr & PIX_INT_ADDRESS_MASK;
			else
			    PIX_VM_LOG( "Pixilang VM Error. %u: OPCODE_GO. Address %u is incorrect\n", (unsigned int)pc, (unsigned int)stack[ sp ].i );
		    }
		    sp++;
		    break;
		case OPCODE_JMP_i:
		    pc += ( (signed)c >> PIX_OPCODE_BITS ) - 1;
		    break;
		case OPCODE_JMP_IF_FALSE_i:
		    if( ( stack_types[ sp ] == 0 && stack[ sp ].i == 0 ) ||
		        ( stack_types[ sp ] == 1 && stack[ sp ].f == 0 ) )
		    {
			pc += ( (signed)c >> PIX_OPCODE_BITS ) - 1;
		    }
		    sp++;
		    break;
		    
		case OPCODE_SAVE_TO_VAR_v:
		    val = c >> PIX_OPCODE_BITS;
		    var_types[ val ] = stack_types[ sp ];
		    vars[ val ] = stack[ sp ];
		    sp++;
		    break;

		case OPCODE_SAVE_TO_PROP_I:
		    {
			LOAD_INT( val_i );
			PIX_CID cnum;
			int prop_hash;
			utf8_char* prop_name;
	                cnum = (PIX_CID)stack[ sp + 1 ].i;
	            	prop_name = pix_vm_get_variable_name( vm, val_i );
	            	if( prop_name )
	            	{
	            	    prop_hash = (int)vm->vars[ val_i ].i;
	            	    pix_vm_set_container_property( cnum, prop_name + 1, prop_hash, stack_types[ sp ], stack[ sp ], vm );
	            	}
	                sp += 2;
		    }
		    break;
		case OPCODE_LOAD_FROM_PROP_I:
		    {
			LOAD_INT( val_i );
			PIX_CID cnum;
			size_t prop_var;
			utf8_char* prop_name;
	                cnum = (PIX_CID)stack[ sp ].i;
	            	prop_name = pix_vm_get_variable_name( vm, val_i );
	            	bool loaded = 0;
	            	if( prop_name )
	            	{
	            	    pix_sym* sym = pix_vm_get_container_property( 
	            		cnum, 
	            		prop_name + 1, //Property name
	            		(int)vm->vars[ val_i ].i, //Property hash
	            		vm );
	            	    if( sym )
	            	    {
	            		stack[ sp ] = sym->val;
	            		if( sym->type == SYMTYPE_NUM_I )
	            		    stack_types[ sp ] = 0;
	            		else
	            		    stack_types[ sp ] = 1;
	            		loaded = 1;
	            	    }
	            	}
	            	if( loaded == 0 )
	            	{
	            	    stack[ sp ].i = 0;
	            	    stack_types[ sp ] = 0;
			}
		    }
		    break;
		    
		case OPCODE_SAVE_TO_MEM:
		    {
			PIX_CID cnum;
	                cnum = (PIX_CID)stack[ sp + 2 ].i;
			if( (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
			{
			    pix_vm_container* cont = vm->c[ cnum ];
	                    PIX_INT offset;
			    if( stack_types[ sp + 1 ] == 0 )
				offset = stack[ sp + 1 ].i;
			    else
				offset = (PIX_INT)stack[ sp + 1 ].f;
			    if( (unsigned)offset < cont->size )
			    {
				if( stack_types[ sp ] == 0 )
				{
				    //Integer value:
				    switch( cont->type )
				    {
					case PIX_CONTAINER_TYPE_INT8: ((signed char*)cont->data)[ offset ] = (signed char)stack[ sp ].i; break;
					case PIX_CONTAINER_TYPE_INT16: ((signed short*)cont->data)[ offset ] = (signed short)stack[ sp ].i; break;
					case PIX_CONTAINER_TYPE_INT32: ((signed int*)cont->data)[ offset ] = (signed int)stack[ sp ].i; break;
#ifdef PIX_INT64_ENABLED
					case PIX_CONTAINER_TYPE_INT64: ((long long*)cont->data)[ offset ] = (long long)stack[ sp ].i; break;
#endif
					case PIX_CONTAINER_TYPE_FLOAT32: ((float*)cont->data)[ offset ] = (float)stack[ sp ].i; break;
#ifdef PIX_FLOAT64_ENABLED
					case PIX_CONTAINER_TYPE_FLOAT64: ((double*)cont->data)[ offset ] = (double)stack[ sp ].i; break;
#endif
				    }
				}
				else 
				{
				    //Floating point value:
				    switch( cont->type )
				    {
					case PIX_CONTAINER_TYPE_INT8: ((signed char*)cont->data)[ offset ] = (signed char)stack[ sp ].f; break;
					case PIX_CONTAINER_TYPE_INT16: ((signed short*)cont->data)[ offset ] = (signed short)stack[ sp ].f; break;
					case PIX_CONTAINER_TYPE_INT32: ((signed int*)cont->data)[ offset ] = (signed int)stack[ sp ].f; break;
#ifdef PIX_INT64_ENABLED
					case PIX_CONTAINER_TYPE_INT64: ((long long*)cont->data)[ offset ] = (long long)stack[ sp ].f; break;
#endif
					case PIX_CONTAINER_TYPE_FLOAT32: ((float*)cont->data)[ offset ] = (float)stack[ sp ].f; break;
#ifdef PIX_FLOAT64_ENABLED
					case PIX_CONTAINER_TYPE_FLOAT64: ((double*)cont->data)[ offset ] = (double)stack[ sp ].f; break;
#endif
				    }
				}
			    }
			}
		    }
		    sp += 3;
		    break;
		case OPCODE_SAVE_TO_MEM_2D:
		    {
			PIX_CID cnum;
	                cnum = (PIX_CID)stack[ sp + 3 ].i;
			if( (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
			{
			    pix_vm_container* cont = vm->c[ cnum ];
	                    PIX_INT offset;
			    if( stack_types[ sp + 2 ] == 0 )
				offset = stack[ sp + 2 ].i;
			    else
				offset = (PIX_INT)stack[ sp + 2 ].f;
			    if( stack_types[ sp + 1 ] == 0 )
				offset += stack[ sp + 1 ].i * cont->xsize;
			    else
				offset += (PIX_INT)stack[ sp + 1 ].f * cont->xsize;
			    if( (unsigned)offset < cont->size )
			    {
				if( stack_types[ sp ] == 0 )
				{
				    //Integer value:
				    switch( cont->type )
				    {
					case PIX_CONTAINER_TYPE_INT8: ((signed char*)cont->data)[ offset ] = (signed char)stack[ sp ].i; break;
					case PIX_CONTAINER_TYPE_INT16: ((signed short*)cont->data)[ offset ] = (signed short)stack[ sp ].i; break;
					case PIX_CONTAINER_TYPE_INT32: ((signed int*)cont->data)[ offset ] = (signed int)stack[ sp ].i; break;
#ifdef PIX_INT64_ENABLED
					case PIX_CONTAINER_TYPE_INT64: ((long long*)cont->data)[ offset ] = (long long)stack[ sp ].i; break;
#endif
					case PIX_CONTAINER_TYPE_FLOAT32: ((float*)cont->data)[ offset ] = (float)stack[ sp ].i; break;
#ifdef PIX_FLOAT64_ENABLED
					case PIX_CONTAINER_TYPE_FLOAT64: ((double*)cont->data)[ offset ] = (double)stack[ sp ].i; break;
#endif
				    }
				}
				else 
				{
				    //Floating point value:
				    switch( cont->type )
				    {
					case PIX_CONTAINER_TYPE_INT8: ((signed char*)cont->data)[ offset ] = (signed char)stack[ sp ].f; break;
					case PIX_CONTAINER_TYPE_INT16: ((signed short*)cont->data)[ offset ] = (signed short)stack[ sp ].f; break;
					case PIX_CONTAINER_TYPE_INT32: ((signed int*)cont->data)[ offset ] = (signed int)stack[ sp ].f; break;
#ifdef PIX_INT64_ENABLED
					case PIX_CONTAINER_TYPE_INT64: ((long long*)cont->data)[ offset ] = (long long)stack[ sp ].f; break;
#endif
					case PIX_CONTAINER_TYPE_FLOAT32: ((float*)cont->data)[ offset ] = (float)stack[ sp ].f; break;
#ifdef PIX_FLOAT64_ENABLED
					case PIX_CONTAINER_TYPE_FLOAT64: ((double*)cont->data)[ offset ] = (double)stack[ sp ].f; break;
#endif
				    }
				}
			    }
			}
		    }
		    sp += 4;
		    break;
		case OPCODE_LOAD_FROM_MEM:
		    {
			sp++;
			PIX_CID cnum;
			cnum = (PIX_CID)stack[ sp ].i;
			if( (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
			{
			    pix_vm_container* cont = vm->c[ cnum ];
			    PIX_INT offset;
			    if( stack_types[ sp - 1 ] == 0 )
				offset = stack[ sp - 1 ].i;
			    else
				offset = (PIX_INT)stack[ sp - 1 ].f;
			    if( (unsigned)offset < cont->size )
			    {
				switch( cont->type )
				{
				    case PIX_CONTAINER_TYPE_INT8: stack_types[ sp ] = 0; stack[ sp ].i = (PIX_INT)( ((signed char*)cont->data)[ offset ] ); break;
				    case PIX_CONTAINER_TYPE_INT16: stack_types[ sp ] = 0; stack[ sp ].i = (PIX_INT)( ((signed short*)cont->data)[ offset ] ); break;
				    case PIX_CONTAINER_TYPE_INT32: stack_types[ sp ] = 0; stack[ sp ].i = (PIX_INT)( ((signed int*)cont->data)[ offset ] ); break;
#ifdef PIX_INT64_ENABLED
				    case PIX_CONTAINER_TYPE_INT64: stack_types[ sp ] = 0; stack[ sp ].i = (PIX_INT)( ((long long*)cont->data)[ offset ] ); break;
#endif
				    case PIX_CONTAINER_TYPE_FLOAT32: stack_types[ sp ] = 1; stack[ sp ].f = (PIX_FLOAT)( ((float*)cont->data)[ offset ] ); break;
#ifdef PIX_FLOAT64_ENABLED
				    case PIX_CONTAINER_TYPE_FLOAT64: stack_types[ sp ] = 1; stack[ sp ].f = (PIX_FLOAT)( ((double*)cont->data)[ offset ] ); break;
#endif
				}
			    }
			    else
			    {
				stack_types[ sp ] = 0;
				stack[ sp ].i = 0;
			    }
			}
			else
			{
			    stack_types[ sp ] = 0;
			    stack[ sp ].i = 0;
			}
	   	    }
		    break;
		case OPCODE_LOAD_FROM_MEM_2D:
		    {
			sp += 2;
			PIX_CID cnum;
			cnum = (PIX_CID)stack[ sp ].i;
			if( (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
			{
			    pix_vm_container* cont = vm->c[ cnum ];
			    PIX_INT offset;
			    if( stack_types[ sp - 1 ] == 0 )
				offset = stack[ sp - 1 ].i;
			    else
				offset = (PIX_INT)stack[ sp - 1 ].f;
			    if( stack_types[ sp - 2 ] == 0 )
				offset += stack[ sp - 2 ].i * cont->xsize;
			    else
				offset += (PIX_INT)stack[ sp - 2 ].f * cont->xsize;
			    if( (unsigned)offset < cont->size )
			    {
				switch( cont->type )
				{
				    case PIX_CONTAINER_TYPE_INT8: stack_types[ sp ] = 0; stack[ sp ].i = (PIX_INT)( ((signed char*)cont->data)[ offset ] ); break;
				    case PIX_CONTAINER_TYPE_INT16: stack_types[ sp ] = 0; stack[ sp ].i = (PIX_INT)( ((signed short*)cont->data)[ offset ] ); break;
				    case PIX_CONTAINER_TYPE_INT32: stack_types[ sp ] = 0; stack[ sp ].i = (PIX_INT)( ((signed int*)cont->data)[ offset ] ); break;
#ifdef PIX_INT64_ENABLED
				    case PIX_CONTAINER_TYPE_INT64: stack_types[ sp ] = 0; stack[ sp ].i = (PIX_INT)( ((long long*)cont->data)[ offset ] ); break;
#endif
				    case PIX_CONTAINER_TYPE_FLOAT32: stack_types[ sp ] = 1; stack[ sp ].f = (PIX_FLOAT)( ((float*)cont->data)[ offset ] ); break;
#ifdef PIX_FLOAT64_ENABLED
				    case PIX_CONTAINER_TYPE_FLOAT64: stack_types[ sp ] = 1; stack[ sp ].f = (PIX_FLOAT)( ((double*)cont->data)[ offset ] ); break;
#endif
				}
			    }
			    else
			    {
				stack_types[ sp ] = 0;
				stack[ sp ].i = 0;
			    }
			}
			else
			{
			    stack_types[ sp ] = 0;
			    stack[ sp ].i = 0;
			}
	   	    }
		    break;
		
		case OPCODE_SAVE_TO_STACKFRAME_i:
		    val = (signed)c >> PIX_OPCODE_BITS;
		    stack_types[ fp + (signed)val ] = stack_types[ sp ];
		    stack[ fp + (signed)val ] = stack[ sp ];
		    sp++;
		    break;
		case OPCODE_LOAD_FROM_STACKFRAME_i:
		    val = (signed)c >> PIX_OPCODE_BITS;
		    sp--;
		    stack_types[ sp ] = stack_types[ fp + (signed)val ];
		    stack[ sp ] = stack[ fp + (signed)val ];
		    break;
		
		case OPCODE_SUB:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i -= stack[ sp ].i; break;
			case 1: /*IF*/ stack_types[ sp2 ] = 1; stack[ sp2 ].f = (PIX_FLOAT)stack[ sp2 ].i - stack[ sp ].f; break;
			case 2: /*FI*/ stack[ sp2 ].f = stack[ sp2 ].f - (PIX_FLOAT)stack[ sp ].i; break;
			case 3: /*FF*/ stack[ sp2 ].f -= stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_ADD:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i += stack[ sp ].i; break;
			case 1: /*IF*/ stack_types[ sp2 ] = 1; stack[ sp2 ].f = (PIX_FLOAT)stack[ sp2 ].i + stack[ sp ].f; break;
			case 2: /*FI*/ stack[ sp2 ].f = stack[ sp2 ].f + (PIX_FLOAT)stack[ sp ].i; break;
			case 3: /*FF*/ stack[ sp2 ].f += stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_MUL:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i *= stack[ sp ].i; break;
			case 1: /*IF*/ stack_types[ sp2 ] = 1; stack[ sp2 ].f = (PIX_FLOAT)stack[ sp2 ].i * stack[ sp ].f; break;
			case 2: /*FI*/ stack[ sp2 ].f = stack[ sp2 ].f * (PIX_FLOAT)stack[ sp ].i; break;
			case 3: /*FF*/ stack[ sp2 ].f *= stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_IDIV:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i /= stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i / (PIX_INT)stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f / stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f / (PIX_INT)stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_DIV:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack_types[ sp2 ] = 1; stack[ sp2 ].f = (PIX_FLOAT)stack[ sp2 ].i / (PIX_FLOAT)stack[ sp ].i; break;
			case 1: /*IF*/ stack_types[ sp2 ] = 1; stack[ sp2 ].f = (PIX_FLOAT)stack[ sp2 ].i / stack[ sp ].f; break;
			case 2: /*FI*/ stack[ sp2 ].f = stack[ sp2 ].f / (PIX_FLOAT)stack[ sp ].i; break;
			case 3: /*FF*/ stack[ sp2 ].f /= stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_MOD:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i %= stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i % (PIX_INT)stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f % stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f % (PIX_INT)stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_AND:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i &= stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i & (PIX_INT)stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f & stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f & (PIX_INT)stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_OR:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i |= stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i | (PIX_INT)stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f | stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f | (PIX_INT)stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_XOR:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i ^= stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i ^ (PIX_INT)stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f ^ stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f ^ (PIX_INT)stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_ANDAND:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i && stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i && stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f && stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f && stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_OROR:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i || stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i || stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f || stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f || stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_EQ:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i == stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i == stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f == (PIX_FLOAT)stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f == stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_NEQ:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i != stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i != stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f != (PIX_FLOAT)stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f != stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_LESS:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i < stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i < stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f < (PIX_FLOAT)stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f < stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_LEQ:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i <= stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i <= stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f <= (PIX_FLOAT)stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f <= stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_GREATER:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i > stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i > stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f > (PIX_FLOAT)stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f > stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_GEQ:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i >= stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i >= stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f >= (PIX_FLOAT)stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = stack[ sp2 ].f >= stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_LSHIFT:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i << (unsigned)stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i << (unsigned)(PIX_INT)stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f << (unsigned)stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f << (unsigned)(PIX_INT)stack[ sp ].f; break;
		    }
		    sp++;
		    break;
		case OPCODE_RSHIFT:
		    sp2 = sp + 1;
		    switch( ( stack_types[ sp2 ] << 1 ) + stack_types[ sp ] )
		    {
			case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i >> (unsigned)stack[ sp ].i; break;
			case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i >> (unsigned)(PIX_INT)stack[ sp ].f; break;
			case 2: /*FI*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f >> (unsigned)stack[ sp ].i; break;
			case 3: /*FF*/ stack_types[ sp2 ] = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f >> (unsigned)(PIX_INT)stack[ sp ].f; break;
		    }
		    sp++;
		    break;

		case OPCODE_NEG:
		    if( stack_types[ sp ] == 0 )
			stack[ sp ].i = -stack[ sp ].i;
		    else
			stack[ sp ].f = -stack[ sp ].f;
		    break;
		    
		case OPCODE_CALL_BUILTIN_FN:
		    {
			int pars_num = c >> ( PIX_OPCODE_BITS + PIX_FN_BITS );
			int fn_num = ( c >> PIX_OPCODE_BITS ) & PIX_FN_MASK;
			g_pix_fns[ fn_num ]( fn_num, pars_num, sp, th, vm );
		        sp += pars_num - 1;
		    }
		    break;
		case OPCODE_CALL_BUILTIN_FN_VOID:
		    {
			int pars_num = c >> ( PIX_OPCODE_BITS + PIX_FN_BITS );
			int fn_num = ( c >> PIX_OPCODE_BITS ) & PIX_FN_MASK;
			g_pix_fns[ fn_num ]( fn_num, pars_num, sp, th, vm );
		        sp += pars_num;
		    }
		    break;
		case OPCODE_CALL_i:
		    {
			size_t addr;
		        if( stack_types[ sp ] == 0 )
			    addr = (size_t)stack[ sp ].i;
		        else
			    addr = (size_t)stack[ sp ].f;
			if( IS_ADDRESS_CORRECT( addr ) )
                        {
		    	    //Set new PC:
			    PIX_INT old_pc = (PIX_INT)pc;
			    pc = addr & PIX_INT_ADDRESS_MASK;
		    	    //Save number of parameters:
		    	    stack_types[ sp ] = 0;
		    	    stack[ sp ].i = (PIX_INT)( c >> PIX_OPCODE_BITS );
			    //Save FP:
    			    sp--;
		    	    //Dont touch the type of stack item, because user can't access this item directly
		    	    stack[ sp ].i = (PIX_INT)fp;
			    //Save PC:
			    sp--;
		    	    stack[ sp ].i = old_pc;
		    	    //Set new FP:
		    	    fp = sp + 2;
		    	}
		    	else
		    	{
		    	    int pars = (int)( c >> PIX_OPCODE_BITS );
			    PIX_VM_LOG( "Pixilang VM Error. %u: call function i(%d). Address %u is incorrect\n", (unsigned int)pc, pars, (unsigned int)addr );
			    sp += pars - 1;
			    stack_types[ sp ] = 0;
                            stack[ sp ].i = 0;
		    	}
		    }
		    break;
		case OPCODE_INC_SP_i:
		    sp += (signed)c >> PIX_OPCODE_BITS;
		    break;
		case OPCODE_RET_i:
		    {
			PIX_INT pars_num;
			if( stack_types[ fp ] == 0 )
			    pars_num = stack[ fp ].i;
			else
			    pars_num = (PIX_INT)stack[ fp ].f;
			sp = fp + pars_num;
			stack_types[ fp + pars_num ] = 0;
			stack[ fp + pars_num ].i = (PIX_INT)( (signed)c >> PIX_OPCODE_BITS );
			pc = stack[ fp - 2 ].i;
			fp = stack[ fp - 1 ].i;
		    }
		    break;
		case OPCODE_RET_I:
		    {
			LOAD_INT( val_i );
			PIX_INT pars_num;
			if( stack_types[ fp ] == 0 )
			    pars_num = stack[ fp ].i;
			else
			    pars_num = (PIX_INT)stack[ fp ].f;
			sp = fp + pars_num;
			stack_types[ fp + pars_num ] = 0;
			stack[ fp + pars_num ].i = val_i;
			pc = stack[ fp - 2 ].i;
			fp = stack[ fp - 1 ].i;
		    }
		    break;
		case OPCODE_RET:
		    {
			PIX_INT pars_num;
			if( stack_types[ fp ] == 0 )
			    pars_num = stack[ fp ].i;
			else
			    pars_num = (PIX_INT)stack[ fp ].f;
			stack_types[ fp + pars_num ] = stack_types[ sp ];
			stack[ fp + pars_num ] = stack[ sp ];
			sp = fp + pars_num;
			pc = stack[ fp - 2 ].i;
			fp = stack[ fp - 1 ].i;
		    }
		    break;
	    }
	}
	DPRINT( "Thread %d halted. PC: %d; SP: %d; FP: %d.\n", thread_num, (int)pc, (int)sp, (int)fp );
	//Correct values after return from the main function:
	//PC = 1 (next instruction after HALT);
	//SP = PIX_VM_STACK_SIZE - 1 (points to the returned value);
	//FP = PIX_VM_STACK_SIZE - 1;
	th->pc = pc;
	th->sp = sp;
	th->fp = fp;
    }
    
    return thread_num;
}

int pix_vm_save_code( const utf8_char* name, pix_vm* vm )
{
    int rv = 0;
    
    bfs_file f = bfs_open( name, "wb" );
    if( f )
    {
	const utf8_char* signature = "PIXICODE";
	uint version = PIXILANG_VERSION;
	bfs_write( (void*)signature, 1, 8, f );
	bfs_write( &version, 1, 4, f );
	for( int i = 0; i < 4; i++ ) bfs_putc( 0, f );
	
	bfs_write( &vm->code_size, 1, 4, f );
	bfs_write( vm->code, sizeof( PIX_OPCODE ), vm->code_size, f );
	bfs_write( &vm->code_ptr, 1, 4, f );
	bfs_write( &vm->halt_addr, 1, 4, f );
	
	bfs_write( &vm->vars_num, 1, 4, f );
	bfs_write( vm->var_types, 1, vm->vars_num, f );
	for( int i = 0; i < (int)vm->vars_num; i++ )
	{
	    if( vm->var_types[ i ] == 0 )
	    {
		//Int:
		PIX_INT v = vm->vars[ i ].i;
		bfs_write( &v, 1, sizeof( v ), f );
	    }
	    else
	    {
		//Float:
		PIX_FLOAT v = vm->vars[ i ].f;
		bfs_write( &v, 1, sizeof( v ), f );
	    }
	    utf8_char* var_name = vm->var_names[ i ];
	    int name_len = 0;
	    if( var_name ) name_len = bmem_strlen( var_name );
	    bfs_write( &name_len, 1, 4, f );
	    if( name_len > 0 )
		bfs_write( var_name, 1, name_len, f );
	}
	
	bfs_write( &vm->screen, 1, 4, f );
	
	for( int i = 0; i < PIX_VM_FONTS; i++ )
	{
	    pix_vm_font* font = &vm->fonts[ i ];
	    bfs_write( &font->font, 1, sizeof( PIX_CID ), f );
	    bfs_write( &font->xchars, 1, 4, f );
	    bfs_write( &font->ychars, 1, 4, f );
	    bfs_write( &font->first, 1, 4, f );
	    bfs_write( &font->last, 1, 4, f );
	}
	
	bfs_write( &vm->event, 1, sizeof( PIX_CID ), f );
	
	bfs_write( &vm->current_path, 1, sizeof( PIX_CID ), f );
	bfs_write( &vm->user_path, 1, sizeof( PIX_CID ), f );
	bfs_write( &vm->temp_path, 1, sizeof( PIX_CID ), f );
	bfs_write( &vm->os_name, 1, sizeof( PIX_CID ), f );
	bfs_write( &vm->arch_name, 1, sizeof( PIX_CID ), f );
	bfs_write( &vm->lang_name, 1, sizeof( PIX_CID ), f );
	
	for( PIX_CID i = 0; i < vm->c_num; i++ )
	{
	    pix_vm_container* c = vm->c[ i ];
	    if( c )
	    {
		bfs_write( &i, 1, sizeof( PIX_CID ), f );
		bfs_write( &c->type, 1, 1, f );
		bfs_write( &c->flags, 1, 4, f );
		bfs_write( &c->xsize, 1, 4, f );
		bfs_write( &c->ysize, 1, 4, f );
		uchar key_color[ 3 ];
		key_color[ 0 ] = red( c->key );
		key_color[ 1 ] = green( c->key );
		key_color[ 2 ] = blue( c->key );
		bfs_write( &key_color, 1, 3, f );
		bfs_write( &c->alpha, 1, sizeof( PIX_CID ), f );
		bfs_write( c->data, g_pix_container_type_sizes[ c->type ], c->xsize * c->ysize, f );
	    }
	}
	
	bfs_close( f );
    }
    else
    {
	PIX_VM_LOG( "Can't open %s for writing.\n", name );
	rv = 1;
    }
    
    return rv;
}

void pix_vm_set_systeminfo_containers( pix_vm* vm )
{
    //System name (container):
    {
        size_t slen = bmem_strlen( OS_NAME );
        if( vm->os_name == -1 )
    	    vm->os_name = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
    	    pix_vm_resize_container( vm->os_name, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->os_name, pix_vm_get_container_flags( vm->os_name, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        bmem_copy( pix_vm_get_container_data( vm->os_name, vm ), OS_NAME, slen );
    }
    //Architecture name (container):
    {
        const utf8_char* name = ARCH_NAME;
        size_t slen = bmem_strlen( name );
        if( vm->arch_name == -1 )
    	    vm->arch_name = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
    	    pix_vm_resize_container( vm->arch_name, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->arch_name, pix_vm_get_container_flags( vm->arch_name, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        bmem_copy( pix_vm_get_container_data( vm->arch_name, vm ), name, slen );
    }
    //Language name (container):
    {
        const utf8_char* name = blocale_get_lang();
        size_t slen = bmem_strlen( name );
        if( vm->lang_name == -1 )
    	    vm->lang_name = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
    	    pix_vm_resize_container( vm->lang_name, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->lang_name, pix_vm_get_container_flags( vm->lang_name, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        bmem_copy( pix_vm_get_container_data( vm->lang_name, vm ), name, slen );
    }
    //Current working path (container):
    {
        utf8_char* p = bfs_make_filename( vm->base_path );
        size_t slen = bmem_strlen( p );
        if( slen == 0 )
        {
    	    slen = 1;
    	    bmem_free( p );
    	    p = (utf8_char*)bmem_new( 1 );
    	    p[ 0 ] = 0;
        }
        if( vm->current_path == -1 )
    	    vm->current_path = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
    	    pix_vm_resize_container( vm->current_path, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->current_path, pix_vm_get_container_flags( vm->current_path, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        bmem_copy( pix_vm_get_container_data( vm->current_path, vm ), p, slen );
        bmem_free( p );
    }
    //User data path (container):
    {
        utf8_char* p = bfs_make_filename( bfs_get_conf_path() );
        size_t slen = bmem_strlen( p );
        if( slen == 0 )
        {
    	    slen = 1;
    	    bmem_free( p );
    	    p = (utf8_char*)bmem_new( 1 );
    	    p[ 0 ] = 0;
        }
        if( vm->user_path == -1 )
    	    vm->user_path = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
	    pix_vm_resize_container( vm->user_path, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->user_path, pix_vm_get_container_flags( vm->user_path, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        bmem_copy( pix_vm_get_container_data( vm->user_path, vm ), p, slen );
        bmem_free( p );
    }
    //Temp path (container):
    {
        utf8_char* p = bfs_make_filename( bfs_get_temp_path() );
        size_t slen = bmem_strlen( p );
        if( slen == 0 )
        {
    	    slen = 1;
    	    bmem_free( p );
    	    p = (utf8_char*)bmem_new( 1 );
    	    p[ 0 ] = 0;
        }
        if( vm->temp_path == -1 )
    	    vm->temp_path = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
    	    pix_vm_resize_container( vm->temp_path, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->temp_path, pix_vm_get_container_flags( vm->temp_path, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        bmem_copy( pix_vm_get_container_data( vm->temp_path, vm ), p, slen );
        bmem_free( p );
    }
}

int pix_vm_load_code( const utf8_char* name, utf8_char* base_path, pix_vm* vm )
{
    int rv = 0;

    bfs_file f = bfs_open( name, "rb" );
    if( f )
    {
	char sign[ 16 ];
	bfs_read( sign, 1, 16, f );
	
	bfs_read( &vm->code_size, 1, 4, f );
	vm->code = (PIX_OPCODE*)bmem_new( sizeof( PIX_OPCODE ) * vm->code_size );
	bfs_read( vm->code, sizeof( PIX_OPCODE ), vm->code_size, f );
	bfs_read( &vm->code_ptr, 1, 4, f );
	bfs_read( &vm->halt_addr, 1, 4, f );
	
	bfs_read( &vm->vars_num, 1, 4, f );
	pix_vm_resize_variables( vm );
	bfs_read( vm->var_types, 1, vm->vars_num, f );
	for( int i = 0; i < (int)vm->vars_num; i++ )
    	{
	    if( vm->var_types[ i ] == 0 )
	    {
		//Int:
		PIX_INT v = 0;
		bfs_read( &v, 1, sizeof( v ), f );
		vm->vars[ i ].i = v;
	    }
	    else
	    {
		//Float:
		PIX_FLOAT v = 0;
		bfs_read( &v, 1, sizeof( v ), f );
		vm->vars[ i ].f = v;
	    }
    	    int name_len;
    	    bfs_read( &name_len, 1, 4, f );
    	    if( name_len > 0 )
    	    {
    		utf8_char* var_name = (utf8_char*)bmem_new( name_len + 1 );
    		bfs_read( var_name, 1, name_len, f );
    		var_name[ name_len ] = 0;
    		vm->var_names[ i ] = var_name;
    	    }
	}
	
	bfs_read( &vm->screen, 1, 4, f );
	
	for( int i = 0; i < PIX_VM_FONTS; i++ )
	{
	    pix_vm_font* font = &vm->fonts[ i ];
	    bfs_read( &font->font, 1, sizeof( PIX_CID ), f );
	    bfs_read( &font->xchars, 1, 4, f );
	    bfs_read( &font->ychars, 1, 4, f );
	    bfs_read( &font->first, 1, 4, f );
	    bfs_read( &font->last, 1, 4, f );
	}
	
	bfs_read( &vm->event, 1, sizeof( PIX_CID ), f );

	bfs_read( &vm->current_path, 1, sizeof( PIX_CID ), f );
	bfs_read( &vm->user_path, 1, sizeof( PIX_CID ), f );
	bfs_read( &vm->temp_path, 1, sizeof( PIX_CID ), f );
	bfs_read( &vm->os_name, 1, sizeof( PIX_CID ), f );
	bfs_read( &vm->arch_name, 1, sizeof( PIX_CID ), f );
	bfs_read( &vm->lang_name, 1, sizeof( PIX_CID ), f );
	
	while( 1 )
	{
	    PIX_CID cnum;
	    if( bfs_read( &cnum, 1, sizeof( PIX_CID ), f ) != 4 ) break;
	    int type = 0;
	    uint flags = 0;
	    int xsize = 0;
	    int ysize = 0;
	    uchar key_color[ 3 ];
	    COLOR color;
	    PIX_CID alpha = 0;
	    bfs_read( &type, 1, 1, f );
	    bfs_read( &flags, 1, 4, f );
	    bfs_read( &xsize, 1, 4, f );
	    bfs_read( &ysize, 1, 4, f );
	    bfs_read( &key_color, 1, 3, f );
	    color = get_color( key_color[ 0 ], key_color[ 1 ], key_color[ 2 ] );
	    bfs_read( &alpha, 1, sizeof( PIX_CID ), f );
	    size_t size = g_pix_container_type_sizes[ type ] * xsize * ysize;
	    void* data = bmem_new( size );
	    bfs_read( data, 1, size, f ); 
	    pix_vm_new_container( cnum, xsize, ysize, type, data, vm );
	    pix_vm_container* c = vm->c[ cnum ];
	    if( c )
	    {
		pix_vm_set_container_flags( cnum, flags, vm );
		pix_vm_set_container_key_color( cnum, color, vm );
		pix_vm_set_container_alpha( cnum, alpha, vm );
	    }
	}
	
	bfs_close( f );
	
	//Set base VM path:
	bmem_free( vm->base_path );
	vm->base_path = (utf8_char*)bmem_new( bmem_strlen( base_path ) + 1 );
	vm->base_path[ 0 ] = 0;
	bmem_strcat_resize( vm->base_path, base_path );
	
        //Set system info containers:
	pix_vm_set_systeminfo_containers( vm );

    }
    else
    {
	PIX_VM_LOG( "Can't open %s for reading.\n", name );
	rv = 1;
    }

    return rv;
}
