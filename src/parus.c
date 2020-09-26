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

#include "parus.h"


static void apply_compound(ParusData* mcr, Stack* stk, Lexicon* lex);
static void apply(ParusData* mcr, Stack* stk, Lexicon* lex);
// HELPERS
// ----------------------------------------------------------------------------------------------------
static char is_integer(char* s) {
    if (s == NULL || *s == '\0' || isspace(*s))
      return 0;
    char * p;
    strtol(s, &p, 10);
    return *p == '\0';
}

static char is_decimal(char* s) {
    if (s == NULL || *s == '\0' || isspace(*s)) 
      return 0;
    char * p;
    strtod(s, &p);
    return *p == '\0';
}

static char is_quoted(char* s) {
	return s[0] == '\'';
}

static int strcount(char* s) {
	int i = 0;
	while (!isspace(s[i]) && s[i] != '\0')
		i++;

	return i;
}

static char is_imperative(char* s) {
	return s[0] == '!' && strcount(s) == 1;
}

static char is_compound(char* s) {
	return s[0] == '(' && strcount(s) == 1;
}

static char is_termination(char* s) {
	return s[0] == ')' && strcount(s) == 1;
}

static char is_symbol(char* s) {
	int i = 0;
	char valid = 0;
	while (s[i] != '\0' && s[i] != '\n') {
		if (isspace(s[i]))
			return 0;
		if (s[i] != '\'')
			valid = 1;
		i += 1;
	}
	return valid;
}

static char* copy_string(char* s) {
	int size = 0;
	while (s[size] != '\0')
		++size;
	char* ns = calloc(size, sizeof(char));
	for (int i = 0; i < size; i++)
		ns[i] = s[i];

	return ns;
}

static void insert_instruction(ParusData* mcr, ParusData* instr) {
	if (mcr->type != COMPOUND_MACRO) {
		printf("CANNOT INSERT INSTRUCTION FOR A NON MACRO\n");
		return;
	}
	
	if (mcr->data.compound.size < mcr->data.compound.max -1)
			mcr->data.compound.instructions[mcr->data.compound.size++] = instr;
	
	else {
		mcr->data.compound.instructions = realloc(mcr->data.compound.instructions,
					(mcr->data.compound.max + COMPOUND_GROWTH) * sizeof(ParusData));

		if (mcr->data.compound.instructions != 0) {
			mcr->data.compound.max += COMPOUND_GROWTH;
			mcr->data.compound.instructions[mcr->data.compound.size++] = instr;
		}
	}
}

// PARUSDATA
// ----------------------------------------------------------------------------------------------------
/* Returns a new copy of ParusData* */
ParusData* parusdata_copy(ParusData* original) {
	if (original->type == INTEGER)
		return new_parusdata_integer(parusdata_tointeger(original));

	else if (original->type == DECIMAL)
		return new_parusdata_decimal(parusdata_todecimal(original));

	else if (original->type == SYMBOL)
		return new_parusdata_symbol(copy_string(parusdata_getsymbol(original)));

	else if (original->type == PRIMITIVE_MACRO)
		return new_parusdata_primitive(original->data.primitve);

	else if (original->type == COMPOUND_MACRO) {
		ParusData* mcr 	= calloc(1, sizeof(ParusData));
		mcr->type 		= COMPOUND_MACRO;

		mcr->data.compound.size 		= original->data.compound.size;
		mcr->data.compound.instructions = calloc(mcr->data.compound.size, sizeof(ParusData));

		for (int i = 0; i < mcr->data.compound.size; i++)
			// recursive call
			mcr->data.compound.instructions[i] = parusdata_copy(original->data.compound.instructions[i]);

		return mcr;
	}
	return NULL;
}

/* makes a new parusdata as an integer */ 
ParusData* new_parusdata_integer(integer_t i) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.integer 	= i;
		pd->type 			= INTEGER;
	}
	return pd;
}

/* returns an integer */
integer_t parusdata_tointeger(ParusData* pd) {
	return pd->data.integer;
}

/* makes a new parusdata as a decimal */ 
ParusData* new_parusdata_decimal(decimal_t d) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.decimal 	= d;
		pd->type 			= DECIMAL;
	}
	return pd;
}

