/*
    pixilang_vm_builtin_fns_native.cpp
    This file is part of the Pixilang programming language.
    
    [ MIT license ]

    Copyright (c) 2012 - 2016, Alexander Zolotov <nightradio@gmail.com>
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

#include "pixilang_vm_builtin_fns.h"

//
// Native code
//

int test_function( int p1, int p2, int p3, uint64 p4 )
{
    volatile int a = 33;
    return a;
}

void test_call( void )
{
    test_function( 1, 2, 4, 5 );
    test_function( 2, 3, 4, 5 );
}

#ifdef DYNAMIC_LIB_SUPPORT

struct pix_vm_dl_sym
{
    utf8_char* name;
    utf8_char* pars; 
    //parameters string: 1(2)
    //where 1 - return type; 2 - parameter types;
    // v - void;
    // c - signed int8;
    // C - unsigned int8
    // s - signed int16;
    // S - unsigned int16;
    // i - signed int32;
    // I - unsigned int32;
    // l - signed int64;
    // L - unsigned int64;
    // f - float32;
    // d - double64;
    // p - pointer;
    int calling_convention;
    void* ptr;
};

struct pix_vm_dl
{
#ifdef UNIX
    void* dl_handler;
#endif
#ifdef WIN
    HMODULE dl_handler;
#endif
    pix_vm_dl_sym** syms; //Functions / variables  
};

#endif

void fn_dlopen( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;

#ifdef DYNAMIC_LIB_SUPPORT

    bool name_need_to_free = 0;
    utf8_char* name = 0;
    while( 1 )
    {
	if( pars_num < 1 ) break;
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	name = pix_vm_make_cstring_from_container( cnum, &name_need_to_free, vm );
	if( name == 0 ) break;
	rv = pix_vm_new_container( -1, (PIX_INT)sizeof( pix_vm_dl ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
	if( rv < 0 ) break;
	pix_vm_dl* dl = (pix_vm_dl*)pix_vm_get_container_data( rv, vm );
	if( dl == 0 ) break;
	bmem_set( dl, sizeof( pix_vm_dl ), 0 );
	utf8_char* full_path = pix_compose_full_path( vm->base_path, name, vm );
        utf8_char* full_path2 = bfs_make_filename( full_path );
        const char* errstr = "";
#ifdef UNIX
#ifdef ANDROID
	const utf8_char* fname = bfs_get_filename_without_dir( name );
	utf8_char* fname2 = (utf8_char*)bmem_new( bmem_strlen( fname ) + 32 );
	if( fname2 )
	{
	    sprintf( fname2, "2:/%s", fname );
	    bfs_copy_file( fname2, full_path2 );
	    bmem_free( full_path2 );
	    full_path2 = bfs_make_filename( fname2 );
	    bmem_free( fname2 );
	}
#endif
        dl->dl_handler = dlopen( full_path2, RTLD_NOW );
        if( dl->dl_handler == 0 )
    	    errstr = dlerror();
#endif
#ifdef WIN
	utf16_char* ts = (utf16_char*)bmem_new( ( bmem_strlen( full_path2 ) + 1 ) * 2 );
	utf8_to_utf16( ts, MAX_DIR_LEN, full_path2 );
	utf16_unix_slash_to_windows( ts );
        dl->dl_handler = LoadLibraryW( (const WCHAR*)ts );
        bmem_free( ts );
#endif
        bmem_free( full_path2 );
	bmem_free( full_path );
	if( dl->dl_handler == 0 )
	{
	    PIX_VM_LOG( "Can't load dynamic library %s. %s\n", name, errstr );
	    pix_vm_remove_container( rv, vm );
	    rv = -1;
	    break;
	}
	break;
    }
    if( name_need_to_free ) bmem_free( name );

#endif
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_dlclose( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

#ifdef DYNAMIC_LIB_SUPPORT

    if( pars_num < 1 ) return;

    PIX_CID cnum;
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    pix_vm_dl* dl = (pix_vm_dl*)pix_vm_get_container_data( cnum, vm );
    if( dl )
    {
	if( dl->dl_handler )
	{
#ifdef UNIX
	    dlclose( dl->dl_handler );
#endif
	}
	if( dl->syms )
	{
	    for( size_t i = 0; i < bmem_get_size( dl->syms ) / sizeof( void* ); i++ )
	    {
		pix_vm_dl_sym* s = dl->syms[ i ];
		if( s )
		{
		    bmem_free( s->name );
		    bmem_free( s->pars );
		    bmem_free( s );
		}
	    }
	    bmem_free( dl->syms );
	}
	pix_vm_remove_container( cnum, vm );
    }

#endif
}

void fn_dlsym( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;

#ifdef DYNAMIC_LIB_SUPPORT

    bool name_need_to_free = 0;
    bool pars_need_to_free = 0;
    utf8_char* name = 0;
    utf8_char* pars = 0;
    int cconv = PIX_CCONV_DEFAULT;
    while( 1 )
    {
	if( pars_num < 2 ) break;
	PIX_CID dl_cnum;
	PIX_CID cnum;
	GET_VAL_FROM_STACK( dl_cnum, 0, PIX_CID );
	if( dl_cnum < 0 ) break;
	pix_vm_dl* dl = (pix_vm_dl*)pix_vm_get_container_data( dl_cnum, vm );
	if( dl == 0 ) break;
	GET_VAL_FROM_STACK( cnum, 1, PIX_CID );
	name = pix_vm_make_cstring_from_container( cnum, &name_need_to_free, vm );
	if( name == 0 ) break;
	if( pars_num >= 3 )
	{
	    GET_VAL_FROM_STACK( cnum, 2, PIX_CID );
	    pars = pix_vm_make_cstring_from_container( cnum, &pars_need_to_free, vm );
	}
	if( pars_num >= 4 )
	{
	    GET_VAL_FROM_STACK( cconv, 3, int );
	}

	void* ptr = 0;
#ifdef UNIX
	ptr = (void*)dlsym( dl->dl_handler, name );
#endif
#ifdef WIN
	ptr = (void*)GetProcAddress( dl->dl_handler, name );
#endif
	if( ptr == 0 )
	{
	    PIX_VM_LOG( "Function/variable %s not found.\n", name );
	    break;
	}
	
	if( dl->syms == 0 )
	{
	    dl->syms = (pix_vm_dl_sym**)bmem_new( sizeof( void* ) * 32 );
	    if( dl->syms == 0 ) break;
	    bmem_zero( dl->syms );
	}
	size_t sym_num = 0;
	for( ; sym_num < bmem_get_size( dl->syms ) / sizeof( void* ); sym_num++ )
	{
	    pix_vm_dl_sym* s = dl->syms[ sym_num ];
	    if( s == 0 )
	    {
		rv = (int)sym_num;
		break;
	    }
	}
	if( rv == -1 )
	{
	    size_t old_size = bmem_get_size( dl->syms );
	    dl->syms = (pix_vm_dl_sym**)bmem_resize( dl->syms, old_size * 2 );
	    if( dl->syms == 0 ) break;
	    bmem_set( (char*)dl->syms + old_size, old_size, 0 );
	    rv = old_size / sizeof( void* );
	}
	if( rv != -1 )
	{
	    pix_vm_dl_sym* sym = (pix_vm_dl_sym*)bmem_new( sizeof( pix_vm_dl_sym ) );
	    if( sym == 0 )
	    {
		rv = -1;
		break;
	    }
	    bmem_zero( sym );
	    dl->syms[ rv ] = sym;
	    sym->ptr = ptr;
	    sym->name = (utf8_char*)bmem_new( bmem_strlen( name ) + 1 );
	    sym->name[ 0 ] = 0;
	    bmem_strcat_resize( sym->name, name );
	    if( pars && bmem_strlen( pars ) >= 3 )
	    {
		sym->pars = (utf8_char*)bmem_new( bmem_strlen( pars ) + 1 );
		sym->pars[ 0 ] = 0;
    		bmem_strcat_resize( sym->pars, pars );
	    }
	    sym->calling_convention = cconv;
	}
	
	break;
    }
    if( name_need_to_free ) bmem_free( name );
    if( pars_need_to_free ) bmem_free( pars );

#endif
    
    stack_types[ sp + ( pars_num - 1 ) ] = 0;
    stack[ sp + ( pars_num - 1 ) ].i = rv;
}

void fn_dlcall( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

#ifdef DYNAMIC_LIB_SUPPORT

    while( 1 )
    {
	if( pars_num < 2 ) break;
    
        PIX_CID cnum;
        PIX_INT sym_num;
        GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
        GET_VAL_FROM_STACK( sym_num, 1, PIX_INT );
	pix_vm_dl* dl = (pix_vm_dl*)pix_vm_get_container_data( cnum, vm );
	if( dl )
        {
    	    pix_vm_dl_sym* s = 0;
    	    if( dl->syms )
    	    {
    		if( (unsigned)sym_num < bmem_get_size( dl->syms ) / sizeof( void* ) )
    		{
    		    s = dl->syms[ sym_num ];
    		}
    	    }
    	    if( s == 0 ) break;
    	    int native_pars_num = pars_num - 2;
    	    if( native_pars_num )
    	    {
    		if( s->pars == 0 )
    		{
    		    PIX_VM_LOG( "Function %s has no parameters.\n", s->name );
    		    break;
    		}
    		if( native_pars_num > bmem_get_size( s->pars ) - 3 )
    		{
    		    PIX_VM_LOG( "Too many parameters for function %s.\n", s->name );
    		    break;
    		}
    	    }
    	    void* fn_addr = s->ptr;
    	    int ret_type = -1;
    	    int ret_is_fp = 0;
    	    if( s->pars && bmem_get_size( s->pars ) >= 1 )
    	    {
		ret_type = s->pars[ 0 ];
		if( ret_type == 'f' || ret_type == 'd' )
		    ret_is_fp = 1;
	    }

#ifdef ARCH_X86_64
	    if( s->calling_convention == PIX_CCONV_DEFAULT )
#ifdef WIN
		s->calling_convention = PIX_CCONV_WIN64;
#else
		s->calling_convention = PIX_CCONV_UNIX_AMD64;
#endif
#endif

#ifdef ARCH_X86
	    if( s->calling_convention == PIX_CCONV_DEFAULT )
#ifdef WIN
		s->calling_convention = PIX_CCONV_STDCALL;
#else
		s->calling_convention = PIX_CCONV_CDECL;
#endif
#endif

#ifdef ARCH_ARM
	    if( s->calling_convention == PIX_CCONV_DEFAULT )
		s->calling_convention = PIX_CCONV_CDECL;
#endif

#ifdef ARCH_X86_64
	    uint64 retval = 0;
	    switch( s->calling_convention )
	    {
		case PIX_CCONV_UNIX_AMD64:
		{
		    //Alloc stack frame:
		    const int stack_frame_size = 16;
		    uint64 stack_frame[ stack_frame_size ];
		    uint64* stack_frame_addr = &stack_frame[ 0 ];
		    
		    //Create lists with parameters:
		    uint64 int_regs[ 6 ]; //RDI, RSI, RDX, RCX, R8, R9
		    uint64 fp_regs[ 8 ]; //XMM0 - XMM7
		    uint64* int_regs_addr = &int_regs[ 0 ];
		    uint64* fp_regs_addr = &fp_regs[ 0 ];
		    int int_num = 0;
		    int fp_num = 0;
		    int stack_num = 0;
		    int pars_error = 0;
		    for( int i = 0; i < native_pars_num; i++ )
		    {
			if( pars_error ) break;
			int ctype = s->pars[ 2 + i ];
			int type = stack_types[ sp + 2 + i ];
			PIX_INT ival = stack[ sp + 2 + i ].i;
			PIX_FLOAT fval = stack[ sp + 2 + i ].f;
			switch( ctype )
			{
			    case 'f':
				// FLOAT 32
				{
				    float* reg;
				    if( fp_num >= 8 )
				    {
					if( stack_num >= stack_frame_size ) { pars_error = 1; break; }
					reg = (float*)&stack_frame[ stack_num ];
					stack_num++;
				    }
				    else
				    {
					reg = (float*)&fp_regs[ fp_num ];
					fp_num++;
				    }
				    if( type == 0 )
					*reg = (float)ival;
				    else
					*reg = (float)fval;
				}
				break;
			    case 'd':
				// FLOAT 64
				{
				    double* reg;
				    if( fp_num >= 8 )
				    {
					if( stack_num >= stack_frame_size ) { pars_error = 1; break; }
					reg = (double*)&stack_frame[ stack_num ];
					stack_num++;
				    }
				    else
				    {
					reg = (double*)&fp_regs[ fp_num ];
					fp_num++;
				    }
				    if( type == 0 )
					*reg = (double)ival;
				    else
					*reg = (double)fval;
				}
				break;
			    default:
				// INT / POINTER
				{
				    if( type == 1 ) ival = (PIX_INT)fval;
				    uint64* reg;
				    if( int_num >= 6 )
				    {
					if( stack_num >= stack_frame_size ) { pars_error = 1; break; }
					reg = &stack_frame[ stack_num ];
					stack_num++;
				    }
				    else
				    {
					reg = &int_regs[ int_num ];
					int_num++;
				    }
				    if( ctype == 'p' )
				    {
					//Pointer:
					void* ptr = pix_vm_get_container_data( ival, vm );
					*reg = (uint64)ptr;
				    }
				    else
				    {
					//Int:
					switch( ctype )
					{
					    case 'c':
					    case 'C':
						{
						    *reg = (unsigned)ival & 0xFF;
						}
						break;
					    case 's':
					    case 'S':
						{
						    *reg = (unsigned)ival & 0xFFFF;
						}
						break;
					    case 'i':
					    case 'I':
						{
						    *reg = (unsigned)ival & 0xFFFFFFFF;
						}
						break;
					    case 'l':
					    case 'L':
						{
						    ((signed long long*)reg)[ 0 ] = ival;
						}
						break;
					}
				    }
				}
				break;
			}
		    }
		    if( pars_error )
		    {
			if( pars_error == 1 )
			{
			    PIX_VM_LOG( "Not enough space in stack frame (max %d parameters). Function: %s\n", stack_frame_size, s->name );
			}
			break;
		    }
		    
		    //Execute:
		    asm volatile (
			//Stack parameters:
			"movq %%rsp, %%rcx\n" //rcx = rsp - stack_frame_size
			"subq %[stack_frame_size], %%rcx\n"
			"movq %[stack_frame], %%rbx\n" //rbx = &stack_frame
			"movl %[stack_num], %%eax\n" //eax = stack_num (number of parameters in stack)
			"cmpl $0, %%eax\n"
			"jz no_stack\n"
			"next_stack_item:\n"
			"movq (%%rbx), %%rdx\n"
			"movq %%rdx, (%%rcx)\n"
			"addq $8, %%rcx\n"
			"addq $8, %%rbx\n"
			"decl %%eax\n"
			"jnz next_stack_item\n"
			"no_stack:\n"

			//Int registers:
			"movq %[int_regs], %%rbx\n" //rbx = &int_regs
			"movl %[int_num], %%eax\n" //eax = int_num (number of int parameters in int registers)
			"cmpl $0, %%eax\n"
			"jz no_int_regs\n"
			"movq (%%rbx), %%rdi\n" //int reg 0 (RDI)
			"decl %%eax\n"
			"jz no_int_regs\n"
			"movq 8(%%rbx), %%rsi\n" //int reg 1 (RSI)
			"decl %%eax\n"
			"jz no_int_regs\n"
			"movq 16(%%rbx), %%rdx\n" //int reg 2 (RDX)
			"decl %%eax\n"
			"jz no_int_regs\n"
			"movq 24(%%rbx), %%rcx\n" //int reg 3 (RCX)
			"decl %%eax\n"
			"jz no_int_regs\n"
			"movq 32(%%rbx), %%r8\n" //int reg 4 (R8)
			"decl %%eax\n"
			"jz no_int_regs\n"
			"movq 40(%%rbx), %%r9\n" //int reg 5 (R9)
			"no_int_regs:\n"
					
			//FP registers:
			"movq %[fp_regs], %%rbx\n" //rbx = &fp_regs
			"movl %[fp_num], %%eax\n" //eax = fp_num (number of float parameters in xmm registers)
			"cmpl $0, %%eax\n"
			"jz no_fp_regs\n"
			"movq (%%rbx), %%xmm0\n" //fp reg 0 (xmm0)
			"decl %%eax\n"
			"jz no_fp_regs\n"
			"movq 8(%%rbx), %%xmm1\n" //fp reg 1 (xmm1)
			"decl %%eax\n"
			"jz no_fp_regs\n"
			"movq 16(%%rbx), %%xmm2\n" //fp reg 2 (xmm2)
			"decl %%eax\n"
			"jz no_fp_regs\n"
			"movq 24(%%rbx), %%xmm3\n" //fp reg 3 (xmm3)
			"decl %%eax\n"
			"jz no_fp_regs\n"
			"movq 32(%%rbx), %%xmm4\n" //fp reg 4 (xmm4)
			"decl %%eax\n"
			"jz no_fp_regs\n"
			"movq 40(%%rbx), %%xmm5\n" //fp reg 5 (xmm5)
			"decl %%eax\n"
			"jz no_fp_regs\n"
			"movq 48(%%rbx), %%xmm6\n" //fp reg 6 (xmm6)
			"decl %%eax\n"
			"jz no_fp_regs\n"
			"movq 56(%%rbx), %%xmm7\n" //fp reg 7 (xmm7)
			"no_fp_regs:\n"

			//Execute:
			"movl %[stack_num], %%eax\n" //eax = stack_num (number of parameters in stack)
			"cmpl $0, %%eax\n"
			"jz call_without_stack\n"
			    "movq %[fn_addr], %%rax\n"
			    "subq %[stack_frame_size], %%rsp\n" //Alloc stack frame
			    "callq *%%rax\n"
			    "addq %[stack_frame_size], %%rsp\n" //Dealloc stack frame
			    "jmp call_end\n"
			"call_without_stack:\n"
			"movq %[fn_addr], %%rax\n"
			"callq *%%rax\n"
			"call_end:\n"
			"cmpl $0, %[ret_is_fp]\n"
			"jnz ret_fp\n"
			"movq %%rax, %[retval]\n" //Save return value (int)
			"jmp call_end2\n"
			"ret_fp:\n"
			"movq %%xmm0, %[retval]\n" //Save return value (float)
			"call_end2:\n"
					
			: //output:
			[retval]"=m"(retval)
			: //input:
			[int_num]"m"(int_num),
			[int_regs]"m"(int_regs_addr),
			[fp_num]"m"(fp_num),
			[fp_regs]"m"(fp_regs_addr),
			[stack_num]"m"(stack_num),
			[stack_frame]"m"(stack_frame_addr),
			[stack_frame_size]"i"(stack_frame_size*8),
			[fn_addr]"m"(fn_addr),
			[ret_is_fp]"m"(ret_is_fp)
			: //clobbered:
			"memory", 
			"cc", //condition code register
			"rax", "rbx", "rcx", "rdx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
		    );
		    
		    break;
		}
		
		default:
		    PIX_VM_LOG( "Can't call function %s. Unsupported calling convention %d.\n", s->name, s->calling_convention );
		    break;
	    }
#endif
	    
#ifdef ARCH_X86
	    uint retval = 0;
	    switch( s->calling_convention )
	    {
		case PIX_CCONV_CDECL:
		case PIX_CCONV_STDCALL:
		{
		    //Alloc stack frame:
		    const int stack_frame_size = 32 * 2;
		    uint stack_frame[ stack_frame_size ];
		    uint* stack_frame_addr = &stack_frame[ 0 ];
		    
		    //Create lists with parameters:
		    int stack_num = 0;
		    int pars_error = 0;
		    for( int i = 0; i < native_pars_num; i++ )
		    {
			int ctype = s->pars[ 2 + i ];
			int type = stack_types[ sp + 2 + i ];
			PIX_INT ival = stack[ sp + 2 + i ].i;
			PIX_FLOAT fval = stack[ sp + 2 + i ].f;
			switch( ctype )
			{
			    case 'f':
				// FLOAT 32
				{
				    if( stack_num >= stack_frame_size ) { pars_error = 1; break; }
				    float* v = (float*)&stack_frame[ stack_num ];
				    stack_num++;
				    if( type == 0 )
					*v = (float)ival;
				    else
					*v = (float)fval;
				}
				break;
			    case 'd':
				// FLOAT 64
				{
				    if( stack_num + 1 >= stack_frame_size ) { pars_error = 1; break; }
				    double v;
				    if( type == 0 )
					v = (double)ival;
				    else
					v = (double)fval;
				    uint* iv = (uint*)&v;
				    stack_frame[ stack_num ] = iv[ 0 ]; stack_num++;
				    stack_frame[ stack_num ] = iv[ 1 ]; stack_num++;
				}
				break;
			    case 'l':
				// SIGNED INT 64
				{
				    if( stack_num + 1 >= stack_frame_size ) { pars_error = 1; break; }
				    int64 v;
				    if( type == 0 )
					v = (int64)ival;
				    else
					v = (int64)fval;
				    uint* iv = (uint*)&v;
				    stack_frame[ stack_num ] = iv[ 0 ]; stack_num++;
				    stack_frame[ stack_num ] = iv[ 1 ]; stack_num++;
				}
				break;
			    case 'L':
				// UNSIGNED INT 64
				{
				    if( stack_num + 1 >= stack_frame_size ) { pars_error = 1; break; }
				    uint64 v;
				    if( type == 0 )
					v = (uint64)((unsigned)ival);
				    else
					v = (uint64)fval;
				    uint* iv = (uint*)&v;
				    stack_frame[ stack_num ] = iv[ 0 ]; stack_num++;
				    stack_frame[ stack_num ] = iv[ 1 ]; stack_num++;
				}
				break;
			    default:
				// INT / POINTER
				{
				    if( stack_num >= stack_frame_size ) { pars_error = 1; break; }
				    if( type == 1 ) ival = (PIX_INT)fval;
				    uint* v = &stack_frame[ stack_num ];
				    stack_num++;
				    if( ctype == 'p' )
				    {
					//Pointer:
					void* ptr = pix_vm_get_container_data( ival, vm );
					*v = (uint)ptr;
				    }
				    else
				    {
					//Int:
					switch( ctype )
					{
					    case 'c':
					    case 'C':
						{
						    *v = (unsigned)ival & 0xFF;
						}
						break;
					    case 's':
					    case 'S':
						{
						    *v = (unsigned)ival & 0xFFFF;
						}
						break;
					    case 'i':
					    case 'I':
						{
						    *v = (unsigned)ival & 0xFFFFFFFF;
						}
						break;
					}
				    }
				}
				break;
			}
		    }
		    if( pars_error )
		    {
			if( pars_error == 1 )
			{
			    PIX_VM_LOG( "Not enough space in stack frame (max %d parameters). Function: %s\n", stack_frame_size, s->name );
			}
			break;
		    }
		    
		    //Execute:
		    if( s->calling_convention == PIX_CCONV_CDECL )
		    {
			asm volatile (
			    //Push the new stack frame with parameters:
			    "movl %%esp, %%edi\n" //edi = esp - stack_frame_size
			    "subl %[stack_frame_size], %%edi\n"
			    "movl %[stack_frame], %%esi\n" //ebx = &stack_frame
			    "movl %[stack_num], %%eax\n" //eax = stack_num (number of parameters in stack)
			    "cmpl $0, %%eax\n"
			    "jz no_stack\n"
			    "next_stack_item:\n"
			    "movl (%%esi), %%edx\n"
			    "movl %%edx, (%%edi)\n"
			    "addl $4, %%edi\n"
			    "addl $4, %%esi\n"
			    "decl %%eax\n"
			    "jnz next_stack_item\n"
			    "no_stack:\n"
			    
			    //Execute:
			    "movl %[stack_num], %%eax\n" //eax = stack_num (number of parameters in stack)
			    "cmpl $0, %%eax\n"
			    "jz call_without_stack\n"
				"movl %[fn_addr], %%eax\n"
				"subl %[stack_frame_size], %%esp\n" //Alloc stack frame
				"call *%%eax\n"
				"addl %[stack_frame_size], %%esp\n" //Dealloc stack frame
				"jmp call_end\n"
			    "call_without_stack:\n"
			    "movl %[fn_addr], %%eax\n"
			    "call *%%eax\n"
			    "call_end:\n"
			    "cmpl $0, %[ret_is_fp]\n"
			    "jnz ret_fp\n"
			    "movl %%eax, %[retval]\n" //Save return value (int)
			    "jmp call_end2\n"
			    "ret_fp:\n"
			    "fst %[retval]\n" //Save return value (float)
			    "call_end2:\n"

			    : //output:
			    : //input:
			    [stack_num]"m"(stack_num),
			    [stack_frame]"m"(stack_frame_addr),
			    [stack_frame_size]"i"(stack_frame_size),
			    [fn_addr]"m"(fn_addr),
			    [retval]"m"(retval),
			    [ret_is_fp]"m"(ret_is_fp)
			    : //clobbered:
			    "memory",
			    "cc", //condition code register
#ifndef __PIC__
			    "ebx", //don't touch this register in PIC (position-independent code)
#endif
			    "eax", "ecx", "edx", "edi", "esi"
			);
		    }
		    else
		    {
			asm volatile (
			    "movl %[fn_addr], %%ecx\n" //ecx = fn_addr
			    
			    //Push the new stack frame with parameters:
			    "movl %[stack_frame], %%esi\n" //ebx = &stack_frame
			    "movl %[stack_num], %%eax\n" //eax = stack_num (number of parameters in stack)
			    "cmpl $0, %%eax\n"
			    "jz std_no_stack\n"
			    "shl $2, %%eax\n" //eax *= 4
			    "addl %%eax, %%esi\n" //esi += stack_num
			    "std_next_stack_item:\n"
			    "subl $4, %%esi\n"
			    "pushl (%%esi)\n"
			    "subl $4, %%eax\n"
			    "jnz std_next_stack_item\n"
			    "std_no_stack:\n"
			    
			    //Execute:
			    "call *%%ecx\n"
			    "cmpl $0, %[ret_is_fp]\n"
			    "jnz std_ret_fp\n"
			    "movl %%eax, %[retval]\n" //Save return value (int)
			    "jmp std_call_end\n"
			    "std_ret_fp:\n"
			    "fst %[retval]\n" //Save return value (float)
			    "std_call_end:\n"
			
			    : //output:
			    : //input:
			    [stack_num]"m"(stack_num),
			    [stack_frame]"m"(stack_frame_addr),
			    [stack_frame_size]"i"(stack_frame_size),
			    [fn_addr]"m"(fn_addr),
			    [retval]"m"(retval),
			    [ret_is_fp]"m"(ret_is_fp)
			    : //clobbered:
			    "memory",
			    "cc", //condition code register
#ifndef __PIC__
			    "ebx", //don't touch this register in PIC (position-independent code)
#endif
			    "eax", "ecx", "edx", "edi", "esi"
			);
		    }
		    
		    break;
		}
		    
		default:
		    PIX_VM_LOG( "Can't call function %s. Unsupported calling convention %d.\n", s->name, s->calling_convention );
		    break;
	    }
#endif

#ifdef ARCH_ARM
	    uint retval = 0;
	    switch( s->calling_convention )
	    {
		case PIX_CCONV_CDECL:
		{
		    //Alloc stack frame:
		    const int stack_frame_size = 16;
		    uint stack_frame[ stack_frame_size ];
		    uint* stack_frame_addr = &stack_frame[ 0 ];
		    
		    //Create lists with parameters:
		    const int int_regs_num = 4;
		    uint int_regs[ int_regs_num ]; //R0 .. R3
		    uint* int_regs_addr = &int_regs[ 0 ];
#ifdef HARDFP
		    const int fp_regs_num = 16;
		    uint64 fp_regs[ fp_regs_num ]; //D0 .. D15
		    uint64* fp_regs_addr = &fp_regs[ 0 ];
		    int fp_num = 0;
#endif
		    int int_num = 0;
		    int stack_num = 0;
		    int pars_error = 0;
		    for( int i = 0; i < native_pars_num; i++ )
		    {
			if( pars_error ) break;
			int ctype = s->pars[ 2 + i ];
			int type = stack_types[ sp + 2 + i ];
			PIX_INT ival = stack[ sp + 2 + i ].i;
			PIX_FLOAT fval = stack[ sp + 2 + i ].f;
			switch( ctype )
			{
			    case 'f':
				// FLOAT 32
				{
				    float* reg;
#ifdef HARDFP
				    if( fp_num < fp_regs_num )
				    {
					reg = (float*)&fp_regs[ fp_num ];
					fp_num++;
				    }
				    else
#else
				    if( int_num < int_regs_num )
				    {
					reg = (float*)&int_regs[ int_num ];
					int_num++;
				    }
				    else
#endif
				    {
					if( stack_num >= stack_frame_size ) { pars_error = 1; break; }
					reg = (float*)&stack_frame[ stack_num ];
					stack_num++;
				    }
				    if( type == 0 )
					*reg = (float)ival;
				    else
					*reg = (float)fval;
				}
				break;
			    case 'd':
				// FLOAT 64
				{
				    double* reg;
				    bool write_to_stack = 1;
#ifdef HARDFP
				    if( fp_num < fp_regs_num )
				    {
					reg = (double*)&fp_regs[ fp_num ];
					fp_num++;
					write_to_stack = 0;
				    }
#else
				    if( int_num < int_regs_num - 1 )
				    {
					if( int_num & 1 ) int_num++;
					if( int_num < int_regs_num - 1 )
					{
					    reg = (double*)&int_regs[ int_num ];
					    int_num += 2;
					    write_to_stack = 0;
					}
				    }
#endif
				    if( write_to_stack )
				    {
					if( stack_num >= stack_frame_size - 1 ) { pars_error = 1; break; }
					if( stack_num & 1 ) stack_num++;
					if( stack_num >= stack_frame_size - 1 ) { pars_error = 1; break; }
					reg = (double*)&stack_frame[ stack_num ];
					stack_num += 2;
				    }
				    if( type == 0 )
					*reg = (double)ival;
				    else
					*reg = (double)fval;
				}
				break;
			    case 'l':
			    case 'L':
				// INT 64
				{
				    int64* reg;
				    bool write_to_stack = 1;
				    if( int_num < int_regs_num - 1 )
				    {
					if( int_num & 1 ) int_num++;
					if( int_num < int_regs_num - 1 )
					{
					    reg = (int64*)&int_regs[ int_num ];
					    int_num += 2;
					    write_to_stack = 0;
					}
				    }
				    if( write_to_stack )
				    {
					if( stack_num >= stack_frame_size - 1 ) { pars_error = 1; break; }
					if( stack_num & 1 ) stack_num++;
					if( stack_num >= stack_frame_size - 1 ) { pars_error = 1; break; }
					reg = (int64*)&stack_frame[ stack_num ];
					stack_num += 2;
				    }
				    if( type == 0 )
					*reg = (int64)ival;
				    else
					*reg = (int64)fval;
				}
				break;
			    default:
				// INT / POINTER
				{
				    if( type == 1 ) ival = (PIX_INT)fval;
				    uint* reg;
				    if( int_num >= int_regs_num )
				    {
					if( stack_num >= stack_frame_size ) { pars_error = 1; break; }
					reg = &stack_frame[ stack_num ];
					stack_num++;
				    }
				    else
				    {
					reg = &int_regs[ int_num ];
					int_num++;
				    }
				    if( ctype == 'p' )
				    {
					//Pointer:
					void* ptr = pix_vm_get_container_data( ival, vm );
					*reg = (uint)ptr;
				    }
				    else
				    {
					//Int:
					switch( ctype )
					{
					    case 'c':
					    case 'C':
						{
						    *reg = (unsigned)ival & 0xFF;
						}
						break;
					    case 's':
					    case 'S':
						{
						    *reg = (unsigned)ival & 0xFFFF;
						}
						break;
					    case 'i':
					    case 'I':
						{
						    *reg = (unsigned)ival & 0xFFFFFFFF;
						}
						break;
					}
				    }
				}
				break;
			}
		    }
		    if( pars_error )
		    {
			if( pars_error == 1 )
			{
			    PIX_VM_LOG( "Not enough space in stack frame (max %d parameters). Function: %s\n", stack_frame_size, s->name );
			}
			break;
		    }

		    //Execute:
		    asm volatile (
			//Stack parameters:
			"mov r4, sp\n" //r4 = sp - stack_frame_size
			"mov r5, %[stack_frame_size]\n"
			"sub r4, r4, r5\n"
			"ldr r5, %[stack_frame]\n" //r5 = &stack_frame
			"ldr r6, %[stack_num]\n" //r6 = stack_num (number of parameters in stack)
			"cmp r6, $0\n"
			"beq no_stack\n"
			"next_stack_item:\n"
			"ldr r8, [r5]\n"
			"str r8, [r4]\n"
			"add r4, r4, $4\n"
			"add r5, r5, $4\n"
			"subs r6, r6, $1\n"
			"bne next_stack_item\n"
			"no_stack:\n"

			//Int registers:
			"ldr r4, %[int_regs]\n" //r4 = &int_regs
			"ldr r5, %[int_num]\n" //r5 = int_num (number of int parameters in int registers)
			"cmp r5, $0\n"
			"beq no_int_regs\n"
			"ldr r0, [r4]\n" //int reg 0 (r0)
			"subs r5, r5, $1\n"
			"beq no_int_regs\n"
			"ldr r1, [r4,$4]\n" //int reg 1 (r1)
			"subs r5, r5, $1\n"
			"beq no_int_regs\n"
			"ldr r2, [r4,$8]\n" //int reg 2 (r2)
			"subs r5, r5, $1\n"
			"beq no_int_regs\n"
			"ldr r3, [r4,$12]\n" //int reg 3 (r3)
			"no_int_regs:\n"

			//Execute:
			"ldr r4, %[fn_addr]\n"
			"ldr r6, %[stack_num]\n" //r6 = stack_num (number of parameters in stack)
			"cmp r6, $0\n"
			"beq call_without_stack\n"
			    "sub sp, sp, %[stack_frame_size]\n" //Alloc stack frame
			    "mov lr, pc\n"
			    "bx r4\n"
			    "add sp, sp, %[stack_frame_size]\n" //Dealloc stack frame
			    "b call_end\n"
			"call_without_stack:\n"
			"mov lr, pc\n"
			"bx r4\n"
			"call_end:\n"
			"str r0, %[retval]\n" //Save return value (int)

			: //output:
			[retval]"=m"(retval)
			: //input:
			[int_num]"m"(int_num),
			[int_regs]"m"(int_regs_addr),
			[stack_num]"m"(stack_num),
			[stack_frame]"m"(stack_frame_addr),
			[stack_frame_size]"i"(stack_frame_size*4),
			[fn_addr]"m"(fn_addr)
			: //clobbered:
			"memory", 
			"cc", //condition code register
#ifdef ARM_VFP
			"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
#endif
			"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r12", "r13", "r14"
		    );
		    
		    break;
		}

		default:
		    PIX_VM_LOG( "Can't call function %s. Unsupported calling convention %d.\n", s->name, s->calling_convention );
		    break;
	    }
#endif
	    
	    //Save return value:
    	    if( ret_type >= 0 )
    	    {
    		PIX_VAL r;
    		r.i = 0;
    		r.f = 0;
    		int r_type = 0;
    		switch( ret_type )
    		{
    		    case 'c':
    			{
    			    signed char* v = (signed char*)&retval;
    			    r_type = 0;
			    r.i = *v;
			}
    			break;
    		    case 'C':
    			{
    			    unsigned char* v = (unsigned char*)&retval;
    			    r_type = 0;
			    r.i = *v;
			}
    			break;
    		    case 's':
    			{
    			    signed short* v = (signed short*)&retval;
    			    r_type = 0;
			    r.i = *v;
			}
    			break;
    		    case 'S':
    			{
    			    unsigned short* v = (unsigned short*)&retval;
    			    r_type = 0;
			    r.i = *v;
			}
    			break;
    		    case 'i':
    			{
    			    signed int* v = (signed int*)&retval;
    			    r_type = 0;
			    r.i = *v;
			}
    			break;
    		    case 'I':
    			{
    			    unsigned int* v = (unsigned int*)&retval;
    			    r_type = 0;
			    r.i = *v;
			}
    			break;
    		    case 'l':
    			{
    			    signed long long* v = (signed long long*)&retval;
    			    r_type = 0;
			    r.i = *v;
			}
    			break;
    		    case 'L':
    			{
    			    unsigned long long* v = (unsigned long long*)&retval;
    			    r_type = 0;
			    r.i = *v;
			}
    			break;
    		    case 'f':
    			{
    			    float* v = (float*)&retval;
    			    r_type = 1;
			    r.f = *v;
			}
    			break;
    		    case 'd':
    			{
    			    double* v = (double*)&retval;
    			    r_type = 1;
			    r.f = *v;
			}
    			break;
    		    case 'p':
    			{
    			    void** v = (void**)&retval;
    			    r_type = 0;
    			    if( *v == 0 )
    				r.i = -1;
    			    else
    			    {
    				r.i = pix_vm_new_container( -1, 1024 * 1024 * 1024, 1, PIX_CONTAINER_TYPE_INT8, *v, vm );
    				pix_vm_set_container_flags( r.i, pix_vm_get_container_flags( r.i, vm ) | PIX_CONTAINER_FLAG_STATIC_DATA, vm );
    			    }
			}
    			break;
		    default:
			break;
    		}
    		stack_types[ sp + ( pars_num - 1 ) ] = r_type;
	        stack[ sp + ( pars_num - 1 ) ] = r;
    	    }
	}
	
	break;
    }
    
#endif
}
