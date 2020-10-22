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

#include "parus_predefined.h"
#include <time.h>
#include <math.h>


#define READ_BUFFER 128

static decimal_t force_decimal(ParusData* pd) {
	if (pd->type == INTEGER)
		return (decimal_t)parusdata_tointeger(pd);
	else if (pd->type == DECIMAL)
		return parusdata_todecimal(pd);
	else
		return 0;
}

static char is_number(ParusData* pd) {
	return pd != NULL && (pd->type == INTEGER || pd->type == DECIMAL);
}

static char equivalent(ParusData* pd1, ParusData* pd2) {
	if (pd1 == NULL || pd2 == NULL)
		return pd1 == NULL && pd2 == NULL;

	if (pd1->type == SYMBOL && pd2->type == SYMBOL)
		return strcmp(parusdata_getsymbol(pd1), parusdata_getsymbol(pd2)) == 0;

	else if (pd1->type == INTEGER && pd2->type == INTEGER)
		return parusdata_tointeger(pd1) == parusdata_tointeger(pd2);

	else if (is_number(pd1) && is_number(pd2))
		return force_decimal(pd1) == force_decimal(pd2);
	
	else if (pd1->type == QUOTED && pd2->type == QUOTED)
		return equivalent(parusdata_unquote(pd1), parusdata_unquote(pd2));

	else 
		return pd1 == pd2;
}

static int define(void* stk, void* lex) {
	ParusData* sym = stack_pull(stk);
	ParusData* val = stack_pull(stk);

	if (sym == NULL || val == NULL || sym->type != SYMBOL) {
		free_parusdata(sym);
		free_parusdata(val);
		fprintf(stderr, "CAN ONLY BIND TO SYMBOLS\n");
		return 1;
	}
	lexicon_define(lex, parusdata_getsymbol(sym), val);
	free_parusdata(sym);
	
	return 0;
}

static int delete(void* stk, void* lex) {
	ParusData* sym = stack_pull(stk);
	if (sym == NULL || sym->type != SYMBOL) {
		free_parusdata(sym);
		fprintf(stderr, "CAN ONLY DELETE BINDED SYMBOLS\n");
		return 1;
	}
	lexicon_delete(lex, parusdata_getsymbol(sym));
	free_parusdata(sym);
	return 0;
}

static int if_func(void* stk, void* lex) {
	ParusData* do_false	= stack_pull(stk);
	ParusData* do_true 	= stack_pull(stk);
	ParusData* cond		= stack_pull(stk);

	if (cond == NULL || do_true == NULL || do_false == NULL) {
		fprintf(stderr, "CAN NOT PREFORM IF OPERATION\n");
		free_parusdata(cond);
		free_parusdata(do_true);
		free_parusdata(do_false);
		return 1;
	}

	char act = 0;

	if (cond->type == INTEGER && parusdata_tointeger(cond) != 0)
		act = 1;
	else if (cond->type == DECIMAL && parusdata_todecimal(cond) != 0)
		act = 1;

	if (act) {
		stack_push(stk, do_true);
		free_parusdata(cond);
		free_parusdata(do_false);
		return 0;
	}
	else {
		stack_push(stk, do_false);
		free_parusdata(cond);
		free_parusdata(do_true);
		return 0;

	}
}

static int quote(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);

	if (pd == NULL || !(pd->type == SYMBOL || pd->type == QUOTED)) {
		free_parusdata(pd);
		fprintf(stderr, "CAN NOT PREFORM QUOTE OPERATION\n");
		return 1;
	}

	else 
		stack_push(stk, new_parusdata_quote(pd));


	return 0;
}

static int fetch(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);

	if (pd == NULL || pd->type != INTEGER) {
		fprintf(stderr, "INDEX MUST BE AN INTEGER\n");
		free_parusdata(pd);
		return 1;
	}
	if (parusdata_tointeger(pd) < ((Stack*)stk)->size && parusdata_tointeger(pd) >= 0) {
		ParusData* res = stack_get_at(stk, parusdata_tointeger(pd));

		stack_remove_at(stk, parusdata_tointeger(pd));
		stack_push(stk, res);
		free_parusdata(pd);
		return 0;
	}
	else {
		fprintf(stderr, "INDEX OUT OF RANGE\n");
		free_parusdata(pd);
		return 1;
	}
}

