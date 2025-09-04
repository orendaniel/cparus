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
#include <math.h>

#define READ_BUFFER 1024

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

static ParusData* top_of_stack(void* stk, void* lex) {
	return stack_pull(stk);
}


// BASIC
// ----------------------------------------------------------------------------------------------------

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

static int apply_top(void* stk, void* lex) {
	parus_set_applier(&apply_top, &top_of_stack);
	int e = parus_apply(NULL, stk, lex);
	parus_set_applier(NULL, NULL);
	if (e)
		fprintf(stderr, "CANNOT APPLY TOP OF STACK\n");

	return e;

}

static int quotate(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);
	if (pd != NULL) {
		stack_push(stk, make_parus_quote(pd));
		return 0;
	}
	else {
		fprintf(stderr, "NOTHING TO QUOTATE\n");
		return 1;
	}
}

static int if_op(void* stk, void* lex) {
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

	char act = 1;

	if (cond->type == INTEGER && parusdata_tointeger(cond) == 0)
		act = 0;
	else if (cond->type == DECIMAL && parusdata_todecimal(cond) == 0)
		act = 0;

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

static int eqv(void* stk, void* lex) {
	ParusData* pd2 = stack_pull(stk);
	ParusData* pd1 = stack_pull(stk);

	if (pd1 == NULL || pd2 == NULL) {
		free_parusdata(pd1);
		free_parusdata(pd2);
		fprintf(stderr, "ATTEMPT TO COMPARE NULLITY\n");
		return 1;
	}

	stack_push(stk, make_parus_integer(equivalent(pd1, pd2)));
	
	free_parusdata(pd1);
	free_parusdata(pd2);
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
	stack_push(stk, make_parus_integer((integer_t) ((Stack*)stk)->size));

	return 0;
}

static int drop(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);
	free_parusdata(pd);

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

	if (index != -1)
		stack_push(stk, make_parus_integer(pstk->size - (index +1)));
	else
		stack_push(stk, make_parus_integer(-1));

	free_parusdata(pd);
	return 0;

}

// ARTHMATICS
// ----------------------------------------------------------------------------------------------------

#define GET_TWO_NUMBERS 							\
	ParusData* pd2 = stack_pull(stk); 				\
	ParusData* pd1 = stack_pull(stk); 				\
													\
	if (!is_number(pd1) || !is_number(pd2)) { 		\
		free_parusdata(pd1); 						\
		free_parusdata(pd2); 						\
		fprintf(stderr, "EXPECTED TWO NUMBERS\n"); 	\
		return 1; 									\
	}

#define ARTH_FN(FUNCINT, FUNCDEC, OP) 						\
	if (pd1->type == INTEGER && pd2->type == INTEGER) { 	\
		integer_t a = parusdata_tointeger(pd1); 			\
		integer_t b = parusdata_tointeger(pd2); 			\
		stack_push(stk, FUNCINT(a OP b)); 					\
	} 														\
	else { 													\
		decimal_t a = force_decimal(pd1); 					\
		decimal_t b = force_decimal(pd2); 					\
		stack_push(stk, FUNCDEC(a OP b)); 					\
															\
	} 														\
	free_parusdata(pd1); 									\
	free_parusdata(pd2);

static int add(void* stk, void* lex) {
	GET_TWO_NUMBERS;
	ARTH_FN(make_parus_integer, make_parus_decimal, +);
	return 0;
}

static int subtract(void* stk, void* lex) {
	GET_TWO_NUMBERS;
	ARTH_FN(make_parus_integer, make_parus_decimal, -);
	return 0;
}

static int multiply(void* stk, void* lex) {
	GET_TWO_NUMBERS;
	ARTH_FN(make_parus_integer, make_parus_decimal, *);
	return 0;
}

