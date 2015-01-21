// UCLA CS 111 Lab 1 command execution

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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

void setup_io(command_t c); //Done
void execute_simple(command_t c, int profiling); //Done
void execute_sequence(command_t c, int profiling); //Done
void execute_if(command_t c, int profiling); //Done
void execute_while(command_t c, int profiling); //Done
void execute_until(command_t c, int profiling);
void execute_pipe(command_t c, int profiling); //Done
void execute_subshell(command_t c, int profiling); //Done

int
prepare_profiling (char const *name)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  error (0, 0, "warning: profiling not yet implemented");
  return -1;
}

void setup_io(command_t c)
{
  // Check for an input characteristic, which can be read
  if(c->input)
  {
    int fd_input = open(c->input, O_RDWR);
    if( fd_input < 0)
      error(1, 0, "Cannot read input file: %s\n", c->input);

    if( dup2(fd_input, 0) < 0)
      error(1, 0, "Cannot redirect STDIN to input file: %s\n", c->input);

    if( close(fd_input) < 0)
      error(1, 0, "Cannot close input file: %s\n", c->input);
    }

    // Check for an output characteristic, which can be read and written on,
    // and if it doesn't exist yet, should be created
    if(c->output)
    {
      // Be sure to set flags
      int fd_output = open(c->output, O_CREAT | O_WRONLY | O_TRUNC,
      S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
      if( fd_output < 0)
      error(1, 0, "Cannot read output file: %s\n", c->output);

      if( dup2(fd_output, 1) < 0)
        error(1, 0, "Cannot redirect STDOUT to output file: %s\n", c->output);

      if( close(fd_output) < 0)
          error(1, 0, "Cannot close output file: %s\n", c->output);
    }
}

void execute_sequence(command_t c, int profiling)
{
  int status;
  pid_t pid = fork();
  if(pid > 0)
  {
    // Parent process
    waitpid(pid, &status, 0);
    c->status = status;
  }
  else if(pid == 0) {
  //Child process
    pid = fork();
    if( pid > 0) {
      waitpid(pid, &status, 0);
      execute_command(c->u.command[1], profiling);
      _exit(c->u.command[1]->status);
    }
    else if( pid == 0)
    {
      // The child of the child now runs
      execute_command(c->u.command[0], profiling);
      _exit(c->u.command[0]->status);
    }
    else
      error(1, 0, "Could not fork child process.\n");
  }
  else
    error(1, 0, "Could not fork parent process.\n");
}

void execute_while(command_t c, int profiling) {
  //TODO: Check false case
  while(c->u.command[0]->status == 0)
  {
    execute_command(c->u.command[1], profiling);
    c->status = c->u.command[1]->status;
  }

}

void execute_until (command_t c, int profiling) {
  //TODO: Needs to be fixed

  do
  {
    execute_command(c->u.command[1], profiling);
    c->status = c->u.command[1]->status;
  } while ((c->u.command[0]->status != 0));

}

void
execute_pipe (command_t c, int profiling)
{
  int status;
  int fd[2];
  pid_t return_pid;
  pid_t pid_1;
  pid_t pid_2;

  if ( pipe(fd) == -1 )
    error (1, errno, "Pipe could not be created.\n");
  pid_1 = fork();
  if( pid_1 > 0 ) { //Parent process
    pid_2 = fork();
    if( pid_2 > 0 ) { //Parent process
      //Close inputs and outputs of parent process
      close(fd[0]);
      close(fd[1]);
      // Wait for any process to finish
      return_pid = waitpid(-1, &status, 0);
      if( return_pid == pid_1 ) {
        c->status = status;
        waitpid(pid_2, &status, 0);
        return;
      }
      else if(return_pid == pid_2) {
        waitpid(pid_1, &status, 0);
        c->status = status;
        return;
      }
    }
    else if( pid_2 == 0 ) {
      // The 2nd child now runs, left command of the pipe
      close(fd[0]);
      if( dup2(fd[1], 1) < 0 )
        error (1, errno,  "There was an error redirecting STDOUT.\n");
      execute_command(c->u.command[0], profiling);
      _exit(c->u.command[0]->status);
    }
    else
      error(1, 0, "Could not fork\n");
  }
  else if( pid_1 == 0) {
    // First child, right command in the pipe
    close(fd[1]);
    if( dup2(fd[0], 0) < 0 )
      error (1, errno,  "There was an error redirecting STDIN\n");
    execute_command(c->u.command[1], profiling);
    _exit(c->u.command[1]->status);
  }
  else
    error(1, 0, "Could not fork");
}

void execute_subshell(command_t c, int profiling) {
  setup_io(c);
  execute_command(c->u.command[0], profiling);
  c->status = c->u.command[0]->status;
}

void execute_simple(command_t c, int profiling)
{
  int status;
  pid_t pid = fork();

  switch(pid)
  {
  case -1:
    error(1, 0, "Could not fork proccess");
    break;
  case 0: //child process
    setup_io(c);
    if(c->u.word[0][0] == ':')
      _exit(0);
    // Execute the simple command program
    execvp(c->u.word[0], c->u.word );
    error(1, 0, "Simple command '%s' not found\n", c->u.word[0]);
    break;
  default:
    waitpid(pid, &status, 0);
    c->status = status;
    break;
  }
}


int
command_status (command_t c)
{
  return c->status;
}

void execute_if(command_t c, int profiling)
{
  execute_command(c->u.command[0], profiling);
  if (c->u.command[0]->status == 0)		// a is true
  {
    execute_command(c->u.command[1], profiling);
    c->status = c->u.command[1]->status;
  }
  else if (c->u.command[2]){				// if a is false and there is else clause
    execute_command(c->u.command[2], profiling);
    c->status = c->u.command[2]->status;
  }
}

void
execute_command (command_t c, int profiling)
{

  switch(c->type)
  {
    case IF_COMMAND:
      execute_if(c, profiling);
      break;
    case WHILE_COMMAND:
      execute_while(c, profiling);
      break;
    case UNTIL_COMMAND:
      execute_until(c, profiling);
      break;
    case SEQUENCE_COMMAND:
      execute_sequence(c, profiling);
      break;
    case PIPE_COMMAND:
      execute_pipe(c, profiling);
      break;
    case SIMPLE_COMMAND:
      execute_simple(c, profiling);
      break;
    case SUBSHELL_COMMAND:
      execute_subshell(c, profiling);
      break;
    default:
      error(1, 0, "Invalid command type");
    }

}
