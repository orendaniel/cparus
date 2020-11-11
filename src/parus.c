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

// store the address of the base operator which called parus_apply
static baseop_t apply_caller;

// back door operation to implement base operators like apply top
static applier_t apply_shortcut;

// HELPERS
// ----------------------------------------------------------------------------------------------------
/* copies the string, and insertes spaces between parentheses */
static char* copy_string(char* s) {
	int size 	= 1;
	int len 	= 1;

	for (int i = 0; s[i] != '\0'; i++) {
		if (s[i] == LP_CHAR || s[i] == RP_CHAR)
			size += 2; // insert area for two spaces

		size++;
		len++;
	}

	char* ns = calloc(size, sizeof(char));

	for (int i = 0, j = 0; i < len; i++, j++) {
		if (s[i] == LP_CHAR || s[i] == RP_CHAR) {
			ns[j++] = ' ';
			ns[j++] = s[i];
			ns[j] = ' ';
		}
		else
			ns[j] = isspace(s[i]) ? ' ' : s[i];
	}

	return ns;
}

static char is_user_operator(char* s) {
	return s != NULL && s[0] == LP_CHAR;
}

static char is_termination(char* s) {
	return s != NULL && s[0] == RP_CHAR;
}

static char is_integer(char* s) {
	if (s == NULL || s[0] == '\0' || isspace(s[0]))
		return 0;
	char* p;
	strtol(s, &p, 10);
	return p[0] == '\0';
}

static char is_decimal(char* s) {
	if (s == NULL || s[0] == '\0' || isspace(s[0])) 
		return 0;
	char* p;
	strtod(s, &p);
	return p[0] == '\0';
}

static char is_quoted(char* s) {
	return s != NULL && s[0] == QUOTE_CHAR;
}

static char is_symbol(char* s) {
	return s != NULL
		&& !is_termination(s)
		&& !is_user_operator(s)
		&& !is_integer(s) 
		&& !is_decimal(s) 
		&& !is_quoted(s)
		&& s[0] != '\0';
}

static ParusData* quotate_symbol(char* expr) {
	if (expr[1] != QUOTE_CHAR)
		return make_parus_symbol(expr +1);
	else
		return make_parus_quote(quotate_symbol(expr +1));
}

static int quote_count(char* s) {
	int i = 0;
	while (s[i] == QUOTE_CHAR && s[i] != '\0') i++;
	return i;
}

/* inserts an instruction to a op */
static void insert_instruction(ParusData* op, ParusData* instr) {
	if (op->type != USEROP) {
		fprintf(stderr, "CANNOT INSERT INSTRUCTION FOR A NON OPERATOR\n");
		return;
	}
	
	if (op->data.userop.size < op->data.userop.max -1)
			op->data.userop.instructions[op->data.userop.size++] = instr;
	
	else {
		op->data.userop.instructions = realloc(op->data.userop.instructions,
					(op->data.userop.max + USEROP_INSTR_GROWTH) * sizeof(ParusData));

		if (op->data.userop.instructions != 0) {
			op->data.userop.max += USEROP_INSTR_GROWTH;
			op->data.userop.instructions[op->data.userop.size++] = instr;
		}
		else
			fprintf(stderr, "CANNOT INSERT INSTRUCTION\n");
	}
}

static ParusData* init_userop() {
	ParusData* op = calloc(1, sizeof(ParusData));

	if (op != NULL) {
		op->data.userop.instructions	= calloc(USEROP_INSTR_GROWTH, sizeof(ParusData));
		op->data.userop.max 			= USEROP_INSTR_GROWTH;
		op->data.userop.size 			= 0;
		op->type 						= USEROP;
	}

	return op;
}

/*
make a new parusdata as a user defined operator 
A user defined operator is represented by an array of ParusData that are evaluated sequentially

expr is the textual representation of the operator with the openning emitted sense it is unneeded
spaces between parentheses are required

Example: DPL * )

expr must be mutable
*/
static ParusData* make_userop(char* expr) {
	Stack* 	opstk 		= make_stack(); // a stack to store nested operators
	char* 	token 		= strtok(expr, " ");
	char 	terminated 	= 0;


	if (opstk == NULL) {
		fprintf(stderr, "CANNOT ALLOCATE OPERATOR\n");
		return NULL;
	}
	
	stack_push(opstk, init_userop());

	while (token != NULL) {
		
		if (opstk->size == 0)
			break;

		ParusData* op = opstk->items[opstk->size -1];

		if (is_user_operator(token))
			stack_push(opstk, init_userop());
			
		else if (is_termination(token)) {
			if (opstk->size > 1) {
				ParusData* internal = stack_pull(opstk);
				ParusData* external = stack_pull(opstk);

				insert_instruction(external, internal);

				stack_push(opstk, external);
			}
			else {
				terminated = 1;
				break;
			}
		}

		else if (is_integer(token))
			insert_instruction(op, make_parus_integer(atoi(token)));

		else if (is_decimal(token))
			insert_instruction(op, make_parus_decimal(atof(token)));

		else if (is_quoted(token) && is_symbol(token + quote_count(token)))
			insert_instruction(op, make_parus_quote(quotate_symbol(token)));

		else if (is_symbol(token))
			insert_instruction(op, make_parus_symbol(token));

		else {
			fprintf(stderr, "INVALID TOKEN IN USER DEFINED OPERATOR - %s\n", expr);
			free_stack(opstk);
			return NULL;
		}

		token = strtok(NULL, " ");
	}
		
	if (!terminated) {
		fprintf(stderr, "UN-TERMINATED OPERATOR\n");
		free_stack(opstk);
		return NULL;
	}

	ParusData* pd = stack_pull(opstk);
	free_stack(opstk);
	return pd;
}