static int fetch_copy(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);

	if (pd == NULL || pd->type != INTEGER) {
		fprintf(stderr, "INDEX MUST BE AN INTEGER\n");
		free_parusdata(pd);
		return 1;
	}
	if (parusdata_tointeger(pd) < ((Stack*)stk)->size && parusdata_tointeger(pd) >= 0) {
		ParusData* res = stack_get_at(stk, parusdata_tointeger(pd));

		stack_push(stk, res);
		free_parusdata(pd);
		return 0;
	}
	else {
		fprintf(stderr, "INDEX OUT OF RANGE\n");
		free_parusdata(pd);
		return 1;
	}
}

static int length(void* stk, void* lex) {
	stack_push(stk, new_parusdata_integer((integer_t) ((Stack*)stk)->size));

	return 0;
}

static int find(void* stk, void* lex) {
	Stack* 		pstk 	= (Stack*)stk;
	ParusData* 	pd 		= stack_pull(stk);

	if (pd == NULL) {
		fprintf(stderr, "ATTEMPT TO COMPARE NULLITY\n");
		return 1;
		
	}

	int index = -1;

	for (int i = pstk->size -1; i >= 0; i--) {
		if (equivalent(pd, pstk->items[i])) {
			index = i;
			break;
		}
	}
	stack_push(stk, new_parusdata_integer(pstk->size - (index +1)));

	free_parusdata(pd);
	return 0;

}

static int eqv(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);

	if (pd1 == NULL || pd2 == NULL) {
		free_parusdata(pd1);
		free_parusdata(pd2);
		fprintf(stderr, "ATTEMPT TO COMPARE NULLITY\n");
		return 1;
		
	}

	stack_push(stk, new_parusdata_integer(equivalent(pd1, pd2)));
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int is_top_integer(void* stk, void* lex) {
	ParusData* pd = stack_get_at(stk, 0);
	if (pd != NULL && pd->type == INTEGER)
		stack_push(stk, new_parusdata_integer(1));
	else
		stack_push(stk, new_parusdata_integer(0));

	free_parusdata(pd);
	return 0;
}

static int is_top_decimal(void* stk, void* lex) {
	ParusData* pd = stack_get_at(stk, 0);
	if (pd != NULL && pd->type == DECIMAL)
		stack_push(stk, new_parusdata_integer(1));
	else
		stack_push(stk, new_parusdata_integer(0));

	free_parusdata(pd);
	return 0;

}

static int is_top_macro(void* stk, void* lex) {
	ParusData* pd = stack_get_at(stk, 0);
	if (pd != NULL && pd->type == USER_MACRO || pd->type == PRIMITIVE_MACRO)
		stack_push(stk, new_parusdata_integer(1));
	else
		stack_push(stk, new_parusdata_integer(0));

	free_parusdata(pd);
	return 0;


}

static int is_top_symbol(void* stk, void* lex) {
	ParusData* pd = stack_get_at(stk, 0);
	if (pd != NULL && pd->type == SYMBOL)
		stack_push(stk, new_parusdata_integer(1));
	else
		stack_push(stk, new_parusdata_integer(0));

	free_parusdata(pd);
	return 0;

}

static int is_top_quoted(void* stk, void* lex) {
	ParusData* pd = stack_get_at(stk, 0);
	if (pd != NULL && pd->type == QUOTED)
		stack_push(stk, new_parusdata_integer(1));
	else
		stack_push(stk, new_parusdata_integer(0));

	free_parusdata(pd);
	return 0;

}

