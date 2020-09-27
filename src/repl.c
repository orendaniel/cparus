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
#include <readline/readline.h>
#include <readline/history.h>

#include "parus.h"
#include "parus_predefined.h"



int main() {
	Stack* 		stk = new_stack();
	Lexicon* 	lex = predefined_lexicon();

	printf("CParus version 0\n");
	printf("CParus is free software under the GPLV3 license\n");
	printf("Oren Daniel, Ra'anana - Israel, 2020\n\n");


	/*while (1) {
		
		char* input = readline("CPARUS> ");

		char* token;
		char* rest = input;

		if (!input)
			break;

		add_history(input);
		
		while ((token = strtok_r(rest, " ", &rest))) {
			if (strcmp(token, '(') != 0)
				parus_eval(token, stk, lex);
			else {
				int 	paren_count = 1;
				char* 	buffer 		= calloc(strlen(rest) +1, sizeof(char)); 
				buffer[0] = '(';
				int i = 0;
				while (rest[i] != '\0') {
					buffer[i +2];
					i++;
				}
			}
		}
		parus_eval(input, stk, lex);
		free(input);
	}*/

	free_stack(stk);
	free_lexicon(lex);

	return 0;
}