/* 

Evaluates a single expression at a time, not including comments

pass only mutable strings to eval as this function mutates the string
spaces between parentheses are required

returns 0 only if parsed correctly
NOT IF EXPRESSION RAN CORRECTLY
*/
static int eval(char* expr, Stack* stk, Lexicon* lex) {
	/* skip spaces */
	while (isspace(expr[0])) expr++;

	/* pass */
	if (expr[0] == '\0')
		return 0;

	/* invalid syntax */
	if (is_termination(expr)) {
		fprintf(stderr, "EXPECTED OPERATOR\n");
		return 1;
	}

	/* self evaluating forms */
	if (is_user_operator(expr)) {
		ParusData* op = make_parus_userop(expr);
		if (op == NULL)
			return 0;
		stack_push(stk, op);

	}

	else if (is_integer(expr)) 
		stack_push(stk, make_parus_integer(atoi(expr)));

	else if (is_decimal(expr))
		stack_push(stk, make_parus_decimal(atof(expr)));

	/* quoted forms */
	else if (is_quoted(expr) && is_symbol(expr + quote_count(expr)))
		stack_push(stk, quotate_symbol(expr));

	/* apply for given symbol */
	else if (is_symbol(expr)) {
		ParusData* pd = lexicon_get(lex, expr);
		parus_apply(pd, stk, lex);
	}

	else {
		fprintf(stderr, "INVALID EXPRESSION - %s\n", expr);
		return 1;
	} 

	return 0;
}

// PARUSDATA
// ----------------------------------------------------------------------------------------------------

/* Returns a new copy of ParusData* */
ParusData* parusdata_copy(ParusData* original) {
	if (original->type == INTEGER)
		return make_parus_integer(parusdata_tointeger(original));

	else if (original->type == DECIMAL)
		return make_parus_decimal(parusdata_todecimal(original));

	else if (original->type == SYMBOL)
		return make_parus_symbol(parusdata_getsymbol(original));

	else if (original->type == QUOTED)
		return make_parus_quote(parusdata_copy(parusdata_unquote(original)));

	else if (original->type == BASEOP)
		return make_parus_baseop(original->data.baseop);

	else if (original->type == USEROP) {
		ParusData* op 	= calloc(1, sizeof(ParusData));
		op->type 		= USEROP;

		op->data.userop.size 			= original->data.userop.size;
		op->data.userop.instructions 	= calloc(op->data.userop.size, sizeof(ParusData));

		for (int i = 0; i < op->data.userop.size; i++)
			op->data.userop.instructions[i] = parusdata_copy(original->data.userop.instructions[i]);

		return op;
	}
	return NULL;
}

/* makes a new parusdata as an integer */ 
ParusData* make_parus_integer(integer_t i) {
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
ParusData* make_parus_decimal(decimal_t d) {
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
ParusData* make_parus_symbol(char* s) {
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
ParusData* make_parus_quote(ParusData* quoted) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.quoted = quoted;
		pd->type 		= QUOTED;
	}
	return pd;
}

/* returns the quoted ParusData* */
ParusData* parusdata_unquote(ParusData* pd) {
	return pd->data.quoted;
}

/* makes a new parusdata as a base operator */ 
ParusData* make_parus_baseop(baseop_t op) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.baseop 	= op;
		pd->type 			= BASEOP;
	}
	return pd;
}

/*
returns a new user defined operator
comments are not allowed, and only ' ' is a valid space
*/
ParusData* make_parus_userop(char* expr) {
	int 	offset 	= 0;
	char* 	nexpr 	= copy_string(expr);

	while (isspace(nexpr[offset])) offset++;

	ParusData* op = make_userop(nexpr + offset +1);
	free(nexpr);
	return op;
}

/* free the parusdata */
void free_parusdata(ParusData* pd) {
	if (pd != NULL && pd->type != NONE) {
		if (pd->type == SYMBOL)
			free(pd->data.symbol);

		else if (pd->type == QUOTED)
			free_parusdata((ParusData*)pd->data.quoted);

		else if (pd->type == USEROP) {
			for (int i = 0; i < pd->data.userop.size; i++) 
				free_parusdata(pd->data.userop.instructions[i]);

			free(pd->data.userop.instructions);
		}	

		free(pd);
		pd->type = NONE; // mark as cleaned
	}
}

