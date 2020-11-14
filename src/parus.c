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

/* Copies the string, and insertes spaces between parentheses and quote and removes comments*/
static char* copy_string(char* s) {
	int size 	= 1;
	int len 	= 1;

	for (int i = 0; s[i] != '\0'; i++) {
		if (s[i] == LP_CHAR || s[i] == RP_CHAR || s[i] == QUOTE_CHAR)
			size += 2; // insert area for two spaces
		size++;
		len++;
	}

	char* ns = calloc(size, sizeof(char));

	for (int i = 0, j = 0; i < len; i++, j++) {
		if (s[i] == COMMENT_CHAR) {
			ns[j] = ' ';
			while (s[i] != '\n' && s[i] != '\0') i++;
		}

		if (s[i] == LP_CHAR || s[i] == RP_CHAR || s[i] == QUOTE_CHAR) {
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

// PARUSDATA
// ----------------------------------------------------------------------------------------------------

/* Returns a new copy of ParusData* */
ParusData* parusdata_copy(ParusData* original) {
	if (original == NULL) return NULL;

	else if (original->type == INTEGER)
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

/* Makes a new parusdata as an integer */ 
ParusData* make_parus_integer(integer_t i) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.integer 	= i;
		pd->type 			= INTEGER;
	}
	return pd;
}

/* Returns an integer */
integer_t parusdata_tointeger(ParusData* pd) {
	return pd->data.integer;
}

/* Makes a new parusdata as a decimal */ 
ParusData* make_parus_decimal(decimal_t d) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.decimal 	= d;
		pd->type 			= DECIMAL;
	}
	return pd;
}

/* Returns a decimal */
decimal_t parusdata_todecimal(ParusData* pd) {
	return pd->data.decimal;
}

/* Makes a new parusdata as a symbol */ 
ParusData* make_parus_symbol(char* s) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.symbol 	= copy_string(s);
		pd->type 			= SYMBOL;
	}
	return pd;
}

/* Returns a symbol */
char* parusdata_getsymbol(ParusData* pd) {
	return pd->data.symbol;
}

/* Returns a new quoted value */
ParusData* make_parus_quote(ParusData* quoted) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.quoted = quoted;
		pd->type 		= QUOTED;
	}
	return pd;
}

/* Returns the quoted ParusData* */
ParusData* parusdata_unquote(ParusData* pd) {
	return pd->data.quoted;
}

/* Makes a new parusdata as a base operator */ 
ParusData* make_parus_baseop(baseop_t op) {
	ParusData* pd = calloc(1, sizeof(ParusData));
	if (pd != NULL) {
		pd->data.baseop 	= op;
		pd->type 			= BASEOP;
	}
	return pd;
}

/* Makes a new userop insert instructions with parus_insert_instr */
ParusData* make_parus_userop() {
	ParusData* op = calloc(1, sizeof(ParusData));

	if (op != NULL) {
		op->data.userop.instructions	= calloc(USEROP_INSTR_GROWTH, sizeof(ParusData));
		op->data.userop.max 			= USEROP_INSTR_GROWTH;
		op->data.userop.size 			= 0;
		op->type 						= USEROP;
	}

	return op;
}


/* Free the parusdata */
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

