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


// copied from parus.c
int parencount(char* str) {
	int result = 0;
	int i = 0;
	while (str[i] != '\0') {
		if (str[i] == '(' && (str[i +1] == '\0' || isspace(str[i +1])))
			result++;

		else if (str[i] == ')' && (str[i +1] == '\0' || isspace(str[i +1])))
			result--;
		i++;

	}   
	return result;
}

char file_exists(char* filename) {
	struct stat buffer;   
	return stat(filename, &buffer) == 0;
}

void clear_buffer(char* buffer) {
    int i = 0;
    while (buffer[i] != '\0') {
        buffer[i] = '\0';
		i++;
	}
}


void print_title() {
	printf("CParus version 0.55\n");
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

int main(int argc, char** argv) {
	char 	norepl 		= 0;
	char 	help 		= 0;
	char 	notitle 	= 0;
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

	/*if (file_name != NULL) {
		FILE* f = fopen(file_name, "r");
		if (f != NULL)
			do_file(f, stk, lex);
		else
			printf("Cannot open file\n");
	}*/

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


		parus_evaluate(input, stk, lex);
		
		free(input);
	}
	break_main_loop:

	free_stack(stk);
	free_lexicon(lex);

	return 0;
}
