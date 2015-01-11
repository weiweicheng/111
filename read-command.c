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
			; //idk, but note that newline can only follow ; | ( ) and simple commands

		// Look at word chars?
		else if(word_char(sequence[i])) {	// likely, you skip over words in the validating stage

		if(i[0] == '\n')
			; //idk

		// Look at word chars?
		else if(word_char(i[0])) {
			after_token = false;
			in_word = true;
			continue;
		}

		else if(token_char(sequence[i])) {
			if(!in_word && sequence[i] != '(' && *sequence != ';')
				return false; 		// token cannot be first thing? except open paren&sequence? can we ever have a token directly following a token? certainly cannot have semicolon following semicolon

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

command_type token_to_command(char *token) {
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

char * lowest_precedence_token(char const* sequence) {
	// In order of precedence (high to low): (), |, ;		IS SIMPLE HIGHEST?
	
	// First, look for lowest precedence

}

// IMPORTANT!!!!!!!! Before calling, make sure you make a copy that's safe to modify of line_number & pass that in

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

	/*char const *token = get_pivot_token (sequence);
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

	    /* Need to handle subshell commands 
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
	      } */
	  return new_command;
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
