%{
/*
    pixilang_compiler.y
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
#include "pixilang_font.h"

#include "zlib.h"

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
    #define DPRINT( fmt, ARGS... ) blog( fmt, ## ARGS )
#else
    #define DPRINT( fmt, ARGS... ) {}
#endif
    
#define ERROR( fmt, ARGS... ) blog( "ERROR in %s: " fmt "\n", __FUNCTION__, ## ARGS )
#define SHOW_ERROR( fmt, ARGS... ) \
    { \
	utf8_char* ts = (utf8_char*)bmem_new( bmem_strlen( g_comp->src_name ) + 2048 ); \
	blog( "ERROR (line %d) in %s: " fmt "\n", g_comp->src_line + 1, g_comp->src_name, ## ARGS ); \
	sprintf( ts, "ERROR (line %d)\nin %s:\n" fmt, g_comp->src_line + 1, g_comp->src_name, ## ARGS ); \
	dialog( ts, wm_get_string( STR_WM_CLOSE ), g_comp->vm->wm ); \
	bmem_free( ts ); \
    }
    
#define NUMERIC( val ) ( val >= '0' && val <= '9' )
#define ABC( val ) ( ( val >= 'a' && val <= 'z' ) || ( val >= 'A' && val <= 'Z' ) || ( (unsigned)val >= 128 && val != -1 ) )

enum lnode_type
{
    lnode_empty,
    
    lnode_statlist, //statements
    
    lnode_int,
    lnode_float,
    lnode_var,
    
    lnode_halt,
    
    lnode_label,
    lnode_function_label_from_node,
    
    lnode_go,
    
    lnode_jmp_to_node,
    lnode_jmp_to_end_of_node,
    
    lnode_if,
    lnode_if_else,
    lnode_while,
    
    lnode_save_to_var,
    
    lnode_save_to_prop,
    lnode_load_from_prop,

    lnode_save_to_mem,
    lnode_load_from_mem,
    
    lnode_save_to_stackframe,
    lnode_load_from_stackframe,
    
    lnode_sub,
    lnode_add,
    lnode_mul,
    lnode_idiv,
    lnode_div,
    lnode_mod,
    lnode_and,
    lnode_or,
    lnode_xor,
    lnode_andand,
    lnode_oror,
    lnode_eq,
    lnode_neq,
    lnode_less,
    lnode_leq,
    lnode_greater,
    lnode_geq,
    lnode_lshift,
    lnode_rshift,
    
    lnode_neg,

    lnode_exprlist,
    lnode_call_builtin_fn,
    lnode_call_builtin_fn_void,
    lnode_call,
    lnode_call_void,
    lnode_ret_int,
    lnode_ret,
    lnode_inc_sp,
};
    
#define LNODE_FLAG_STATLIST_AS_EXPRESSION		1
#define LNODE_FLAG_STATLIST_WITH_JMP_HEADER		2
#define LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER	4
#define LNODE_FLAG_STATLIST_SKIP_NEXT_HEADER		8

struct lnode
{
    lnode_type type;
    uchar flags;
    size_t code_ptr; //Start of node
    size_t code_ptr2; //End of node (start of the next node)
    PIX_VAL val;
    lnode** n; //Children
    int nn; //Children count
};

#define VAR_FLAG_LABEL					1
#define VAR_FLAG_FUNCTION				2
#define VAR_FLAG_INITIALIZED				4
#define VAR_FLAG_USED					8

#define LVAR_OFFSET 3
    
struct pix_lsymtab
{
    pix_symtab* lsym; //Local symbol table
    int lvars_num; //Number of local variables
    char* lvar_flags; //Flags of local variables
    utf8_char** lvar_names;
    size_t lvar_flags_size;
    int pars_num; //Number of local function parameters
};
    
struct pix_include
{
    utf8_char* src;
    int src_ptr;
    int src_line;
    int src_size;
    utf8_char* src_name;
    utf8_char* base_path;
};

struct pix_compiler
{
    lnode* root; //Root lexical node
    
    utf8_char* src;
    int src_ptr;
    int src_line;
    int src_size;
    utf8_char* src_name;
    
    pix_symtab sym; //Global symbol table
    pix_lsymtab* lsym; //Local symbol tables
    int lsym_num; //Number of current local symbol table
    utf8_char temp_sym_name[ 256 + 1 ];
    
    char* var_flags;
    size_t var_flags_size;

    int statlist_header_size; //Maximum length of JMP instruction
    
    bool fn_pars_mode; //Treat new lvars as function parameters
    
    utf8_char* base_path; //May be not equal to vm->base_path
    
    pix_include* inc; //Stack for includes
    int inc_num;
    
    lnode** while_stack;
    int while_stack_ptr;
    
    lnode** fixup;
    int fixup_num;
    
    pix_vm* vm;
    pix_data* pd;
};

pix_compiler* g_comp = 0;

int yylex( void );
void yyerror( char const* str );
void push_int( PIX_INT v );
lnode* node( lnode_type type, int nn );
void resize_node( lnode* n, int nn );
lnode* clone_tree( lnode* n );
void remove_tree( lnode* n );
void optimize_tree( lnode* n );
void compile_tree( lnode* n );

lnode* make_expr_node_from_var( PIX_INT i )
{
    lnode* n = node( lnode_var, 0 ); 
    n->val.i = i;
    return n;
}

lnode* make_expr_node_from_local_var( PIX_INT i )
{
    lnode* n = node( lnode_load_from_stackframe, 0 );
    n->val.i = i;
    return n;
}

void create_empty_lsym_table( void )
{
    g_comp->lsym_num++;
    if( g_comp->lsym_num >= bmem_get_size( g_comp->lsym ) / sizeof( pix_lsymtab ) )
	g_comp->lsym = (pix_lsymtab*)bmem_resize( g_comp->lsym, ( g_comp->lsym_num + 8 ) * sizeof( pix_lsymtab ) );
    pix_lsymtab* l = &g_comp->lsym[ g_comp->lsym_num ];
    bmem_set( l, sizeof( pix_lsymtab ), 0 );
    l->lvar_flags = (char*)bmem_new( 8 );
    l->lvar_names = (utf8_char**)bmem_new( 8 * sizeof( utf8_char* ) );
    bmem_zero( l->lvar_flags );
    bmem_zero( l->lvar_names );
    l->lvar_flags_size = 8;
}

lnode* remove_lsym_table( lnode* statlist )
{
    lnode* new_tree;
    
    int lvars_num = g_comp->lsym[ g_comp->lsym_num ].lvars_num;
    if( lvars_num )
    {
	new_tree = node( lnode_statlist, 2 );
	new_tree->n[ 0 ] = node( lnode_inc_sp, 0 );
	new_tree->n[ 0 ]->val.i = -lvars_num;
	new_tree->n[ 1 ] = statlist;
    }
    else 
    {
	new_tree = statlist;
    }

    pix_lsymtab* l = &g_comp->lsym[ g_comp->lsym_num ];
    for( size_t n = 0; n < l->lvar_flags_size; n++ )
    {
	if( l->lvar_names[ n ] )
	{
	    if( ( l->lvar_flags[ n ] & VAR_FLAG_INITIALIZED ) == 0 )
	    {
		SHOW_ERROR( "local variable %s is not initialized", l->lvar_names[ n ] );
		remove_tree( new_tree );
		new_tree = 0;
		break;
	    }
	    bmem_free( l->lvar_names[ n ] );
	}
    }
    bmem_free( l->lvar_flags );
    bmem_free( l->lvar_names );
    l->lvar_flags = 0;
    l->lvar_names = 0;
    if( l->lsym )
    {
	pix_symtab_deinit( l->lsym );
	bmem_free( l->lsym );
	l->lsym = 0;
	l->lvars_num = 0;
	l->pars_num = 0;
    }
    g_comp->lsym_num--;
    
    return new_tree;
}

%}

%union //Possible types for yylval and yyval:
{
    PIX_INT i;
    PIX_FLOAT f;
    lnode* n;
}

%token NUM_I NUM_F GVAR LVAR WHILE BREAK CONTINUE IF ELSE GO RET FNNUM FNDEF INCLUDE HALT
%left OROR ANDAND
%left OR XOR AND
%left EQ NEQ '<' '>' LEQ GEQ
%left LSHIFT RSHIFT
%left '+' '-'
%left '*' '/' IDIV '%' HASH
%nonassoc NEG /* negation--unary minus */

%%
    //#######################
    //## PIXILANG RULES    ##
    //#######################
    
    // 2 * 3 * 4 = node2( node1( 2 * 3 ) * 4 )

input
    : /* empty */
    | input stat
        {
            DPRINT( "input stat\n" );
            resize_node( g_comp->root, g_comp->root->nn + 1 );
            g_comp->root->n[ g_comp->root->nn - 1 ] = $2.n;
        }
    ;
lvarslist
    : /* empty */
    | LVAR
    | lvarslist ',' LVAR
    ;
mem_offset
    : expr { $$.n = $1.n; }
    | expr ',' expr 
	{
	    $$.n = node( lnode_empty, 2 );
	    $$.n->n[ 0 ] = $1.n;
	    $$.n->n[ 1 ] = $3.n;
	}
    ;
stat_math_op
    : '-' { $$.i = 0; }
    | '+' { $$.i = 1; }
    | '*' { $$.i = 2; }
    | IDIV { $$.i = 3; }
    | '/' { $$.i = 4; }
    | '%' { $$.i = 5; }
    | AND { $$.i = 6; }
    | OR { $$.i = 7; }
    | XOR { $$.i = 8; }
    | ANDAND { $$.i = 9; }
    | OROR { $$.i = 10; }
    | EQ { $$.i = 11; }
    | NEQ { $$.i = 12; }
    | '<' { $$.i = 13; }
    | LEQ { $$.i = 14; }
    | '>' { $$.i = 15; }
    | GEQ { $$.i = 16; }
    | LSHIFT { $$.i = 17; }
    | RSHIFT { $$.i = 18; }
    ;
statlist
    : /* empty */ { $$.n = node( lnode_statlist, 0 ); }
    | statlist stat
        {
	    DPRINT( "statlist stat\n" );
            resize_node( $1.n, $1.n->nn + 1 );
	    $1.n->n[ $1.n->nn - 1 ] = $2.n;
	    $$.n = $1.n;
        }
    ;
stat
    : HALT { DPRINT( "HALT\n" ); $$.n = node( lnode_halt, 0 ); }
    | GVAR ':' 
	{ 
	    DPRINT( "VAR(%d) :\n", (int)$1.i ); 
	    if( g_comp->var_flags[ $1.i ] & VAR_FLAG_LABEL )
	    {
		SHOW_ERROR( "label %s is already defined", pix_vm_get_variable_name( g_comp->vm, $1.i ) );
                YYERROR;
	    }
	    if( g_comp->var_flags[ $1.i ] & VAR_FLAG_FUNCTION )
	    {
		SHOW_ERROR( "label %s is already defined as function", pix_vm_get_variable_name( g_comp->vm, $1.i ) );
                YYERROR;
	    }
	    $$.n = node( lnode_label, 0 ); 
	    $$.n->val.i = $1.i;
	    g_comp->var_flags[ $1.i ] |= VAR_FLAG_LABEL | VAR_FLAG_INITIALIZED;
	}
    | GO expr { DPRINT( "GO expr\n" ); $$.n = node( lnode_go, 1 ); $$.n->n[ 0 ] = $2.n; }
    | GVAR '=' expr 
        { 
	    DPRINT( "GVAR(%d) = expr\n", (int)$1.i ); 
            $$.n = node( lnode_save_to_var, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = $3.n;
	    g_comp->var_flags[ $1.i ] |= VAR_FLAG_INITIALIZED;
        }
    | LVAR '=' expr 
        { 
	    DPRINT( "LVAR(%d) = expr\n", (int)$1.i );
	    $$.n = node( lnode_save_to_stackframe, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = $3.n;
	    if( $1.i < 0 )
	    {
		int lvar_num = -$1.i - LVAR_OFFSET;
		g_comp->lsym[ g_comp->lsym_num ].lvar_flags[ lvar_num ] |= VAR_FLAG_INITIALIZED;
	    }
        }
    | prop_expr '=' expr 
	{
	    DPRINT( "prop_expr = expr\n" );
	    $$.n = $1.n;
	    $$.n->type = lnode_save_to_prop;
	    resize_node( $$.n, $$.n->nn + 1 );
	    $$.n->n[ 1 ] = $3.n;
	}
    | prop_expr stat_math_op expr 
	{
	    DPRINT( "prop_expr stat_math_op(%d) expr\n", (int)$2.i );
	    //Create math operation:
	    lnode* op = node( (lnode_type)( lnode_sub + $2.i ), 2 ); 
	    op->n[ 0 ] = $1.n;
	    op->n[ 1 ] = $3.n;
	    //Result:
	    $$.n = clone_tree( $1.n );
	    $$.n->type = lnode_save_to_prop;
	    resize_node( $$.n, $$.n->nn + 1 );
	    $$.n->n[ 1 ] = op;
	}
    | mem_expr '=' expr
	{
	    DPRINT( "mem_expr = expr\n" );
	    $$.n = $1.n;
	    $$.n->type = lnode_save_to_mem;
	    resize_node( $$.n, $$.n->nn + 1 );
	    $$.n->n[ 2 ] = $3.n;
        }
    | mem_expr stat_math_op expr
	{
	    DPRINT( "mem_expr stat_math_op(%d) expr\n", (int)$2.i );
	    //Create math operation:
	    lnode* op = node( (lnode_type)( lnode_sub + $2.i ), 2 ); 
	    op->n[ 0 ] = $1.n;
	    op->n[ 1 ] = $3.n;
	    //Result:
	    $$.n = clone_tree( $1.n );
	    $$.n->type = lnode_save_to_mem;
	    resize_node( $$.n, $$.n->nn + 1 );
	    $$.n->n[ 2 ] = op;
	}    
    | GVAR stat_math_op expr 
	{
	    DPRINT( "GVAR(%d) stat_math_op(%d) expr\n", (int)$1.i, (int)$2.i );
	    //Create first operand:
	    lnode* n1 = make_expr_node_from_var( $1.i );
	    //Create math operation:
	    lnode* n2 = node( (lnode_type)( lnode_sub + $2.i ), 2 ); 
	    n2->n[ 0 ] = n1; 
	    n2->n[ 1 ] = $3.n;
	    //Save result:
            $$.n = node( lnode_save_to_var, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = n2;
	}
    | LVAR stat_math_op expr
	{
	    DPRINT( "LVAR(%d) stat_math_op(%d) expr\n", (int)$1.i, (int)$2.i );
	    //Create first operand:
	    lnode* n1 = make_expr_node_from_local_var( $1.i );
	    //Create math operation:
	    lnode* n2 = node( (lnode_type)( lnode_sub + $2.i ), 2 ); 
	    n2->n[ 0 ] = n1; 
	    n2->n[ 1 ] = $3.n;
	    //Save result:
	    $$.n = node( lnode_save_to_stackframe, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = n2;
	}
    | FNNUM '(' exprlist ')' 
        {
	    DPRINT( "FNNUM(%d) ( exprlist )\n", (int)$1.i );
	    $$.n = node( lnode_call_builtin_fn_void, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = $3.n;
        }
    | fn_expr 
	{
	    DPRINT( "fn_expr (call void function. statement)\n" );
	    $$.n = $1.n;
	    $$.n->type = lnode_call_void;
	}
    | RET 
        {
	    DPRINT( "RET\n" );
	    $$.n = node( lnode_ret_int, 0 );
	    $$.n->val.i = 0;
        }
    | RET '(' expr ')' 
        {
	    DPRINT( "RET ( expr )\n" );
	    $$.n = node( lnode_ret, 1 );
	    $$.n->n[ 0 ] = $3.n;
        }
    | IF expr '{' statlist '}'
	{ 
	    DPRINT( "IF expr statlist\n" );
	    $$.n = node( lnode_if, 2 );
	    $$.n->n[ 0 ] = $2.n;
	    $$.n->n[ 1 ] = $4.n;
	    $4.n->flags |= LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER;
	}
    | IF expr '{' statlist '}' ELSE '{' statlist '}'
	{ 
	    DPRINT( "IF expr statlist ELSE statlist\n" );
	    $$.n = node( lnode_if_else, 3 );
	    $$.n->n[ 0 ] = $2.n;
	    $$.n->n[ 1 ] = $4.n;
	    $$.n->n[ 2 ] = $8.n;
	    $4.n->flags |= LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER | LNODE_FLAG_STATLIST_SKIP_NEXT_HEADER;
	    $8.n->flags |= LNODE_FLAG_STATLIST_WITH_JMP_HEADER;
	}
    | WHILE expr '{' 
	{
	    DPRINT( "WHILE expr {\n" );
	    $$.n = node( lnode_while, 3 );
	    $$.n->n[ 0 ] = $2.n;
	    $$.n->n[ 2 ] = node( lnode_jmp_to_node, 0 );
	    $$.n->n[ 2 ]->val.p = $2.n;
	    //Push it to stack:
	    if( g_comp->while_stack == 0 )
		g_comp->while_stack = (lnode**)bmem_new( sizeof( lnode* ) );
	    g_comp->while_stack[ g_comp->while_stack_ptr ] = $$.n;
	    g_comp->while_stack_ptr++;
	    if( g_comp->while_stack_ptr >= bmem_get_size( g_comp->while_stack ) / sizeof( lnode* ) )
		g_comp->while_stack = (lnode**)bmem_resize( g_comp->while_stack, bmem_get_size( g_comp->while_stack ) + sizeof( lnode* ) );
	}
      statlist '}'
	{ 
	    DPRINT( "statlist } (while)\n" );
	    $$.n = $4.n;
	    $$.n->n[ 1 ] = $5.n;
	    $5.n->flags |= LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER | LNODE_FLAG_STATLIST_SKIP_NEXT_HEADER;
	    //Pop from stack:
	    g_comp->while_stack_ptr--;
	}
    | BREAK 
        { 
	    DPRINT( "BREAK (level %d)\n", (int)$1.i );
	    if( g_comp->while_stack_ptr > 0 )
	    {
        	$$.n = node( lnode_jmp_to_end_of_node, 0 );
		if( $1.i == -1 )
        	    $$.n->val.p = g_comp->while_stack[ 0 ]->n[ 2 ];
        	else
        	{
        	    if( g_comp->while_stack_ptr - $1.i >= 0 )
        		$$.n->val.p = g_comp->while_stack[ g_comp->while_stack_ptr - $1.i ]->n[ 2 ];
        	    else
        	    {
            		SHOW_ERROR( "wrong level number %d for 'break' operator", (int)$1.i );
            		YYERROR;
        	    }
        	}
    	    }
    	    else
    	    {
                SHOW_ERROR( "operator 'break' can not be outside of while loop" );
                YYERROR;
            }
        }
    | CONTINUE 
        { 
	    DPRINT( "CONTINUE\n" );
	    if( g_comp->while_stack_ptr > 0 )
	    {
        	$$.n = node( lnode_jmp_to_node, 0 );
        	$$.n->val.p = g_comp->while_stack[ g_comp->while_stack_ptr - 1 ]->n[ 2 ];
    	    }
    	    else
    	    {
                SHOW_ERROR( "operator 'continue' can not be outside of while loop" );
                YYERROR;
            }
        }
    | FNDEF
	{
	    DPRINT( "function begin\n" );
	    //Create new empty local symbol table:
	    create_empty_lsym_table();
	    g_comp->fn_pars_mode = 1;
	}
      GVAR '(' lvarslist ')' 
	{
	    g_comp->fn_pars_mode = 0;
	    if( g_comp->var_flags[ $3.i ] & VAR_FLAG_FUNCTION )
	    {
		SHOW_ERROR( "function %s is already defined", pix_vm_get_variable_name( g_comp->vm, $3.i ) );
                YYERROR;
	    }
	    if( g_comp->var_flags[ $3.i ] & VAR_FLAG_LABEL )
	    {
		SHOW_ERROR( "function %s is already defined as label", pix_vm_get_variable_name( g_comp->vm, $3.i ) );
                YYERROR;
	    }
	    g_comp->var_flags[ $3.i ] |= VAR_FLAG_FUNCTION | VAR_FLAG_INITIALIZED;
	}
      '{' statlist '}'
	{
	    DPRINT( "function\n" );
	    //Remove local symbol table:
	    $$.n = remove_lsym_table( $9.n );
	    if( $$.n == 0 ) YYERROR;
	    //Add the header:
	    $$.n->flags |= LNODE_FLAG_STATLIST_WITH_JMP_HEADER;
            //Add ret instruction to this statlist, because it is the function now:
            resize_node( $$.n, $$.n->nn + 2 );
	    $$.n->n[ $$.n->nn - 2 ] = node( lnode_ret_int, 0 );
	    $$.n->n[ $$.n->nn - 2 ]->val.i = 0;
	    //Save address of this function to global variable:
	    $$.n->n[ $$.n->nn - 1 ] = node( lnode_function_label_from_node, 1 );
	    $$.n->n[ $$.n->nn - 1 ]->val.i = $3.i;
	    $$.n->n[ $$.n->nn - 1 ]->n[ 0 ] = node( lnode_empty, 0 );
	    $$.n->n[ $$.n->nn - 1 ]->n[ 0 ]->val.p = $$.n;
	}
    | INCLUDE NUM_I
	{
	    if( (unsigned)$2.i < (unsigned)g_comp->vm->c_num )
	    {
		int name_size = g_comp->vm->c[ $2.i ]->size;
		utf8_char* name = (utf8_char*)bmem_new( name_size + 1 );
		bmem_copy( name, g_comp->vm->c[ $2.i ]->data, name_size );
		name[ name_size ] = 0;
		utf8_char* new_name = pix_compose_full_path( g_comp->base_path, name, 0 );
		bmem_free( name );
		pix_vm_remove_container( $2.i, g_comp->vm );
		DPRINT( "include \"%s\"\n", new_name );
		
		size_t fsize = bfs_get_file_size( new_name );
		if( fsize == 0 )
		{
		    SHOW_ERROR( "%s not found", new_name );
		    bmem_free( new_name );
		    YYERROR;
		}
		
		//Save previous compiler state:
		if( g_comp->inc == 0 )
		{
		    g_comp->inc = (pix_include*)bmem_new( 2 * sizeof( pix_include ) );
		}
		if( g_comp->inc_num >= bmem_get_size( g_comp->inc ) / sizeof( pix_include ) )
		{
		    g_comp->inc = (pix_include*)bmem_resize( g_comp->inc, ( g_comp->inc_num + 2 ) * sizeof( pix_include ) );
		}
		g_comp->inc[ g_comp->inc_num ].src = g_comp->src;
		g_comp->inc[ g_comp->inc_num ].src_ptr = g_comp->src_ptr;
		g_comp->inc[ g_comp->inc_num ].src_line = g_comp->src_line;
		g_comp->inc[ g_comp->inc_num ].src_size = g_comp->src_size;
		g_comp->inc[ g_comp->inc_num ].src_name = g_comp->src_name;
		g_comp->inc[ g_comp->inc_num ].base_path = g_comp->base_path;
		g_comp->inc_num++;
		
		//Set new compiler state:
		g_comp->src = (utf8_char*)bmem_new( fsize );
		bfs_file f = bfs_open( new_name, "rb" );
		if( fsize >= 3 )
    		{
        	    bfs_read( g_comp->src, 1, 3, f );
        	    if( (uchar)g_comp->src[ 0 ] == 0xEF && (uchar)g_comp->src[ 1 ] == 0xBB && (uchar)g_comp->src[ 2 ] == 0xBF )
        	    {
            		//Byte order mark found. Just ignore it:
            		fsize -= 3;
        	    }
	            else
    		    {
            		bfs_rewind( f );
	            }
    		}
		bfs_read( g_comp->src, 1, fsize, f );	
		bfs_close( f );
		g_comp->src_ptr = 0;
		g_comp->src_line = 0;
		g_comp->src_size = fsize;
		g_comp->src_name = new_name;
		g_comp->base_path = pix_get_base_path( new_name );
		DPRINT( "New base path: %s\n", g_comp->base_path );
	    }
	    else 
	    {
		YYERROR;
	    }
	    $$.n = node( lnode_empty, 0 );
	}
    ;
exprlist
    : /*empty*/ { $$.n = node( lnode_exprlist, 0 ); }
    | expr { $$.n = node( lnode_exprlist, 1 ); $$.n->n[ 0 ] = $1.n; }
    | exprlist ',' expr
	{
	    //Add new node (expr) to list of parameters (exprlist):
	    resize_node( $1.n, $1.n->nn + 1 );
            $1.n->n[ $1.n->nn - 1 ] = $3.n;
	    $$.n = $1.n;
        }
    ;
basic_expr
    : NUM_I { DPRINT( "NUM_I(%d)\n", (int)$1.i ); $$.n = node( lnode_int, 0 ); $$.n->val.i = $1.i; }
    | NUM_F { DPRINT( "NUM_F(%d)\n", (int)$1.f ); $$.n = node( lnode_float, 0 ); $$.n->val.f = $1.f; }
    | GVAR { DPRINT( "GVAR(%d)\n", (int)$1.i ); $$.n = make_expr_node_from_var( $1.i ); }
    | LVAR { DPRINT( "LVAR(%d)\n", (int)$1.i ); $$.n = make_expr_node_from_local_var( $1.i ); }
    | FNNUM '(' exprlist ')'
        {
	    DPRINT( "FNNUM(%d) ( exprlist )\n", (int)$1.i );
	    $$.n = node( lnode_call_builtin_fn, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = $3.n;
        }
    ;
fn_expr
    : basic_expr '(' exprlist ')' { DPRINT( "basic_expr ( exprlist )\n" ); $$.n = node( lnode_call, 2 ); $$.n->n[ 0 ] = $3.n; $$.n->n[ 1 ] = $1.n; }
    | fn_expr '(' exprlist ')' { DPRINT( "fn_expr ( exprlist )\n" ); $$.n = node( lnode_call, 2 ); $$.n->n[ 0 ] = $3.n; $$.n->n[ 1 ] = $1.n; }
    | mem_expr '(' exprlist ')' { DPRINT( "mem_expr ( exprlist )\n" ); $$.n = node( lnode_call, 2 ); $$.n->n[ 0 ] = $3.n; $$.n->n[ 1 ] = $1.n; }
    | prop_expr '(' exprlist ')' { DPRINT( "prop_expr ( exprlist )\n" ); $$.n = node( lnode_call, 2 ); $$.n->n[ 0 ] = $3.n; $$.n->n[ 1 ] = $1.n; }
    ;
mem_expr
    : basic_expr '[' mem_offset ']' { DPRINT( "basic_expr [ mem_offset ]\n" ); $$.n = node( lnode_load_from_mem, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | fn_expr '[' mem_offset ']' { DPRINT( "fn_expr [ mem_offset ]\n" ); $$.n = node( lnode_load_from_mem, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | mem_expr '[' mem_offset ']' { DPRINT( "mem_expr [ mem_offset ]\n" ); $$.n = node( lnode_load_from_mem, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | prop_expr '[' mem_offset ']' { DPRINT( "prop_expr [ mem_offset ]\n" ); $$.n = node( lnode_load_from_mem, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    ;
prop_expr
    : basic_expr HASH { DPRINT( "basic_expr.%d\n", (int)$2.i ); $$.n = node( lnode_load_from_prop, 1 ); $$.n->n[ 0 ] = $1.n; $$.n->val.i = $2.i; }
    | fn_expr HASH { DPRINT( "fn_expr.%d\n", (int)$2.i ); $$.n = node( lnode_load_from_prop, 1 ); $$.n->n[ 0 ] = $1.n; $$.n->val.i = $2.i; }
    | mem_expr HASH { DPRINT( "mem_expr.%d\n", (int)$2.i ); $$.n = node( lnode_load_from_prop, 1 ); $$.n->n[ 0 ] = $1.n; $$.n->val.i = $2.i; }
    | prop_expr HASH { DPRINT( "prop_expr.%d\n", (int)$2.i ); $$.n = node( lnode_load_from_prop, 1 ); $$.n->n[ 0 ] = $1.n; $$.n->val.i = $2.i; }
    ;
expr
    : basic_expr { DPRINT( "basic expression\n" ); $$.n = $1.n; }
    | fn_expr { DPRINT( "fn_expr\n" ); $$.n = $1.n; }
    | mem_expr { DPRINT( "mem_expr\n" ); $$.n = $1.n; }
    | prop_expr { DPRINT( "prop_expr\n" ); $$.n = $1.n; }
    | '{' 
	{ 
	    DPRINT( "statlist begin (expr)\n" );
	    //Create new empty local symbol table:
	    create_empty_lsym_table();
	}
      statlist '}'
        { 
	    DPRINT( "statlist (expr)\n" );
	    //Remove local symbol table:
	    $$.n = remove_lsym_table( $3.n );
	    if( $$.n == 0 ) YYERROR;
	    //Add the header:
	    $$.n->flags |= LNODE_FLAG_STATLIST_WITH_JMP_HEADER | LNODE_FLAG_STATLIST_AS_EXPRESSION;
            //Add ret instruction to this statlist, because it is the function now:
            resize_node( $$.n, $$.n->nn + 1 );
	    $$.n->n[ $$.n->nn - 1 ] = node( lnode_ret_int, 0 );
	    $$.n->n[ $$.n->nn - 1 ]->val.i = 0;
        }
    | '(' expr ')' { $$.n = $2.n; }
    | expr '-' expr { DPRINT( "SUB\n" ); $$.n = node( lnode_sub, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '+' expr { DPRINT( "ADD\n" ); $$.n = node( lnode_add, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '*' expr { DPRINT( "MUL\n" ); $$.n = node( lnode_mul, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr IDIV expr { DPRINT( "IDIV\n" ); $$.n = node( lnode_idiv, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '/' expr { DPRINT( "DIV\n" ); $$.n = node( lnode_div, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '%' expr { DPRINT( "MOD\n" ); $$.n = node( lnode_mod, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr AND expr { DPRINT( "AND\n" ); $$.n = node( lnode_and, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr OR expr { DPRINT( "OR\n" ); $$.n = node( lnode_or, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr XOR expr { DPRINT( "XOR\n" ); $$.n = node( lnode_xor, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr ANDAND expr { DPRINT( "ANDAND\n" ); $$.n = node( lnode_andand, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr OROR expr { DPRINT( "OROR\n" ); $$.n = node( lnode_oror, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr EQ expr { DPRINT( "EQ\n" ); $$.n = node( lnode_eq, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr NEQ expr { DPRINT( "NEQ\n" ); $$.n = node( lnode_neq, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '<' expr { DPRINT( "LESS\n" ); $$.n = node( lnode_less, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr LEQ expr { DPRINT( "LEQ\n" ); $$.n = node( lnode_leq, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '>' expr { DPRINT( "GREATER\n" ); $$.n = node( lnode_greater, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr GEQ expr { DPRINT( "GEQ\n" ); $$.n = node( lnode_geq, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr LSHIFT expr { DPRINT( "LSHIFT\n" ); $$.n = node( lnode_lshift, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr RSHIFT expr { DPRINT( "RSHIFT\n" ); $$.n = node( lnode_rshift, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | '-' expr %prec NEG { DPRINT( "NEG\n" ); $$.n = node( lnode_neg, 1 ); $$.n->n[ 0 ] = $2.n; }
    ;
%%

//Compilation error message:
void yyerror( char const* str )
{
    SHOW_ERROR( "%s", str );
}

size_t handle_control_characters( utf8_char* str, size_t size )
{
    size_t r = 0;
    size_t w = 0;
    for( ; r < size; r++, w++ )
    {
	int c = str[ r ];
	if( c == '\\' && r < size - 1 )
	{
	    r++;
	    int next_c = str[ r ];
	    switch( next_c )
	    {
		case '0': str[ w ] = 0x00; break;
		case 'a': str[ w ] = 0x07; break; //alert
		case 'b': str[ w ] = 0x08; break; //backspace
		case 't': str[ w ] = 0x09; break; //tab
		case 'r': str[ w ] = 0x0D; break; //carriage-return
		case 'n': str[ w ] = 0x0A; break; //newline
		case 'v': str[ w ] = 0x0B; break; //vertical-tab
		case 'f': str[ w ] = 0x0C; break; //form-feed
		default: str[ w ] = (utf8_char)next_c; break;
	    }
	}
	else
	{
	    str[ w ] = str[ r ];
	}
    }
    return w;
}

void resize_var_flags( void )
{
    if( g_comp->vm->vars_num > g_comp->var_flags_size )
    {
        size_t new_size = g_comp->vm->vars_num + 64;
	g_comp->var_flags = (char*)bmem_resize( g_comp->var_flags, new_size );
	bmem_set( g_comp->var_flags + g_comp->var_flags_size, new_size - g_comp->var_flags_size, 0 );
        g_comp->var_flags_size = new_size;
    }
}

void resize_local_variables( void )
{
    pix_lsymtab* l = &g_comp->lsym[ g_comp->lsym_num ];
    if( l->lvars_num > l->lvar_flags_size )
    {
        size_t new_size = l->lvars_num + 64;
	l->lvar_flags = (char*)bmem_resize( l->lvar_flags, new_size );
	l->lvar_names = (utf8_char**)bmem_resize( l->lvar_names, new_size * sizeof( utf8_char* ) );
	bmem_set( l->lvar_flags + l->lvar_flags_size, new_size - l->lvar_flags_size, 0 );
	bmem_set( l->lvar_names + l->lvar_flags_size, ( new_size - l->lvar_flags_size ) * sizeof( utf8_char* ), 0 );
	l->lvar_flags_size = new_size;
    }
}

//Parser. yylex() get next token from the source:
int yylex( void )
{
    utf8_char* src = g_comp->src;

    int c = -1;
    
    int str_symbol = 0;
    int str_start = -1;
    bool str_first_char_is_digit = 0;
    
    while( g_comp->src_ptr < g_comp->src_size + 2 )
    {
	if( g_comp->src_ptr >= g_comp->src_size )
	{
	    if( g_comp->src_ptr == g_comp->src_size )
	    {
		c = ' '; //Last empty character of the file (we need this to finish some strings)
	    }
	    else
	    {
		c = -1; //EOF
		if( g_comp->inc_num > 0 )
		{
		    DPRINT( "Return from include\n" );
		    //Remove current state:
		    bmem_free( g_comp->src );
		    bmem_free( g_comp->src_name );
		    bmem_free( g_comp->base_path );
		    //Restore previous compiler state:
		    g_comp->inc_num--;
		    g_comp->src = g_comp->inc[ g_comp->inc_num ].src;
		    src = g_comp->src;
		    g_comp->src_ptr = g_comp->inc[ g_comp->inc_num ].src_ptr;
		    g_comp->src_line = g_comp->inc[ g_comp->inc_num ].src_line;
		    g_comp->src_size = g_comp->inc[ g_comp->inc_num ].src_size;
		    g_comp->src_name = g_comp->inc[ g_comp->inc_num ].src_name;
		    g_comp->base_path = g_comp->inc[ g_comp->inc_num ].base_path;
		    //Reset parser state:
		    str_symbol = 0;
		    str_start = -1;
		    str_first_char_is_digit = 0;
		    continue;
		}
	    }
	}
	else
	{
	    c = src[ g_comp->src_ptr ];
	}
	g_comp->src_ptr++;
	
	//Ignore text strings (quoted):
        if( c == '"' || c == '\'' )
	{
	    if( !str_symbol )
            {
                //String start:
                str_start = g_comp->src_ptr;
                str_symbol = c;
                continue;
            }
            else
            {
                if( str_symbol == c )
                {
                    //End of string:
                    int str_size = g_comp->src_ptr - str_start - 1;
		    if( c == '"' )
                    {
                        //Return number of text container:
			utf8_char* data;
			if( str_size > 0 )
			{
			    data = (utf8_char*)bmem_new( str_size );
			    bmem_copy( data, src + str_start, str_size );
			    str_size = handle_control_characters( data, str_size );
			}
			else 
			{
			    data = (utf8_char*)bmem_new( 1 );
			    data[ 0 ] = 0;
			    str_size = 1;
			}
			yylval.i = pix_vm_new_container( -1, str_size, 1, PIX_CONTAINER_TYPE_INT8, data, g_comp->vm );
			pix_vm_set_container_flags( yylval.i, pix_vm_get_container_flags( yylval.i, g_comp->vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, g_comp->vm );
			DPRINT( "NEW TEXT CONTAINER %d (%d bytes)\n", (int)yylval.i, str_size );
		    }
		    else
		    {
			//Return string converted to integer:
			utf8_char ts[ 4 ];
			ts[ 0 ] = src[ str_start ];
			ts[ 1 ] = src[ str_start + 1 ];
			ts[ 2 ] = src[ str_start + 2 ];
			ts[ 3 ] = src[ str_start + 3 ];
			str_size = handle_control_characters( ts, str_size );
			if( str_size == 1 ) 
			    yylval.i = (uchar)ts[ 0 ];
                        if( str_size == 2 ) 
			    yylval.i = (uchar)ts[ 0 ] 
				+ ( (uchar)ts[ 1 ] << 8 );
                        if( str_size == 3 ) 
			    yylval.i = (uchar)ts[ 0 ]
				+ ( (uchar)ts[ 1 ] << 8 )
				+ ( (uchar)ts[ 2 ] << 16 );
                        if( str_size >= 4 ) 
			    yylval.i = (uchar)ts[ 0 ] 
				+ ( (uchar)ts[ 1 ] << 8 )
				+ ( (uchar)ts[ 2 ] << 16 )
				+ ( (uchar)ts[ 3 ] << 24 );
		    }
		    c = NUM_I;
		    break;
		}
		else
		{
		    continue;
		}
	    }
	}
	else 
	{
	    if( str_symbol )
	    {
		//String reading.
		if( c == '\\' )
		{
		    //Skip control character:
		    g_comp->src_ptr++;
		}
		continue;
	    }
	}
	
	//No. It's not a quoted string:
	bool numeric = NUMERIC( c );
	if( numeric || ABC( c ) || c == '_' || c == '#' || c == '.' || c == '$' )
        {
	    if( str_start == -1 )
	    {
		//String begin:
		if( numeric ) str_first_char_is_digit = 1;
		str_start = g_comp->src_ptr - 1;
	    }
	    else
	    {
		if( c == '.' )
		{
		    if( str_first_char_is_digit == 0 )
		    {
			goto string_end;
		    }
		}
	    }
	    continue;
	}
	else
	{
string_end:
	    if( str_start >= 0 )
	    {
		int str_size = g_comp->src_ptr - str_start - 1;
                if( src[ str_start ] == '#' || NUMERIC( src[ str_start ] ) || ( str_size > 1 && src[ str_start ] == '.' && NUMERIC( src[ str_start + 1 ] ) ) )
		{
		    //Number:
		    c = NUM_I;
                    int ni;
                    if( src[ str_start ] == '#' )
                    {
                        //HEX COLOR:
                        yylval.i = 0;
                        for( ni = str_start + 1; ni < str_start + str_size; ni++ )
                        {
                            yylval.i <<= 4;
                            if( src[ ni ] < 58 ) yylval.i += src[ ni ] - '0';
                            else if( src[ ni ] > 64 && src[ ni ] < 91 ) yylval.i += src[ ni ] - 'A' + 10;
                            else yylval.i += src[ ni ] - 'a' + 10;
                        }
                        yylval.i = get_color( 
			    ( yylval.i >> 16 ) & 255,
                            ( yylval.i >> 8  ) & 255,
                            ( yylval.i       ) & 255 );
                    }
                    else
                    {
			if( str_size > 2 && src[ str_start + 1 ] == 'x' )
			{
			    //HEX:
			    yylval.i = 0;
			    for( ni = str_start + 2; ni < str_start + str_size; ni++ )
			    {
				yylval.i <<= 4;
				if( src[ ni ] < 58 ) yylval.i += src[ ni ] - '0';
				else if( src[ ni ] > 64 && src[ ni ] < 91 ) yylval.i += src[ ni ] - 'A' + 10;
				else yylval.i += src[ ni ] - 'a' + 10;
			    }
			}
			else if( str_size > 2 && src[ str_start + 1 ] == 'b' )
			{
			    //BIN:
		            yylval.i = 0;
			    for( ni = str_start + 2; ni < str_start + str_size; ni++ )
			    {
				yylval.i <<= 1;
				yylval.i += src[ ni ] - '0';
			    }
			}
			else
			{
			    bool float_num = 0;
			    for( ni = str_start; ni < str_start + str_size; ni++ )
				if( src[ ni ] == '.' ) { float_num = 1; break; }
			    if( float_num )
			    {
				//FLOATING POINT:
				if( str_size > 256 )
				    str_size = 256;
				bmem_copy( g_comp->temp_sym_name, &src[ str_start ], str_size );
				g_comp->temp_sym_name[ str_size ] = 0;
				c = NUM_F;
				yylval.f = atof( g_comp->temp_sym_name );
			    }
			    else 
			    {
				//DEC:
				yylval.i = 0;
				for( ni = str_start; ni < str_start + str_size; ni++ )
				{
				    yylval.i *= 10;
				    yylval.i += src[ ni ] - '0';
				}
			    }
			}
                    }
		}
		else
		{
		    //Name of some symbol:
		    if( str_size == 1 && src[ str_start ] > 0 && src[ str_start ] < 127 )
		    {
			//Standard variable with one char ASCII name:
			c = GVAR;
			yylval.i = src[ str_start ];
			g_comp->var_flags[ yylval.i ] |= VAR_FLAG_USED;
		    }
		    else 
		    {
			//Variable with long name:
			if( str_size > 256 )
			    str_size = 256;
			bmem_copy( g_comp->temp_sym_name, &src[ str_start ], str_size );
			g_comp->temp_sym_name[ str_size ] = 0;
			if( src[ str_start ] == '.' )
			{
			    //Container property:
			    c = HASH;
			    //Create global variable with the name and hash of this property:
			    bool created;
                            pix_sym* sym = pix_symtab_lookup( g_comp->temp_sym_name, -1, 1, SYMTYPE_GVAR, 0, 0, &created, &g_comp->sym );
                            if( sym )
                            {
                        	if( created )
                        	{
                            	    //Create new variable:
                            	    sym->val.i = g_comp->vm->vars_num;
                            	    DPRINT( "New global variable: %s (%d)\n", g_comp->temp_sym_name, (int)sym->val.i );
                            	    g_comp->vm->vars_num++;
                            	    pix_vm_resize_variables( g_comp->vm );
                            	    resize_var_flags();
                            	    g_comp->var_flags[ sym->val.i ] |= VAR_FLAG_USED | VAR_FLAG_INITIALIZED;
                            	    //Save variable name:
                            	    utf8_char* var_name = (utf8_char*)bmem_new( str_size + 1 );
                            	    if( var_name )
                            	    {
                                	g_comp->vm->var_names[ sym->val.i ] = var_name;
                                	bmem_copy( var_name, g_comp->temp_sym_name, str_size + 1 );
                            	    }
                        	}
                        	yylval.i = sym->val.i;
                        	g_comp->vm->vars[ yylval.i ].i = pix_symtab_hash( (const utf8_char*)( g_comp->temp_sym_name + 1 ), PIX_CONTAINER_SYMTAB_SIZE );
			    }
			    else
			    {
				SHOW_ERROR( "can't create a new symbol for property" );
            			return -1;
			    }
			}
			else
			if( src[ str_start ] == '$' )
			{
			    //Local variable:
			    c = LVAR;
			    if( str_size == 1 )
			    {
				yylval.i = 0;
			    }
			    else 
			    {
				if( NUMERIC( src[ str_start + 1 ] ) )
				{
				    yylval.i = string_to_int( g_comp->temp_sym_name + 1 );
				}
				else 
				{
				    //Named local variable:
				    pix_symtab* lsym = g_comp->lsym[ g_comp->lsym_num ].lsym;
				    if( lsym == 0 )
				    {
					lsym = (pix_symtab*)bmem_new( sizeof( pix_symtab ) );
					g_comp->lsym[ g_comp->lsym_num ].lsym = lsym;
					pix_symtab_init( PIX_COMPILER_SYMTAB_SIZE, lsym );
				    }
				    bool created;
				    pix_sym* sym = pix_symtab_lookup( g_comp->temp_sym_name + 1, -1, 1, SYMTYPE_LVAR, 0, 0, &created, lsym );
				    if( sym )
				    {
					if( created )
					{
					    if( g_comp->fn_pars_mode )
					    {
						//Fn parameters:
						sym->val.i = 1 + g_comp->lsym[ g_comp->lsym_num ].pars_num;
						g_comp->lsym[ g_comp->lsym_num ].pars_num++;
						DPRINT( "New local variable (fn parameter): %s (%d)\n", g_comp->temp_sym_name, (int)sym->val.i );
					    }
					    else
					    {
						//Variables:
						pix_lsymtab* l = &g_comp->lsym[ g_comp->lsym_num ];
						sym->val.i = -LVAR_OFFSET - l->lvars_num;
						l->lvars_num++;
						resize_local_variables();
						int lvar_num = -sym->val.i - LVAR_OFFSET;
						l->lvar_flags[ lvar_num ] |= VAR_FLAG_USED;
						l->lvar_names[ lvar_num ] = (utf8_char*)bmem_new( str_size + 1 );
						bmem_copy( l->lvar_names[ lvar_num ], g_comp->temp_sym_name, str_size + 1 );
						DPRINT( "New local variable: %s (%d)\n", g_comp->temp_sym_name, (int)sym->val.i );
					    }
				        }
				        else
				        {
				    	    //Already created:
				    	    if( g_comp->fn_pars_mode )
				    	    {
				    		SHOW_ERROR( "parameter %s is already defined", g_comp->temp_sym_name );
				    		return -1;
				    	    }
				        }
					yylval.i = sym->val.i;
				    }
				    else
                        	    {
                            		SHOW_ERROR( "can't create a new symbol for local variable" );
                            		return -1;
                        	    }
				}
			    }
			}
			else
			{
			    //Global variable
			    bool created;
			    pix_sym* sym = pix_symtab_lookup( g_comp->temp_sym_name, -1, 1, SYMTYPE_GVAR, 0, 0, &created, &g_comp->sym );
			    if( sym == 0 )
                            {
                                SHOW_ERROR( "can't create a new symbol for global variable" );
                                return -1;
                            }
			    if( created )
			    {
				//Create new variable:
				sym->val.i = g_comp->vm->vars_num;
				yylval.i = sym->val.i;
				c = GVAR;
				DPRINT( "New global variable: %s (%d)\n", g_comp->temp_sym_name, (int)sym->val.i );
				g_comp->vm->vars_num++;
				pix_vm_resize_variables( g_comp->vm );
				resize_var_flags();
				g_comp->var_flags[ sym->val.i ] |= VAR_FLAG_USED;
				//Save variable name:
				utf8_char* var_name = (utf8_char*)bmem_new( str_size + 1 );
				if( var_name )
				{
				    g_comp->vm->var_names[ sym->val.i ] = var_name;
                            	    bmem_copy( var_name, g_comp->temp_sym_name, str_size + 1 );
                            	}
			    }
			    else
			    {
				switch( sym->type )
				{
				    case SYMTYPE_GVAR:
					yylval.i = sym->val.i;
					c = GVAR;
					break;
				    case SYMTYPE_LVAR:
					yylval.i = sym->val.i;
					c = LVAR;
					break;
				    case SYMTYPE_NUM_I:
					yylval.i = sym->val.i;
					c = NUM_I;
					break;
				    case SYMTYPE_NUM_F:
					yylval.f = sym->val.f;
					c = NUM_F;
					break;
				    case SYMTYPE_WHILE:
					c = WHILE;
					break;
				    case SYMTYPE_BREAK:
					yylval.i = 1;
					c = BREAK;
					break;
				    case SYMTYPE_BREAK2:
					yylval.i = 2;
					c = BREAK;
					break;
				    case SYMTYPE_BREAK3:
					yylval.i = 3;
					c = BREAK;
					break;
				    case SYMTYPE_BREAK4:
					yylval.i = 4;
					c = BREAK;
					break;
				    case SYMTYPE_BREAKALL:
					yylval.i = -1;
					c = BREAK;
					break;
				    case SYMTYPE_CONTINUE:
					c = CONTINUE;
					break;
				    case SYMTYPE_IF:
					c = IF;
					break;
				    case SYMTYPE_ELSE:
					c = ELSE;
					break;
				    case SYMTYPE_GO:
					c = GO;
					break;
				    case SYMTYPE_RET:
					c = RET;
					break;
				    case SYMTYPE_IDIV:
					c = IDIV;
					break;
				    case SYMTYPE_FNNUM:
					c = FNNUM;
					yylval.i = sym->val.i;
					break;
				    case SYMTYPE_FNDEF:
					c = FNDEF;
					break;
				    case SYMTYPE_INCLUDE:
					c = INCLUDE;
					break;
				    case SYMTYPE_HALT:
					c = HALT;
					break;
				}
			    }
			}
		    }
		}
		g_comp->src_ptr--;
		break;
	    }
        }
	
	//Parse other symbols:
        if( c == '/' )
        {
            if( g_comp->src_ptr < g_comp->src_size && src[ g_comp->src_ptr ] == '/' )
            { 
		//COMMENTS:
                for(;;)
                {
                    g_comp->src_ptr++;
                    if( g_comp->src_ptr >= g_comp->src_size ) break;
                    if( src[ g_comp->src_ptr ] == 0xD || src[ g_comp->src_ptr ] == 0xA ) break;
                }
                continue;
            }
            else
            if( g_comp->src_ptr < g_comp->src_size && src[ g_comp->src_ptr ] == '*' )
            {
                //COMMENTS 2:
                for(;;)
                {
                    g_comp->src_ptr++;
                    if( g_comp->src_ptr >= g_comp->src_size ) break;
                    if( src[ g_comp->src_ptr ] == 0xA ) g_comp->src_line++;
                    if( g_comp->src_ptr + 1 < g_comp->src_size && 
		        src[ g_comp->src_ptr ] == '*' && 
			src[ g_comp->src_ptr + 1 ] == '/' ) 
		    { 
			g_comp->src_ptr += 2; break; 
		    }
                }
                continue;
            }
        }
	
	bool need_to_break = 0;
        switch( c )
        {
	    case 0xA:
		//New line:
		g_comp->src_line++;
		break;
        
            case '-':
            case '+':
            case '*':
            case '/':
            case '%':
            case ':':
            case '(':
            case ')':
            case ',':
            case '[':
            case ']':
            case '{':
            case '}':
	    case '~':
                need_to_break = 1;
                break;

	    case '^':
		c = XOR;
		need_to_break = 1;
		break;
	    case '&':
		if( src[ g_comp->src_ptr ] == '&' )
		{
		    c = ANDAND;
		    g_comp->src_ptr++;
		}
		else
		{
		    c = AND;
		}
		need_to_break = 1;
		break;
	    case '|':
		if( src[ g_comp->src_ptr ] == '|' )
		{
		    c = OROR;
		    g_comp->src_ptr++;
		}
		else
		{
		    c = OR;
		}
		need_to_break = 1;
		break;
	    
	    case '<':
                if( src[ g_comp->src_ptr ] == '=' )
                {
                    c = LEQ;
                    g_comp->src_ptr++;
                }
		else
		if( src[ g_comp->src_ptr ] == '<' )
		{
		    c = LSHIFT;
		    g_comp->src_ptr++;
		}
                need_to_break = 1;
                break;
            case '>':
                if( src[ g_comp->src_ptr ] == '=' )
                {
                    c = GEQ;
		    g_comp->src_ptr++;
                }
		else
		if( src[ g_comp->src_ptr ] == '>' )
		{
		    c = RSHIFT;
		    g_comp->src_ptr++;
		}
                need_to_break = 1;
                break;
            case '!':
                if( src[ g_comp->src_ptr ] == '=' )
                {
                    c = NEQ;
                    g_comp->src_ptr++;
                    need_to_break = 1;
                }
                break;
	    case '=':
		if( src[ g_comp->src_ptr ] == '=' )
		{
		    c = EQ;
		    g_comp->src_ptr++;
		}
		need_to_break = 1;
		break;
	}
	if( need_to_break ) break;
    }
    
    return c;
}

void push_int( PIX_INT v )
{
    if( v < ( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) &&
	v > -( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) )
    {
	DPRINT( "%d: PUSH_i ( %d << OB )\n", (int)g_comp->vm->code_ptr, (int)v );
	pix_vm_put_opcode( OPCODE_PUSH_i | ( (PIX_OPCODE)v << PIX_OPCODE_BITS ), g_comp->vm );
    }
    else
    {
	DPRINT( "%d: PUSH_I %d\n", (int)g_comp->vm->code_ptr, (int)v );
	pix_vm_put_opcode( OPCODE_PUSH_I, g_comp->vm );
	pix_vm_put_int( v, g_comp->vm );
    }
}

void push_jmp( size_t dest_ptr )
{
    size_t code_ptr = g_comp->vm->code_ptr;
    PIX_INT offset = (PIX_INT)dest_ptr - (PIX_INT)code_ptr;
    if( offset < ( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) &&
        offset > -( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) )
    {
        DPRINT( "%d: JMP_i ( %d << OB )\n", (int)code_ptr, (int)offset );
        pix_vm_put_opcode( OPCODE_JMP_i | ( (PIX_OPCODE)offset << PIX_OPCODE_BITS ), g_comp->vm );
    }
    else 
    {
        if( 0 )
        {
    	    //DPRINT( "%d: JMP_I %d\n", (int)code_ptr, (int)offset );	
    	    //pix_vm_put_opcode( OPCODE_JMP_I, g_comp->vm );
    	    //pix_vm_put_int( offset, g_comp->vm );
    	}
    	else
    	{
    	    blog( "JMP is too far! %d", (int)offset );
    	}
    }
    for( int i = 0; i < g_comp->statlist_header_size - ( g_comp->vm->code_ptr - code_ptr ); i++ )
    {
        DPRINT( "%d: NOP\n", (int)g_comp->vm->code_ptr, (int)offset );
        pix_vm_put_opcode( OPCODE_NOP, g_comp->vm );
    }
}

//Add lexical node:
lnode* node( lnode_type type, int nn )
{
    lnode* n = (lnode*)bmem_new( sizeof( lnode ) );
    if( nn > 0 )
    {
        n->n = (lnode**)bmem_new( sizeof( lnode* ) * nn );
    }
    else
    {
	n->n = 0;
    }
    n->type = type;
    n->nn = nn;
    n->flags = 0;
    n->code_ptr = 0;
    n->code_ptr2 = 0;
    return n;
}

void resize_node( lnode* n, int nn )
{
    if( n == 0 ) return;

    if( n->n == 0 )
    {
	n->n = (lnode**)bmem_new( sizeof( lnode* ) * nn );
    }
    else
    {
	if( nn > bmem_get_size( n->n ) / sizeof( lnode* ) )
	{
	    n->n = (lnode**)bmem_resize( n->n, sizeof( lnode* ) * ( nn + 8 ) );
	}
    }
    n->nn = nn;
}

lnode* clone_tree( lnode* n )
{
    if( n == 0 ) return 0;
    
    lnode* new_n = node( n->type, n->nn );
    
    new_n->flags = n->flags;
    new_n->val = n->val;
    
    for( int i = 0; i < n->nn; i++ )
    {
	new_n->n[ i ] = clone_tree( n->n[ i ] );
    }
    
    return new_n;
}

void remove_tree( lnode* n )
{
    if( n == 0 ) return;
    
    if( n->n )
    {
	for( int i = 0; i < n->nn; i++ )
	{
	    remove_tree( n->n[ i ] );
	}
	bmem_free( n->n );
    }
    bmem_free( n );
}

void optimize_tree( lnode* n )
{
    if( n == 0 ) return;
    
    if( n->n )
    {
	for( int i = 0; i < n->nn; i++ )
	{
	    optimize_tree( n->n[ i ] );
	}
    }
    
    switch( n->type )
    {
	case lnode_if:
	    {
		int v = 0;
		if( n->n[ 0 ]->type == lnode_int ) 
		{
		    if( n->n[ 0 ]->val.i == 0 )
			v = 1;
		    else
			v = 2;
		}
		else if( n->n[ 0 ]->type == lnode_float )
		{
		    if( n->n[ 0 ]->val.f == 0 )
			v = 1;
		    else
			v = 2;
		}
		if( v )
		{
		    if( v == 1 )
		    {
			//IF 0 { CODE } - IGNORE THIS CODE:
			remove_tree( n->n[ 0 ] );
			remove_tree( n->n[ 1 ] );
			n->nn = 0;
		    }
		    else
		    {
			//IF 1 { CODE } - ALWAYS TRUE:
			remove_tree( n->n[ 0 ] );
			n->n[ 0 ] = 0;
			n->n[ 1 ]->flags = 0; //Just a statlist without any headers
		    }
		}
	    }
	    break;
	case lnode_if_else:
	    {
		int v = 0;
		if( n->n[ 0 ]->type == lnode_int ) 
		{
		    if( n->n[ 0 ]->val.i == 0 )
			v = 1;
		    else
			v = 2;
		}
		else if( n->n[ 0 ]->type == lnode_float )
		{
		    if( n->n[ 0 ]->val.f == 0 )
			v = 1;
		    else
			v = 2;
		}
		if( v )
		{
		    if( v == 1 )
		    {
			//IF 0 { CODE1 } ELSE { CODE2 } - IGNORE CODE1; ALWAYS EXECUTE CODE2:
			remove_tree( n->n[ 0 ] );
			remove_tree( n->n[ 1 ] );
			n->n[ 0 ] = 0;
			n->n[ 1 ] = 0;
			n->n[ 2 ]->flags = 0; //Just a statlist without any headers
		    }
		    else
		    {
			//IF 1 { CODE1 } ELSE { CODE2 } - IGNORE CODE2; ALWAYS EXECUTE CODE1:
			remove_tree( n->n[ 0 ] );
			remove_tree( n->n[ 2 ] );
			n->n[ 0 ] = 0;
			n->n[ 1 ]->flags = 0; //Just a statlist without any headers
			n->n[ 2 ] = 0;
		    }
		}
	    }
	    break;
	case lnode_while:
	    {
		int v = 0;
		if( n->n[ 0 ]->type == lnode_int ) 
		{
		    if( n->n[ 0 ]->val.i == 0 )
			v = 1;
		    else
			v = 2;
		}
		else if( n->n[ 0 ]->type == lnode_float )
		{
		    if( n->n[ 0 ]->val.f == 0 )
			v = 1;
		    else
			v = 2;
		}
		if( v )
		{
		    if( v == 1 )
		    {
			//WHILE 0 { CODE1 } - IGNORE THIS CODE:
			remove_tree( n->n[ 0 ] );
			remove_tree( n->n[ 1 ] );
			remove_tree( n->n[ 2 ] );
			n->nn = 0;
		    }
		    else
		    {
			//WHILE 1 { CODE1 } - INFINITE LOOP:
			remove_tree( n->n[ 0 ] );
			n->n[ 0 ] = 0;
			n->n[ 1 ]->flags = 0; //Just a statlist without any headers
			n->n[ 2 ]->val.p = n->n[ 1 ]; //jmp to CODE1
		    }
		}
	    }
	    break;
	    
	case lnode_sub:
	case lnode_add:
	case lnode_mul:
	case lnode_idiv:
	case lnode_div:
	case lnode_mod:
	case lnode_and:
	case lnode_or:
	case lnode_xor:
	case lnode_andand:
	case lnode_oror:
	case lnode_eq:
	case lnode_neq:
	case lnode_less:
	case lnode_leq:
	case lnode_greater:
	case lnode_geq:
	case lnode_lshift:
	case lnode_rshift:
	    if( ( n->n[ 0 ]->type == lnode_int || n->n[ 0 ]->type == lnode_float ) &&
		( n->n[ 1 ]->type == lnode_int || n->n[ 1 ]->type == lnode_float ) )
	    {
		bool integer_op = 0;
		if( n->n[ 0 ]->type == lnode_int && n->n[ 1 ]->type == lnode_int ) //Both operands are integer
		    integer_op = 1;
		switch( n->type )
		{
		    case lnode_idiv:
		    case lnode_mod:
		    case lnode_and:
		    case lnode_or:
		    case lnode_xor:
		    case lnode_lshift:
		    case lnode_rshift:
			integer_op = 1; //Allways integer
			break;
		    case lnode_div:
			integer_op = 0; //Allways float
			break;
		    default:
		        break;
		}
		if( integer_op )
		{
		    PIX_INT i1, i2, res;
		    if( n->n[ 0 ]->type == lnode_float )
			i1 = (PIX_INT)n->n[ 0 ]->val.f;
		    else
			i1 = n->n[ 0 ]->val.i;
		    if( n->n[ 1 ]->type == lnode_float )
			i2 = (PIX_INT)n->n[ 1 ]->val.f;
		    else
			i2 = n->n[ 1 ]->val.i;
		    switch( n->type )
		    {
			case lnode_sub: res = i1 - i2; break;
			case lnode_add: res = i1 + i2; break;
			case lnode_mul: res = i1 * i2; break;
			case lnode_idiv: res = i1 / i2; break;
			case lnode_mod: res = i1 % i2; break;
			case lnode_and: res = i1 & i2; break;
			case lnode_or: res = i1 | i2; break;
			case lnode_xor: res = i1 ^ i2; break;
			case lnode_andand: res = ( i1 && i2 ); break;
			case lnode_oror: res = ( i1 || i2 ); break;
			case lnode_eq: res = ( i1 == i2 ); break;
			case lnode_neq: res = !( i1 == i2 ); break;
			case lnode_less: res = ( i1 < i2 ); break;
			case lnode_leq: res = ( i1 <= i2 ); break;
			case lnode_greater: res = ( i1 > i2 ); break;
			case lnode_geq: res = ( i1 >= i2 ); break;
			case lnode_lshift: res = ( i1 << (unsigned)i2 ); break;
			case lnode_rshift: res = ( i1 >> (unsigned)i2 ); break;
			default:
		    	    break;
		    }
		    n->type = lnode_int;
		    n->val.i = res;
		}
		else
		{
		    PIX_FLOAT f1, f2, res;
		    if( n->n[ 0 ]->type == lnode_float )
			f1 = n->n[ 0 ]->val.f;
		    else
			f1 = (PIX_FLOAT)n->n[ 0 ]->val.i;
		    if( n->n[ 1 ]->type == lnode_float )
			f2 = n->n[ 1 ]->val.f;
		    else
			f2 = (PIX_FLOAT)n->n[ 1 ]->val.i;
		    bool int_res = 0;
		    switch( n->type )
		    {
			case lnode_sub: res = f1 - f2; break;
			case lnode_add: res = f1 + f2; break;
			case lnode_mul: res = f1 * f2; break;
			case lnode_div: res = f1 / f2; break;
			case lnode_andand: res = ( f1 && f2 ); int_res = 1; break;
			case lnode_oror: res = ( f1 || f2 ); int_res = 1; break;
			case lnode_eq: res = ( f1 == f2 ); int_res = 1; break;
			case lnode_neq: res = !( f1 == f2 ); int_res = 1; break;
			case lnode_less: res = ( f1 < f2 ); int_res = 1; break;
			case lnode_leq: res = ( f1 <= f2 ); int_res = 1; break;
			case lnode_greater: res = ( f1 > f2 ); int_res = 1; break;
			case lnode_geq: res = ( f1 >= f2 ); int_res = 1; break;
			default:
		    	    break;
		    }
		    if( int_res )
		    {
			n->type = lnode_int;
			n->val.i = (PIX_INT)res;
		    }
		    else
		    {
			n->type = lnode_float;
			n->val.f = res;
		    }
		}
		remove_tree( n->n[ 0 ] );
		remove_tree( n->n[ 1 ] );
		n->nn = 0;
	    }
	    break;
	
	case lnode_neg:
	    switch( n->n[ 0 ]->type )
	    {
		case lnode_int:
		    n->type = lnode_int;
		    n->val.i = -n->n[ 0 ]->val.i;
		    remove_tree( n->n[ 0 ] );
		    n->nn = 0;
		    break;
		case lnode_float:
		    n->type = lnode_float;
		    n->val.f = -n->n[ 0 ]->val.f;
		    remove_tree( n->n[ 0 ] );
		    n->nn = 0;
		    break;
		default:
		    break;
	    }
	    break;
	
	case lnode_call_builtin_fn:
	case lnode_call_builtin_fn_void:
	case lnode_call:
	case lnode_call_void:
	    {
		lnode* pars = n->n[ 0 ];
	        //Flip parameters:
	        for( int i = 0; i < pars->nn / 2; i++ )
	        {
		    lnode* temp_node = pars->n[ i ];
		    pars->n[ i ] = pars->n[ pars->nn - 1 - i ];
		    pars->n[ pars->nn - 1 - i ] = temp_node;
	        }
	    }
	    break;
	    
	default:
	    break;
    }
}

void compile_tree( lnode* n )
{
    if( n == 0 ) return;

    n->code_ptr = g_comp->vm->code_ptr;
    
    switch( n->type )
    {
	case lnode_statlist:
	    if( ( n->flags & LNODE_FLAG_STATLIST_WITH_JMP_HEADER ) ||
	        ( n->flags & LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER ) )
	    {
		DPRINT( "%d: STATLIST_HEADER (%d NOPs)\n", (int)g_comp->vm->code_ptr, g_comp->statlist_header_size );
		for( int i = 0; i < g_comp->statlist_header_size; i++ )
		    pix_vm_put_opcode( OPCODE_NOP, g_comp->vm );
	    }
	    break;
	default:
	    break;
    }
    
    if( n->n )
    {
	for( int i = 0; i < n->nn; i++ )
	{
	    compile_tree( n->n[ i ] );
	}
    }
    
    switch( n->type )
    {
	case lnode_statlist:
	    if( ( n->flags & LNODE_FLAG_STATLIST_WITH_JMP_HEADER ) ||
	        ( n->flags & LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER ) )
	    {
		//Add "skip statlist" code:
		PIX_INT offset = (PIX_INT)g_comp->vm->code_ptr - (PIX_INT)n->code_ptr;
		PIX_OPCODE header_opcode = OPCODE_JMP_i;
		const utf8_char* header_opcode_name = "?";
		if( n->flags & LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER ) header_opcode = OPCODE_JMP_IF_FALSE_i;
		if( n->flags & LNODE_FLAG_STATLIST_SKIP_NEXT_HEADER ) offset += g_comp->statlist_header_size;
		bool short_int;
		if( offset < ( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) &&
		    offset > -( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) )
		{
		    short_int = 1;
		}
		else 
		{
		    short_int = 0;
		    header_opcode++;
		}
		switch( header_opcode )
		{
		    case OPCODE_JMP_i: header_opcode_name = "JMP_i"; break;
		    case OPCODE_JMP_IF_FALSE_i: header_opcode_name = "JMP_IF_FALSE_i"; break;
		}
		if( short_int )
		{
		    DPRINT( "%d: %s ( %d << OB ) (skip statlist)\n", (int)n->code_ptr, header_opcode_name, (int)offset );
		    size_t prev_code_ptr = g_comp->vm->code_ptr;
		    g_comp->vm->code_ptr = n->code_ptr;
		    pix_vm_put_opcode( header_opcode | ( (PIX_OPCODE)offset << PIX_OPCODE_BITS ), g_comp->vm );
		    g_comp->vm->code_ptr = prev_code_ptr;
		}
		else
		{
		    DPRINT( "%d: %s %d (skip statlist)\n", (int)n->code_ptr, header_opcode_name, (int)offset );
		    size_t prev_code_ptr = g_comp->vm->code_ptr;
		    g_comp->vm->code_ptr = n->code_ptr;
		    pix_vm_put_opcode( header_opcode, g_comp->vm );
		    pix_vm_put_int( offset, g_comp->vm );
		    g_comp->vm->code_ptr = prev_code_ptr;
		}
		if( n->flags & LNODE_FLAG_STATLIST_AS_EXPRESSION )
		{
		    push_int( ( n->code_ptr + g_comp->statlist_header_size ) | PIX_INT_ADDRESS_MARKER );
		}
	    }
	    break;
	    
	case lnode_int:
	    push_int( n->val.i );
	    break;
	case lnode_float:
	    DPRINT( "%d: PUSH_F %d\n", (int)g_comp->vm->code_ptr, (int)n->val.f );
	    pix_vm_put_opcode( OPCODE_PUSH_F, g_comp->vm );
	    pix_vm_put_float( n->val.f, g_comp->vm );
	    break;
	case lnode_var:
	    DPRINT( "%d: PUSH_v %d\n", (int)g_comp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_PUSH_v | ( n->val.i << PIX_OPCODE_BITS ), g_comp->vm );
	    break;
    
	case lnode_halt:
	    DPRINT( "%d: HALT\n", (int)g_comp->vm->code_ptr );
	    pix_vm_put_opcode( OPCODE_HALT, g_comp->vm );
	    break;
    
        case lnode_label:
	    DPRINT( "LABEL var:%d offset:%d\n", (int)n->val.i, (int)g_comp->vm->code_ptr );
	    g_comp->vm->vars[ n->val.i ].i = g_comp->vm->code_ptr | PIX_INT_ADDRESS_MARKER;
	    g_comp->vm->var_types[ n->val.i ] = 0;
	    break;
	case lnode_function_label_from_node:
	    DPRINT( "FUNCTION var:%d offset:%d\n", (int)n->val.i, (int)((lnode*)n->n[ 0 ]->val.p)->code_ptr + g_comp->statlist_header_size );
	    g_comp->vm->vars[ n->val.i ].i = ( ((lnode*)n->n[ 0 ]->val.p)->code_ptr + g_comp->statlist_header_size ) | PIX_INT_ADDRESS_MARKER;
	    g_comp->vm->var_types[ n->val.i ] = 0;
	    break;
	    
	case lnode_go:
	    DPRINT( "%d: GO\n", (int)g_comp->vm->code_ptr );
	    pix_vm_put_opcode( OPCODE_GO, g_comp->vm );
	    break;
	    
	case lnode_jmp_to_node:
	case lnode_jmp_to_end_of_node:
	    {
		size_t ptr;
		if( n->type == lnode_jmp_to_node )
		    ptr = ((lnode*)n->val.p)->code_ptr;
		else
		    ptr = ((lnode*)n->val.p)->code_ptr2;
		push_jmp( ptr );
		if( ptr == 0 )
		{
		    if( g_comp->fixup == 0 )
			g_comp->fixup = (lnode**)bmem_new( sizeof( lnode* ) );
		    g_comp->fixup[ g_comp->fixup_num ] = n;
		    g_comp->fixup_num++;
		    if( g_comp->fixup_num >= bmem_get_size( g_comp->fixup ) / sizeof( lnode* ) )
			g_comp->fixup = (lnode**)bmem_resize( g_comp->fixup, bmem_get_size( g_comp->fixup ) + 8 * sizeof( lnode* ) );
		}
	    }
	    break;

	case lnode_save_to_var:
	    DPRINT( "%d: SAVE_TO_VAR_v ( %d << OB )\n", (int)g_comp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_SAVE_TO_VAR_v | ( n->val.i << PIX_OPCODE_BITS ), g_comp->vm );
	    break;

	case lnode_save_to_prop:
	    DPRINT( "%d: SAVE_TO_PROP_I %d\n", (int)g_comp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_SAVE_TO_PROP_I, g_comp->vm );
	    pix_vm_put_int( n->val.i, g_comp->vm );
	    break;
	case lnode_load_from_prop:
	    DPRINT( "%d: LOAD_FROM_PROP_I %d\n", (int)g_comp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_LOAD_FROM_PROP_I, g_comp->vm );
	    pix_vm_put_int( n->val.i, g_comp->vm );
	    break;
	    
	case lnode_save_to_mem:
	    DPRINT( "%d: SAVE_TO_MEM\n", (int)g_comp->vm->code_ptr );
	    if( n->n[ 1 ]->type == lnode_empty && n->n[ 1 ]->nn == 2 )
		pix_vm_put_opcode( OPCODE_SAVE_TO_MEM_2D, g_comp->vm );
	    else
		pix_vm_put_opcode( OPCODE_SAVE_TO_MEM, g_comp->vm );
	    break;
	case lnode_load_from_mem:
	    DPRINT( "%d: LOAD_FROM_MEM\n", (int)g_comp->vm->code_ptr );
	    if( n->n[ 1 ]->type == lnode_empty && n->n[ 1 ]->nn == 2 )
		pix_vm_put_opcode( OPCODE_LOAD_FROM_MEM_2D, g_comp->vm );
	    else
		pix_vm_put_opcode( OPCODE_LOAD_FROM_MEM, g_comp->vm );
	    break;
	
	case lnode_save_to_stackframe:
	    DPRINT( "%d: SAVE_TO_STACKFRAME_i ( %d << OB )\n", (int)g_comp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_SAVE_TO_STACKFRAME_i | ( (PIX_OPCODE)n->val.i << PIX_OPCODE_BITS ), g_comp->vm );
	    break;
	case lnode_load_from_stackframe:
	    DPRINT( "%d: LOAD_FROM_STACKFRAME_i ( %d << OB )\n", (int)g_comp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_LOAD_FROM_STACKFRAME_i | ( (PIX_OPCODE)n->val.i << PIX_OPCODE_BITS ), g_comp->vm );
	    break;
    
	case lnode_sub:
	case lnode_add:
	case lnode_mul:
	case lnode_idiv:
	case lnode_div:
	case lnode_mod:
	case lnode_and:
	case lnode_or:
	case lnode_xor:
	case lnode_andand:
	case lnode_oror:
	case lnode_eq:
	case lnode_neq:
	case lnode_less:
	case lnode_leq:
	case lnode_greater:
	case lnode_geq:
	case lnode_lshift:
	case lnode_rshift:
	    DPRINT( "%d: MATH OP %d\n", (int)g_comp->vm->code_ptr, n->type - lnode_sub );
	    pix_vm_put_opcode( n->type - lnode_sub + OPCODE_SUB, g_comp->vm );
	    break;
    
	case lnode_neg:
	    DPRINT( "%d: NEG\n", (int)g_comp->vm->code_ptr );
	    pix_vm_put_opcode( OPCODE_NEG, g_comp->vm );
	    break;
	    
	case lnode_call_builtin_fn:
	    DPRINT( "%d: CALL_BUILTIN_FN ( %d << OB+FB ) ( %d << OB ) (%s)\n", (int)g_comp->vm->code_ptr, (int)n->n[ 0 ]->nn, (int)n->val.i, g_pix_fn_names[ n->val.i ] );
	    pix_vm_put_opcode( OPCODE_CALL_BUILTIN_FN | ( n->n[ 0 ]->nn << ( PIX_OPCODE_BITS + PIX_FN_BITS ) ) | ( n->val.i << PIX_OPCODE_BITS ), g_comp->vm );
	    break;
	case lnode_call_builtin_fn_void:
	    DPRINT( "%d: CALL_BUILTIN_FN_VOID ( %d << OB+FB ) ( %d << OB ) (%s)\n", (int)g_comp->vm->code_ptr, (int)n->n[ 0 ]->nn, (int)n->val.i, g_pix_fn_names[ n->val.i ] );
	    pix_vm_put_opcode( OPCODE_CALL_BUILTIN_FN_VOID | ( n->n[ 0 ]->nn << ( PIX_OPCODE_BITS + PIX_FN_BITS ) ) | ( n->val.i << PIX_OPCODE_BITS ), g_comp->vm );
	    break;
	case lnode_call:
	case lnode_call_void:
	    DPRINT( "%d: CALL_i ( %d << OB )\n", (int)g_comp->vm->code_ptr, (int)n->n[ 0 ]->nn );
	    pix_vm_put_opcode( OPCODE_CALL_i | ( n->n[ 0 ]->nn << PIX_OPCODE_BITS ), g_comp->vm );
	    if( n->type == lnode_call_void )
	    {
		DPRINT( "%d: INC_SP_i ( %d << OB )\n", (int)g_comp->vm->code_ptr, 1 );
		pix_vm_put_opcode( OPCODE_INC_SP_i | ( 1 << PIX_OPCODE_BITS ), g_comp->vm );
	    }
	    break;
	case lnode_ret_int:
	    if( n->val.i < ( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) &&
		n->val.i > -( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) )
	    {
		DPRINT( "%d: RET_i ( %d << OB )\n", (int)g_comp->vm->code_ptr, (int)n->val.i );
		pix_vm_put_opcode( OPCODE_RET_i | ( (PIX_OPCODE)n->val.i << PIX_OPCODE_BITS ), g_comp->vm );
	    }
	    else
	    {
		DPRINT( "%d: RET_I %d\n", (int)g_comp->vm->code_ptr, (int)n->val.i );
		pix_vm_put_opcode( OPCODE_RET_I, g_comp->vm );
		pix_vm_put_int( n->val.i, g_comp->vm );
	    }
	    break;
	case lnode_ret:
	    DPRINT( "%d: RET\n", (int)g_comp->vm->code_ptr );
	    pix_vm_put_opcode( OPCODE_RET, g_comp->vm );
	    break;
	case lnode_inc_sp:
	    DPRINT( "%d: INC_SP_i ( %d << OB )\n", (int)g_comp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_INC_SP_i | ( (PIX_OPCODE)n->val.i << PIX_OPCODE_BITS ), g_comp->vm );
	    break;

	default:
	    break;
    }

    n->code_ptr2 = g_comp->vm->code_ptr;
}

void fix_up( void )
{
    if( g_comp->fixup && g_comp->fixup_num )
    {
	for( int i = 0; i < g_comp->fixup_num; i++ )
	{
	    lnode* n = g_comp->fixup[ i ];
	    size_t prev_code_ptr = g_comp->vm->code_ptr;
    	    g_comp->vm->code_ptr = n->code_ptr;
	    switch( n->type )
	    {
		case lnode_jmp_to_node:
            	    push_jmp( ((lnode*)n->val.p)->code_ptr );
		    break;
		case lnode_jmp_to_end_of_node:
            	    push_jmp( ((lnode*)n->val.p)->code_ptr2 );
		    break;
		default:
		    break;
	    }
    	    g_comp->vm->code_ptr = prev_code_ptr;
	}
    }
}

#define ADD_SYMBOL( sname, stype, sval ) { if( stype == SYMTYPE_NUM_F ) pix_symtab_lookup( sname, -1, 1, stype, 0, sval, 0, &g_comp->sym ); else pix_symtab_lookup( sname, -1, 1, stype, sval, 0, 0, &g_comp->sym ); }

int pix_compile_from_memory( utf8_char* src, int src_size, utf8_char* src_name, utf8_char* base_path, pix_vm* vm, pix_data* pd )
{
    int rv = 0;

    DPRINT( "Pixilang compiler started.\n" );

    ticks_t start_time = time_ticks();
    
    if( bmutex_lock( &pd->compiler_mutex ) )
    {
	rv = 1;
	goto compiler_end2;
    }

    DPRINT( "Init...\n" );
    g_comp = (pix_compiler*)bmem_new( sizeof( pix_compiler ) );
    if( g_comp == 0 ) 
    {
	rv = 2;
	goto compiler_end;
    }
    bmem_zero( g_comp );
    g_comp->vm = vm;
    g_comp->pd = pd;
    g_comp->src = src;
    g_comp->src_size = src_size;
    g_comp->src_name = src_name;
    if( 0 ) 
    {
	if( sizeof( PIX_INT ) >= sizeof( PIX_OPCODE ) )
    	    g_comp->statlist_header_size = 1 + sizeof( PIX_INT ) / sizeof( PIX_OPCODE );
        else
	    g_comp->statlist_header_size = 1 + 1;
    }
    else
    {
	g_comp->statlist_header_size = 1;
    }
    //Global symbol table:
    if( pix_symtab_init( PIX_COMPILER_SYMTAB_SIZE, &g_comp->sym ) )
    {
	rv = 3;
	goto compiler_end;
    }
    //Local symbol tables:
    g_comp->lsym = (pix_lsymtab*)bmem_new( 8 * sizeof( pix_lsymtab ) );
    if( g_comp->lsym == 0 )
    {
	rv = 4;
	goto compiler_end;
    }
    bmem_zero( g_comp->lsym );
    //Set base path:
    g_comp->base_path = base_path;
    DPRINT( "Base path: %s\n", base_path );
    
    DPRINT( "VM init...\n" );
    vm->vars_num = 128; //standard set of variables with one-char ASCII names
    vm->vars_num += PIX_GVARS - vm->vars_num;
    pix_vm_resize_variables( vm );
    g_comp->var_flags = (char*)bmem_new( vm->vars_num );
    g_comp->var_flags_size = vm->vars_num;
    bmem_zero( g_comp->var_flags );
    bmem_free( vm->base_path );
    vm->base_path = (utf8_char*)bmem_new( bmem_strlen( base_path ) + 1 );
    if( vm->base_path == 0 )
    {
	rv = 5;
	goto compiler_end;
    }    
    vm->base_path[ 0 ] = 0;
    bmem_strcat_resize( vm->base_path, base_path );
    //Screen (container):
    vm->screen = pix_vm_new_container( -1, 16, 16, 32, 0, vm );
    pix_vm_set_container_flags( vm->screen, pix_vm_get_container_flags( vm->screen, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
    pix_vm_gfx_set_screen( vm->screen, vm );
    //Fonts:
    {
	for( int i = 0; i < PIX_VM_FONTS; i++ )
	    vm->fonts[ i ].font = -1;
	COLORPTR font_data = (COLORPTR)bmem_new( g_font8x8_xsize * g_font8x8_ysize * COLORLEN );
	if( font_data == 0 )
	{
	    rv = 6;
	    goto compiler_end;
	}
	int src_ptr = 0;
	int dest_ptr = 0;
	for( int y = 0; y < g_font8x8_ysize; y++ )
	{
	    for( int x = 0; x < g_font8x8_xsize; x += 8 )
	    {
		int byte = g_font8x8[ src_ptr ];
		for( int i = 0; i < 8; i++ )
		{
		    if( byte & 128 ) font_data[ dest_ptr ] = get_color( 255, 255, 255 ); else font_data[ dest_ptr ] = get_color( 0, 0, 0 );
		    byte <<= 1;
		    dest_ptr++;
		}
		src_ptr++;
	    }
	}
	PIX_CID fc = pix_vm_new_container( -1, g_font8x8_xsize, g_font8x8_ysize, 32, font_data, vm );
	pix_vm_set_container_flags( fc, pix_vm_get_container_flags( fc, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
	vm->fonts[ 0 ].font = fc;
	vm->fonts[ 0 ].xchars = g_font8x8_xchars;
	vm->fonts[ 0 ].ychars = g_font8x8_ychars;
	vm->fonts[ 0 ].first = 32;
	vm->fonts[ 0 ].last = 32 + g_font8x8_xchars * g_font8x8_ychars - 1;
	pix_vm_set_container_flags( fc, pix_vm_get_container_flags( fc, vm ) | PIX_CONTAINER_FLAG_USES_KEY, vm );
	pix_vm_set_container_key_color( fc, font_data[ 0 ], vm );
    }
    //Current event (container):
    vm->event = pix_vm_new_container( -1, 16, 1, PIX_CONTAINER_TYPE_INT32, 0, vm );
    pix_vm_set_container_flags( vm->event, pix_vm_get_container_flags( vm->event, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
    //System info (containers):
    pix_vm_set_systeminfo_containers( vm );
    //Window size:
    vm->var_types[ PIX_GVAR_WINDOW_XSIZE ] = 0;
    vm->var_types[ PIX_GVAR_WINDOW_YSIZE ] = 0;
    vm->var_types[ PIX_GVAR_FPS ] = 0;
    vm->var_types[ PIX_GVAR_PPI ] = 0;
    vm->var_types[ PIX_GVAR_SCALE ] = 1;
    vm->var_types[ PIX_GVAR_FONT_SCALE ] = 1;
    vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i = 16;
    vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i = 16;
    vm->vars[ PIX_GVAR_FPS ].i = 0;
    vm->vars[ PIX_GVAR_PPI ].i = 0;
    vm->vars[ PIX_GVAR_SCALE ].f = 1;
    vm->vars[ PIX_GVAR_FONT_SCALE ].f = 1;
    
    DPRINT( "Adding base symbols...\n" );
    ADD_SYMBOL( "while", SYMTYPE_WHILE, 0 );
    ADD_SYMBOL( "break", SYMTYPE_BREAK, 0 );
    ADD_SYMBOL( "break2", SYMTYPE_BREAK2, 0 );
    ADD_SYMBOL( "break3", SYMTYPE_BREAK3, 0 );
    ADD_SYMBOL( "break4", SYMTYPE_BREAK4, 0 );
    ADD_SYMBOL( "breakall", SYMTYPE_BREAKALL, 0 );
    ADD_SYMBOL( "continue", SYMTYPE_CONTINUE, 0 );
    ADD_SYMBOL( "if", SYMTYPE_IF, 0 );
    ADD_SYMBOL( "else", SYMTYPE_ELSE, 0 );
    ADD_SYMBOL( "go", SYMTYPE_GO, 0 );
    ADD_SYMBOL( "goto", SYMTYPE_GO, 0 );
    ADD_SYMBOL( "ret", SYMTYPE_RET, 0 );
    ADD_SYMBOL( "div", SYMTYPE_IDIV, 0 );
    ADD_SYMBOL( "halt", SYMTYPE_HALT, 0 );
    ADD_SYMBOL( "fn", SYMTYPE_FNDEF, 0 );
    ADD_SYMBOL( "include", SYMTYPE_INCLUDE, 0 );

    DPRINT( "Adding functions...\n" );
    //Containers (memory management):
    ADD_SYMBOL( "new", SYMTYPE_FNNUM, FN_NEW_PIXI );
    ADD_SYMBOL( "remove", SYMTYPE_FNNUM, FN_REMOVE_PIXI );
    ADD_SYMBOL( "remove_with_alpha", SYMTYPE_FNNUM, FN_REMOVE_PIXI_WITH_ALPHA );
    ADD_SYMBOL( "resize", SYMTYPE_FNNUM, FN_RESIZE_PIXI );
    ADD_SYMBOL( "rotate", SYMTYPE_FNNUM, FN_ROTATE_PIXI );
    ADD_SYMBOL( "convert_type", SYMTYPE_FNNUM, FN_CONVERT_PIXI_TYPE );
    ADD_SYMBOL( "clean", SYMTYPE_FNNUM, FN_CLEAN_PIXI );
    ADD_SYMBOL( "clone", SYMTYPE_FNNUM, FN_CLONE_PIXI );
    ADD_SYMBOL( "copy", SYMTYPE_FNNUM, FN_COPY_PIXI );
    ADD_SYMBOL( "get_size", SYMTYPE_FNNUM, FN_GET_PIXI_SIZE );
    ADD_SYMBOL( "get_xsize", SYMTYPE_FNNUM, FN_GET_PIXI_XSIZE );
    ADD_SYMBOL( "get_ysize", SYMTYPE_FNNUM, FN_GET_PIXI_YSIZE );
    ADD_SYMBOL( "get_esize", SYMTYPE_FNNUM, FN_GET_PIXI_ESIZE );
    ADD_SYMBOL( "get_type", SYMTYPE_FNNUM, FN_GET_PIXI_TYPE );
    ADD_SYMBOL( "get_flags", SYMTYPE_FNNUM, FN_GET_PIXI_FLAGS );
    ADD_SYMBOL( "set_flags", SYMTYPE_FNNUM, FN_SET_PIXI_FLAGS );
    ADD_SYMBOL( "reset_flags", SYMTYPE_FNNUM, FN_RESET_PIXI_FLAGS );
    ADD_SYMBOL( "get_prop", SYMTYPE_FNNUM, FN_GET_PIXI_PROP );
    ADD_SYMBOL( "set_prop", SYMTYPE_FNNUM, FN_SET_PIXI_PROP );
    ADD_SYMBOL( "remove_props", SYMTYPE_FNNUM, FN_REMOVE_PIXI_PROPS );
    ADD_SYMBOL( "show_memory_debug_messages", SYMTYPE_FNNUM, FN_SHOW_MEM_DEBUG_MESSAGES );
    ADD_SYMBOL( "zlib_pack", SYMTYPE_FNNUM, FN_ZLIB_PACK );
    ADD_SYMBOL( "zlib_unpack", SYMTYPE_FNNUM, FN_ZLIB_UNPACK );
    //Working with strings:
    ADD_SYMBOL( "num_to_str", SYMTYPE_FNNUM, FN_NUM_TO_STRING );
    ADD_SYMBOL( "num2str", SYMTYPE_FNNUM, FN_NUM_TO_STRING );
    ADD_SYMBOL( "str_to_num", SYMTYPE_FNNUM, FN_STRING_TO_NUM );
    ADD_SYMBOL( "str2num", SYMTYPE_FNNUM, FN_STRING_TO_NUM );
    //Working with strings (posix):
    ADD_SYMBOL( "strcat", SYMTYPE_FNNUM, FN_STRCAT );
    ADD_SYMBOL( "strcmp", SYMTYPE_FNNUM, FN_STRCMP );
    ADD_SYMBOL( "strlen", SYMTYPE_FNNUM, FN_STRLEN );
    ADD_SYMBOL( "strstr", SYMTYPE_FNNUM, FN_STRSTR );
    ADD_SYMBOL( "sprintf", SYMTYPE_FNNUM, FN_SPRINTF );
    ADD_SYMBOL( "printf", SYMTYPE_FNNUM, FN_PRINTF );
    ADD_SYMBOL( "fprintf", SYMTYPE_FNNUM, FN_FPRINTF );
    //Log management:
    ADD_SYMBOL( "logf", SYMTYPE_FNNUM, FN_LOGF );
    ADD_SYMBOL( "get_log", SYMTYPE_FNNUM, FN_GET_LOG );
    ADD_SYMBOL( "get_system_log", SYMTYPE_FNNUM, FN_GET_SYSTEM_LOG );
    //Working with files:
    ADD_SYMBOL( "load", SYMTYPE_FNNUM, FN_LOAD );
    ADD_SYMBOL( "fload", SYMTYPE_FNNUM, FN_FLOAD );
    ADD_SYMBOL( "save", SYMTYPE_FNNUM, FN_SAVE );
    ADD_SYMBOL( "fsave", SYMTYPE_FNNUM, FN_FSAVE );
    ADD_SYMBOL( "get_real_path", SYMTYPE_FNNUM, FN_GET_REAL_PATH );
    ADD_SYMBOL( "new_flist", SYMTYPE_FNNUM, FN_NEW_FLIST );
    ADD_SYMBOL( "remove_flist", SYMTYPE_FNNUM, FN_REMOVE_FLIST );
    ADD_SYMBOL( "get_flist_name", SYMTYPE_FNNUM, FN_GET_FLIST_NAME );
    ADD_SYMBOL( "get_flist_type", SYMTYPE_FNNUM, FN_GET_FLIST_TYPE );
    ADD_SYMBOL( "flist_next", SYMTYPE_FNNUM, FN_FLIST_NEXT );
    ADD_SYMBOL( "get_file_size", SYMTYPE_FNNUM, FN_GET_FILE_SIZE );
    ADD_SYMBOL( "remove_file", SYMTYPE_FNNUM, FN_REMOVE_FILE );
    ADD_SYMBOL( "rename_file", SYMTYPE_FNNUM, FN_RENAME_FILE );
    ADD_SYMBOL( "copy_file", SYMTYPE_FNNUM, FN_COPY_FILE );
    ADD_SYMBOL( "create_directory", SYMTYPE_FNNUM, FN_CREATE_DIRECTORY );
    ADD_SYMBOL( "set_disk0", SYMTYPE_FNNUM, FN_SET_DISK0 );
    ADD_SYMBOL( "get_disk0", SYMTYPE_FNNUM, FN_GET_DISK0 );
    //Working with files (posix):
    ADD_SYMBOL( "fopen", SYMTYPE_FNNUM, FN_FOPEN );
    ADD_SYMBOL( "fopen_mem", SYMTYPE_FNNUM, FN_FOPEN_MEM );
    ADD_SYMBOL( "fclose", SYMTYPE_FNNUM, FN_FCLOSE );
    ADD_SYMBOL( "fputc", SYMTYPE_FNNUM, FN_FPUTC );
    ADD_SYMBOL( "fputs", SYMTYPE_FNNUM, FN_FPUTS );
    ADD_SYMBOL( "fwrite", SYMTYPE_FNNUM, FN_FWRITE );
    ADD_SYMBOL( "fgetc", SYMTYPE_FNNUM, FN_FGETC );
    ADD_SYMBOL( "fgets", SYMTYPE_FNNUM, FN_FGETS );
    ADD_SYMBOL( "fread", SYMTYPE_FNNUM, FN_FREAD );
    ADD_SYMBOL( "feof", SYMTYPE_FNNUM, FN_FEOF );
    ADD_SYMBOL( "fflush", SYMTYPE_FNNUM, FN_FFLUSH );
    ADD_SYMBOL( "fseek", SYMTYPE_FNNUM, FN_FSEEK );
    ADD_SYMBOL( "ftell", SYMTYPE_FNNUM, FN_FTELL );    
    ADD_SYMBOL( "setxattr", SYMTYPE_FNNUM, FN_SETXATTR );
    //Graphics:
    ADD_SYMBOL( "frame", SYMTYPE_FNNUM, FN_FRAME );
    ADD_SYMBOL( "vsync", SYMTYPE_FNNUM, FN_VSYNC );
    ADD_SYMBOL( "set_pixel_size", SYMTYPE_FNNUM, FN_SET_PIXEL_SIZE );
    ADD_SYMBOL( "get_pixel_size", SYMTYPE_FNNUM, FN_GET_PIXEL_SIZE );
    ADD_SYMBOL( "set_screen", SYMTYPE_FNNUM, FN_SET_SCREEN );
    ADD_SYMBOL( "get_screen", SYMTYPE_FNNUM, FN_GET_SCREEN );
    ADD_SYMBOL( "set_zbuf", SYMTYPE_FNNUM, FN_SET_ZBUF );
    ADD_SYMBOL( "get_zbuf", SYMTYPE_FNNUM, FN_GET_ZBUF );
    ADD_SYMBOL( "clear_zbuf", SYMTYPE_FNNUM, FN_CLEAR_ZBUF );
    ADD_SYMBOL( "get_color", SYMTYPE_FNNUM, FN_GET_COLOR );
    ADD_SYMBOL( "get_red", SYMTYPE_FNNUM, FN_GET_RED );
    ADD_SYMBOL( "get_green", SYMTYPE_FNNUM, FN_GET_GREEN );
    ADD_SYMBOL( "get_blue", SYMTYPE_FNNUM, FN_GET_BLUE );
    ADD_SYMBOL( "get_blend", SYMTYPE_FNNUM, FN_GET_BLEND );
    ADD_SYMBOL( "transp", SYMTYPE_FNNUM, FN_TRANSP );
    ADD_SYMBOL( "get_transp", SYMTYPE_FNNUM, FN_GET_TRANSP );
    ADD_SYMBOL( "clear", SYMTYPE_FNNUM, FN_CLEAR );
    ADD_SYMBOL( "dot", SYMTYPE_FNNUM, FN_DOT );
    ADD_SYMBOL( "dot3d", SYMTYPE_FNNUM, FN_DOT3D );
    ADD_SYMBOL( "get_dot", SYMTYPE_FNNUM, FN_GET_DOT );
    ADD_SYMBOL( "get_dot3d", SYMTYPE_FNNUM, FN_GET_DOT3D );
    ADD_SYMBOL( "line", SYMTYPE_FNNUM, FN_LINE );
    ADD_SYMBOL( "line3d", SYMTYPE_FNNUM, FN_LINE3D );
    ADD_SYMBOL( "box", SYMTYPE_FNNUM, FN_BOX );
    ADD_SYMBOL( "fbox", SYMTYPE_FNNUM, FN_FBOX );
    ADD_SYMBOL( "pixi", SYMTYPE_FNNUM, FN_PIXI );
    ADD_SYMBOL( "triangles3d", SYMTYPE_FNNUM, FN_TRIANGLES );
    ADD_SYMBOL( "sort_triangles3d", SYMTYPE_FNNUM, FN_SORT_TRIANGLES );
    ADD_SYMBOL( "set_key_color", SYMTYPE_FNNUM, FN_SET_KEY_COLOR );
    ADD_SYMBOL( "get_key_color", SYMTYPE_FNNUM, FN_GET_KEY_COLOR );
    ADD_SYMBOL( "set_alpha", SYMTYPE_FNNUM, FN_SET_ALPHA );
    ADD_SYMBOL( "get_alpha", SYMTYPE_FNNUM, FN_GET_ALPHA );
    ADD_SYMBOL( "print", SYMTYPE_FNNUM, FN_PRINT );
    ADD_SYMBOL( "get_text_xsize", SYMTYPE_FNNUM, FN_GET_TEXT_XSIZE );
    ADD_SYMBOL( "get_text_ysize", SYMTYPE_FNNUM, FN_GET_TEXT_YSIZE );
    ADD_SYMBOL( "set_font", SYMTYPE_FNNUM, FN_SET_FONT );
    ADD_SYMBOL( "get_font", SYMTYPE_FNNUM, FN_GET_FONT );
    ADD_SYMBOL( "effector", SYMTYPE_FNNUM, FN_EFFECTOR );
    ADD_SYMBOL( "color_gradient", SYMTYPE_FNNUM, FN_COLOR_GRADIENT );
    ADD_SYMBOL( "split_rgb", SYMTYPE_FNNUM, FN_SPLIT_RGB );
    ADD_SYMBOL( "split_ycbcr", SYMTYPE_FNNUM, FN_SPLIT_YCBCR );
    //OpenGL graphics:
    ADD_SYMBOL( "set_gl_callback", SYMTYPE_FNNUM, FN_SET_GL_CALLBACK );
    ADD_SYMBOL( "remove_gl_data", SYMTYPE_FNNUM, FN_REMOVE_GL_DATA );
    ADD_SYMBOL( "gl_draw_arrays", SYMTYPE_FNNUM, FN_GL_DRAW_ARRAYS );
    ADD_SYMBOL( "gl_blend_func", SYMTYPE_FNNUM, FN_GL_BLEND_FUNC );
    ADD_SYMBOL( "gl_bind_framebuffer", SYMTYPE_FNNUM, FN_GL_BIND_FRAMEBUFFER );
    ADD_SYMBOL( "gl_new_prog", SYMTYPE_FNNUM, FN_GL_NEW_PROG );
    ADD_SYMBOL( "gl_use_prog", SYMTYPE_FNNUM, FN_GL_USE_PROG );
    ADD_SYMBOL( "gl_uniform", SYMTYPE_FNNUM, FN_GL_UNIFORM );
    ADD_SYMBOL( "gl_uniform_matrix", SYMTYPE_FNNUM, FN_GL_UNIFORM_MATRIX );
    //Animation:
    ADD_SYMBOL( "unpack_frame", SYMTYPE_FNNUM, FN_PIXI_UNPACK_FRAME );
    ADD_SYMBOL( "pack_frame", SYMTYPE_FNNUM, FN_PIXI_PACK_FRAME );
    ADD_SYMBOL( "create_anim", SYMTYPE_FNNUM, FN_PIXI_CREATE_ANIM );
    ADD_SYMBOL( "remove_anim", SYMTYPE_FNNUM, FN_PIXI_REMOVE_ANIM );
    ADD_SYMBOL( "clone_frame", SYMTYPE_FNNUM, FN_PIXI_CLONE_FRAME );
    ADD_SYMBOL( "remove_frame", SYMTYPE_FNNUM, FN_PIXI_REMOVE_FRAME );
    ADD_SYMBOL( "play", SYMTYPE_FNNUM, FN_PIXI_PLAY );
    ADD_SYMBOL( "stop", SYMTYPE_FNNUM, FN_PIXI_STOP );    
    //Video:
    ADD_SYMBOL( "video_open", SYMTYPE_FNNUM, FN_VIDEO_OPEN );
    ADD_SYMBOL( "video_close", SYMTYPE_FNNUM, FN_VIDEO_CLOSE );
    ADD_SYMBOL( "video_start", SYMTYPE_FNNUM, FN_VIDEO_START );
    ADD_SYMBOL( "video_stop", SYMTYPE_FNNUM, FN_VIDEO_STOP );
    ADD_SYMBOL( "video_set_props", SYMTYPE_FNNUM, FN_VIDEO_SET_PROPS );
    ADD_SYMBOL( "video_get_props", SYMTYPE_FNNUM, FN_VIDEO_GET_PROPS );
    ADD_SYMBOL( "video_capture_frame", SYMTYPE_FNNUM, FN_VIDEO_CAPTURE_FRAME );
    //Transformation:
    ADD_SYMBOL( "t_reset", SYMTYPE_FNNUM, FN_T_RESET );
    ADD_SYMBOL( "t_rotate", SYMTYPE_FNNUM, FN_T_ROTATE );
    ADD_SYMBOL( "t_translate", SYMTYPE_FNNUM, FN_T_TRANSLATE );
    ADD_SYMBOL( "t_scale", SYMTYPE_FNNUM, FN_T_SCALE );
    ADD_SYMBOL( "t_push_matrix", SYMTYPE_FNNUM, FN_T_PUSH_MATRIX );
    ADD_SYMBOL( "t_pop_matrix", SYMTYPE_FNNUM, FN_T_POP_MATRIX );
    ADD_SYMBOL( "t_get_matrix", SYMTYPE_FNNUM, FN_T_GET_MATRIX );
    ADD_SYMBOL( "t_set_matrix", SYMTYPE_FNNUM, FN_T_SET_MATRIX );
    ADD_SYMBOL( "t_mul_matrix", SYMTYPE_FNNUM, FN_T_MUL_MATRIX );
    ADD_SYMBOL( "t_point", SYMTYPE_FNNUM, FN_T_POINT );
    //Audio:
    ADD_SYMBOL( "set_audio_callback", SYMTYPE_FNNUM, FN_SET_AUDIO_CALLBACK );
    ADD_SYMBOL( "enable_audio_input", SYMTYPE_FNNUM, FN_ENABLE_AUDIO_INPUT );
    ADD_SYMBOL( "get_note_freq", SYMTYPE_FNNUM, FN_GET_NOTE_FREQ );
    //MIDI:
    ADD_SYMBOL( "midi_open_client", SYMTYPE_FNNUM, FN_MIDI_OPEN_CLIENT );
    ADD_SYMBOL( "midi_close_client", SYMTYPE_FNNUM, FN_MIDI_CLOSE_CLIENT );
    ADD_SYMBOL( "midi_get_device", SYMTYPE_FNNUM, FN_MIDI_GET_DEVICE );
    ADD_SYMBOL( "midi_open_port", SYMTYPE_FNNUM, FN_MIDI_OPEN_PORT );
    ADD_SYMBOL( "midi_reopen_port", SYMTYPE_FNNUM, FN_MIDI_REOPEN_PORT );
    ADD_SYMBOL( "midi_close_port", SYMTYPE_FNNUM, FN_MIDI_CLOSE_PORT );
    ADD_SYMBOL( "midi_get_event", SYMTYPE_FNNUM, FN_MIDI_GET_EVENT );
    ADD_SYMBOL( "midi_get_event_time", SYMTYPE_FNNUM, FN_MIDI_GET_EVENT_TIME );
    ADD_SYMBOL( "midi_next_event", SYMTYPE_FNNUM, FN_MIDI_NEXT_EVENT );
    ADD_SYMBOL( "midi_send_event", SYMTYPE_FNNUM, FN_MIDI_SEND_EVENT );
    //Time:
    ADD_SYMBOL( "start_timer", SYMTYPE_FNNUM, FN_START_TIMER );
    ADD_SYMBOL( "get_timer", SYMTYPE_FNNUM, FN_GET_TIMER );
    ADD_SYMBOL( "get_year", SYMTYPE_FNNUM, FN_GET_YEAR );
    ADD_SYMBOL( "get_month", SYMTYPE_FNNUM, FN_GET_MONTH );
    ADD_SYMBOL( "get_day", SYMTYPE_FNNUM, FN_GET_DAY );
    ADD_SYMBOL( "get_hours", SYMTYPE_FNNUM, FN_GET_HOURS );
    ADD_SYMBOL( "get_minutes", SYMTYPE_FNNUM, FN_GET_MINUTES );
    ADD_SYMBOL( "get_seconds", SYMTYPE_FNNUM, FN_GET_SECONDS );
    ADD_SYMBOL( "get_ticks", SYMTYPE_FNNUM, FN_GET_TICKS );
    ADD_SYMBOL( "get_tps", SYMTYPE_FNNUM, FN_GET_TPS );
    ADD_SYMBOL( "sleep", SYMTYPE_FNNUM, FN_SLEEP );
    //Events:
    ADD_SYMBOL( "get_event", SYMTYPE_FNNUM, FN_GET_EVENT );
    ADD_SYMBOL( "set_quit_action", SYMTYPE_FNNUM, FN_SET_QUIT_ACTION );
    //Threads:
    ADD_SYMBOL( "thread_create", SYMTYPE_FNNUM, FN_THREAD_CREATE );
    ADD_SYMBOL( "thread_destroy", SYMTYPE_FNNUM, FN_THREAD_DESTROY );
    ADD_SYMBOL( "mutex_create", SYMTYPE_FNNUM, FN_MUTEX_CREATE );
    ADD_SYMBOL( "mutex_destroy", SYMTYPE_FNNUM, FN_MUTEX_DESTROY );
    ADD_SYMBOL( "mutex_lock", SYMTYPE_FNNUM, FN_MUTEX_LOCK );
    ADD_SYMBOL( "mutex_trylock", SYMTYPE_FNNUM, FN_MUTEX_TRYLOCK );
    ADD_SYMBOL( "mutex_unlock", SYMTYPE_FNNUM, FN_MUTEX_UNLOCK );
    //Mathematical functions:
    ADD_SYMBOL( "acos", SYMTYPE_FNNUM, FN_ACOS );
    ADD_SYMBOL( "acosh", SYMTYPE_FNNUM, FN_ACOSH );
    ADD_SYMBOL( "asin", SYMTYPE_FNNUM, FN_ASIN );
    ADD_SYMBOL( "asinh", SYMTYPE_FNNUM, FN_ASINH );
    ADD_SYMBOL( "atan", SYMTYPE_FNNUM, FN_ATAN );
    ADD_SYMBOL( "atanh", SYMTYPE_FNNUM, FN_ATANH );
    ADD_SYMBOL( "ceil", SYMTYPE_FNNUM, FN_CEIL );
    ADD_SYMBOL( "cos", SYMTYPE_FNNUM, FN_COS );
    ADD_SYMBOL( "cosh", SYMTYPE_FNNUM, FN_COSH );
    ADD_SYMBOL( "exp", SYMTYPE_FNNUM, FN_EXP );
    ADD_SYMBOL( "exp2", SYMTYPE_FNNUM, FN_EXP2 );
    ADD_SYMBOL( "expm1", SYMTYPE_FNNUM, FN_EXPM1 );
    ADD_SYMBOL( "abs", SYMTYPE_FNNUM, FN_ABS );
    ADD_SYMBOL( "floor", SYMTYPE_FNNUM, FN_FLOOR );
    ADD_SYMBOL( "mod", SYMTYPE_FNNUM, FN_MOD );
    ADD_SYMBOL( "log", SYMTYPE_FNNUM, FN_LOG );
    ADD_SYMBOL( "log2", SYMTYPE_FNNUM, FN_LOG2 );
    ADD_SYMBOL( "log10", SYMTYPE_FNNUM, FN_LOG10 );
    ADD_SYMBOL( "pow", SYMTYPE_FNNUM, FN_POW );
    ADD_SYMBOL( "sin", SYMTYPE_FNNUM, FN_SIN );
    ADD_SYMBOL( "sinh", SYMTYPE_FNNUM, FN_SINH );
    ADD_SYMBOL( "sqrt", SYMTYPE_FNNUM, FN_SQRT );
    ADD_SYMBOL( "tan", SYMTYPE_FNNUM, FN_TAN );
    ADD_SYMBOL( "tanh", SYMTYPE_FNNUM, FN_TANH );
    ADD_SYMBOL( "rand", SYMTYPE_FNNUM, FN_RAND );
    ADD_SYMBOL( "rand_seed", SYMTYPE_FNNUM, FN_RAND_SEED );
    //Data processing:
    ADD_SYMBOL( "op_cn", SYMTYPE_FNNUM, FN_OP_CN );
    ADD_SYMBOL( "op_cc", SYMTYPE_FNNUM, FN_OP_CC );
    ADD_SYMBOL( "op_ccn", SYMTYPE_FNNUM, FN_OP_CCN );
    ADD_SYMBOL( "generator", SYMTYPE_FNNUM, FN_GENERATOR );
    ADD_SYMBOL( "wavetable_generator", SYMTYPE_FNNUM, FN_WAVETABLE_GENERATOR );
    ADD_SYMBOL( "sampler", SYMTYPE_FNNUM, FN_SAMPLER );
    ADD_SYMBOL( "envelope2p", SYMTYPE_FNNUM, FN_ENVELOPE2P );
    ADD_SYMBOL( "gradient", SYMTYPE_FNNUM, FN_GRADIENT );
    ADD_SYMBOL( "fft", SYMTYPE_FNNUM, FN_FFT );
    ADD_SYMBOL( "new_filter", SYMTYPE_FNNUM, FN_NEW_FILTER );
    ADD_SYMBOL( "remove_filter", SYMTYPE_FNNUM, FN_REMOVE_FILTER );
    ADD_SYMBOL( "init_filter", SYMTYPE_FNNUM, FN_INIT_FILTER );
    ADD_SYMBOL( "reset_filter", SYMTYPE_FNNUM, FN_RESET_FILTER );
    ADD_SYMBOL( "apply_filter", SYMTYPE_FNNUM, FN_APPLY_FILTER );
    ADD_SYMBOL( "replace_values", SYMTYPE_FNNUM, FN_REPLACE_VALUES );
    ADD_SYMBOL( "copy_and_resize", SYMTYPE_FNNUM, FN_COPY_AND_RESIZE );
    //Dialogs:
    ADD_SYMBOL( "file_dialog", SYMTYPE_FNNUM, FN_FILE_DIALOG );
    ADD_SYMBOL( "prefs_dialog", SYMTYPE_FNNUM, FN_PREFS_DIALOG );
    //Network:
    ADD_SYMBOL( "open_url", SYMTYPE_FNNUM, FN_OPEN_URL );    
    //Native code:
    ADD_SYMBOL( "dlopen", SYMTYPE_FNNUM, FN_DLOPEN );
    ADD_SYMBOL( "dlclose", SYMTYPE_FNNUM, FN_DLCLOSE );
    ADD_SYMBOL( "dlsym", SYMTYPE_FNNUM, FN_DLSYM );
    ADD_SYMBOL( "dlcall", SYMTYPE_FNNUM, FN_DLCALL );
    //Posix compatibility:
    ADD_SYMBOL( "system", SYMTYPE_FNNUM, FN_SYSTEM );
    ADD_SYMBOL( "argc", SYMTYPE_FNNUM, FN_ARGC );
    ADD_SYMBOL( "argv", SYMTYPE_FNNUM, FN_ARGV );
    ADD_SYMBOL( "exit", SYMTYPE_FNNUM, FN_EXIT );
    //Private API:
    ADD_SYMBOL( "system_copy", SYMTYPE_FNNUM, FN_SYSTEM_COPY );
    ADD_SYMBOL( "system_paste", SYMTYPE_FNNUM, FN_SYSTEM_PASTE );
    ADD_SYMBOL( "send_file_to_email", SYMTYPE_FNNUM, FN_SEND_FILE_TO_EMAIL );
    ADD_SYMBOL( "send_file_to_gallery", SYMTYPE_FNNUM, FN_SEND_FILE_TO_GALLERY );
    ADD_SYMBOL( "open_webserver", SYMTYPE_FNNUM, FN_OPEN_WEBSERVER );
    ADD_SYMBOL( "set_audio_play_status", SYMTYPE_FNNUM, FN_SET_AUDIO_PLAY_STATUS );
    ADD_SYMBOL( "get_audio_event", SYMTYPE_FNNUM, FN_GET_AUDIO_EVENT );
    ADD_SYMBOL( "wm_video_capture_supported", SYMTYPE_FNNUM, FN_WM_VIDEO_CAPTURE_SUPPORTED );
    ADD_SYMBOL( "wm_video_capture_start", SYMTYPE_FNNUM, FN_WM_VIDEO_CAPTURE_START );
    ADD_SYMBOL( "wm_video_capture_stop", SYMTYPE_FNNUM, FN_WM_VIDEO_CAPTURE_STOP );
    ADD_SYMBOL( "wm_video_capture_get_ext", SYMTYPE_FNNUM, FN_WM_VIDEO_CAPTURE_GET_EXT );
    ADD_SYMBOL( "wm_video_capture_encode", SYMTYPE_FNNUM, FN_WM_VIDEO_CAPTURE_ENCODE );

    DPRINT( "Adding constants...\n" );
    //STDIO constants:
    ADD_SYMBOL( "FOPEN_MAX", SYMTYPE_NUM_I, BFS_FOPEN_MAX ); //Number of streams which the implementation guarantees can be open simultaneously.
    ADD_SYMBOL( "SEEK_CUR", SYMTYPE_NUM_I, SEEK_CUR ); //Seek relative to current position.
    ADD_SYMBOL( "SEEK_END", SYMTYPE_NUM_I, SEEK_END ); //Seek relative to end-of-file.
    ADD_SYMBOL( "SEEK_SET", SYMTYPE_NUM_I, SEEK_SET ); //Seek relative to start-of-file.
    ADD_SYMBOL( "EOF", SYMTYPE_NUM_I, -1 ); //End-of-file return value.
    ADD_SYMBOL( "STDIN", SYMTYPE_NUM_I, BFS_STDIN ); //Standard input stream.
    ADD_SYMBOL( "STDOUT", SYMTYPE_NUM_I, BFS_STDOUT ); //Standard output stream.
    ADD_SYMBOL( "STDERR", SYMTYPE_NUM_I, BFS_STDERR ); //Standard error output stream.
    //ZLib constants:
    ADD_SYMBOL( "Z_NO_COMPRESSION", SYMTYPE_NUM_I, Z_NO_COMPRESSION );
    ADD_SYMBOL( "Z_BEST_SPEED", SYMTYPE_NUM_I, Z_BEST_SPEED );
    ADD_SYMBOL( "Z_BEST_COMPRESSION", SYMTYPE_NUM_I, Z_BEST_COMPRESSION );
    ADD_SYMBOL( "Z_DEFAULT_COMPRESSION", SYMTYPE_NUM_I, Z_DEFAULT_COMPRESSION );
    //Container flags:
    ADD_SYMBOL( "GL_MIN_LINEAR", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_MIN_LINEAR );
    ADD_SYMBOL( "GL_MAG_LINEAR", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_MAG_LINEAR );
    ADD_SYMBOL( "GL_NO_XREPEAT", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_NO_XREPEAT );
    ADD_SYMBOL( "GL_NO_YREPEAT", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_NO_YREPEAT );
    ADD_SYMBOL( "GL_NICEST", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_NICEST );
    ADD_SYMBOL( "GL_NO_ALPHA", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_NO_ALPHA );
    ADD_SYMBOL( "CFLAG_INTERP", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_INTERP );
    //Container copying flags:
    ADD_SYMBOL( "COPY_NO_AUTOROTATE", SYMTYPE_NUM_I, PIX_COPY_NO_AUTOROTATE );
    ADD_SYMBOL( "COPY_CLIPPING", SYMTYPE_NUM_I, PIX_COPY_CLIPPING );
    //Container resizing flags:
    ADD_SYMBOL( "RESIZE_INTERP1", SYMTYPE_NUM_I, PIX_RESIZE_INTERP1 );
    ADD_SYMBOL( "RESIZE_INTERP2", SYMTYPE_NUM_I, PIX_RESIZE_INTERP2 );
    ADD_SYMBOL( "RESIZE_UNSIGNED_INTERP2", SYMTYPE_NUM_I, PIX_RESIZE_INTERP2 | PIX_RESIZE_INTERP_UNSIGNED );
    ADD_SYMBOL( "RESIZE_COLOR_INTERP1", SYMTYPE_NUM_I, PIX_RESIZE_INTERP1 );
    ADD_SYMBOL( "RESIZE_COLOR_INTERP2", SYMTYPE_NUM_I, PIX_RESIZE_INTERP2 | PIX_RESIZE_INTERP_COLOR );
    //Colors:
    ADD_SYMBOL( "ORANGE", SYMTYPE_NUM_I, (COLORSIGNED)get_color( 255, 128, 16 ) );
    ADD_SYMBOL( "ORANJ", SYMTYPE_NUM_I, (COLORSIGNED)get_color( 255, 128, 16 ) );
    ADD_SYMBOL( "BLACK", SYMTYPE_NUM_I, (COLORSIGNED)get_color( 0, 0, 0 ) );
    ADD_SYMBOL( "WHITE", SYMTYPE_NUM_I, (COLORSIGNED)get_color( 255, 255, 255 ) );
    ADD_SYMBOL( "SNEG", SYMTYPE_NUM_I, (COLORSIGNED)get_color( 255, 255, 255 ) );
    ADD_SYMBOL( "YELLOW", SYMTYPE_NUM_I, (COLORSIGNED)get_color( 255, 255, 0 ) );
    ADD_SYMBOL( "SUN", SYMTYPE_NUM_I, (COLORSIGNED)get_color( 255, 255, 0 ) );
    ADD_SYMBOL( "RED", SYMTYPE_NUM_I, (COLORSIGNED)get_color( 255, 0, 0 ) );
    ADD_SYMBOL( "GREEN", SYMTYPE_NUM_I, (COLORSIGNED)get_color( 0, 255, 0 ) );
    ADD_SYMBOL( "ZELEN", SYMTYPE_NUM_I, (COLORSIGNED)get_color( 0, 255, 0 ) );
    ADD_SYMBOL( "BLUE", SYMTYPE_NUM_I, (COLORSIGNED)get_color( 0, 0, 255 ) );
    //Alignment:
    ADD_SYMBOL( "TOP", SYMTYPE_NUM_I, 1 );
    ADD_SYMBOL( "BOTTOM", SYMTYPE_NUM_I, 2 );
    ADD_SYMBOL( "LEFT", SYMTYPE_NUM_I, 4 );
    ADD_SYMBOL( "RIGHT", SYMTYPE_NUM_I, 8 );
    //Effects:
    ADD_SYMBOL( "EFF_NOISE", SYMTYPE_NUM_I, PIX_EFFECT_NOISE );
    ADD_SYMBOL( "EFF_SPREAD_LEFT", SYMTYPE_NUM_I, PIX_EFFECT_SPREAD_LEFT );
    ADD_SYMBOL( "EFF_SPREAD_RIGHT", SYMTYPE_NUM_I, PIX_EFFECT_SPREAD_RIGHT );
    ADD_SYMBOL( "EFF_SPREAD_UP", SYMTYPE_NUM_I, PIX_EFFECT_SPREAD_UP );
    ADD_SYMBOL( "EFF_SPREAD_DOWN", SYMTYPE_NUM_I, PIX_EFFECT_SPREAD_DOWN );
    ADD_SYMBOL( "EFF_VBLUR", SYMTYPE_NUM_I, PIX_EFFECT_VBLUR );
    ADD_SYMBOL( "EFF_HBLUR", SYMTYPE_NUM_I, PIX_EFFECT_HBLUR );
    ADD_SYMBOL( "EFF_COLOR", SYMTYPE_NUM_I, PIX_EFFECT_COLOR );
    //Video:
    ADD_SYMBOL( "VIDEO_PROP_FRAME_WIDTH", SYMTYPE_NUM_I, BVIDEO_PROP_FRAME_WIDTH_I );
    ADD_SYMBOL( "VIDEO_PROP_FRAME_HEIGHT", SYMTYPE_NUM_I, BVIDEO_PROP_FRAME_HEIGHT_I );
    ADD_SYMBOL( "VIDEO_PROP_FPS", SYMTYPE_NUM_I, BVIDEO_PROP_FPS_I );
    ADD_SYMBOL( "VIDEO_PROP_FOCUS_MODE", SYMTYPE_NUM_I, BVIDEO_PROP_FOCUS_MODE_I );
    ADD_SYMBOL( "VIDEO_FOCUS_MODE_AUTO", SYMTYPE_NUM_I, BVIDEO_FOCUS_MODE_AUTO );
    ADD_SYMBOL( "VIDEO_FOCUS_MODE_CONTINUOUS", SYMTYPE_NUM_I, BVIDEO_FOCUS_MODE_CONTINUOUS );
    ADD_SYMBOL( "VIDEO_FOCUS_MODE_FIXED", SYMTYPE_NUM_I, BVIDEO_FOCUS_MODE_FIXED );
    ADD_SYMBOL( "VIDEO_FOCUS_MODE_INFINITY", SYMTYPE_NUM_I, BVIDEO_FOCUS_MODE_INFINITY );
    ADD_SYMBOL( "VIDEO_OPEN_FLAG_READ", SYMTYPE_NUM_I, BVIDEO_OPEN_FLAG_READ );
    ADD_SYMBOL( "VIDEO_OPEN_FLAG_WRITE", SYMTYPE_NUM_I, BVIDEO_OPEN_FLAG_WRITE );
    //OpenGL:
#ifdef OPENGL
    //gl_draw_arrays() (analog of the glDrawArrays()) modes:
    ADD_SYMBOL( "GL_POINTS", SYMTYPE_NUM_I, GL_POINTS );
    ADD_SYMBOL( "GL_LINE_STRIP", SYMTYPE_NUM_I, GL_LINE_STRIP );
    ADD_SYMBOL( "GL_LINE_LOOP", SYMTYPE_NUM_I, GL_LINE_LOOP );
    ADD_SYMBOL( "GL_LINES", SYMTYPE_NUM_I, GL_LINES );
    ADD_SYMBOL( "GL_TRIANGLE_STRIP", SYMTYPE_NUM_I, GL_TRIANGLE_STRIP );
    ADD_SYMBOL( "GL_TRIANGLE_FAN", SYMTYPE_NUM_I, GL_TRIANGLE_FAN );
    ADD_SYMBOL( "GL_TRIANGLES", SYMTYPE_NUM_I, GL_TRIANGLES );
    //gl_blend_func() (analog of the glBlendFunc()) operations:
    ADD_SYMBOL( "GL_ZERO", SYMTYPE_NUM_I, GL_ZERO );
    ADD_SYMBOL( "GL_ONE", SYMTYPE_NUM_I, GL_ONE );
    ADD_SYMBOL( "GL_SRC_COLOR", SYMTYPE_NUM_I, GL_SRC_COLOR );
    ADD_SYMBOL( "GL_ONE_MINUS_SRC_COLOR", SYMTYPE_NUM_I, GL_ONE_MINUS_SRC_COLOR );
    ADD_SYMBOL( "GL_DST_COLOR", SYMTYPE_NUM_I, GL_DST_COLOR );
    ADD_SYMBOL( "GL_ONE_MINUS_DST_COLOR", SYMTYPE_NUM_I, GL_ONE_MINUS_DST_COLOR );
    ADD_SYMBOL( "GL_SRC_ALPHA", SYMTYPE_NUM_I, GL_SRC_ALPHA );
    ADD_SYMBOL( "GL_ONE_MINUS_SRC_ALPHA", SYMTYPE_NUM_I, GL_ONE_MINUS_SRC_ALPHA );
    ADD_SYMBOL( "GL_DST_ALPHA", SYMTYPE_NUM_I, GL_DST_ALPHA );
    ADD_SYMBOL( "GL_ONE_MINUS_DST_ALPHA", SYMTYPE_NUM_I, GL_ONE_MINUS_DST_ALPHA );
    ADD_SYMBOL( "GL_SRC_ALPHA_SATURATE", SYMTYPE_NUM_I, GL_SRC_ALPHA_SATURATE );
    //gl_new_prog() default shader names:
    ADD_SYMBOL( "GL_SHADER_SOLID", SYMTYPE_NUM_I, -1 - GL_SHADER_SOLID );
    ADD_SYMBOL( "GL_SHADER_GRAD", SYMTYPE_NUM_I, -1 - GL_SHADER_GRAD );
    ADD_SYMBOL( "GL_SHADER_TEX_ALPHA_SOLID", SYMTYPE_NUM_I, -1 - GL_SHADER_TEX_ALPHA_SOLID );
    ADD_SYMBOL( "GL_SHADER_TEX_ALPHA_GRAD", SYMTYPE_NUM_I, -1 - GL_SHADER_TEX_ALPHA_GRAD );
    ADD_SYMBOL( "GL_SHADER_TEX_RGB_SOLID", SYMTYPE_NUM_I, -1 - GL_SHADER_TEX_RGB_SOLID );
    ADD_SYMBOL( "GL_SHADER_TEX_RGB_GRAD", SYMTYPE_NUM_I, -1 - GL_SHADER_TEX_RGB_GRAD );
#endif
    //File formats:
    ADD_SYMBOL( "FORMAT_RAW", SYMTYPE_NUM_I, PIX_FORMAT_RAW );
    ADD_SYMBOL( "FORMAT_JPEG", SYMTYPE_NUM_I, PIX_FORMAT_JPEG );
    ADD_SYMBOL( "FORMAT_PNG", SYMTYPE_NUM_I, PIX_FORMAT_PNG );
    ADD_SYMBOL( "FORMAT_GIF", SYMTYPE_NUM_I, PIX_FORMAT_GIF );
    ADD_SYMBOL( "FORMAT_WAVE", SYMTYPE_NUM_I, PIX_FORMAT_WAVE );
    ADD_SYMBOL( "FORMAT_AIFF", SYMTYPE_NUM_I, PIX_FORMAT_AIFF );
    ADD_SYMBOL( "FORMAT_PIXICONTAINER", SYMTYPE_NUM_I, PIX_FORMAT_PIXICONTAINER );
    //Load/Save options (flags):
    ADD_SYMBOL( "GIF_GRAYSCALE", SYMTYPE_NUM_I, PIX_GIF_GRAYSCALE );
    ADD_SYMBOL( "GIF_DITHER", SYMTYPE_NUM_I, PIX_GIF_DITHER );
    ADD_SYMBOL( "JPEG_H1V1", SYMTYPE_NUM_I, PIX_JPEG_H1V1 );
    ADD_SYMBOL( "JPEG_H2V1", SYMTYPE_NUM_I, PIX_JPEG_H2V1 );
    ADD_SYMBOL( "JPEG_H2V2", SYMTYPE_NUM_I, PIX_JPEG_H2V2 );
    ADD_SYMBOL( "JPEG_TWOPASS", SYMTYPE_NUM_I, PIX_JPEG_TWOPASS );
    ADD_SYMBOL( "LOAD_FIRST_FRAME", SYMTYPE_NUM_I, PIX_LOAD_FIRST_FRAME );
    //Audio:
    ADD_SYMBOL( "AUDIO_FLAG_INTERP2", SYMTYPE_NUM_I, PIX_AUDIO_FLAG_INTERP2 );
    //MIDI:
    ADD_SYMBOL( "MIDI_PORT_READ", SYMTYPE_NUM_I, MIDI_PORT_READ );
    ADD_SYMBOL( "MIDI_PORT_WRITE", SYMTYPE_NUM_I, MIDI_PORT_WRITE );
    ADD_SYMBOL( "MIDI_NO_DEVICE", SYMTYPE_NUM_I, MIDI_NO_DEVICE );
    //Events:
    ADD_SYMBOL( "EVT", SYMTYPE_NUM_I, vm->event );
    ADD_SYMBOL( "EVT_TYPE", SYMTYPE_NUM_I, 0 );
    ADD_SYMBOL( "EVT_FLAGS", SYMTYPE_NUM_I, 1 );
    ADD_SYMBOL( "EVT_TIME", SYMTYPE_NUM_I, 2 );
    ADD_SYMBOL( "EVT_X", SYMTYPE_NUM_I, 3 );
    ADD_SYMBOL( "EVT_Y", SYMTYPE_NUM_I, 4 );
    ADD_SYMBOL( "EVT_KEY", SYMTYPE_NUM_I, 5 );
    ADD_SYMBOL( "EVT_SCANCODE", SYMTYPE_NUM_I, 6 );
    ADD_SYMBOL( "EVT_PRESSURE", SYMTYPE_NUM_I, 7 );
    ADD_SYMBOL( "EVT_UNICODE", SYMTYPE_NUM_I, 8 );
    ADD_SYMBOL( "EVT_MOUSEBUTTONDOWN", SYMTYPE_NUM_I, PIX_EVT_MOUSEBUTTONDOWN );
    ADD_SYMBOL( "EVT_MOUSEBUTTONUP", SYMTYPE_NUM_I, PIX_EVT_MOUSEBUTTONUP );
    ADD_SYMBOL( "EVT_MOUSEMOVE", SYMTYPE_NUM_I, PIX_EVT_MOUSEMOVE );
    ADD_SYMBOL( "EVT_TOUCHBEGIN", SYMTYPE_NUM_I, PIX_EVT_TOUCHBEGIN );
    ADD_SYMBOL( "EVT_TOUCHEND", SYMTYPE_NUM_I, PIX_EVT_TOUCHEND );
    ADD_SYMBOL( "EVT_TOUCHMOVE", SYMTYPE_NUM_I, PIX_EVT_TOUCHMOVE );
    ADD_SYMBOL( "EVT_BUTTONDOWN", SYMTYPE_NUM_I, PIX_EVT_BUTTONDOWN );
    ADD_SYMBOL( "EVT_BUTTONUP", SYMTYPE_NUM_I, PIX_EVT_BUTTONUP );
    ADD_SYMBOL( "EVT_SCREENRESIZE", SYMTYPE_NUM_I, PIX_EVT_SCREENRESIZE );
    ADD_SYMBOL( "EVT_QUIT", SYMTYPE_NUM_I, PIX_EVT_QUIT );
    ADD_SYMBOL( "EVT_FLAG_SHIFT", SYMTYPE_NUM_I, EVT_FLAG_SHIFT );
    ADD_SYMBOL( "EVT_FLAG_CTRL", SYMTYPE_NUM_I, EVT_FLAG_CTRL );
    ADD_SYMBOL( "EVT_FLAG_ALT", SYMTYPE_NUM_I, EVT_FLAG_ALT );
    ADD_SYMBOL( "EVT_FLAG_MODE", SYMTYPE_NUM_I, EVT_FLAG_MODE );
    ADD_SYMBOL( "EVT_FLAG_CMD", SYMTYPE_NUM_I, EVT_FLAG_CMD );
    ADD_SYMBOL( "EVT_FLAG_MODS", SYMTYPE_NUM_I, EVT_FLAG_MODS );
    ADD_SYMBOL( "EVT_FLAG_DOUBLECLICK", SYMTYPE_NUM_I, EVT_FLAG_DOUBLECLICK );
    ADD_SYMBOL( "EVT_FLAGS_NUM", SYMTYPE_NUM_I, EVT_FLAGS_NUM );
    ADD_SYMBOL( "KEY_MOUSE_LEFT", SYMTYPE_NUM_I, MOUSE_BUTTON_LEFT );
    ADD_SYMBOL( "KEY_MOUSE_MIDDLE", SYMTYPE_NUM_I, MOUSE_BUTTON_MIDDLE );
    ADD_SYMBOL( "KEY_MOUSE_RIGHT", SYMTYPE_NUM_I, MOUSE_BUTTON_RIGHT );
    ADD_SYMBOL( "KEY_MOUSE_SCROLLUP", SYMTYPE_NUM_I, MOUSE_BUTTON_SCROLLUP );
    ADD_SYMBOL( "KEY_MOUSE_SCROLLDOWN", SYMTYPE_NUM_I, MOUSE_BUTTON_SCROLLDOWN );
    ADD_SYMBOL( "KEY_BACKSPACE", SYMTYPE_NUM_I, KEY_BACKSPACE );
    ADD_SYMBOL( "KEY_TAB", SYMTYPE_NUM_I, KEY_TAB );
    ADD_SYMBOL( "KEY_ENTER", SYMTYPE_NUM_I, KEY_ENTER );
    ADD_SYMBOL( "KEY_ESCAPE", SYMTYPE_NUM_I, KEY_ESCAPE );
    ADD_SYMBOL( "KEY_SPACE", SYMTYPE_NUM_I, KEY_SPACE );
    ADD_SYMBOL( "KEY_F1", SYMTYPE_NUM_I, KEY_F1 );
    ADD_SYMBOL( "KEY_F2", SYMTYPE_NUM_I, KEY_F2 );
    ADD_SYMBOL( "KEY_F3", SYMTYPE_NUM_I, KEY_F3 );
    ADD_SYMBOL( "KEY_F4", SYMTYPE_NUM_I, KEY_F4 );
    ADD_SYMBOL( "KEY_F5", SYMTYPE_NUM_I, KEY_F5 );
    ADD_SYMBOL( "KEY_F6", SYMTYPE_NUM_I, KEY_F6 );
    ADD_SYMBOL( "KEY_F7", SYMTYPE_NUM_I, KEY_F7 );
    ADD_SYMBOL( "KEY_F8", SYMTYPE_NUM_I, KEY_F8 );
    ADD_SYMBOL( "KEY_F9", SYMTYPE_NUM_I, KEY_F9 );
    ADD_SYMBOL( "KEY_F10", SYMTYPE_NUM_I, KEY_F10 );
    ADD_SYMBOL( "KEY_F11", SYMTYPE_NUM_I, KEY_F11 );
    ADD_SYMBOL( "KEY_F12", SYMTYPE_NUM_I, KEY_F12 );
    ADD_SYMBOL( "KEY_UP", SYMTYPE_NUM_I, KEY_UP );
    ADD_SYMBOL( "KEY_DOWN", SYMTYPE_NUM_I, KEY_DOWN );
    ADD_SYMBOL( "KEY_LEFT", SYMTYPE_NUM_I, KEY_LEFT );
    ADD_SYMBOL( "KEY_RIGHT", SYMTYPE_NUM_I, KEY_RIGHT );
    ADD_SYMBOL( "KEY_INSERT", SYMTYPE_NUM_I, KEY_INSERT );
    ADD_SYMBOL( "KEY_DELETE", SYMTYPE_NUM_I, KEY_DELETE );
    ADD_SYMBOL( "KEY_HOME", SYMTYPE_NUM_I, KEY_HOME );
    ADD_SYMBOL( "KEY_END", SYMTYPE_NUM_I, KEY_END );
    ADD_SYMBOL( "KEY_PAGEUP", SYMTYPE_NUM_I, KEY_PAGEUP );
    ADD_SYMBOL( "KEY_PAGEDOWN", SYMTYPE_NUM_I, KEY_PAGEDOWN );
    ADD_SYMBOL( "KEY_CAPS", SYMTYPE_NUM_I, KEY_CAPS );
    ADD_SYMBOL( "KEY_SHIFT", SYMTYPE_NUM_I, KEY_SHIFT );
    ADD_SYMBOL( "KEY_CTRL", SYMTYPE_NUM_I, KEY_CTRL );
    ADD_SYMBOL( "KEY_ALT", SYMTYPE_NUM_I, KEY_ALT );
    ADD_SYMBOL( "KEY_CMD", SYMTYPE_NUM_I, KEY_CMD );
    ADD_SYMBOL( "KEY_MENU", SYMTYPE_NUM_I, KEY_MENU );
    ADD_SYMBOL( "KEY_UNKNOWN", SYMTYPE_NUM_I, KEY_UNKNOWN );
    ADD_SYMBOL( "QA_NONE", SYMTYPE_NUM_I, 0 );
    ADD_SYMBOL( "QA_CLOSE_VM", SYMTYPE_NUM_I, 1 );
    //Threads:
    ADD_SYMBOL( "THREAD_FLAG_AUTO_DESTROY", SYMTYPE_NUM_I, PIX_THREAD_FLAG_AUTO_DESTROY );
    //Mathematical:
    ADD_SYMBOL( "M_E", SYMTYPE_NUM_F, M_E );
    ADD_SYMBOL( "M_LOG2E", SYMTYPE_NUM_F, M_LOG2E );
    ADD_SYMBOL( "M_LOG10E", SYMTYPE_NUM_F, M_LOG10E );
    ADD_SYMBOL( "M_LN2", SYMTYPE_NUM_F, M_LN2 );
    ADD_SYMBOL( "M_LN10", SYMTYPE_NUM_F, M_LN10 );
    ADD_SYMBOL( "M_PI", SYMTYPE_NUM_F, M_PI );
    ADD_SYMBOL( "M_2_SQRTPI", SYMTYPE_NUM_F, M_2_SQRTPI );
    ADD_SYMBOL( "M_SQRT2", SYMTYPE_NUM_F, M_SQRT2 );
    ADD_SYMBOL( "M_SQRT1_2", SYMTYPE_NUM_F, M_SQRT1_2 );
    //Data processing operations (op_cn):
    ADD_SYMBOL( "OP_MIN", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MIN );
    ADD_SYMBOL( "OP_MAX", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MAX );
    ADD_SYMBOL( "OP_MAXMOD", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MAXABS );
    ADD_SYMBOL( "OP_MAXABS", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MAXABS );
    ADD_SYMBOL( "OP_SUM", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SUM );
    ADD_SYMBOL( "OP_LIMIT_TOP", SYMTYPE_NUM_I, PIX_DATA_OPCODE_LIMIT_TOP );
    ADD_SYMBOL( "OP_LIMIT_BOTTOM", SYMTYPE_NUM_I, PIX_DATA_OPCODE_LIMIT_BOTTOM );
    ADD_SYMBOL( "OP_ABS", SYMTYPE_NUM_I, PIX_DATA_OPCODE_ABS );
    ADD_SYMBOL( "OP_SUB2", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SUB2 );
    ADD_SYMBOL( "OP_COLOR_SUB2", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COLOR_SUB2 );
    ADD_SYMBOL( "OP_DIV2", SYMTYPE_NUM_I, PIX_DATA_OPCODE_DIV2 );
    ADD_SYMBOL( "OP_H_INTEGRAL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_H_INTEGRAL );
    ADD_SYMBOL( "OP_V_INTEGRAL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_V_INTEGRAL );
    ADD_SYMBOL( "OP_H_DERIVATIVE", SYMTYPE_NUM_I, PIX_DATA_OPCODE_H_DERIVATIVE );
    ADD_SYMBOL( "OP_V_DERIVATIVE", SYMTYPE_NUM_I, PIX_DATA_OPCODE_V_DERIVATIVE );
    ADD_SYMBOL( "OP_H_FLIP", SYMTYPE_NUM_I, PIX_DATA_OPCODE_H_FLIP );
    ADD_SYMBOL( "OP_V_FLIP", SYMTYPE_NUM_I, PIX_DATA_OPCODE_V_FLIP );
    //Data processing operations (op_cn, op_cc):
    ADD_SYMBOL( "OP_ADD", SYMTYPE_NUM_I, PIX_DATA_OPCODE_ADD );
    ADD_SYMBOL( "OP_SADD", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SADD );
    ADD_SYMBOL( "OP_COLOR_ADD", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COLOR_ADD );
    ADD_SYMBOL( "OP_SUB", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SUB );
    ADD_SYMBOL( "OP_SSUB", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SSUB );
    ADD_SYMBOL( "OP_COLOR_SUB", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COLOR_SUB );
    ADD_SYMBOL( "OP_MUL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MUL );
    ADD_SYMBOL( "OP_SMUL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SMUL );
    ADD_SYMBOL( "OP_MUL_RSHIFT15", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MUL_RSHIFT15 );
    ADD_SYMBOL( "OP_COLOR_MUL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COLOR_MUL );
    ADD_SYMBOL( "OP_DIV", SYMTYPE_NUM_I, PIX_DATA_OPCODE_DIV );
    ADD_SYMBOL( "OP_COLOR_DIV", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COLOR_DIV );
    ADD_SYMBOL( "OP_AND", SYMTYPE_NUM_I, PIX_DATA_OPCODE_AND );
    ADD_SYMBOL( "OP_OR", SYMTYPE_NUM_I, PIX_DATA_OPCODE_OR );
    ADD_SYMBOL( "OP_XOR", SYMTYPE_NUM_I, PIX_DATA_OPCODE_XOR );
    ADD_SYMBOL( "OP_LSHIFT", SYMTYPE_NUM_I, PIX_DATA_OPCODE_LSHIFT );
    ADD_SYMBOL( "OP_RSHIFT", SYMTYPE_NUM_I, PIX_DATA_OPCODE_RSHIFT );
    ADD_SYMBOL( "OP_EQUAL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_EQUAL );
    ADD_SYMBOL( "OP_LESS", SYMTYPE_NUM_I, PIX_DATA_OPCODE_LESS );
    ADD_SYMBOL( "OP_GREATER", SYMTYPE_NUM_I, PIX_DATA_OPCODE_GREATER );
    ADD_SYMBOL( "OP_COPY", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COPY );
    ADD_SYMBOL( "OP_COPY_LESS", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COPY_LESS );
    ADD_SYMBOL( "OP_COPY_GREATER", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COPY_GREATER );
    //Data processing operations (op_cc):
    ADD_SYMBOL( "OP_BMUL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_BMUL );
    ADD_SYMBOL( "OP_EXCHANGE", SYMTYPE_NUM_I, PIX_DATA_OPCODE_EXCHANGE );
    ADD_SYMBOL( "OP_COMPARE", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COMPARE )
    //Data processing operations (op_ccn):
    ADD_SYMBOL( "OP_MUL_DIV", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MUL_DIV );
    ADD_SYMBOL( "OP_MUL_RSHIFT", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MUL_RSHIFT );
    //Data processing operations (generator):
    ADD_SYMBOL( "OP_SIN", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SIN );
    ADD_SYMBOL( "OP_SIN8", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SIN8 );
    ADD_SYMBOL( "OP_RAND", SYMTYPE_NUM_I, PIX_DATA_OPCODE_RAND );
    //Sampler:
    ADD_SYMBOL( "SMP_DEST", SYMTYPE_NUM_I, PIX_SAMPLER_DEST );
    ADD_SYMBOL( "SMP_DEST_OFF", SYMTYPE_NUM_I, PIX_SAMPLER_DEST_OFF );
    ADD_SYMBOL( "SMP_DEST_LEN", SYMTYPE_NUM_I, PIX_SAMPLER_DEST_LEN );
    ADD_SYMBOL( "SMP_SRC", SYMTYPE_NUM_I, PIX_SAMPLER_SRC );
    ADD_SYMBOL( "SMP_SRC_OFF_H", SYMTYPE_NUM_I, PIX_SAMPLER_SRC_OFF_H );
    ADD_SYMBOL( "SMP_SRC_OFF_L", SYMTYPE_NUM_I, PIX_SAMPLER_SRC_OFF_L );
    ADD_SYMBOL( "SMP_SRC_SIZE", SYMTYPE_NUM_I, PIX_SAMPLER_SRC_SIZE );
    ADD_SYMBOL( "SMP_LOOP", SYMTYPE_NUM_I, PIX_SAMPLER_LOOP );
    ADD_SYMBOL( "SMP_LOOP_LEN", SYMTYPE_NUM_I, PIX_SAMPLER_LOOP_LEN );
    ADD_SYMBOL( "SMP_VOL1", SYMTYPE_NUM_I, PIX_SAMPLER_VOL1 );
    ADD_SYMBOL( "SMP_VOL2", SYMTYPE_NUM_I, PIX_SAMPLER_VOL2 );
    ADD_SYMBOL( "SMP_DELTA", SYMTYPE_NUM_I, PIX_SAMPLER_DELTA );
    ADD_SYMBOL( "SMP_FLAGS", SYMTYPE_NUM_I, PIX_SAMPLER_FLAGS );
    ADD_SYMBOL( "SMP_INFO_SIZE", SYMTYPE_NUM_I, PIX_SAMPLER_PARAMETERS );
    ADD_SYMBOL( "SMP_FLAG_INTERP2", SYMTYPE_NUM_I, PIX_SAMPLER_FLAG_INTERP2 );
    ADD_SYMBOL( "SMP_FLAG_INTERP4", SYMTYPE_NUM_I, PIX_SAMPLER_FLAG_INTERP4 );
    ADD_SYMBOL( "SMP_FLAG_PINGPONG", SYMTYPE_NUM_I, PIX_SAMPLER_FLAG_PINGPONG );
    ADD_SYMBOL( "SMP_FLAG_REVERSE", SYMTYPE_NUM_I, PIX_SAMPLER_FLAG_REVERSE );
    //Native code:
    ADD_SYMBOL( "CCONV_DEFAULT", SYMTYPE_NUM_I, PIX_CCONV_DEFAULT );
    ADD_SYMBOL( "CCONV_CDECL", SYMTYPE_NUM_I, PIX_CCONV_CDECL );
    ADD_SYMBOL( "CCONV_STDCALL", SYMTYPE_NUM_I, PIX_CCONV_STDCALL );
    ADD_SYMBOL( "CCONV_UNIX_AMD64", SYMTYPE_NUM_I, PIX_CCONV_UNIX_AMD64 );
    ADD_SYMBOL( "CCONV_WIN64", SYMTYPE_NUM_I, PIX_CCONV_WIN64 );
    //Another constants:
    ADD_SYMBOL( "INT_SIZE", SYMTYPE_NUM_I, sizeof( PIX_INT ) );
    ADD_SYMBOL( "INT_MAX", SYMTYPE_NUM_I, PIX_INT_MAX_POSITIVE );
    ADD_SYMBOL( "FLOAT_SIZE", SYMTYPE_NUM_I, sizeof( PIX_FLOAT ) );
    ADD_SYMBOL( "COLORBITS", SYMTYPE_NUM_I, COLORBITS );
    ADD_SYMBOL( "INT8", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT8 );
    ADD_SYMBOL( "INT16", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT16 );
    ADD_SYMBOL( "INT32", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT32 );
    ADD_SYMBOL( "INT64", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT64 );
    ADD_SYMBOL( "FLOAT32", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_FLOAT32 );
    ADD_SYMBOL( "FLOAT64", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_FLOAT64 );
    if( sizeof( COLOR ) == 1 ) ADD_SYMBOL( "PIXEL", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT8 );
    if( sizeof( COLOR ) == 2 ) ADD_SYMBOL( "PIXEL", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT16 );
    if( sizeof( COLOR ) == 4 ) ADD_SYMBOL( "PIXEL", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT32 );
    if( sizeof( COLOR ) == 8 ) ADD_SYMBOL( "PIXEL", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT64 );
    if( sizeof( PIX_INT ) == 4 ) ADD_SYMBOL( "INT", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT32 );
    if( sizeof( PIX_INT ) == 8 ) ADD_SYMBOL( "INT", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT64 );
    if( sizeof( PIX_FLOAT ) == 4 ) ADD_SYMBOL( "FLOAT", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_FLOAT32 );
    if( sizeof( PIX_FLOAT ) == 8 ) ADD_SYMBOL( "FLOAT", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_FLOAT64 );
    ADD_SYMBOL( "PIXILANG_VERSION", SYMTYPE_NUM_I, PIXILANG_VERSION );
    ADD_SYMBOL( "OS_NAME", SYMTYPE_NUM_I, vm->os_name );
    ADD_SYMBOL( "ARCH_NAME", SYMTYPE_NUM_I, vm->arch_name );
    ADD_SYMBOL( "LANG_NAME", SYMTYPE_NUM_I, vm->lang_name );
    ADD_SYMBOL( "CURRENT_PATH", SYMTYPE_NUM_I, vm->current_path );
    ADD_SYMBOL( "USER_PATH", SYMTYPE_NUM_I, vm->user_path );
    ADD_SYMBOL( "TEMP_PATH", SYMTYPE_NUM_I, vm->temp_path );
#ifdef OPENGL
    ADD_SYMBOL( "OPENGL", SYMTYPE_NUM_I, 1 );
#else
    ADD_SYMBOL( "OPENGL", SYMTYPE_NUM_I, 0 );
#endif
    ADD_SYMBOL( "GL_SCREEN", SYMTYPE_NUM_I, PIX_GL_SCREEN );
    ADD_SYMBOL( "GL_ZBUF", SYMTYPE_NUM_I, PIX_GL_ZBUF );
    
    DPRINT( "Adding global variables...\n" );
    ADD_SYMBOL( "WINDOW_XSIZE", SYMTYPE_GVAR, PIX_GVAR_WINDOW_XSIZE );
    ADD_SYMBOL( "WINDOW_YSIZE", SYMTYPE_GVAR, PIX_GVAR_WINDOW_YSIZE );
    ADD_SYMBOL( "FPS", SYMTYPE_GVAR, PIX_GVAR_FPS );
    ADD_SYMBOL( "PPI", SYMTYPE_GVAR, PIX_GVAR_PPI );
    ADD_SYMBOL( "UI_SCALE", SYMTYPE_GVAR, PIX_GVAR_SCALE );
    ADD_SYMBOL( "UI_FONT_SCALE", SYMTYPE_GVAR, PIX_GVAR_FONT_SCALE );

    DPRINT( "Compilation: lexical tree generation...\n" );
    g_comp->root = node( lnode_statlist, 0 );
    if( g_comp->root == 0 )
    {
	rv = 7;
	goto compiler_end;
    }    
    if( yyparse() )
    {
	rv = 8;
	goto compiler_end;
    }
    //if( yyss ) { free( yyss ); yyss = 0; }
    //if( yyvs ) { free( yyvs ); yyvs = 0; }
    //yystacksize = 0;
    //Close last local symbol table:
    g_comp->root = remove_lsym_table( g_comp->root );
    if( g_comp->root == 0 )
    {
	rv = 9;
	goto compiler_end;
    }
    DPRINT( "Compilation: optimization...\n" );
    optimize_tree( g_comp->root );
    DPRINT( "Compilation: code generation...\n" );
    DPRINT( "0: HALT\n" );
    pix_vm_put_opcode( OPCODE_HALT, vm );
    vm->halt_addr = 0;
    compile_tree( g_comp->root );
    fix_up();
    remove_tree( g_comp->root );
    DPRINT( "%d: RET_i ( 0 << OB )\n", (int)g_comp->vm->code_ptr );
    pix_vm_put_opcode( OPCODE_RET_i, vm );

    if( 0 )
    {
	pix_vm_code_analyzer( PIX_CODE_ANALYZER_SHOW_STATS, vm );
    }
    
    for( size_t i = 0; i < g_comp->vm->vars_num; i++ )
    {
	uint flags = g_comp->var_flags[ i ];
	if( flags & VAR_FLAG_USED )
	{
	    if( ( g_comp->var_flags[ i ] & VAR_FLAG_INITIALIZED ) == 0 )
	    {
	        blog( "Variable %s is not initialized. Default value = 0.\n", pix_vm_get_variable_name( g_comp->vm, i ) );
	    }
	}
    }

    DPRINT( "Deinit...\n" );
    pix_symtab_deinit( &g_comp->sym );
    bmem_free( g_comp->var_flags );
    bmem_free( g_comp->lsym );
    bmem_free( g_comp->inc );
    bmem_free( g_comp->while_stack );
    bmem_free( g_comp->fixup );
    bmem_free( g_comp );
    g_comp = 0;

compiler_end:
	
    bmutex_unlock( &pd->compiler_mutex );
        
compiler_end2:

    ticks_t end_time = time_ticks();
    
    DPRINT( "Pixilang compiler finished. %d ms.\n", ( ( end_time - start_time ) * 1000 ) / time_ticks_per_second() );
    
    if( rv )
    {
	ERROR( "%d", rv );
    }

    return rv;
}

#ifdef PIX_ENCODED_SOURCE
extern void pix_decode_source( void* src, size_t size );
#endif

int pix_compile( const utf8_char* name, pix_vm* vm, pix_data* pd )
{
    int rv = 0;
    
    utf8_char* src = 0;
    utf8_char* base_path = 0;
    
    size_t fsize = bfs_get_file_size( name );
    if( fsize >= 8 )
    {
	bfs_file f = bfs_open( name, "rb" );
	if( f )
	{
	    char sign[ 9 ];
	    sign[ 8 ] = 0;
	    bfs_read( &sign, 1, 8, f );
	    bfs_close( f );
	    if( bmem_strcmp( (const char*)sign, "PIXICODE" ) == 0 )
	    {
		//Binary code:
		base_path = pix_get_base_path( name );
		pix_vm_load_code( name, base_path, vm );
		goto pix_compile_end;
	    }
	}
    }
    if( fsize )
    {
	src = (utf8_char*)bmem_new( fsize );
	if( src == 0 ) 
	{
	    rv = 1;
	    goto pix_compile_end;
	}
	bfs_file f = bfs_open( name, "rb" );
	if( f == 0 )
	{
	    rv = 2;
	    ERROR( "can't open file %s", name );
	    goto pix_compile_end;
	}
	if( fsize >= 3 )
	{
	    bfs_read( src, 1, 3, f );
	    if( (uchar)src[ 0 ] == 0xEF && (uchar)src[ 1 ] == 0xBB && (uchar)src[ 2 ] == 0xBF )
	    {
		//Byte order mark found. Just ignore it:
		fsize -= 3;
	    }
	    else
	    {
		bfs_rewind( f );
	    }
	}
	bfs_read( src, 1, fsize, f );	
	bfs_close( f );
	base_path = pix_get_base_path( name );
#ifdef PIX_ENCODED_SOURCE
	pix_decode_source( src, fsize );
#endif
	if( pix_compile_from_memory( src, fsize, (utf8_char*)name, base_path, vm, pd ) )
	{
	    rv = 3;
	    goto pix_compile_end;
	}
    }
    else 
    {
	ERROR( "%s not found (or it's empty)", name );
	rv = 4;
    }
    
pix_compile_end:
    
    bmem_free( src );
    bmem_free( base_path );
    
    if( rv )
    {
	ERROR( "%d", rv );
    }
    
    return rv;
}