/* returns a decimal */
decimal_t parusdata_todecimal(ParusData* pd) {
	return pd->data.decimal;
}

/* makes a new parusdata as a symbol */ 
ParusData* new_parusdata_symbol(char* s) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.symbol 	= s;
		pd->type 			= SYMBOL;
	}
	return pd;
}

/* returns a symbol */
char* parusdata_getsymbol(ParusData* pd) {
	return pd->data.symbol;
}

/* makes a new parusdata as a primitive macro */ 
ParusData* new_parusdata_primitive(primitve_t p) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.primitve 	= p;
		pd->type 			= PRIMITIVE_MACRO;
	}
	return pd;
}

/* make a new parusdata as compounded macro */
ParusData* new_parusdata_compound(char* expr) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.compound.instructions 	= calloc(COMPOUND_GROWTH, sizeof(ParusData));
		pd->data.compound.max 			= COMPOUND_GROWTH;
		pd->data.compound.size 			= 0;
		pd->type = COMPOUND_MACRO;

		static char* token;
		static char* rest;
		rest = expr;

		char terminated = 0;

		while ((token = strtok_r(rest, " ", &rest))) {

			if (is_compound(token)) {
				ParusData* submcr = new_parusdata_compound(rest);

				if (submcr == NULL) {
					free_parusdata(submcr);
					goto unterminated;
				}

				insert_instruction(pd, submcr);
				continue; // continue to next token

			}
				
			else if (is_termination(token)) {
				terminated = 1;
				break;
			}

			if (is_integer(token))
				insert_instruction(pd, new_parusdata_integer(atoi(token)));

			else if (is_decimal(token))
				insert_instruction(pd, new_parusdata_decimal(atof(token)));

			else if (is_symbol(token))
				insert_instruction(pd, new_parusdata_symbol(copy_string(token)));
		}
		
		if (!terminated) {
			printf("UN-TERMINATED MACRO\n");
			unterminated:
			free_parusdata(pd);
			return NULL;
		}
	}

	return pd;
}

/* free the parusdata */
void free_parusdata(ParusData* pd) {
	if (pd != NULL && pd->type != NONE) {
		if (pd->type == SYMBOL)
			free(pd->data.symbol);
		else if (pd->type == COMPOUND_MACRO) {
			for (int i = 0; i < pd->data.compound.size; i++) 
				// recursive call
				free_parusdata(pd->data.compound.instructions[i]);

			free(pd->data.compound.instructions);
		}	

		free(pd);
		pd->type = NONE; // mark as cleaned
	}
}

// STACK
// ----------------------------------------------------------------------------------------------------
/* 
makes a new parus stack 
EVERY ITEM SHOULD BE UNIQUE
*/
Stack* new_stack() {
	Stack* stk = calloc(1, sizeof(Stack*));

	stk->max    = STACK_GROWTH;
	stk->size   = 0;
	stk->items  = calloc(STACK_GROWTH, sizeof(ParusData));

	return stk;
}

/* pushes a new parusdata item to the stack */
void stack_push(Stack* stk, ParusData* pd) {
	if (stk->size != stk->max - 1) 
		stk->items[stk->size++] = pd;
	else {
		stk->items = realloc(stk->items, (stk->max + STACK_GROWTH) * sizeof(ParusData));
		if (stk->items != 0) {
			stk->max += STACK_GROWTH;
			stk->items[stk->size++] = pd;
		}
	}
}

/*
pulls an item from the stack
the item needs to be freed after usage
*/
ParusData* stack_pull(Stack* stk) {
	if (stk->size > 0)
		return stk->items[--stk->size];
	else
		return NULL;
}

/*
gets a copy from the stack
index is counted from the end of the stack
so 0 is the first item
*/
ParusData* stack_get_at(Stack* stk, size_t index) {
	if (index < stk->size)
		return parusdata_copy(stk->items[stk->size -(index +1)]);
	else
		return NULL;
}

