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
	size_t size;
	int iter;
	char **iplist;
	char **oplist;
	command_t command;
	struct command_stream* next;


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
	NEWLINE,
	ERROR,
};

typedef struct keyword {
	char* word;
	enum keywordtype type;
	int line;
}keyword;

typedef struct keyword_node {
	struct keyword* data;
	struct keyword_node* next;
	struct keyword_node* prev;
} keyword_node;

keyword_node *key_stack = NULL;
command_t *cmd_stack = NULL;

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

void validate_stream(keyword_node *root) {			// const stuff?
	int active_parens = 0;
	int iter = 0;

	enum keywordtype k_type_cur;
	enum keywordtype k_type_prev;
	keyword_node *current = root;
	int in_if = 0;
	int in_while = 0;
	int in_until = 0;


	while(current) {
		k_type_cur = current->data->type;
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
			case IF:
				k_type_prev = IF;
				in_if++;
				break;
			case ELSE:
				if (in_if==0) {
					fprintf(stderr, "Line %d: Not in an if statement", current->data->line);
					exit(1);
				}
				k_type_prev = ELSE;
				break;
			case FI:
				if(in_if==0){
					fprintf(stderr, "Line %d: Not in an if statement", current->data->line);
					exit(1);
				}
				k_type_prev = FI;
				in_if--;
				break;
			case WHILE:
				k_type_prev = WHILE;
				in_while++;
				break;
			case UNTIL:
				k_type_prev = UNTIL;
				in_until++;
				break;
			case DO:
				if(in_while==0 && in_until==0){
					fprintf(stderr, "Line %d: Not in an if statement", current->data->line);
					exit(1);
				}
				k_type_prev = DO;
				break;
			case DONE:
				//NEED TO CHECK IF MOST RECENT IS WHILE OR UNTIL
				k_type_prev=DO;
			case WORD:
				k_type_prev = WORD;
				break;
			default:
				break;
		}
		current = current->next;
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

void cmd_push(command_t cmd, int *top, size_t* cmd_stack_size) {
	if (cmd == NULL)
		return;
	if (*cmd_stack_size == ((*top+1)*sizeof(command_t)))
			cmd_stack = (command_t*) checked_grow_alloc(cmd_stack, cmd_stack_size);
	cmd_stack[++*top] = cmd;
}

command_t cmd_pop (int *top) {
	command_t cmd = NULL;
	if (*top >= 0) { //not empty
		cmd = cmd_stack[*top];
		if (*top)
			--*top;
		else
				*top = -1;
		}
	return cmd;
}

command_t cmd_merge (command_t cmd1, command_t cmd2, keyword_node* keyword) {
	if (!keyword) {
		fprintf(stderr, "Syntax error - command type unidentified\n");
		exit(1);
	}
	command_t cmd = NULL;
	if (!(cmd1 && cmd2)) {
		char *ctype = NULL;
		if (keyword->data->type == SEQUENCE)
			ctype = ";";
		if (keyword->data->type == PIPELINE)
			ctype = "|";
		if (keyword->data->type == IF)
			ctype = "i";
		if (keyword->data->type == WHILE)
			ctype = "w";
		if (keyword->data->type == UNTIL)
			ctype = "u";
		else
			fprintf(stderr, "Syntax error - command type incorrect\n");

		if (cmd1)
			fprintf(stderr, "Syntax error - RHS of %s command missing\n",ctype);
		else
			fprintf(stderr, "Syntax error - LHS of %s command missing\n",ctype);
			exit(1);
		}

		cmd = new_command();
		cmd->u.command[0] = cmd1;
		cmd->u.command[1] = cmd2;

		if (keyword->data->type == SEQUENCE)
			cmd->type = SEQUENCE_COMMAND;
		if (keyword->data->type == PIPELINE)
			cmd->type = PIPE_COMMAND;
		if (keyword->data->type == IF) //Needs to consider else
			cmd->type = IF_COMMAND;
		if (keyword->data->type == WHILE)
			cmd->type = WHILE_COMMAND;
		if (keyword->data->type == UNTIL)
			cmd->type = UNTIL_COMMAND;
		return cmd;
}

void node_push (keyword_node* keyword) {
	//printf("tspushed %d\n", item->m_token.type);
	if (keyword == NULL)
		return;
	if (key_stack == NULL) { //
			key_stack = keyword;
			key_stack->prev = key_stack->next = NULL;
		}
	else {
			keyword->next = key_stack;
			keyword->prev = NULL;
			key_stack->prev = keyword;
			key_stack = keyword;
		}
	}

enum keywordtype node_type_peek() {
		if (key_stack)
			return key_stack->data->type;
		else
			return ERROR;
}

int stackPrec(enum keywordtype keyword) {
	return -1;
}

int streamPrec(enum keywordtype keyword) {
	return -1;
}


keyword_node* node_pop () {
	keyword_node* node_top = NULL;
	if (key_stack) {
		node_top = key_stack;
		key_stack = key_stack->next;
		node_top->next = node_top->prev = NULL;
		if(key_stack)
			key_stack->prev = NULL;
		}
	return node_top;
}

command_stream_t cmd_stream_append (command_stream_t cmd_stream, command_stream_t cmd) {
	//printf("cappend\n");
	if (!cmd)
		return cmd_stream;
		if (!cmd_stream) { //empty
			cmd_stream = cmd;
			cmd->next = NULL;
			cmd->size = 1;
		}
		else {
			command_stream_t cmd_stream_loop = cmd_stream;
			size_t counter = 1;
			while (cmd_stream_loop->next) {
				cmd_stream_loop->size = counter++;
				cmd_stream_loop = cmd_stream_loop->next;
			}
			cmd_stream_loop->size = counter++;
			cmd_stream_loop->next = cmd;
			cmd->size = counter;
			cmd->next = NULL;
		}
		return cmd_stream;
	}

command_stream_t token_2_command_stream (keyword_node *keyword_stream) {
	struct keyword_node* current_keyword, next_keyword;
	command_t ct_temp1, ct_temp2, cmd1, cmd2;
	command_stream_t cmd_stream_temp, cmd_stream;

	current_keyword = keyword_stream;
	ct_temp1 = ct_temp2 = cmd1 = cmd2 = NULL;
	cmd_stream_temp = cmd_stream = NULL;

	char **simple_command_a = NULL;
	int paren_open = 0;
	int if_open = 0;
	int while_open = 0;
	int until_open =0;

	int top = -1;
	size_t ctstacksize = 16*sizeof(command_t);
	cmd_stack = (command_t*) checked_malloc(ctstacksize);

	while(current_keyword) {
		next_keyword = *(current_keyword->next);
		switch(current_keyword->data->type) {
			case OPEN_PARENS:
				cmd_push (ct_temp1, &top, &ctstacksize);
				ct_temp1 = NULL;
				simple_command_a = NULL;
				paren_open++;
				node_push(current_keyword);
				break;
			case CLOSE_PARENS:
				if (!paren_open) {
					fprintf(stderr, "No matching '(' for ')'\n");
					exit(1);
				}
				cmd_push (ct_temp1, &top, &ctstacksize);
				paren_open--;
				while (node_type_peek() != OPEN_PARENS) {
					if (node_type_peek() == ERROR) {
						fprintf(stderr, "Shell command syntax error, unmatched ')'\n");
						exit(1);
					}
					cmd2 = cmd_pop(&top);
					cmd1 = cmd_pop(&top);
					ct_temp1 = cmd_merge(cmd1, cmd2, node_pop());
					cmd_push (ct_temp1, &top, &ctstacksize);
					ct_temp1 = NULL;
				}
				ct_temp2 = new_command();
				ct_temp2->type = SUBSHELL_COMMAND;
				ct_temp2->u.command[0] = cmd_pop(&top);
				cmd_push (ct_temp2, &top, &ctstacksize);
				ct_temp1 = ct_temp2 = NULL;
				simple_command_a = NULL;
				node_pop();
				break;
			case WORD:
				if (!ct_temp1) {
					ct_temp1 = new_command();
					simple_command_a = (char **) checked_malloc(160*sizeof(char*));
					ct_temp1->u.word = simple_command_a;
				}
				*simple_command_a = current_keyword->data->word;
				*++simple_command_a = NULL;
				break;
			case OUTPUT:
				if (!ct_temp1) {
					ct_temp1 = cmd_pop(&top);
					if (!ct_temp1) {
						fprintf(stderr,"Syntax incorrect for OP REDIR command\n");
						exit(1);
					}
				}
				if (current_keyword->next != NULL && next_keyword.data->type == WORD) {
					ct_temp1->output = next_keyword.data->word;
					cmd_push (ct_temp1, &top, &ctstacksize);
					ct_temp1 = NULL;
					simple_command_a = NULL;
					current_keyword = &next_keyword;
					if (current_keyword)
						next_keyword = *(current_keyword->next);
					}
				break;

			case INPUT:
				if (!ct_temp1) {
					ct_temp1 = cmd_pop(&top);
					if (!ct_temp1) {
						fprintf(stderr,"Syntax incorrect for IP REDIR command\n");
						exit(1);
					}
				}
				if (current_keyword->next && next_keyword.data->type == WORD) {
					ct_temp1->input = next_keyword.data->word;
					cmd_push (ct_temp1, &top, &ctstacksize);
					ct_temp1 = NULL;
					simple_command_a = NULL;
					current_keyword = &next_keyword;
					if (current_keyword)
							next_keyword = *(current_keyword->next);
				}
				break;

			case PIPELINE:
			case SEQUENCE:
				cmd_push (ct_temp1, &top, &ctstacksize);
				while (stackPrec(node_type_peek()) > streamPrec(current_keyword->data->type)) {
					cmd2 = cmd_pop(&top);
					cmd1 = cmd_pop(&top);
					ct_temp1 = cmd_merge(cmd1, cmd2, node_pop());
					cmd_push (ct_temp1, &top, &ctstacksize);
				}
				ct_temp1 = NULL;
				simple_command_a = NULL;
				node_push(current_keyword);
				break;
			case NEWLINE:
				cmd_push (ct_temp1, &top, &ctstacksize);
				while (stackPrec(node_type_peek()) > streamPrec(current_keyword->data->type)) {
					cmd2 = cmd_pop(&top);
					cmd1 = cmd_pop(&top);
					ct_temp1 = cmd_merge(cmd1, cmd2, node_pop());
					cmd_push (ct_temp1, &top, &ctstacksize);
				}
				if (!paren_open) {
					cmd_stream_temp = (command_stream_t) checked_malloc(sizeof(command_stream));
					cmd_stream_temp->command = cmd_pop(&top);
					cmd_stream = cmd_stream_append(cmd_stream, cmd_stream_temp);
				}
				ct_temp1 = NULL;
				simple_command_a = NULL;
				break;
			default:
				break;
		}
		cmd_push (ct_temp1, &top, &ctstacksize);
		while (node_type_peek() != ERROR) {
			cmd2 = cmd_pop(&top);
			cmd1 = cmd_pop(&top);
			ct_temp1 = cmd_merge(cmd1, cmd2, node_pop());
			cmd_push (ct_temp1, &top, &ctstacksize);
		}
		cmd_stream_temp = (command_stream_t) checked_malloc(sizeof(command_stream));
		cmd_stream_temp->command = cmd_pop(&top);

		cmd_stream = cmd_stream_append(cmd_stream, cmd_stream_temp);
		ct_temp1 = NULL;
		if (cmd_pop(&top)) {
			fprintf(stderr,"Syntax error\n");
			exit(1);
		}
		return cmd_stream;

	}
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument) {

	/* A sequence will be defined as one line of code, from beginning to end statement */

	/* Initialize the command_stream with 16 possible commands */

	char *input_stream = read_input(get_next_byte, get_next_byte_argument);
	char current_c = *input_stream;
	size_t input_iterator = 0;

	size_t current_line = 1;
	size_t num_of_open_parens = 0;
	size_t num_of_close_parens = 0;

	keyword_node *root = NULL;
	keyword_node *cur = NULL;

	cur = root;

	enum keywordtype type;

	while((current_c = *(input_stream+input_iterator)) != '\0') {
		switch(current_c) {
		     case '\n':
					type = NEWLINE;
					current_line++;
					int counter = 1;
					while (input_stream[input_iterator+counter] == '\n') {
						current_line++;
		 	 			counter++;
					}
	   			input_iterator +=counter-1;
					break;
		     /*Comments cannot occur before a word or token */
		     case '#':
	 				if (input_iterator != 0 && ((word_char(input_stream[input_iterator-1])) || (token_char(&input_stream[input_iterator-1])))) {
						fprintf(stderr, "A comment character (#) cannot follow a word character or token character"); //WE NEED THE LINES
						exit(1);
						continue;
					}
					counter = 1;
					while ((input_stream[input_iterator+counter] != '\0') && (input_stream[input_iterator+counter] != '\n'))
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
		size_t word_len = 1;
		char* word = NULL;
		if(word_char(current_c)) {
			type = WORD;
			while (word_char(*(input_stream+input_iterator+word_len)))
				word_len++;
			input_iterator += word_len-1;
			word= checked_malloc(sizeof(char)*(word_len+1));
			bzero(word, sizeof(char)*(word_len+1));
			size_t i=0;
			for(; i < word_len; i++) {
				word[i]=*(input_stream+beginning_of_word+i);
			}
			if (strcmp(word, "if") == 0 && (cur->data->type == NEWLINE || cur->data->type == PIPELINE || cur->data->type == SEQUENCE)){
				cur->data->type = IF;
				free(word);
				word = NULL;
			}
			else if (strcmp(word, "then") == 0) {
				cur->data->type = THEN;
				free(word);
				word = NULL;
			}
			else if (strcmp(word, "else") == 0) {
				cur->data->type = ELSE;
				free(word);
				word = NULL;
			}
			else if (strcmp(word, "fi") == 0) {
				cur->data->type = FI;
				free(word);
				word = NULL;
			}
			else if (strcmp(word, "while") == 0 && (cur->data->type == NEWLINE || cur->data->type == PIPELINE || cur->data->type == SEQUENCE)) {
				cur->data->type = WHILE;
				free(word);
				word = NULL;
			}
			else if (strcmp(word, "until") == 0 && (cur->data->type == NEWLINE || cur->data->type == PIPELINE || cur->data->type == SEQUENCE)) {
				cur->data->type = UNTIL;
				free(word);
				word = NULL;
			}
			else if (strcmp(word, "do") == 0) {
				cur->data->type = DO;
				free(word);
				word = NULL;
			}
			else if (strcmp(word, "done") == 0) {
				cur->data->type = DONE;
				free(word);
				word = NULL;
			}
		}


		if(token_char(&current_c)) {
			word = NULL;
			switch(current_c) {
				case ';':
					type = SEQUENCE;
					break;
				case '|':
					type = PIPELINE;
					break;
				case '(':
					num_of_open_parens++;
					type = OPEN_PARENS;
					break;
				case ')':
					num_of_close_parens++;
					if(num_of_close_parens > num_of_open_parens){
						fprintf(stderr, "No matching '(' for ')'");
						exit(1); //Need to do line number
					}
					type = CLOSE_PARENS;
					break;
				case '<':
					type = INPUT;
					break;
				case '>':
					type = OUTPUT;
					break;
				default:
					break;
			}
		}


		keyword_node* temp = (keyword_node*) checked_malloc(sizeof(keyword_node));
		temp->data = checked_malloc(sizeof(keyword));
		temp->next = NULL;
		temp->prev = NULL;
		temp->data->type = type;
		temp->data->line = current_line;
		temp->data->word = word;

		if (cur == NULL) {
			root = temp;
			cur = root;
		} else {
			cur -> next = temp;
			temp -> prev = cur;
			cur = cur-> next;
		}

		input_iterator++; //Check later
	} //END-WHILE

/*	cur = root;
	int i = 0;
	while(cur->next != NULL) {
		if(cur->data->word)
			printf("Keyword %d: %s\n", (int) i, cur->data->word);
		else
			switch(cur->data->type) {
				case SEQUENCE:
					printf("Keyword %d: SEQUENCE\n", i);
					break;
				case PIPELINE:
					printf("Keyword %d: PIPELINE\n", i);
					break;
				case OPEN_PARENS:
					printf("Keyword %d: OPEN_PARENS\n", i);
					break;
				case CLOSE_PARENS:
					printf("Keyword %d: CLOSE_PARENS\n", i);
					break;
				case INPUT:
					printf("Keyword %d: INPUT\n", i);
					break;
				case OUTPUT:
					printf("Keyword %d: OUTPUT\n", i);
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
				case NEWLINE:
					printf("NEWLINE\n");
					break;
			}
		cur = cur->next;
		i++;
	} */

		//VALIDATE
		command_stream_t sequence_stream = token_2_command_stream(root);

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

	if (s->iter == 0){
		s->iter = 1;
		return s->command;
	}
	else if (s->next != NULL){
		return read_command_stream(s->next);
	}
	return NULL;
}
