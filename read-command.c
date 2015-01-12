// UCLA CS 111 Lab 1 command reading

// Copyright 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

typedef struct command_stream {
	command_t *commands;
	int iter;
	int commands_size;
	int alloc_size;

}command_stream;

enum keywordtype {
	SEQUENCE,
	PIPELINE,
	OPEN_PARENS,
	CLOSE_PARENS,
	INPUT,
	OUTPUT,
	IF,
	THEN,
	ELSE,
	FI,
	WHILE,
	UNTIL,
	DO,
	DONE,
	WORD,
};

typedef struct keyword {
	char* word;
	enum keywordtype type;
	int line;
}keyword;


char *read_input(int (*get_next_byte) (void *), void *get_next_byte_argument) {
	char char_c;
	size_t sequence_alloc_size = 1024;
	size_t buf_iterator = 0;
	char *sequence_buf = checked_malloc(sequence_alloc_size*sizeof(char));
	bzero(sequence_buf, sequence_alloc_size*sizeof(char));
	while((char_c = get_next_byte(get_next_byte_argument)) != EOF) {
		sequence_buf[buf_iterator++] = char_c;
    		if (sequence_alloc_size == buf_iterator) {
		  int old_alloc_size = sequence_alloc_size;
     		  sequence_buf =  (char*) checked_grow_alloc(sequence_buf, &sequence_alloc_size);
		  bzero(sequence_buf+old_alloc_size, old_alloc_size*sizeof(char));
		}
	}

	return sequence_buf;
}

bool word_char(char c) {

	// check if c is alphanumeric or within allowed set of special characters

	if(isalnum(c))
		return true;
	else if (c == '!' || c == '%' || c == '+' || c == ',' || c == '-' || c == '.' || c == '/' || c == ':' || c == '@' || c == '^' || c == '_')
		return true;
	else
		return false;
}

// Possibly no longer necssary

char* append_char(char c, char *sequence, size_t *sequence_len, size_t *sequence_size) {
	// append character to sequence
	sequence[(*sequence_len)] = c;
	(*sequence_len)++;

	// check if we have filled allocated size, if so allocate more space
	if((*sequence_len) == (*sequence_size)) {
		(*sequence_size) += 512;
		checked_realloc(sequence, 512);
		bzero(sequence+(*sequence_len), 512);
	}

	return sequence;
}

bool token_char(char *sequence) {
	// token can only be one of 6 characters
	switch(*sequence) {
		case ';':
		case '|':
		case '(':
		case ')':
		case '<':
		case '>':
			return true;
		default:
			return false;
	}
}

void validate_stream(keyword *keyword_stream, size_t *keywords_size) {			// const stuff?
	int active_parens = 0;
	int iter = 0;

	enum keywordtype k_type_cur = keyword_stream[iter].type;
	enum keywordtype k_type_prev = k_type_cur;

	while(keyword_stream) {
		k_type_cur = keyword_stream[iter].type;
		switch(k_type_cur) {
			case OPEN_PARENS:
				active_parens++;
				k_type_prev = OPEN_PARENS;
				break;
			case CLOSE_PARENS:
				if(k_type_prev != WORD || iter == 0) {		// check this??
					fprintf(stderr, "')' must follow word.\n");
					exit(1);
				}
				active_parens--;
				k_type_prev = CLOSE_PARENS;
				if (active_parens < 0) {
					fprintf(stderr, "No '(' to match ')'.\n");		// give line number, probably make this a better message
					exit(1);
				}
				break;
			case SEQUENCE:
				if(iter == 0 || k_type_prev != WORD) {
					fprintf(stderr, "Sequence must follow word.\n");		// line number, make message better
					exit(1);
				}
				k_type_prev = SEQUENCE;
				break;
			case PIPELINE:
				if(k_type_prev != WORD || iter == 0) {		// check this??
					fprintf(stderr, "Pipeline must follow word.\n");
					exit(1);
				}
				k_type_prev = PIPELINE;
				break;
			case INPUT:
				if(k_type_prev != WORD || iter == 0) {		// check this??
					fprintf(stderr, "Input must follow word.\n");
					exit(1);
				}
				// Do we need to check if file exists?
				k_type_prev = INPUT;
				break;
			case OUTPUT:
				if(k_type_prev != WORD || iter == 0) {		// check this??
					fprintf(stderr, "Output must follow word.\n");
					exit(1);
				}
				k_type_prev = OUTPUT;
				break;
			case WORD:
				k_type_prev = WORD;
				break;
			default:
				break;
		}
		iter++;
	}
}