/*
deletes item from the stack an cleans it
index is counted from the end of the stack
so 0 is the first item
*/
void stack_remove_at(Stack* stk, size_t index) {
	if (index < stk->size) {
		ParusData* pd = stk->items[stk->size -(index +1)];
		free_parusdata(pd);
		for (int i = stk->size -(index +1); i < stk->size -1; i++)
			stk->items[i] = stk->items[i +1];

		stk->size--;
	}
}

/* frees the stack */
void free_stack(Stack* stk) {
	for (int i = 0; i < stk->size; i++)
		free_parusdata(stk->items[i]);
	free(stk->items);
	free(stk);
}

/* prints the stack contant */
void stack_print(Stack* stk) {
	for (int i = 0; i < stk->size; i++) {
		if (stk->items[i]->type == INTEGER) 
			printf("%ld, ", parusdata_tointeger(stk->items[i]));
		
		else if (stk->items[i]->type == DECIMAL)
			printf("%f, ", parusdata_todecimal(stk->items[i]));

		else if (stk->items[i]->type == SYMBOL)
			printf("%s, ", parusdata_getsymbol(stk->items[i]));

		else
			printf("parusdata@%x, ", stk->items[i]->data);

	}
	printf("\n");
}

// LEXICON
// ----------------------------------------------------------------------------------------------------
/* 
makes a new lexicon 
outer = NULL for outmost lexicon
*/
Lexicon* new_lexicon() {
	Lexicon* lex = calloc(1, sizeof(Lexicon));

	lex->size = 0;
	lex->entries = calloc(LEXICON_GROWTH, sizeof(struct entry));
	lex->max = LEXICON_GROWTH;

	return lex;
}

/* define a new entry on the lexicon */
void lexicon_define(Lexicon* lex, char* name, ParusData* pd) {
	struct entry ent;
	ent.name 	= copy_string(name);
	ent.value 	= pd;
	if (lex->size != lex->max - 1)
		lex->entries[lex->size++] = ent;
	else {
		lex->entries = realloc(lex->entries, (lex->max + STACK_GROWTH) * sizeof(ParusData));
		if (lex->entries != 0) {
			lex->max += STACK_GROWTH;
			lex->entries[lex->size++] = ent;
		}
		else
			printf("LEXICON OVERFLOW\n");
	}
}

/* deletes an entry from the lexicon */
void lexicon_delete(Lexicon* lex, char* name) {
	int index = 0;
	for (int i = lex->size -1; i >= 0; i--) 
		if (strcmp(lex->entries[i].name, name) == 0) {
			index = i;

			free_parusdata(lex->entries[index].value);

			for (int i = index; i < lex->size; i++)
				lex->entries[i] = lex->entries[i +1];

			lex->size--;
			
			return;
		}
	
	printf("CANNOT DELETE AN UNDEFINED ENTRY - %s\n", name);
}

/* gets a copy of an entry */
ParusData* lexicon_get(Lexicon* lex, char* name) {
	for (int i = lex->size -1; i >= 0; i--) 
		if (strcmp(lex->entries[i].name, name) == 0)
			return parusdata_copy(lex->entries[i].value);

	printf("UNDEFINED ENTRY - %s\n", name);
	return NULL;
}

/* frees the lexicon */
void free_lexicon(Lexicon* lex) {
	for (int i = 0; i < lex->size; i++) {
		free_parusdata(lex->entries[i].value);
		free(lex->entries[i].name);
	}
	free(lex->entries);
	free(lex);
}

/* prints the lexicon contant */
void lexicon_print(Lexicon* lex) {
	for (int i = 0; i < lex->size; i++) {
		if (lex->entries[i].value->type == INTEGER)	
			printf("%s : %ld\n", lex->entries[i].name, parusdata_tointeger(lex->entries[i].value));

		else if (lex->entries[i].value->type == DECIMAL)	
			printf("%s : %f\n", lex->entries[i].name, parusdata_todecimal(lex->entries[i].value));

		else if (lex->entries[i].value->type == SYMBOL)	
			printf("%s : %s\n", lex->entries[i].name, parusdata_getsymbol(lex->entries[i].value));

		else
			printf("%s : parusdata@%x\n", lex->entries[i].name, lex->entries[i].value);
	}
}

// EVALUATOR
// ----------------------------------------------------------------------------------------------------

