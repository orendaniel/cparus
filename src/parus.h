/*
CParus
Copyright (C) 2020  Oren Daniel

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PARUS_H
#define PARUS_H

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#define LP_CHAR 		'('
#define RP_CHAR 		')'
#define QUOTE_CHAR 		'\''
#define COMMENT_CHAR 	';'

#define STACK_GROWTH 		50
#define LEXICON_GROWTH 		50
#define USEROP_INSTR_GROWTH 10

#define MAXIMUM_CALL_DEPTH 50000

#define HELP_MESSAGE "\nParus - Postfixed Reprogrammable Stack language\n" \
	"Visit https://gitlab.com/oren_daniel/cparus for instructions and details.\n" \
	"The language manual can be found at: https://gitlab.com/oren_daniel/parus-manual.\n" \
	"Author's email: oren_daniel@protonmail.com\n\n" \
	"flags: -help -norepl -notitle file\n\n" 

#define TITLE_MESSAGE "CParus version 1.0\n" \
	"CParus is free software under the GPLv3 license.\n" \
	"Copyright (C) 2020  Oren Daniel\n" \
	"Type ?help for help, or in the command line enter 'parus -help.'\n\n"



typedef long 	integer_t;
typedef double 	decimal_t;

typedef int (*baseop_t)(void*, void*);

typedef struct {
	union {
		integer_t	integer;
		decimal_t 	decimal;
		char* 		symbol;
		void* 		quoted; // pointer to ParusData
		baseop_t	baseop;

		struct { 
			void** 	instructions; //array of ParusData*
			size_t 	max;
			size_t 	size;
		} userop;

	} data;
	enum {
		INTEGER,
		DECIMAL,
		SYMBOL,
		QUOTED,
		BASEOP,
		USEROP,
		NONE // marks the instance as freed
	} type;
} ParusData;


typedef struct {
	size_t 		max;
	size_t 		size;
	ParusData**	items;

} Stack;


struct entry {
	char*		name;
	ParusData* 	value;
};

typedef struct lexicon {
	struct entry* 	entries;
	size_t 			max;
	size_t 			size;

} Lexicon;

typedef ParusData* (*applier_t)(void*, void*);


ParusData* 		parusdata_copy(ParusData* original);
ParusData* 		make_parus_integer(integer_t i);
integer_t 		parusdata_tointeger(ParusData* pd);
ParusData* 		make_parus_decimal(decimal_t d);
decimal_t 		parusdata_todecimal(ParusData* pd);
ParusData* 		make_parus_symbol(char* s);
char* 			parusdata_getsymbol(ParusData* pd);
ParusData* 		make_parus_quote(ParusData* quoted);
ParusData* 		parusdata_unquote(ParusData* pd);
ParusData* 		make_parus_baseop(baseop_t op);
ParusData* 		make_parus_userop();
void 			free_parusdata(ParusData* pd);
void 			print_parusdata(ParusData* pd);

Stack* 		make_stack();
void 		stack_push(Stack* stk, ParusData* pd);
ParusData* 	stack_pull(Stack* stk);
ParusData* 	stack_get_at(Stack* stk, size_t index);
void 		stack_remove_at(Stack* stk, size_t index);
void 		free_stack(Stack* stk);
void 		print_stack(Stack* stk);

Lexicon* 	make_lexicon();
void 		lexicon_define(Lexicon* lex, char* name, ParusData* pd);
void 		lexicon_delete(Lexicon* lex, char* name);
ParusData* 	lexicon_get(Lexicon* lex, char* name);
void 		free_lexicon(Lexicon* lex);
void 		print_lexicon(Lexicon* lex);


void 	parus_insert_instr(ParusData* op, ParusData* instr);
int 	parus_parencount(char* str);
void 	parus_set_applier(baseop_t caller, applier_t applier);
int 	parus_apply(ParusData* pd, Stack* stk, Lexicon* lex);
void 	parus_evaluate(char* input, Stack* stk, Lexicon* lex);

#endif