command_t
new_command () {
	command_t cmd = (command_t) checked_malloc(sizeof(struct command));
	cmd->type = SIMPLE_COMMAND; //default
	cmd->input = NULL;
	cmd->output = NULL;
	cmd->u.command[0] = NULL;
	cmd->u.command[1] = NULL;
	cmd->u.command[2] = NULL;
	cmd->u.word = NULL;
	return cmd;
}

command_stream_t keyword_2_command (keyword* keyword_stream, size_t keyword_stream_size) {

	keyword current_keyword;
	keyword next_keyword;

	size_t i= 0;
	for(; i< keyword_stream_size; i++) {
		current_keyword=keyword_stream[i];
		next_keyword=keyword_stream[i+1];
	}

}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument) {

	/* A sequence will be defined as one line of code, from beginning to end statement */

	/* Initialize the command_stream with 16 possible commands */
	command_stream_t sequence_stream = checked_malloc(sizeof(struct command_stream));
	sequence_stream->commands = checked_malloc(16*sizeof(command_t));
	sequence_stream->iter = 0;
	sequence_stream->commands_size = 0;
	sequence_stream->alloc_size = 16;

	char *input_stream = read_input(get_next_byte, get_next_byte_argument);
	char current_c = *input_stream;
	size_t input_iterator = 0;

	size_t current_line = 0;
	size_t num_of_open_parens = 0;
	size_t num_of_close_parens = 0;

	keyword *keyword_stream = checked_malloc(sizeof(struct keyword)*64);
	size_t keyword_iterator = 0;
	size_t keyword_alloc_size = 64;



	while((current_c = *(input_stream+input_iterator)) != '\0') {
		//("%c", current_c);
		//TODO Need to reallocate keyword_stream
		if (keyword_iterator == keyword_alloc_size)
		{
			keyword_alloc_size += 64;
			keyword_stream = checked_realloc (keyword_stream, keyword_alloc_size*sizeof(struct keyword));
			memset (keyword_stream + keyword_iterator, '\0', 64*sizeof(struct keyword));
		}
		switch(current_c) {
		     case '\n':
					current_line++;
					int counter = 1;
					while (input_stream[input_iterator+counter] == '\n') {
						current_line++;
		 	 			counter++;
					}
	   			input_iterator	+= counter-1;
					break;
		     /*Comments cannot occur before a word or token */
		     case '#':
	 				if (input_iterator != 0 && ((word_char(input_stream[input_iterator-1])) || (token_char(&input_stream[input_iterator-1])))) {
						fprintf(stderr, "A comment character (#) cannot follow a word character or token character"); //WE NEED THE LINES
						exit(1);
						continue;
					}
					counter = 1;
					while ((input_stream[input_iterator+counter] != '\n') && (input_stream[input_iterator+counter] != '\0'))
			  		counter++; //we are in a comment
					input_iterator	+= counter;
					continue;
		     case ' ':
		     case '\t':
					input_iterator++;
					continue;
		     default:
					break;
		}


		size_t beginning_of_word = input_iterator;
		if(word_char(current_c)) {
			size_t word_len = 1;
			while(word_char(*(input_stream+input_iterator))) {
				word_len++;
				input_iterator++;
			}
			keyword_stream[keyword_iterator].type = WORD;
			keyword_stream[keyword_iterator].word = checked_malloc(sizeof(char)*(word_len+1));
			bzero(keyword_stream[keyword_iterator].word, sizeof(char)*(word_len+1));
			size_t i=0;
			for(; i < word_len-1; i++) {
				keyword_stream[keyword_iterator].word[i]=*(input_stream+beginning_of_word+i);
			}
			char *word_temp = keyword_stream[keyword_iterator].word;
			if (strcmp(word_temp, "if") == 0){
					keyword_stream[keyword_iterator].type = IF;
					keyword_stream[keyword_iterator].word = NULL;
			}
			else if (strcmp(word_temp, "then") == 0) {
					keyword_stream[keyword_iterator].type = THEN;
					keyword_stream[keyword_iterator].word = NULL;
			}
			else if (strcmp(word_temp, "else") == 0) {
					keyword_stream[keyword_iterator].type = ELSE;
					keyword_stream[keyword_iterator].word = NULL;
			}
			else if (strcmp(word_temp, "fi") == 0) {
					keyword_stream[keyword_iterator].type = FI;
					keyword_stream[keyword_iterator].word = NULL;
			}
			else if (strcmp(word_temp, "while") == 0) {
					keyword_stream[keyword_iterator].type = WHILE;
					keyword_stream[keyword_iterator].word = NULL;
			}
			else if (strcmp(word_temp, "until") == 0) {
					keyword_stream[keyword_iterator].type = UNTIL;
					keyword_stream[keyword_iterator].word = NULL;
			}
			else if (strcmp(word_temp, "do") == 0) {
					keyword_stream[keyword_iterator].type = DO;
					keyword_stream[keyword_iterator].word = NULL;
		}
		else if (strcmp(word_temp, "done") == 0) {
					keyword_stream[keyword_iterator].type = DONE;
					keyword_stream[keyword_iterator].word = NULL;
		}
			keyword_stream[keyword_iterator].line=current_line;
			//printf("Index %d: %s\n", (int) keyword_iterator, keyword_stream[keyword_iterator].word);
			keyword_iterator++;
			current_c = *(input_stream+input_iterator);
			continue;
		}
		else if(token_char(&current_c)) {
			keyword_stream[keyword_iterator].word = NULL;
			keyword_stream[keyword_iterator].line=current_line;
			switch(current_c) {
				case ';':
					keyword_stream[keyword_iterator].type = SEQUENCE;
					break;
				case '|':
					keyword_stream[keyword_iterator].type = PIPELINE;
					break;
				case '(':
					num_of_open_parens++;
					keyword_stream[keyword_iterator].type = OPEN_PARENS;
					break;
				case ')':
					num_of_close_parens++;
					if(num_of_close_parens > num_of_open_parens){
						fprintf(stderr, "No matching '(' for ')'");
						exit(1); //Need to do line number
					}
					keyword_stream[keyword_iterator].type = CLOSE_PARENS;
					break;
				case '<':
					keyword_stream[keyword_iterator].type = INPUT;
					break;
				case '>':
					keyword_stream[keyword_iterator].type = OUTPUT;
					break;
				default:
					break;
			}
		//	printf("Index %d: %d\n", (int) keyword_iterator, (int) keyword_stream[keyword_iterator].type);
			keyword_iterator++;
			input_iterator++;
			continue;
		}
	input_iterator++; //Check later
	} //END-WHILE




/*	size_t i=0;
	for(;i<keyword_iterator;i++)
	{
			switch(	keyword_stream[i].type) {
				case SEQUENCE:
			 	printf("SEQUENCE\n");
				break;
				case PIPELINE:
				printf("PIPELINE\n");
				break;
				case OPEN_PARENS:
				printf("OPEN_PARENS\n");
				break;
				case CLOSE_PARENS:
				printf("CLOSE_PARENS\n");
				break;
				case INPUT:
				printf("INPUT\n");
				break;
				case OUTPUT:
				printf("OUTPUT\n");
				break;
				case WORD:
				printf("%s\n",keyword_stream[i].word);
				break;
				case IF:
				printf("IF\n");
				break;
				case ELSE:
				printf("ELSE\n");
				break;
				case THEN:
				printf("THEN\n");
				break;
				case FI:
				printf("FI\n");
				break;
				case WHILE:
				printf("WHILE\n");
				break;
				case UNTIL:
				printf("UNTIL\n");
				break;
				case DO:
				printf("DO\n");
				break;
				case DONE:
				printf("DONE\n");
				break;

			}
	} */ //Test keyword_stream
  	return sequence_stream;
}

enum command_type token_to_command(char *token) {
	// A simple command contains no tokens
	if(token == NULL)
		return SIMPLE_COMMAND;

	else if(*token == '|')
		return PIPE_COMMAND;

	else if(*token == ';')
		return SEQUENCE_COMMAND;

	else if(*token == '(' || *token == ')')
		return SUBSHELL_COMMAND;

	return SIMPLE_COMMAND; 		// dummy to appease compiler
}

command_t
read_command_stream (command_stream_t s)
{

  /* Reading a command returns only one command. If command is returned,
     we must increment the iterator to point to the next command. */

  //If the iterator reaches the end of the array of commands, return NULL
  if(s->iter == s->commands_size) {
	s->iter = 0;
	return NULL;
  }
  else
	return s->commands[s->iter++];
}