/* applies a parusdata */
static void apply(ParusData* pd, Stack* stk, Lexicon* lex) {
	if (pd == NULL) {
		printf("A\n");
		return;

	}

	if (pd->type == INTEGER || pd->type == DECIMAL) 
		stack_push(stk, pd);
	
	else if (pd->type == SYMBOL) {
		// recursive call
		parus_eval(parusdata_getsymbol(pd), stk, lex);
		free_parusdata(pd);
	}

	else if (pd->type == PRIMITIVE_MACRO) {
		int result = (*pd->data.primitve)(stk, lex);
		if (result)
			printf("ERROR\n");
		free_parusdata(pd);
	}

	else if (pd->type == COMPOUND_MACRO) 
		// mutual recursive call
		apply_compound(pd, stk, lex);
}

/* handles compound macros */
static void apply_compound(ParusData* mcr, Stack* stk, Lexicon* lex) {
	static int call_depth = 0;
	if (call_depth > MAXIMUM_CALL_DEPTH) {
		printf("INSUFFICIENT DATA FOR MEANINGFUL ANSWER\n");
		call_depth = 0;
		return;
	}

	tailcall:
	for (int i = 0; i < mcr->data.compound.size -1; i++) {
		ParusData* instr = mcr->data.compound.instructions[i];

		if (instr->type == INTEGER || instr->type == DECIMAL || instr->type == COMPOUND_MACRO)
			stack_push(stk, parusdata_copy(instr));

		else {
			call_depth++;
			char* sym_expr = copy_string(parusdata_getsymbol(instr));
			parus_eval(sym_expr, stk, lex);
			free(sym_expr);
		}
	}

	/*ParusData* last = mcr->data.compound.instructions[mcr->data.compound.size -1];

	if (last->type == INTEGER || last->type == DECIMAL || last->type == COMPOUND_MACRO)
		stack_push(stk, parusdata_copy(last));

	else {
		char* sym_expr = copy_string(parusdata_getsymbol(last));
		if (is_imperative(sym_expr)) {
			ParusData* top = stack_pull(stk);
			if (top->type == COMPOUND_MACRO) {
				free(sym_expr);
				free_parusdata(mcr);
				mcr = top;
				goto tailcall;
			}
			else {
				apply(top, stk, lex);

			}
		}


	}*/


	call_depth = 0;
	free_parusdata(mcr);
}

/* 
The Parus Evaluator

Evaluates a single expression at a time 
*/
void parus_eval(char* expr, Stack* stk, Lexicon* lex) {

	if (is_termination(expr)) {
		printf("EXPECTED MACRO\n");
		return;
	}

	/* self evaluating forms */
	else if (is_compound(expr)) {
		ParusData* mcr = new_parusdata_compound(expr + 2);
		if (mcr == NULL)
			return;
		stack_push(stk, mcr);

	}

	else if (is_integer(expr)) 
		stack_push(stk, new_parusdata_integer(atoi(expr)));

	else if (is_decimal(expr))
		stack_push(stk, new_parusdata_decimal(atof(expr)));

	/* imperative form ( that is apply according to the top of the stack ) */
	else if (is_imperative(expr)) {
		ParusData* top = stack_pull(stk);
		apply(top, stk, lex);
	}

	/* quoted forms */
	else if (is_quoted(expr)) {

		if (is_integer(expr +1))
			stack_push(stk, new_parusdata_integer(atoi(expr +1)));

		else if (is_decimal(expr +1))
			stack_push(stk, new_parusdata_decimal(atof(expr +1)));

		else if (is_symbol(expr +1))
			stack_push(stk, new_parusdata_symbol(copy_string(expr +1)));

		else 
			printf("INVALID QUOTATION FORM - %s\n", expr);
	}

	/* apply for given symbol */
	else {
		ParusData* pd = lexicon_get(lex, expr);
		apply(pd, stk, lex);
	}
}

/* evaluate a literal string */
void parus_literal_eval(const char* literal, Stack* stk, Lexicon* lex) {
	char* nexpr = copy_string((char*)literal);

	parus_eval(nexpr, stk, lex);
	free(nexpr);
}
