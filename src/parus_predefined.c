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

static int add(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);
	if (pd2 == NULL || pd1 == NULL) {
		printf("EXPECTED TWO NUMBERS\n");
		free_parusdata(pd1);
		free_parusdata(pd2);
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a + b));
	}
	else if (pd1->type == DECIMAL && pd2->type == DECIMAL) {
		decimal_t a = parusdata_todecimal(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_decimal(a + b));
	}
	else if (pd1->type == DECIMAL && pd2->type == INTEGER) {
		decimal_t a = parusdata_todecimal(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_decimal(a + (decimal_t)b));
	}
	else if (pd1->type == INTEGER && pd2->type == DECIMAL) {
		integer_t a = parusdata_tointeger(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_decimal((decimal_t)a + b));
	}
	else {
		printf("EXPECTED TWO NUMBERS\n");
		return 1;

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int subtract(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);
	if (pd2 == NULL || pd1 == NULL) {
		printf("EXPECTED TWO NUMBERS\n");
		free_parusdata(pd1);
		free_parusdata(pd2);
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a - b));
	}
	else if (pd1->type == DECIMAL && pd2->type == DECIMAL) {
		decimal_t a = parusdata_todecimal(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_decimal(a - b));
	}
	else if (pd1->type == DECIMAL && pd2->type == INTEGER) {
		decimal_t a = parusdata_todecimal(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_decimal(a - (decimal_t)b));
	}
	else if (pd1->type == INTEGER && pd2->type == DECIMAL) {
		integer_t a = parusdata_tointeger(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_decimal((decimal_t)a - b));
	}
	else {
		printf("EXPECTED TWO NUMBERS\n");
		return 1;

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int multiply(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);
	if (pd2 == NULL || pd1 == NULL) {
		printf("EXPECTED TWO NUMBERS\n");
		free_parusdata(pd1);
		free_parusdata(pd2);
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a * b));
	}
	else if (pd1->type == DECIMAL && pd2->type == DECIMAL) {
		decimal_t a = parusdata_todecimal(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_decimal(a * b));
	}
	else if (pd1->type == DECIMAL && pd2->type == INTEGER) {
		decimal_t a = parusdata_todecimal(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_decimal(a * (decimal_t)b));
	}
	else if (pd1->type == INTEGER && pd2->type == DECIMAL) {
		integer_t a = parusdata_tointeger(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_decimal((decimal_t)a * b));
	}
	else {
		printf("EXPECTED TWO NUMBERS\n");
		return 1;

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int divide(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);
	if (pd2 == NULL || pd1 == NULL) {
		printf("EXPECTED TWO NUMBERS\n");
		free_parusdata(pd1);
		free_parusdata(pd2);
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_decimal((decimal_t)a / (decimal_t)b));
	}
	else if (pd1->type == DECIMAL && pd2->type == DECIMAL) {
		decimal_t a = parusdata_todecimal(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_decimal(a / b));
	}
	else if (pd1->type == DECIMAL && pd2->type == INTEGER) {
		decimal_t a = parusdata_todecimal(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_decimal(a / (decimal_t)b));
	}
	else if (pd1->type == INTEGER && pd2->type == DECIMAL) {
		integer_t a = parusdata_tointeger(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_decimal((decimal_t)a / b));
	}
	else {
		printf("EXPECTED TWO NUMBERS\n");
		return 1;

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}


static int equal(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);
	if (pd2 == NULL || pd1 == NULL) {
		printf("Excepted two numbers\n");
		free_parusdata(pd1);
		free_parusdata(pd2);
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a == b));
	}
	else if (pd1->type == DECIMAL && pd2->type == DECIMAL) {
		decimal_t a = parusdata_todecimal(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_integer(a == b));
	}
	else if (pd1->type == DECIMAL && pd2->type == INTEGER) {
		decimal_t a = parusdata_todecimal(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a == (decimal_t)b));
	}
	else if (pd1->type == INTEGER && pd2->type == DECIMAL) {
		integer_t a = parusdata_tointeger(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_integer((decimal_t)a == b));
	}
	else {
		printf("EXPECTED TWO NUMBERS\n");
		return 1;

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int less_than(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);
	if (pd2 == NULL || pd1 == NULL) {
		printf("Excepted two numbers\n");
		free_parusdata(pd1);
		free_parusdata(pd2);
		return 1;
	}

	if (pd1->type == INTEGER && pd2->type == INTEGER) {
		integer_t a = parusdata_tointeger(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a < b));
	}
	else if (pd1->type == DECIMAL && pd2->type == DECIMAL) {
		decimal_t a = parusdata_todecimal(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_integer(a < b));
	}
	else if (pd1->type == DECIMAL && pd2->type == INTEGER) {
		decimal_t a = parusdata_todecimal(pd1);
		integer_t b = parusdata_tointeger(pd2);
		stack_push(stk, new_parusdata_integer(a < (decimal_t)b));
	}
	else if (pd1->type == INTEGER && pd2->type == DECIMAL) {
		integer_t a = parusdata_tointeger(pd1);
		decimal_t b = parusdata_todecimal(pd2);
		stack_push(stk, new_parusdata_integer((decimal_t)a < b));
	}
	else {
		printf("EXPECTED TWO NUMBERS\n");
		return 1;

	}
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int out(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);
	if (pd == NULL) 
		return 1;
	
	if (pd->type == INTEGER)
		printf("%ld\n", parusdata_tointeger(pd));
	else if (pd->type == DECIMAL)
		printf("%f\n", parusdata_todecimal(pd));
	else if (pd->type == SYMBOL)
		printf("%s\n", parusdata_getsymbol(pd));

	free_parusdata(pd);
	return 0;

}

