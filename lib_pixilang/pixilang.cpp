/*
    pixilang.cpp
    This file is part of the Pixilang programming language.

    Copyright (c) 2006 - 2016, Alexander Zolotov <nightradio@gmail.com>
    www.warmplace.ru
    All rights reserved.

    Redistribution and use in source and binary forms, with or without 
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, 
    this list of conditions and the following disclaimer in the documentation 
    and/or other materials provided with the distribution.
    * The name of the author may not be used to endorse or promote products derived 
    from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//Modularity: 100%

#include "core/core.h"
#include "pixilang.h"

int pix_init( pix_data* pd )
{
    int rv = 0;
    
    if( NUMBER_OF_OPCODES > ( 1 << PIX_OPCODE_BITS ) )
    {
        blog( "ERROR: NUMBER_OF_OPCODES (%d) > ( 1 << PIX_OPCODE_BITS )\n", NUMBER_OF_OPCODES );
        return -1;
    }
    if( FN_NUM > ( 1 << PIX_FN_BITS ) )
    {
        blog( "ERROR: FN_NUM (%d) > ( 1 << PIX_FN_BITS )\n", FN_NUM );
        return -2;
    }
    if( PIX_OPCODE_BITS + PIX_FN_BITS > sizeof( PIX_OPCODE ) * 8 )
    {
	blog( "ERROR: PIX_OPCODE_BITS (%d) + PIX_FN_BITS (%d)  > PIX_OPCODE SIZE (%d)\n", PIX_OPCODE_BITS, PIX_FN_BITS, sizeof( PIX_OPCODE ) * 8 );
	return -1;
    }
    if( sizeof( PIX_OPCODE ) * 8 - ( PIX_OPCODE_BITS + PIX_FN_BITS ) < 5 )
    {
	blog( "ERROR: Not enough bits (%d) for the number of builtin function parameters\n", sizeof( PIX_OPCODE ) * 8 - ( PIX_OPCODE_BITS + PIX_FN_BITS ) );
	return -1;
    }
    
    bmem_set( pd, sizeof( pix_data ), 0 );

    bmutex_init( &pd->compiler_mutex, 0 );
    
    return rv;
}

int pix_deinit( pix_data* pd )
{
    int rv = 0;

    bmutex_destroy( &pd->compiler_mutex );

    return rv;
}
