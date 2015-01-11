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
	WORD,
};

typedef struct keyword {
	char* word;
	enum keywordtype type;
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

	keyword *keyword_stream = checked_malloc(sizeof(keyword)*64);
	size_t keyword_iterator = 0;
	

	while(current_c != '\0') {
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
			char* word_start = keyword_stream[keyword_iterator].word;							
			for(; word_len > 0; word_start++, beginning_of_word++) {
				*word_start = *(input_stream+beginning_of_word);
				word_len--;
			} //Last character is automatically a zero byte
			keyword_iterator++;
			continue;
		}
		else if(token_char(&current_c)) {
			keyword_stream[keyword_iterator].word = NULL;
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
			keyword_iterator++;
			continue;
		}

	}
  	return sequence_stream;
}

// IMPORTANT!!!!!!!! Before calling, make sure you make a copy that's safe to modify of line_number

/* FOR REFERENCE

struct command {
	enum command_type type;

// Exit status, or -1 if not known (e.g., because it has not exited yet).
int status;

// I/O redirections, or null if none.
	char *input;
	char *output;

	union {
// For SIMPLE_COMMAND.
	char **word;

// For all other kinds of commands.  Trailing entries are unused.
// Only IF_COMMAND uses all three entries.
	struct command *command[3];
	} u;
};

*/

/*command_t make_command(const char * const sequence) {	// later do line number stuff, not yet pls
	command_t new_command = checked_malloc(sizeof(struct command));

	char const *token = get_pivot_token (sequence);
  	char const *first_string; 
 	char const *second_string;
  	char *first_command;
  	char *second_command;
  	size_t first_string_size;
  	size_t second_string_size;

  	enum command_type token_type = token_to_command(token);
	
  	if(token == NULL) // No tokens found, SIMPLE_COMMAND
   	 {
	     	 first_string = sequence;
	      	 first_string_size = strlen(first_string) + 1;
	      	 second_string = NULL;
	      	 second_string_size = 0;
    	 }
  	else
   	 {
	      first_string = sequence;
	      first_string_size = (token - first_string) + 1; // Leave space for the null byte

	      second_string = token + 1;
	      second_string_size = strlen(second_string) + 1;
	 }

	    new_command->type = token_type;
	    new_command->status = -1;
	    new_command->input = NULL;
	    new_command->output = NULL;

	    //Need to handle subshell commands 
	    switch (cmd->type)
	      {
		case SEQUENCE_COMMAND:
		case OR_COMMAND:
		case AND_COMMAND:
		case PIPE_COMMAND:
		  {
		    1st_command = checked_malloc (sizeof (char) * 1st_string);
		    memcpy (1st_command, 1st_string, 1st_string_size);
		    1st_command[1st_string_size-1] = '\0';

		    2nd_command = checked_malloc (sizeof (char) * 2nd_string_size);
		    strcpy (2nd_command, 2nd_string);

		    new_command->u.command[0] = make_command (1st_command);
		    new_command->u.command[1] = make_command (2nd_command);

		    free (1st_command);
		    free (2nd_command);
		    break;
		  }
		case SIMPLE_COMMAND:
		  {
		    char *stripped_expr = handle_and_strip_file_redirects (sequence, new_command, false);
		    split_expression_by_token (new_command, stripped_expr, ' ', p_line_number);
		    free(stripped_expr);
		    break;
		  }
		default: break;
	      } 
	  return new_command;
} */

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