/* prints parusdata */
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
Stack* make_stack() {
	Stack* stk 	= calloc(1, sizeof(Stack*));
	stk->size   = 0;
	stk->max    = STACK_GROWTH;
	stk->items  = calloc(stk->max, sizeof(ParusData));

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
Lexicon* make_lexicon() {
	Lexicon* lex 	= calloc(1, sizeof(Lexicon));
	lex->size 		= 0;
	lex->max 		= LEXICON_GROWTH;
	lex->entries 	= calloc(lex->max, sizeof(struct entry));

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
int parus_parencount(char* str) {
	int 	result  = 0;
	int 	i 		= 0;
	char 	ignore  = 0;
	char 	c;

	while ((c = str[i++]) != '\0') {
		if (c == COMMENT_CHAR)
			ignore = 1;

		else if (c == '\n')
			ignore = 0;

		if (!ignore && c == LP_CHAR)
			result++;

		else if (!ignore && c == RP_CHAR)
			result--;
	}   

	return result;
}

/*
sets apply_caller and apply_shortcut.
make sure to call parus_set_applier(NULL, NULL), 
after calling it in order to reset the values.
*/
void parus_set_applier(baseop_t caller, applier_t applier) {
	apply_caller 	= caller;
	apply_shortcut	= applier;
}

/*
applies a parusdata 

the function will automatically free pd if needed

goto is used to optimize the last call in the operator 
*/
int parus_apply(ParusData* pd, Stack* stk, Lexicon* lex) {
	static int call_depth = 0; // stores the call history

	if (call_depth > MAXIMUM_CALL_DEPTH) {
		fprintf(stderr, "INSUFFICIENT DATA FOR MEANINGFUL ANSWER\n");
		return 1;
	}

	// back door for base operators that use apply themselves
	if (pd == NULL && apply_shortcut != NULL && apply_caller != NULL)
		pd = apply_shortcut(stk, lex);

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

	else if (pd->type == BASEOP) {
		// dont allow mutual recursion between applier and parus_apply
		if (apply_caller != pd->data.baseop) {
			int result = (*pd->data.baseop)(stk, lex);
			if (result)
				fprintf(stderr, "ERROR\n");

			free_parusdata(pd);
		}
		else {
			if (apply_shortcut != NULL) {
				free_parusdata(pd);
				pd = (*apply_shortcut)(stk, lex);
				goto recall;
			}
		}
	}

	else if (pd->type == USEROP) {

		if (pd->data.userop.size == 0) {
			free_parusdata(pd);
			return 0;
		}


		// do all the instruction in the operator except the last instruction
		for (int i = 0; i < pd->data.userop.size -1; i++) {
			
			// if not self evaluating instruction then apply it
			ParusData* instr = pd->data.userop.instructions[i];
			if (instr->type == SYMBOL || instr->type == QUOTED) {

				call_depth++;
				int e = parus_apply(parusdata_copy(instr), stk, lex);
				call_depth--;

				if (e) {
					free_parusdata(pd);
					return 1;
				}
			}

			else 
				stack_push(stk, parusdata_copy(instr));

		}

		ParusData* 	last	= pd->data.userop.instructions[pd->data.userop.size -1];
		ParusData* 	next_pd	= parusdata_copy(last);

		free_parusdata(pd);
		pd = next_pd;

		if (pd != NULL && (pd->type == SYMBOL || pd->type == QUOTED))
			goto recall;
		else
			stack_push(stk, pd);

	}

	return 0;
}

/* The Parus Evaluator */
void parus_evaluate(char* input, Stack* stk, Lexicon* lex) {
	int 	size 	= (strlen(input) +1);
	int 	i 		= 0;
	int 	j 		= 0;
	char* 	buffer 	= calloc(size, sizeof(char));
	char 	c;

	while ((c = input[i++]) != '\0') {
		if (c == COMMENT_CHAR) { // comment skip to next line
			buffer[j++] = ' ';
			while ((c = input[++i]) != '\n' && c != '\0');
		}
		else
			buffer[j++] = isspace(c) ? ' ' : c;
		
		// if read a complete expression
		int pc = parus_parencount(buffer);
		if ((c == LP_CHAR && pc == 1) ||
			((isspace(c) || c == RP_CHAR) && pc <= 0)) {

			buffer[c == RP_CHAR ? j :j -1] = '\0';

			int e = eval(buffer, stk, lex);
			if (e != 0)
				break;

			j = 0;

			if (c == LP_CHAR) // return the removed LP to the buffer if needed
				buffer[j++] = LP_CHAR;
		}
	}

	buffer[j] = '\0';
	eval(buffer, stk, lex);
	free(buffer);

}
