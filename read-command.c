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
		checked_realloc(*sequence, 512);
		bzero(sequence+(*sequence_len), 512);
	}

	return sequence;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument) {

	/* A sequence will be defined as one line of code, from beginning to end statement */

	/* Initialize the command_stream with 16 possible commands */
	command_stream_t sequence_stream = checked_malloc(sizeof(struct command_stream));
	sequence_stream->commands = checked_malloc(16*sizeof(struct command_t));
	sequence_stream->iter = 0;
	sequence_stream->commands_size = 0;
	sequence_stream->alloc_size = 16;

	/* We need to create a buffer for the input that we are reading in.*/

	size_t sequence_buf_size = 512;
	size_t sequence_processed_size = 0;

	char *sequence_buf = checked_malloc(sequence_buf_size*sizeof(char));
	bzero(sequence_buf, sequence_buf_size*sizeof(char));

	char current;
	size_t total_lines_processed = 0;
	size_t current_line = 0;
	size_t num_of_left_parens = 0;
	size_t num_of_right_parens = 0;

	bool in_comment = false;

	while((current = get_next_byte(get_next_byte_argument)) != EOF) {
		/* A newline at the beginning of a sequence should be skipped */
		if(current == '\n' && sequence_processed_size == 0 && in_comment == false) {
			total_lines_processed++;
			continue;
		}

		else if(current == '\n' && num_of_left_parens > 0 && num_of_left_parens == num_of_right_parens) {
			total_lines_processed++;
			continue;
		}
	}
  	return 0;
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