static int divide(void* stk, void* lex) {
	GET_TWO_NUMBERS;

	if ((pd2->type == INTEGER && parusdata_tointeger(pd2) == 0) || 
			(pd2->type == DECIMAL && parusdata_todecimal(pd2) == 0)) 
		fprintf(stderr, "WARNING: DIVISION BY ZERO IS UNDEFINED BEHAVIOR\n");

	decimal_t a = force_decimal(pd1);
	decimal_t b = force_decimal(pd2);
	stack_push(stk, make_parus_decimal(a / b));
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int powerof(void* stk, void* lex) {
	GET_TWO_NUMBERS;

	decimal_t a = force_decimal(pd1);
	decimal_t b = force_decimal(pd2);
	stack_push(stk, make_parus_decimal(pow(a, b)));
	
	free_parusdata(pd1);
	free_parusdata(pd2);
	return 0;
}

static int equal(void* stk, void* lex) {
	GET_TWO_NUMBERS;
	ARTH_FN(make_parus_integer, make_parus_integer, ==);
	return 0;
}

static int less_than(void* stk, void* lex) {
	GET_TWO_NUMBERS;
	ARTH_FN(make_parus_integer, make_parus_integer, <);
	return 0;
}

static int greater_than(void* stk, void* lex) {
	GET_TWO_NUMBERS;
	ARTH_FN(make_parus_integer, make_parus_integer, >);
	return 0;
}

static int round_value(void* stk, void* lex) {
	ParusData* pd = stack_pull(stk);
	if (!is_number(pd)) {
		fprintf(stderr, "CANNOT ROUND A NON NUMERIC VALUE\n");
		free_parusdata(pd);
		return 1;
	}
	stack_push(stk, make_parus_integer(force_decimal(pd)));
	free_parusdata(pd);
	return 0;
}


// REFLECTION
// ----------------------------------------------------------------------------------------------------

#define REFLECTION_TEMPLATE(COND) 						\
	ParusData* pd = stack_pull(stk); 					\
	if (pd != NULL && COND) {							\
		stack_push(stk, pd); 							\
		stack_push(stk, make_parus_integer(1)); 		\
	} 													\
	else { 												\
		stack_push(stk, pd); 							\
		stack_push(stk, make_parus_integer(0)); 		\
	}

static int is_top_integer(void* stk, void* lex) {
	REFLECTION_TEMPLATE(pd->type == INTEGER);
	return 0;
}

static int is_top_decimal(void* stk, void* lex) {
	REFLECTION_TEMPLATE(pd->type == DECIMAL);
	return 0;

}

static int is_top_operator(void* stk, void* lex) {
	REFLECTION_TEMPLATE(pd->type == USEROP || pd->type == BASEOP);
	return 0;
}

static int is_top_symbol(void* stk, void* lex) {
	REFLECTION_TEMPLATE(pd->type == SYMBOL);
	return 0;
}

static int is_top_quoted(void* stk, void* lex) {
	REFLECTION_TEMPLATE(pd->type == QUOTED);
	return 0;
}

// IO
// ----------------------------------------------------------------------------------------------------

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

/* reader evaluates the expression given */
static int read(void* stk, void* lex) {
	int c;
	int i = 1;

	char buffer[READ_BUFFER];
	buffer[0] = QUOTE_CHAR;

	while ((c = getc(stdin)) != EOF && i < READ_BUFFER -1) {
		if (isspace(c)) {
			buffer[i] = '\0';
			break;
		}
		else if (c != LP_CHAR && c != RP_CHAR && c != COMMENT_CHAR) {
			buffer[i++] = c;
		}
	}
	if (buffer[i -1] != QUOTE_CHAR)
		parus_evaluate(buffer, stk, lex);

	return 0;
}

static int getcharacter(void* stk, void* lex) {
	stack_push(stk, make_parus_integer(getc(stdin)));
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


// OPTIONALS
// ----------------------------------------------------------------------------------------------------

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

static int setat(void* stk, void* lex) {
	ParusData* index = stack_pull(stk);
	ParusData* value = stack_pull(stk);

	if (index == NULL || value == NULL || index->type != INTEGER) {
		fprintf(stderr, "INVALID PARAMTERS GIVEN TO SETAT\n");
		free_parusdata(index);
		free_parusdata(value);
		return 1;

	}

	Stack* pstk	= (Stack*)stk;
	int i 		= pstk->size - (parusdata_tointeger(index) +1);
	if (i < pstk->size && i >= 0) {
		ParusData* old = pstk->items[i];
		free_parusdata(old);
		pstk->items[i] = value;
		free_parusdata(index);
	}
	else {
		fprintf(stderr, "INDEX OUT OF RANGE\n");
		free_parusdata(index);
		free_parusdata(value);
		return 1;
	}

	return 0;
}


static int for_op(void* stk, void* lex) {

	ParusData* fn 	= stack_pull(stk);
	ParusData* inc 	= stack_pull(stk);
	ParusData* cmp 	= stack_pull(stk);
	ParusData* max 	= stack_pull(stk);
	ParusData* min 	= stack_pull(stk);
	ParusData* sym 	= stack_pull(stk);

	if (fn == NULL || inc == NULL || cmp == NULL || max == NULL || min == NULL || sym == NULL 
			|| inc->type != INTEGER || min->type != INTEGER || max->type != INTEGER || 
			sym->type != SYMBOL || 
			!(cmp->type == SYMBOL || cmp->type == USEROP || cmp->type == BASEOP)) {
		
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
		stack_push(stk, make_parus_integer(i));
		stack_push(stk, parusdata_copy(max));
		parus_apply(parusdata_copy(cmp), stk, lex);


		ParusData* 	cond 		= stack_pull(stk);
		int 		cond_int 	= parusdata_tointeger(cond);

		free_parusdata(cond);


		if (cond_int != 0) {
			lexicon_define(lex, parusdata_getsymbol(sym), make_parus_integer(i));

			parus_apply(parusdata_copy(fn), stk, lex);

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


static int end_case_op(void* stk, void* lex) {
	Stack* 		pstk 		= (Stack*)stk;
	ParusData* 	case_sym 	= make_parus_symbol("case");
	int 		index 		= -1;

	for (int i = pstk->size -1; i >= 0; i--) {
		if (equivalent(case_sym, pstk->items[i])) {
			index = i;
			break;
		}
	}
	if (index < 0) {
		fprintf(stderr, "NO CASE LABEL FOUND\n");
		free_parusdata(case_sym);
		return 1;
	}

	index = pstk->size - (index +1);
	ParusData* iftrue = NULL;

	while (index > 0) { 
		if (index == 1)
			break;

		ParusData* cond = stack_get_at(stk, index -1);
		ParusData* doif = stack_get_at(stk, index -2);

		stack_remove_at(stk, index -1);
		stack_remove_at(stk, index -2);

		parus_apply(cond, stk, lex);

		ParusData* top = stack_pull(stk);

		char act = 0;
		if (top != NULL && top->type == INTEGER && parusdata_tointeger(top) != 0)
			act = 1;
		else if (top != NULL && top->type == DECIMAL && parusdata_todecimal(top) != 0)
			act = 1;

		free_parusdata(top);

		if (act) {
			iftrue = doif;
			break;
		}
		else
			free_parusdata(doif);
		
		index -= 2;
	}
	

	ParusData* pd = NULL;
	while ((pd = stack_pull(stk)) != NULL && !equivalent(pd, case_sym)) // strip label
		free_parusdata(pd);
	free_parusdata(pd);
	free_parusdata(case_sym);

	parus_apply(iftrue, stk, lex);

	return 0;
}

static int quit(void* stk, void* lex) {
	exit(EXIT_SUCCESS);
	return 0;

}

// DEBUGGING AND HELP
// ----------------------------------------------------------------------------------------------------

static int stkprint(void* stk, void* lex) {
	print_stack(stk);
	return 0;
}

static int lexprint(void* stk, void* lex) {
	print_lexicon(lex);
	return 0;
}

static int help(void* stk, void* lex) {
	printf(HELP_MESSAGE);
	return 0;
}

// EXPERIMENTAL
// ----------------------------------------------------------------------------------------------------

static int seqterm(void* stk, void* lex) {
	Stack* 		pstk 		= (Stack*)stk;
	ParusData* 	seq_sym 	= make_parus_symbol("seq");
	int 		index 		= -1;

	for (int i = pstk->size -1; i >= 0; i--) {
		if (equivalent(seq_sym, pstk->items[i])) {
			index = i;
			break;
		}
	}
	if (index < 0) {
		fprintf(stderr, "NO SEQUENCE LABEL FOUND\n");
		free_parusdata(seq_sym);
		return 1;
	}

	index = pstk->size - (index +1);
	stack_remove_at(stk, index--);
	ParusData* op = make_parus_userop();

	for (int i = index; i >= 0; i--) {
		ParusData* instr = stack_get_at(stk, i);
		parus_insert_instr(op, instr);
		stack_remove_at(stk, i);
	}
	
	stack_push(stk, op);
	return 0;
}

// PREDEFINED LEXICON
// ----------------------------------------------------------------------------------------------------

Lexicon* predefined_lexicon() {
	Lexicon* lex = make_lexicon();

	lexicon_define(lex, "define", make_parus_baseop(&define));
	lexicon_define(lex, "delete", make_parus_baseop(&delete));
	lexicon_define(lex, "!", make_parus_baseop(&apply_top));
	lexicon_define(lex, "quotate", make_parus_baseop(&quotate));
	lexicon_define(lex, "if", make_parus_baseop(&if_op));
	lexicon_define(lex, "eqv?", make_parus_baseop(&eqv));
	lexicon_define(lex, "@", make_parus_baseop(&fetch));
	lexicon_define(lex, "@.", make_parus_baseop(&fetch_copy));
	lexicon_define(lex, "length", make_parus_baseop(&length));
	lexicon_define(lex, "drop", make_parus_baseop(&drop));
	lexicon_define(lex, "find", make_parus_baseop(&find));

	lexicon_define(lex, "+", make_parus_baseop(&add));
	lexicon_define(lex, "-", make_parus_baseop(&subtract));
	lexicon_define(lex, "*", make_parus_baseop(&multiply));
	lexicon_define(lex, "/", make_parus_baseop(&divide));
	lexicon_define(lex, "^", make_parus_baseop(&powerof));
	lexicon_define(lex, "=", make_parus_baseop(&equal));
	lexicon_define(lex, "<", make_parus_baseop(&less_than));
	lexicon_define(lex, ">", make_parus_baseop(&greater_than));
	lexicon_define(lex, "round", make_parus_baseop(&round_value));

	lexicon_define(lex, "integer?", make_parus_baseop(&is_top_integer));
	lexicon_define(lex, "decimal?", make_parus_baseop(&is_top_decimal));
	lexicon_define(lex, "operator?", make_parus_baseop(&is_top_operator));
	lexicon_define(lex, "symbol?", make_parus_baseop(&is_top_symbol));
	lexicon_define(lex, "quoted?", make_parus_baseop(&is_top_quoted));

	lexicon_define(lex, "out", make_parus_baseop(&out));
	lexicon_define(lex, "outln", make_parus_baseop(&outln));
	lexicon_define(lex, "read", make_parus_baseop(&read));
	lexicon_define(lex, "getc", make_parus_baseop(&getcharacter));
	lexicon_define(lex, "putc", make_parus_baseop(&putcharacter));

	lexicon_define(lex, "dpl", make_parus_baseop(&dpl));
	lexicon_define(lex, "setat", make_parus_baseop(&setat));
	lexicon_define(lex, "for", make_parus_baseop(&for_op));
	lexicon_define(lex, "case", make_parus_quote(make_parus_symbol("case")));
	lexicon_define(lex, "else", make_parus_quote(make_parus_symbol("else")));
	lexicon_define(lex, "end-case", make_parus_baseop(&end_case_op));
	lexicon_define(lex, "quit", make_parus_baseop(&quit));

	lexicon_define(lex, "?stk", make_parus_baseop(&stkprint));
	lexicon_define(lex, "?lex", make_parus_baseop(&lexprint));
	lexicon_define(lex, "?help", make_parus_baseop(&help));

	lexicon_define(lex, "seq", make_parus_quote(make_parus_symbol("seq")));
	lexicon_define(lex, "end-seq", make_parus_baseop(&seqterm));

	return lex;

}