static int add(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);

	if (!is_number(pd1) || !is_number(pd2)) {
		free_parusdata(pd1);
		free_parusdata(pd2);
		fprintf(stderr, "EXPECTED TWO NUMBERS\n");
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a + b));
	}
	else {
		decimal_t a = force_decimal(pd1);
		decimal_t b = force_decimal(pd2);
		stack_push(stk, new_parusdata_decimal(a + b));

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int subtract(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);

	if (!is_number(pd1) || !is_number(pd2)) {
		free_parusdata(pd1);
		free_parusdata(pd2);
		fprintf(stderr, "EXPECTED TWO NUMBERS\n");
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a - b));
	}
	else {
		decimal_t a = force_decimal(pd1);
		decimal_t b = force_decimal(pd2);
		stack_push(stk, new_parusdata_decimal(a - b));

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int multiply(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);

	if (!is_number(pd1) || !is_number(pd2)) {
		free_parusdata(pd1);
		free_parusdata(pd2);
		fprintf(stderr, "EXPECTED TWO NUMBERS\n");
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a * b));
	}
	else {
		decimal_t a = force_decimal(pd1);
		decimal_t b = force_decimal(pd2);
		stack_push(stk, new_parusdata_decimal(a * b));

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int divide(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);
	if (!is_number(pd1) || !is_number(pd2)) {
		free_parusdata(pd1);
		free_parusdata(pd2);
		fprintf(stderr, "EXPECTED TWO NUMBERS\n");
		return 1;
	}

	if ((pd2->type == INTEGER && parusdata_tointeger(pd2) == 0) || 
			(pd2->type == DECIMAL && parusdata_todecimal(pd2) == 0)) 
		fprintf(stderr, "WARNING: DIVISION BY ZERO IS UNDEFINED BEHAVIOR\n");


	decimal_t a = force_decimal(pd1);
	decimal_t b = force_decimal(pd2);
	stack_push(stk, new_parusdata_decimal(a / b));
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int powerof(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);

	if (!is_number(pd1) || !is_number(pd2)) {
		free_parusdata(pd1);
		free_parusdata(pd2);
		fprintf(stderr, "EXPECTED TWO NUMBERS\n");
		return 1;
	}

	decimal_t a = force_decimal(pd1);
	decimal_t b = force_decimal(pd2);
	stack_push(stk, new_parusdata_decimal(pow(a, b)));
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int equal(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);

	if (!is_number(pd1) || !is_number(pd2)) {
		free_parusdata(pd1);
		free_parusdata(pd2);
		fprintf(stderr, "EXPECTED TWO NUMBERS\n");
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a == b));
	}
	else {
		decimal_t a = force_decimal(pd1);
		decimal_t b = force_decimal(pd2);
		stack_push(stk, new_parusdata_integer(a == b));

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int less_than(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);

	if (!is_number(pd1) || !is_number(pd2)) {
		free_parusdata(pd1);
		free_parusdata(pd2);
		fprintf(stderr, "EXPECTED TWO NUMBERS\n");
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a < b));
	}
	else {
		decimal_t a = force_decimal(pd1);
		decimal_t b = force_decimal(pd2);
		stack_push(stk, new_parusdata_integer(a < b));

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int greater_than(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);

	if (!is_number(pd1) || !is_number(pd2)) {
		free_parusdata(pd1);
		free_parusdata(pd2);
		fprintf(stderr, "EXPECTED TWO NUMBERS\n");
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a > b));
	}
	else {
		decimal_t a = force_decimal(pd1);
		decimal_t b = force_decimal(pd2);
		stack_push(stk, new_parusdata_integer(a > b));

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int round_value(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);
	if (!is_number(pd)) {
		fprintf(stderr, "CANNOT ROUND A NON NUMERIC VALUE\n");
		free_parusdata(pd);
		return 1;
	}
	stack_push(stk, new_parusdata_integer(force_decimal(pd)));
	free_parusdata(pd);
	return 0;
}


