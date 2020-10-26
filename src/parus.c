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


// HELPERS
// ----------------------------------------------------------------------------------------------------

static char* copy_string(char* s) {
	int size = 0;
	while (s[size] != '\0')
		++size;
	char* ns = calloc(size, sizeof(char));
	for (int i = 0; i < size; i++)
		ns[i] = s[i];

	return ns;
}

static char is_usermacro(char* s) {
	return s[0] == LP_CHAR && (s[1] == '\0' || isspace(s[1]));
}

static char is_termination(char* s) {
	return s[0] == RP_CHAR && (s[1] == '\0' || isspace(s[1]));
}

static char is_integer(char* s) {
	if (s == NULL || *s == '\0' || isspace(*s))
		return 0;
	char* p;
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

static char is_force(char* s) {
	return s[0] == FORCE_CHAR && (s[1] == '\0' || isspace(s[1]));
}

static char is_quoted(char* s) {
	return s[0] == QUOTE_CHAR;
}

static ParusData* quotate_symbol(char* expr) {
	if (expr[1] != QUOTE_CHAR)
		return new_parusdata_symbol(expr +1);
	else
		return new_parusdata_quote(quotate_symbol(expr +1));
}

static char is_symbol(char* s) {
	int 	i 		= 0;
	char 	valid 	= 0;

	return !is_termination(s)
		&& !is_usermacro(s)
		&& !is_integer(s) 
		&& !is_decimal(s) 
		&& !is_force(s) 
		&& !is_quoted(s);
}

static int quote_count(char* s) {
	int i = 0;
	while (s[i] == QUOTE_CHAR && s[i] != '\0')
		i++;
	return i;
}

static void clear_buffer(char* buffer, int size) {
	for (int i = 0; i < size || buffer[i] == '\0'; i++) 
        buffer[i] = '\0';
}


/* inserts an instruction to a mcr */
static void insert_instruction(ParusData* mcr, ParusData* instr) {
	if (mcr->type != USER_MACRO) {
		fprintf(stderr, "CANNOT INSERT INSTRUCTION FOR A NON MACRO\n");
		return;
	}
	
	if (mcr->data.usermacro.size < mcr->data.usermacro.max -1)
			mcr->data.usermacro.instructions[mcr->data.usermacro.size++] = instr;
	
	else {
		mcr->data.usermacro.instructions = realloc(mcr->data.usermacro.instructions,
					(mcr->data.usermacro.max + USER_MACRO_INSTR_GROWTH) * sizeof(ParusData));

		if (mcr->data.usermacro.instructions != 0) {
			mcr->data.usermacro.max += USER_MACRO_INSTR_GROWTH;
			mcr->data.usermacro.instructions[mcr->data.usermacro.size++] = instr;
		}
	}
}

static ParusData* init_macro() {
	ParusData* 	mcr 	= calloc(1, sizeof(ParusData));

	if (mcr != NULL) {
		mcr->data.usermacro.instructions 	= calloc(USER_MACRO_INSTR_GROWTH, sizeof(ParusData));
		mcr->data.usermacro.max 			= USER_MACRO_INSTR_GROWTH;
		mcr->data.usermacro.size 			= 0;
		mcr->type 							= USER_MACRO;
	}

	return mcr;
}

/*
make a new parusdata as a usermacro
A usermacro is represented by a list of ParusData that are evaluated sequentially

expr is the textual representation of the macro with the openning emitted sense it is unneeded 
Example: DPL * )
expr must be mutable
*/
static ParusData* make_usermacro(char* expr) {
	Stack* 	mcrstk 		= new_stack(); // a stack to store external macros
	char* 	token 		= strtok(expr, " ");
	char 	terminated 	= 0;


	if (mcrstk == NULL) {
		fprintf(stderr, "CANNOT ALLOCATE MACRO\n");
		return NULL;
	}
	
	stack_push(mcrstk, init_macro());

	while (token != NULL) {
		
		if (mcrstk->size == 0)
			break;

		ParusData* mcr = mcrstk->items[mcrstk->size -1];

		if (is_usermacro(token))
			stack_push(mcrstk, init_macro());
			
		else if (is_termination(token)) {
			if (mcrstk->size > 1) {
				ParusData* internal = stack_pull(mcrstk);
				ParusData* external = stack_pull(mcrstk);

				insert_instruction(external, internal);

				stack_push(mcrstk, external);
			}
			else {
				terminated = 1;
				break;
			}

		}


		else if (is_integer(token))
			insert_instruction(mcr, new_parusdata_integer(atoi(token)));

		else if (is_decimal(token))
			insert_instruction(mcr, new_parusdata_decimal(atof(token)));

		else if (is_force(token)) { // force form is implementated as a symbol
			char force_sym[2] = {FORCE_CHAR, '\0'};
			insert_instruction(mcr, new_parusdata_symbol(force_sym));
		}

		else if (is_quoted(token) && is_symbol(token + quote_count(token)))
			insert_instruction(mcr, new_parusdata_quote(quotate_symbol(token)));

		else if (is_symbol(token))
			insert_instruction(mcr, new_parusdata_symbol(token));

		else {
			fprintf(stderr, "INVALID TOKEN IN USER DEFINED MACRO - %s\n", expr);
			free_stack(mcrstk);
			return NULL;
		}

		token = strtok(NULL, " ");
	}
		
	if (!terminated) {
		fprintf(stderr, "UN-TERMINATED MACRO\n");
		free_stack(mcrstk);
		return NULL;
	}


	ParusData* pd = stack_pull(mcrstk);

	free_stack(mcrstk);

	return pd;
}

/* 

Evaluates a single expression at a time, not including comments

pass only mutable strings to eval as this function mutates the string

returns 0 only if parsed correctly
NOT IF OPERATION RAN CORRECTLY
*/
static int eval(char* expr, Stack* stk, Lexicon* lex) {
	/* pass */
	if (isspace(expr[0]) || expr[0] == '\0')
		return 0;

	/* validate syntax */
	if (is_termination(expr)) {
		fprintf(stderr, "EXPECTED MACRO\n");
		return 1;
	}

	/* self evaluating forms */
	if (is_usermacro(expr)) {
		ParusData* mcr = new_parusdata_usermacro(expr);
		if (mcr == NULL)
			return 0;
		stack_push(stk, mcr);

	}

	else if (is_integer(expr)) 
		stack_push(stk, new_parusdata_integer(atoi(expr)));

	else if (is_decimal(expr))
		stack_push(stk, new_parusdata_decimal(atof(expr)));

	/* quoted forms */
	else if (is_quoted(expr)) {
		if (is_symbol(expr + quote_count(expr)))
			stack_push(stk, quotate_symbol(expr));

		else {
			fprintf(stderr, "INVALID QUOTATION FORM - %s\n", expr);
			return 1;
		}
	}

	/* force form (that is apply the top of the stack) */
	else if (is_force(expr)) 
		parus_apply(stack_pull(stk), stk, lex);


	/* apply for given symbol */
	else if (is_symbol(expr)) {
		ParusData* pd = lexicon_get(lex, expr);
		parus_apply(pd, stk, lex);
	}

	else {
		fprintf(stderr, "SYNTAX ERROR - %s\n", expr);
		return 1;

	} 

	return 0;
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
		return new_parusdata_symbol(parusdata_getsymbol(original));

	else if (original->type == QUOTED)
		return new_parusdata_quote(parusdata_copy(parusdata_unquote(original)));

	else if (original->type == PRIMITIVE_MACRO)
		return new_parusdata_primitive(original->data.primitive);

	else if (original->type == USER_MACRO) {
		ParusData* mcr 	= calloc(1, sizeof(ParusData));
		mcr->type 		= USER_MACRO;

		mcr->data.usermacro.size 		= original->data.usermacro.size;
		mcr->data.usermacro.instructions = calloc(mcr->data.usermacro.size, sizeof(ParusData));

		for (int i = 0; i < mcr->data.usermacro.size; i++)
			mcr->data.usermacro.instructions[i] = parusdata_copy(original->data.usermacro.instructions[i]);

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
		pd->data.symbol 	= copy_string(s);
		pd->type 			= SYMBOL;
	}
	return pd;
}

/* returns a symbol */
char* parusdata_getsymbol(ParusData* pd) {
	return pd->data.symbol;
}

/* returns a new quoted value */
ParusData* new_parusdata_quote(ParusData* quoted) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.quoted 	= quoted;
		pd->type 			= QUOTED;
	}
	return pd;
}

