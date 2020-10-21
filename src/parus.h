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


#define STACK_GROWTH 	50
#define LEXICON_GROWTH 	50

#define USER_MACRO_INSTR_GROWTH 10

#define MAXIMUM_CALL_DEPTH 25000

#define HELP_MESSAGE "\nParus - Postfixed Reprogrammable Stack language\n" \
	"Visit https://gitlab.com/oren_daniel/cparus for instructions and details.\n" \
	"The language manual can be found at: https://gitlab.com/oren_daniel/parus-manual.\n" \
	"Author's email: oren_daniel@protonmail.com\n\n" \
	"flags: -help -norepl -notitle file\n\n" 

#define TITLE_MESSAGE "CParus version 0.75\n" \
	"CParus is free software under the GPLv3 license.\n" \
	"Copyright (C) 2020  Oren Daniel\n" \
	"Type ?help macro for help, or in the command line enter 'parus -help.'\n\n"


typedef long 	integer_t;
typedef double 	decimal_t;

typedef int (*primitive_t)(void*, void*);

typedef struct {
	union {
		integer_t	integer;
		decimal_t 	decimal;
		char* 		symbol;
		void* 		quoted; // pointer to ParusData
		primitive_t	primitive;

		struct { 
			void** 	instructions; //array of ParusData*
			size_t 	max;
			size_t 	size;
		} usermacro;

	} data;
	enum {
		INTEGER,
		DECIMAL,
		SYMBOL,
		QUOTED,
		PRIMITIVE_MACRO,
		USER_MACRO,
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

ParusData* 		parusdata_copy(ParusData* original);
ParusData* 		new_parusdata_integer(integer_t i);
integer_t 		parusdata_tointeger(ParusData* pd);
ParusData* 		new_parusdata_decimal(decimal_t d);
decimal_t 		parusdata_todecimal(ParusData* pd);
ParusData* 		new_parusdata_symbol(char* s);
char* 			parusdata_getsymbol(ParusData* pd);
ParusData* 		new_parusdata_quote(ParusData* quoted);
ParusData* 		parusdata_unquote(ParusData* pd);
ParusData* 		new_parusdata_primitive(primitive_t p);
ParusData* 		new_parusdata_usermacro(char* expr);
void 			free_parusdata(ParusData* pd);
void 			print_parusdata(ParusData* pd);

Stack* 		new_stack();
void 		stack_push(Stack* stk, ParusData* pd);
ParusData* 	stack_pull(Stack* stk);
ParusData* 	stack_get_at(Stack* stk, size_t index);
void 		stack_remove_at(Stack* stk, size_t index);
void 		free_stack(Stack* stk);
void 		stack_print(Stack* stk);

Lexicon* 	new_lexicon();
void 		lexicon_define(Lexicon* lex, char* name, ParusData* pd);
void 		lexicon_delete(Lexicon* lex, char* name);
ParusData* 	lexicon_get(Lexicon* lex, char* name);
void 		free_lexicon(Lexicon* lex);
void 		lexicon_print(Lexicon* lex);


void parus_evaluate(char* input, Stack* stk, Lexicon* lex);
#endif