static int out(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);
	if (pd == NULL) {
		free_parusdata(pd);
		fprintf(stderr, "CANNOT PRINT NULLITY\n");
		return 1;
	
	}
	print_parusdata(pd);
	free_parusdata(pd);
	return 0;

}

static int outln(void* stk, void* lex) {
	int ret = out(stk, lex);
	if (ret == 0)
		printf("\n");
	return ret;
}

static int read(void* stk, void* lex) {

	/* copied from parus.c */
	char is_integer(char* s) {
		if (s == NULL || *s == '\0' || isspace(*s))
		  return 0;
		char * p;
		strtol(s, &p, 10);
		return *p == '\0';
	}

	char is_decimal(char* s) {
		if (s == NULL || *s == '\0' || isspace(*s)) 
		  return 0;
		char * p;
		strtod(s, &p);
		return *p == '\0';
	}

	int c;
	int i = 1;

	char buffer[READ_BUFFER];
	buffer[0] = '\'';

	while ((c = getc(stdin)) != EOF && c != ';' && i < READ_BUFFER -1) {
		if (isspace(c)) {
			buffer[i] = '\0';
			break;
		}
		else if (c != '(' && c != ')' && c != '\'' && c != '!') {
			buffer[i] = (char)c;
			i++;
		}
	}
	if (is_integer(buffer +1) || is_decimal(buffer +1))
		parus_evaluate(buffer +1, stk, lex);
	else
		parus_evaluate(buffer, stk, lex);


	return 0;
}

static int getcharacter(void* stk, void* lex) {
	stack_push(stk, new_parusdata_integer(getc(stdin)));

	return 0;
}

static int putcharacter(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);

	if (pd == NULL || pd->type != INTEGER) {
		fprintf(stderr, "CHAR CODE MUST BE AN INTEGER\n");
		free_parusdata(pd);
		return 1;
	}

	fputc(parusdata_tointeger(pd), stdout);
	free_parusdata(pd);

	return 0;
}


static int dpl(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);
	
	if (pd == NULL) {
		fprintf(stderr, "NOTHING TO DUPLICATE\n");
		return 1;
	}

	
	stack_push(stk, pd);
	stack_push(stk, parusdata_copy(pd));

	return 0;
}

static int drop(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);
	free_parusdata(pd);

	return 0;
}

static int stkprint(void* stk, void* lex) {
	stack_print(stk);
	return 0;
}

static int lexprint(void* stk, void* lex) {
	lexicon_print(lex);
	return 0;
}

static int for_macro(void* stk, void* lex) {

	ParusData* fn 	= stack_pull(stk);
	ParusData* inc 	= stack_pull(stk);
	ParusData* cmp 	= stack_pull(stk);
	ParusData* max 	= stack_pull(stk);
	ParusData* min 	= stack_pull(stk);
	ParusData* sym 	= stack_pull(stk);

	if (fn == NULL || inc == NULL || cmp == NULL || max == NULL || min == NULL || sym == NULL 
		||inc->type != INTEGER ||
			min->type != INTEGER || max->type != INTEGER ||
			sym->type != SYMBOL || 
			!(cmp->type == SYMBOL || cmp->type == USER_MACRO)) {
		
		fprintf(stderr, "WRONG TYPES OF PARAMETERS GIVEN\n");
		fprintf(stderr, "SYMBOL MIN MAX CMP INC FN\n");
		free_parusdata(fn);
		free_parusdata(inc);
		free_parusdata(cmp);
		free_parusdata(min);
		free_parusdata(max);
		free_parusdata(sym);
		return 1;
	}
	
	int i = parusdata_tointeger(min);

	while (1) {
		stack_push(stk, new_parusdata_integer(i));
		stack_push(stk, parusdata_copy(max));
		stack_push(stk, parusdata_copy(cmp));
		parus_evaluate("!", stk, lex);


		ParusData* 	cond 		= stack_pull(stk);
		int 		cond_int 	= parusdata_tointeger(cond);

		free_parusdata(cond);


		if (cond_int != 0) {
			lexicon_define(lex, parusdata_getsymbol(sym), new_parusdata_integer(i));

			stack_push(stk, parusdata_copy(fn));
			parus_evaluate("!", stk, lex);

			lexicon_delete(lex, parusdata_getsymbol(sym));
			i += parusdata_tointeger(inc);		
		}
		else
			break;
		
	}

	free_parusdata(fn);
	free_parusdata(inc);
	free_parusdata(cmp);
	free_parusdata(min);
	free_parusdata(max);
	free_parusdata(sym);
	return 0;

}