/* returns the quoted ParusData* */
ParusData* parusdata_unquote(ParusData* pd) {
	return pd->data.quoted;
}

/* makes a new parusdata as a primitive macro */ 
ParusData* new_parusdata_primitive(primitive_t p) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.primitive 	= p;
		pd->type 			= PRIMITIVE_MACRO;
	}
	return pd;
}

/* returns a new user defined macro */
ParusData* new_parusdata_usermacro(char* expr) {
	char* 		nexpr 	= copy_string(expr);
	ParusData* 	mcr 	= make_usermacro(nexpr + 2);
	free(nexpr);
	return mcr;
}

/* free the parusdata */
void free_parusdata(ParusData* pd) {
	if (pd != NULL && pd->type != NONE) {
		if (pd->type == SYMBOL)
			free(pd->data.symbol);

		else if (pd->type == QUOTED)
			free_parusdata((ParusData*)pd->data.quoted);

		else if (pd->type == USER_MACRO) {
			for (int i = 0; i < pd->data.usermacro.size; i++) 
				free_parusdata(pd->data.usermacro.instructions[i]);

			free(pd->data.usermacro.instructions);
		}	

		free(pd);
		pd->type = NONE; // mark as cleaned
	}
}

void print_parusdata(ParusData* pd) {
	if (pd->type == INTEGER) 
		printf("%ld", parusdata_tointeger(pd));

	else if (pd->type == DECIMAL)
		printf("%f", parusdata_todecimal(pd));

	else if (pd->type == SYMBOL)
		printf("%s", parusdata_getsymbol(pd));

	else if (pd->type == QUOTED) {
		printf("%c", QUOTE_CHAR);
		print_parusdata(parusdata_unquote(pd));
	}

	else
		printf("parusdata@%x", pd);
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
		stk->items = realloc(stk->items, (stk->max + STACK_GROWTH) * sizeof(ParusData*));
		if (stk->items != 0) {
			stk->max += STACK_GROWTH;
			stk->items[stk->size++] = pd;
		}
		else
			fprintf(stderr, "STACK OVERFLOW\n");
	}
}