/* Prints parusdata */
void print_parusdata(ParusData* pd) {
	if (pd == NULL) return;

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

/* Makes a new parus stack */
Stack* make_stack() {
	Stack* stk 	= calloc(1, sizeof(Stack*));
	stk->size   = 0;
	stk->max    = STACK_GROWTH;
	stk->items  = calloc(stk->max, sizeof(ParusData));

	return stk;
}

/* Pushes a new parusdata item to the stack */
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
Pulls an item from the stack.
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
Gets a copy from the stack.
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
Deletes item from the stack an cleans it.
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

/* Frees the stack */
void free_stack(Stack* stk) {
	if (stk != NULL) {
		for (int i = 0; i < stk->size; i++)
			free_parusdata(stk->items[i]);
		free(stk->items);
		free(stk);
	}
}

/* Prints the stack contant */
void print_stack(Stack* stk) {
	for (int i = 0; i < stk->size; i++) {
		print_parusdata(stk->items[i]);
		printf(", ");

	}
	printf("\n");
}

// LEXICON
// ----------------------------------------------------------------------------------------------------

/* Makes a new lexicon */
Lexicon* make_lexicon() {
	Lexicon* lex 	= calloc(1, sizeof(Lexicon));
	lex->size 		= 0;
	lex->max 		= LEXICON_GROWTH;
	lex->entries 	= calloc(lex->max, sizeof(struct entry));

	return lex;
}

/* Define a new entry on the lexicon */
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

/* Deletes an entry from the lexicon */
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

/* Gets a copy of an entry */
ParusData* lexicon_get(Lexicon* lex, char* name) {
	for (int i = lex->size -1; i >= 0; i--) 
		if (strcmp(lex->entries[i].name, name) == 0)
			return parusdata_copy(lex->entries[i].value);

	fprintf(stderr, "UNDEFINED ENTRY - %s\n", name);
	return NULL;
}

/* Frees the lexicon */
void free_lexicon(Lexicon* lex) {
	if (lex != NULL) {
		for (int i = 0; i < lex->size; i++) {
			free_parusdata(lex->entries[i].value);
			free(lex->entries[i].name);
		}
		free(lex->entries);
		free(lex);
	}
}

/* Prints the lexicon contant */
void print_lexicon(Lexicon* lex) {
	for (int i = 0; i < lex->size; i++) {
		printf("%s : ", lex->entries[i].name);
		print_parusdata(lex->entries[i].value);
		printf("\n");
	}
}

// CPARUS FUNCTIONS
// ----------------------------------------------------------------------------------------------------

/* Inserts an instruction to a user op */
void parus_insert_instr(ParusData* op, ParusData* instr) {
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

/* 
Returns if the evaluator can call the expression by returning the parentheses count

if result > 0 unterminated expression
if result = 0 valid expression
if result < 0 overterminated expression
*/
int parus_parencount(char* expr) {
	int 	result  = 0;
	int 	i 		= 0;
	char 	ignore  = 0;
	char 	c;

	while ((c = expr[i++]) != '\0') {
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
Sets apply_caller and apply_shortcut.
make sure to call parus_set_applier(NULL, NULL), 
after calling it in order to reset the values.
*/
void parus_set_applier(baseop_t caller, applier_t applier) {
	apply_caller 	= caller;
	apply_shortcut	= applier;
}

/*
Applies a parusdata 
the function will automatically free pd if needed
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
void parus_evaluate(char* expr, Stack* stk, Lexicon* lex) {
	char* 	buffer 	= copy_string(expr);

	// stacks are used to store yet to be terminated operators and quotes
	Stack* 	opstk 	= make_stack(); 
	Stack* 	qtstk 	= make_stack();

	char* 	token 	= strtok(buffer, " ");
	
	while (token != NULL) {
		ParusData* pd = NULL;
		
		if (is_termination(token)) {
			if (opstk->size == 1) 
				pd = stack_pull(opstk);

			else if (opstk->size > 1) {
				ParusData* internal = stack_pull(opstk);
				ParusData* external = stack_pull(opstk);

				parus_insert_instr(external, internal);
				stack_push(opstk, external);
			}
			else {
				fprintf(stderr, "INVALID EXPRESSION GIVEN - EXPECTED AN OPERATOR\n");
				break;
			}
		}

		/* self evaluating forms */
		else if (is_user_operator(token)) {
			stack_push(opstk, make_parus_userop());
			stack_push(qtstk, NULL); // keep bookmark of the quotation order
		}

		else if (is_integer(token))
			pd = make_parus_integer(atoi(token));

		else if (is_decimal(token))
			pd = make_parus_decimal(atof(token));

		/* quoted forms */
		else if (is_quoted(token))
			stack_push(qtstk, make_parus_quote(NULL));
		
		/* calls */
		else if (is_symbol(token))
			pd = make_parus_symbol(token);
		
		// validate expression
		if ((token = strtok(NULL, " ")) == NULL) { 
			if (qtstk->size > 0 && qtstk->items[qtstk->size -1] != NULL) { // if nothing to quote
				fprintf(stderr, "INVALID EXPRESSION GIVEN - STANDALONE QUOTE\n");
				break;
			}
			if (opstk->size > 0) { // if unterminated expression given
				fprintf(stderr, "INVALID EXPRESSION GIVEN - UNTERMINATED OPERATOR\n");
				break;
			}
		}

		// if a complete expression is yet to be read continue
		if (pd == NULL) continue;

		// quotates the expression
		while (qtstk->size > 0) {
			ParusData* qtop;
			if (qtstk->size > 0 && (qtop = stack_pull(qtstk)) != NULL) {
				qtop->data.quoted = pd;
				pd = qtop;
			}
			else 
				break;
		}

		if (opstk->size == 0) {
			if (pd->type != SYMBOL && pd->type != QUOTED) 
				stack_push(stk, pd); // self evaluating, push to the stack
			else
				parus_apply(pd, stk, lex); // non self evaluating, apply
		}
		else // inserts instruction to topmost operator
			parus_insert_instr(opstk->items[opstk->size -1], pd);

	}

	free(buffer);
	free_stack(opstk);
	free_stack(qtstk);
}
