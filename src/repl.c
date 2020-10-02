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

#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "parus.h"
#include "parus_predefined.h"

#define MAX_LENGTH 2048



char file_exists(char* filename) {
	struct stat   buffer;   
	return stat(filename, &buffer) == 0;
}

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

void print_title() {
	printf("CParus version 0.5\n");
	printf("CParus is free software under the GPLv3 license\n");
	printf("Oren Daniel 2020 Ra'anana Israel\n");
	printf("Type parus -help for help\n\n");

}

void print_help() {
	printf("\nParus - Postfixed Reprogrammable Stack language\n");
	printf("Visit https://gitlab.com/oren_daniel/cparus for instructions and details\n");
	printf("Author email oren_daniel@protonmail.com\n\n");
	printf("flags: -help -norepl -notitle file\n\n");

}

void do_file(FILE* f, Stack* stk, Lexicon* lex) {
	char buffer[MAX_LENGTH];
	clear_buffer(buffer);

	int i = 0;

	int c;

	while ((c = fgetc(f)) != EOF) {
		if (i == MAX_LENGTH) {
			printf("Maximal statement length of %d character exceeded\n", MAX_LENGTH);
			return;
		}
		
		//ignore until \n or end of file
		if (c == ';') {
			while (c != EOF && c != '\n')
				c = fgetc(f);
			if (c == EOF)
				break;
		} 

		if (isspace(c) && parencount(buffer) <= 0) { // if balanced expression
			buffer[i] = '\0';

			int e = parus_eval(buffer, stk, lex);

			clear_buffer(buffer);
			i = 0;
			if (e != 0)
				return;
		}
		else  {
			if (c != '\n' && c != '\t' && c != '\r' && c != '\v' && c != '\f')
				buffer[i] = c;
			else
				buffer[i] = ' ';
			i++;
		}
	}

	buffer[i] = '\0';
	parus_eval(buffer, stk, lex);

}

void do_line(char* input, Stack* stk, Lexicon* lex) {
	char* buffer = malloc((strlen(input)+1)*sizeof(char));

	clear_buffer(buffer);

	int i = 0;
	int j = 0;
	int c;

	while ((c = input[i++]) != '\0') {
		if (c == ';') // comment escape line
			break;
		if (isspace(c) && parencount(buffer) <= 0) { // if balanced expression
			buffer[j] = '\0';

			int e = parus_eval(buffer, stk, lex);

			clear_buffer(buffer);
			j = 0;
			if (e != 0)
				break;
		}
		else  {
			buffer[j] = c;
			j++;
		}
	}

	buffer[j] = '\0';
	parus_eval(buffer, stk, lex);
	
	free(buffer);
}

int main(int argc, char** argv) {
	char 	norepl 		= 0;
	char 	help 		= 0;
	char 	notitle 		= 0;
	char* 	file_name 	= NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-norepl") == 0)
			norepl = 1;
		else if (strcmp(argv[i], "-help") == 0)
			help = 1;
		else if (strcmp(argv[i], "-notitle") == 0)
			notitle = 1;
		else if (file_exists(argv[i]) && file_name == NULL)
			file_name = argv[i];
		else 
			printf("File doesn't exists: %s\n", argv[i]);

	}

	if (help) {
		print_title();
		print_help();

		return 0;
	}

	Stack* 		stk = new_stack();
	Lexicon* 	lex = predefined_lexicon();

	if (file_name != NULL) {
		FILE* f = fopen(file_name, "r");
		if (f != NULL)
			do_file(f, stk, lex);
		else
			printf("Cannot open file\n");
	}

	if (!norepl && !notitle) 
		print_title();
	while (!norepl) {
		
		char* input = readline("CParus> ");
		add_history(input);
		if (!input)
			break;

		while (parencount(input) > 0) {
			char* addition = readline("...");
			add_history(addition);
			if (!addition)
				goto break_main_loop;


			int input_len 		= strlen(input) +1;
			int addition_len 	= strlen(addition) +1;

			input = realloc(input, input_len + addition_len);

			if (input == NULL) {
				printf("Cannot read command\n");
				exit(EXIT_FAILURE);
			}

			input[input_len-1] = ' '; // replace \0 with space
			for (int i = 0; i < addition_len; i++) 
				input[input_len +i] = addition[i];

			free(addition);
		}


		do_line(input, stk, lex);
		
		free(input);
	}
	break_main_loop:
	
	free_stack(stk);
	free_lexicon(lex);

	return 0;
}