/*
pulls an item from the stack
the item needs to be freed after usage
*/
ParusData* stack_pull(Stack* stk) {
	if (stk->size > 0)
		return stk->items[--stk->size];
	else {
		fprintf(stderr, "STACK UNDERFLOW\n");
		return NULL;
	}
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
void print_stack(Stack* stk) {
	for (int i = 0; i < stk->size; i++) {
		print_parusdata(stk->items[i]);
		printf(", ");

	}
	printf("\n");
}

// LEXICON
// ----------------------------------------------------------------------------------------------------

/* makes a new lexicon */
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
		lex->entries = realloc(lex->entries, (lex->max + LEXICON_GROWTH) * sizeof(struct entry));
		if (lex->entries != 0) {
			lex->max += LEXICON_GROWTH;
			lex->entries[lex->size++] = ent;
		}
		else
			fprintf(stderr, "LEXICON OVERFLOW\n");
	}
}

/* deletes an entry from the lexicon */
void lexicon_delete(Lexicon* lex, char* name) {
	int index = 0;
	for (int i = lex->size -1; i >= 0; i--) 
		if (strcmp(lex->entries[i].name, name) == 0) {
			index = i;

			free_parusdata(lex->entries[index].value);
			free(lex->entries[index].name);

			for (int i = index; i < lex->size; i++)
				lex->entries[i] = lex->entries[i +1];

			lex->size--;
			
			return;
		}
	
	fprintf(stderr, "CANNOT DELETE AN UNDEFINED ENTRY - %s\n", name);
}