int static now(void* stk, void* lex) {
	stack_push((Stack*)stk, new_parusdata_decimal((decimal_t)clock()/CLOCKS_PER_SEC));
	return 0;

}

int static help(void* stk, void* lex) {
	printf(HELP_MESSAGE);
	return 0;
}


Lexicon* predefined_lexicon() {
	Lexicon* lex = new_lexicon(NULL);

	// basic
	lexicon_define(lex, "DEF", new_parusdata_primitive(&define));
	lexicon_define(lex, "DEL", new_parusdata_primitive(&delete));
	lexicon_define(lex, "IF", new_parusdata_primitive(&if_func));
	lexicon_define(lex, "QUOTE", new_parusdata_primitive(&quote));
	lexicon_define(lex, "@", new_parusdata_primitive(&fetch));
	lexicon_define(lex, "@.", new_parusdata_primitive(&fetch_copy));
	lexicon_define(lex, "LEN", new_parusdata_primitive(&length));
	lexicon_define(lex, "FIND", new_parusdata_primitive(&find));
	lexicon_define(lex, "EQV?", new_parusdata_primitive(&eqv));

	// reflection
	lexicon_define(lex, "INTEGER?", new_parusdata_primitive(&is_top_integer));
	lexicon_define(lex, "DECIMAL?", new_parusdata_primitive(&is_top_decimal));
	lexicon_define(lex, "MACRO?", new_parusdata_primitive(&is_top_macro));
	lexicon_define(lex, "SYMBOL?", new_parusdata_primitive(&is_top_symbol));
	lexicon_define(lex, "QUOTED?", new_parusdata_primitive(&is_top_quoted));

	// arithmatics
	lexicon_define(lex, "+", new_parusdata_primitive(&add));
	lexicon_define(lex, "-", new_parusdata_primitive(&subtract));
	lexicon_define(lex, "*", new_parusdata_primitive(&multiply));
	lexicon_define(lex, "/", new_parusdata_primitive(&divide));
	lexicon_define(lex, "^", new_parusdata_primitive(&powerof));
	lexicon_define(lex, "=", new_parusdata_primitive(&equal));
	lexicon_define(lex, "<", new_parusdata_primitive(&less_than));
	lexicon_define(lex, ">", new_parusdata_primitive(&greater_than));
	lexicon_define(lex, "ROUND", new_parusdata_primitive(&round_value));

	// I/O
	lexicon_define(lex, "OUT", new_parusdata_primitive(&out));
	lexicon_define(lex, "OUTLN", new_parusdata_primitive(&outln));
	lexicon_define(lex, "READ", new_parusdata_primitive(&read));
	lexicon_define(lex, "GETC", new_parusdata_primitive(&getcharacter));
	lexicon_define(lex, "PUTC", new_parusdata_primitive(&putcharacter));

	// shortcuts
	lexicon_define(lex, "DPL", new_parusdata_primitive(&dpl));
	lexicon_define(lex, "DROP", new_parusdata_primitive(&drop));

	// debugging
	lexicon_define(lex, "?stk", new_parusdata_primitive(&stkprint));
	lexicon_define(lex, "?lex", new_parusdata_primitive(&lexprint));

	// syntatic forms
	lexicon_define(lex, "FOR", new_parusdata_primitive(&for_macro));

	// misc
	lexicon_define(lex, "NOW", new_parusdata_primitive(&now));

	lexicon_define(lex, "?help", new_parusdata_primitive(&help));

	return lex;

}
