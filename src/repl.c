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

void clear_buffer(char* buffer) {
    int i = 0;
    while (buffer[i] != '\0') {
        buffer[i] = '\0';
		i++;
	}
}

int parencount(char* str) {
	int result = 0;
	int i = 0;
	while (str[i] != '\0') {

		if (str[i] == '(')
			result++;
		else if (str[i] == ')')
			result--;

		i++;

	}   
	return result;
}

		

int main() {
	Stack* 		stk = new_stack();
	Lexicon* 	lex = predefined_lexicon();

	printf("CParus version 0\n");
	printf("CParus is free software under the GPLV3 license\n");
	printf("Oren Daniel, Ra'anana - Israel, 2020\n\n");

	while (1) {
		
		char* input = readline("CPARUS> ");

		if (!input)
			break;

		add_history(input);
		

		char* buffer = malloc((strlen(input)+1)*sizeof(char));
		clear_buffer(buffer);
		int i = 0;
		int j = 0;
		char c;

		while ((c = input[i++]) != '\0') {
			if (c == ';')
				break;
			if (isspace(c) && parencount(buffer) <= 0) {
				buffer[j +1] = '\0';
				parus_eval(buffer, stk, lex);
				clear_buffer(buffer);
				j = 0;
			}
			else  {
				buffer[j] = c;
				j++;
			}
		}

		buffer[j +1] = '\0';
		parus_eval(buffer, stk, lex);
		
		free(buffer);
		free(input);
	}
	
	
	free_stack(stk);
	free_lexicon(lex);

	return 0;
}