/* gets a copy of an entry */
ParusData* lexicon_get(Lexicon* lex, char* name) {
	for (int i = lex->size -1; i >= 0; i--) 
		if (strcmp(lex->entries[i].name, name) == 0)
			return parusdata_copy(lex->entries[i].value);

	fprintf(stderr, "UNDEFINED ENTRY - %s\n", name);
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
void print_lexicon(Lexicon* lex) {
	for (int i = 0; i < lex->size; i++) {
		printf("%s : ", lex->entries[i].name);
		print_parusdata(lex->entries[i].value);
		printf("\n");
	}
}

// CPARUS FUNCTIONS
// ----------------------------------------------------------------------------------------------------

/* 
returns if the evaluator can call the expression by returning the parentheses count

if result > 0 unterminated expression
if result = 0 valid expression
if result < 0 overterminated expression
*/
int valid_parus_expression(char* str) {
	int result = 0;
	int i = 0;
	while (str[i] != '\0') {
		if (is_usermacro(str +i)) 
			result++;

		else if (is_termination(str +i))
			result--;
		i++;

	}   
	return result;
}

/*
applies a parusdata 

the function will automatically free pd if needed

goto is used to optimize the last call in the macro

*/
int parus_apply(ParusData* pd, Stack* stk, Lexicon* lex) {
	static int call_depth = 0; // stores the call history

	if (call_depth > MAXIMUM_CALL_DEPTH) {
		fprintf(stderr, "INSUFFICIENT DATA FOR MEANINGFUL ANSWER\n");
		return 1;
	}

	recall:

	if (pd == NULL || pd->type == NONE)
		return 0;

	if (pd->type == INTEGER || pd->type == DECIMAL) 
		stack_push(stk, pd);
	
	else if (pd->type == SYMBOL) {
		ParusData* binding = lexicon_get(lex, parusdata_getsymbol(pd));
		free_parusdata(pd);
		pd = binding;

		goto recall;
	}

	else if (pd->type == QUOTED) {
		ParusData* unquoted = parusdata_copy(parusdata_unquote(pd));
		stack_push(stk, unquoted);
		free_parusdata(pd);
	}

	else if (pd->type == PRIMITIVE_MACRO) {
		int result = (*pd->data.primitive)(stk, lex);
		if (result)
			fprintf(stderr, "ERROR\n");
		free_parusdata(pd);
	}

	else if (pd->type == USER_MACRO) {

		if (pd->data.usermacro.size == 0) {
			free_parusdata(pd);
			return 0;
		}


		// do all the instruction in the macro except the last instruction
		for (int i = 0; i < pd->data.usermacro.size -1; i++) {
			
			// if not self evaluating instruction than apply it
			ParusData* instr = pd->data.usermacro.instructions[i];
			if (instr->type == SYMBOL || instr->type == QUOTED) {

				call_depth++;
				int e = parus_apply(instr->type == SYMBOL && is_force(parusdata_getsymbol(instr)) ? 
								stack_pull(stk) : parusdata_copy(instr), 
								stk, lex);
				call_depth--;
				if (e) {
					free_parusdata(pd);
					return 1;
				}
			}

			else 
				stack_push(stk, parusdata_copy(instr));

		}

		ParusData* 	last 		= pd->data.usermacro.instructions[pd->data.usermacro.size -1];
		ParusData* 	next_pd		= NULL;

		char is_forcing = 0;

		if (last != NULL && last->type == SYMBOL && is_force(parusdata_getsymbol(last))) {
			next_pd = stack_pull(stk);
			is_forcing = 1;
		}
		else 
			next_pd = parusdata_copy(last);

		free_parusdata(pd);

		pd = next_pd;
		if (pd != NULL && (pd->type == SYMBOL || pd->type == QUOTED || is_forcing))
			goto recall;
		else
			stack_push(stk, pd);

	}

	return 0;

}


/* The Parus Evaluator */
void parus_evaluate(char* input, Stack* stk, Lexicon* lex) {

	int size = (strlen(input) +1);
	char* buffer = malloc(size * sizeof(char));
	clear_buffer(buffer, size);

	int i = 0;
	int j = 0;
	int c;

	while ((c = input[i++]) != '\0') {

		if (c == COMMENT_CHAR) { // comment skip to next line
			buffer[j] = ' ';
			while ((c = input[++i]) != '\n' && c != '\0');
		} 

		else if (isspace(c) && valid_parus_expression(buffer) <= 0) { // if balanced expression
			buffer[j] = '\0';

			int e = eval(buffer, stk, lex);

			clear_buffer(buffer, size);
			j = 0;
			if (e != 0)
				break;
		}
		else  {
			if (isspace(c)) 
				buffer[j] = ' ';
			else
				buffer[j] = c;
			j++;
		}
	}

	buffer[j] = '\0';
	eval(buffer, stk, lex);
	
	free(buffer);
}

