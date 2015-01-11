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

bool valid_seq(char * const sequence) {
	// Check to make sure there are no invalid characters
	char *i;
	for(i = &sequence[0]; *i; i++) {		// sequence is terminated by null byte
		// check for invalid?? tokens and words do not share characters (all are distinct)

		bool in_word = false; 		// Keep track of if currently in a word
		bool after_token = false;	// See if directly prior, have seen token

		// Spaces are ignored
		if(i[0] == ' ')
			continue;		// make sure you don't have to exit word here

		if(sequence[i] == '\n')
			; //idk

		// Look at word chars?
		else if(word_char(i[0])) {
			after_token = false;
			in_word = true;
			continue;
		}

		else if(token_char(i)) {
			if(!in_word && i[0] != '(' && *sequence != ';')
				return false; 		// token cannot be first thing? except open paren&sequence? can we ever have a token directly following a token?
			in_word = false;
			after_token = true;
			continue;
		}
	}

	// Check to make sure number of parens match, and that the order is sensical
	int open_paren = 0;
	int close_paren = 0;
	for(i = sequence; i[0] != '\0' || close_paren > open_paren; i++) {	// second exit condition may not be needed. check this! depends on implementation probably
		if(i[0] == '(')
			open_paren++;
		if(i[0] == ')')
			close_paren++;
	}
	return false;			// to appease compiler for now
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

	/* We need to create a buffer for the input that we are reading in.*/

	size_t sequence_buf_size = 512;
	size_t sequence_processed_size = 0;

	char *sequence_buf = checked_malloc(sequence_buf_size*sizeof(char));
	bzero(sequence_buf, sequence_buf_size*sizeof(char));

	char current_c;
	size_t total_lines_processed = 0;
	size_t current_line = 0;
	size_t num_of_left_parens = 0;
	size_t num_of_right_parens = 0;

	bool in_comment = false;

	while((current_c = get_next_byte(get_next_byte_argument)) != EOF) {
		/* A newline at the beginning of a sequence should be skipped */
		if(current_c == '\n' && sequence_processed_size == 0 && in_comment == false) {
			total_lines_processed++;
			continue;
		}

		else if(current_c == '\n' && num_of_left_parens > 0 && num_of_left_parens == num_of_right_parens) {
			total_lines_processed++;
			sequence_buf = append_char(';', sequence_buf, &sequence_processed_size, &sequence_buf_size);
			sequence_buf = append_char(current_c, sequence_buf, &sequence_processed_size, &sequence_buf_size);
			continue;
		}

		else if(current_c != '\n' && current_c != '#' && in_comment == false)
		{
			sequence_buf = append_char(current_c, sequence_buf, &sequence_processed_size, &sequence_buf_size);
			if (current_c == '(')
				{
					num_of_left_parens++;
				}
			else if (current_c == ')')
				{
					num_of_right_parens++;
				}
			continue;
		}
		else if(current_c == '#')
		{
			//if previous character was a word or token, not a comment
			if (word_char(*(sequence_buf+sequence_processed_size-1)) || token_char((sequence_buf+sequence_processed_size-1)))
				sequence_buf = append_char(current_c, sequence_buf, &sequence_processed_size, &sequence_buf_size);
			else
				in_comment = true;
		}

		else if (current_c == '\n' || (current_c == ';' && num_of_left_parens == num_of_right_parens))
		{
			if (current_c == '\n')
				current_line++;

			if (current_c == '\n' && in_comment)
			{
				in_comment = false;
				sequence_buf = append_char(current_c, sequence_buf, &sequence_processed_size, &sequence_buf_size);
			}
			else if (current_c == ';' && in_comment)
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

command_t make_command(const char * const sequence) {	// later do line number stuff, not yet pls
	command_t new_command = checked_malloc(sizeof(struct command));
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