static int define(void* stk, void* lex) {
	ParusData* sym = stack_pull(stk);
	ParusData* val = stack_pull(stk);

	if (sym == NULL || val == NULL) {
		free_parusdata(sym);
		free_parusdata(val);
		printf("CANNOT DEFINE\n");
		return 1;
	}
	if (sym->type != SYMBOL) {
		free_parusdata(sym);
		free_parusdata(val);
		printf("CAN ONLY BIND TO SYMBOLS\n");
		return 1;
	}
	lexicon_define(lex, parusdata_getsymbol(sym), val);
	free_parusdata(sym);
	
	return 0;
}


static int redefine(void* stk, void* lex) {
	ParusData* sym = stack_pull(stk);
	ParusData* val = stack_pull(stk);
	if (sym == NULL || val == NULL) {
		free_parusdata(sym);
		free_parusdata(val);
		printf("CANNOT REDEFINE\n");
		return 1;
	}
	if (sym->type != SYMBOL) {
		free_parusdata(sym);
		free_parusdata(val);
		printf("CAN ONLY BIND TO SYMBOLS\n");
		return 1;
	}
	lexicon_redefine(lex, parusdata_getsymbol(sym), val);
	free_parusdata(sym);
	return 0;
}

static int delete(void* stk, void* lex) {
	ParusData* sym = stack_pull(stk);
	if (sym == NULL) {
		free_parusdata(sym);
		printf("CANNOT DELETE\n");
		return 1;
	}
	if (sym->type != SYMBOL) {
		free_parusdata(sym);
		printf("CAN ONLY DELETE BINDED SYMBOLS\n");
		return 1;
	}
	lexicon_delete(lex, parusdata_getsymbol(sym));
	free_parusdata(sym);
	return 0;
}

static int apply(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);

	if (pd->type == INTEGER || pd->type == DECIMAL) {
		stack_push(stk, pd);
		return 0;
	}

	else if (pd->type == SYMBOL) 
		parus_eval(parusdata_getsymbol(pd), stk, lex);
	
	else if (pd->type == COMPOUND_MACRO)
		parus_apply_compound(pd, stk, lex);
		
	free_parusdata(pd);

	return 0;
}

static int if_func(void* stk, void* lex) {
	ParusData* do_false	= stack_pull(stk);
	ParusData* do_true 	= stack_pull(stk);
	ParusData* con 		= stack_pull(stk);

	char act = 0;

	if (con->type == INTEGER && parusdata_tointeger(con) != 0)
		act = 1;
	else if (con->type == DECIMAL && parusdata_todecimal(con) != 0)
		act = 1;

	if (con) {
		stack_push(stk, do_true);
		free_parusdata(con);
		free_parusdata(do_false);
		return 0;
	}
	else {
		stack_push(stk, do_false);
		free_parusdata(con);
		free_parusdata(do_true);
		return 0;

	}
}

static int dpl(void* stk, void* lex) {
	ParusData* pd	= stack_pull(stk);
	
	stack_push(stk, pd);
	stack_push(stk, parusdata_copy(pd));

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

Lexicon* predefined_lexicon() {
	Lexicon* lex = new_lexicon(NULL);

	lexicon_define(lex, "+", new_parusdata_primitive(&add));
	lexicon_define(lex, "-", new_parusdata_primitive(&subtract));
	lexicon_define(lex, "*", new_parusdata_primitive(&multiply));
	lexicon_define(lex, "/", new_parusdata_primitive(&divide));
	lexicon_define(lex, "=", new_parusdata_primitive(&equal));
	lexicon_define(lex, "<", new_parusdata_primitive(&less_than));

	lexicon_define(lex, "out", new_parusdata_primitive(&out));
	lexicon_define(lex, "def", new_parusdata_primitive(&define));
	lexicon_define(lex, "redef", new_parusdata_primitive(&redefine));
	lexicon_define(lex, "del", new_parusdata_primitive(&delete));

	lexicon_define(lex, "!", new_parusdata_primitive(&apply));
	lexicon_define(lex, "if", new_parusdata_primitive(&if_func));

	lexicon_define(lex, "dpl", new_parusdata_primitive(&dpl));


	lexicon_define(lex, "?stk", new_parusdata_primitive(&stkprint));
	lexicon_define(lex, "?lex", new_parusdata_primitive(&lexprint));

	return lex;

}
