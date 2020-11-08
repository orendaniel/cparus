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

#define TEXT_BUFFER 1024

#include "parus.h"
#include "parus_predefined.h"

#ifdef USE_READLINE

#include <readline/readline.h>
#include <readline/history.h>

#endif

#ifndef USE_READLINE

/* a replacement for gnu readlie */
char* readline(const char* prompt) { 
	char* line = malloc(TEXT_BUFFER * sizeof(char));

	if (line == NULL)
		return NULL;

	printf("%s", prompt);

	if (fgets(line, TEXT_BUFFER, stdin) != NULL)
		return line;
	else {
		free(line);
		return NULL;
	}

}

#endif

char* read_file(FILE* f) {
	int 	size 	= 0;
	int 	max 	= TEXT_BUFFER;
	char* 	text 	= calloc(max, sizeof(char));

	int c = 0;
	while ((c = fgetc(f)) != EOF) {
		if (size != max - 1) 
			text[size++] = c;
		else {
			text = realloc(text, (max + TEXT_BUFFER) * sizeof(char));
			if (text != 0) {
				max += TEXT_BUFFER;
				text[size++] = c;
			}
		}
	}
	return text;
}

char* repl_read() {
	char* input = readline("CParus> ");
	
	#ifdef USE_READLINE
	add_history(input);
	#endif

	if (!input)
		return NULL;

	while (parus_validate_expression(input) > 0) {
		char* addition = readline("... ");

		#ifdef USE_READLINE
		add_history(input);
		#endif

		if (!addition)
			return NULL;

		int input_len 		= strlen(input) +1;
		int addition_len 	= strlen(addition) +1;

		input = realloc(input, input_len + addition_len);

		if (input == NULL) {
			fprintf(stderr, "CANNOT READ COMMAND\n");
			exit(EXIT_FAILURE);
		}

		input[input_len-1] = '\n'; // replace \0 with a space
		for (int i = 0; i < addition_len; i++) 
			input[input_len +i] = addition[i];

		free(addition);
	}

	return input;

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
		else if (file_name == NULL)
			file_name = argv[i];

	}

	if (help) {
		printf(TITLE_MESSAGE);
		printf(HELP_MESSAGE);

		return 0;
	}

	Stack*		stk = make_stack();
	Lexicon* 	lex = predefined_lexicon();

	if (file_name != NULL) {
		FILE* f = fopen(file_name, "r");
		if (f != NULL) {
			char* text = read_file(f);
			parus_evaluate(text, stk, lex);
			free(text);
		}
		else
			fprintf(stderr, "CANNOT OPEN FILE %s\nMAKE SURE THAT THE FILE EXISTS\n", file_name);
	}

	if (!norepl && !notitle) 
		printf(TITLE_MESSAGE);

	while (!norepl) {
		
		char* input = repl_read();

		if (input == NULL)
			break;

		parus_evaluate(input, stk, lex);
		
		free(input);
	}

	free_stack(stk);
	free_lexicon(lex);

	return 0;
}
