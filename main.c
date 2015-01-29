// UCLA CS 111 Lab 1 main program

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

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <sys/resource.h>
#define BILLION 1E9
#define MILLION 1E6

#include <sys/types.h>
#include <unistd.h>

#include "command.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-p PROF-FILE | -t] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  int command_number = 1;
  bool print_tree = false;
  char const *profile_name = 0;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "p:t"))
      {
      case 'p': profile_name = optarg; break;
      case 't': print_tree = true; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);
  int profiling = -1;
  if (profile_name)
  {
      profiling = prepare_profiling (profile_name);
      if (profiling < 0)
	       error (1, errno, "%s: cannot open", profile_name);
  }

  struct timespec func_start, func_end, realClk;
  struct rusage usage;
  struct timeval sys_start, sys_end, user_start, user_end;

  if(profiling != -1) {
    clock_gettime(CLOCK_MONOTONIC, &func_start);


    getrusage(RUSAGE_CHILDREN, &usage);
    sys_start = usage.ru_stime;
    user_start = usage.ru_utime;

  }

  command_t last_command = NULL;
  command_t command;
  while ((command = read_command_stream (command_stream)))
  {
      if (print_tree)
	    {
	        printf ("# %d\n", command_number++);
	        print_command (command);
	    }
      else
	    {
         last_command = command;
         execute_command (command, profiling);
	    }
  }

  if (profiling !=-1) {
    getrusage(RUSAGE_CHILDREN, &usage);
    sys_end = usage.ru_stime;
    user_end = usage.ru_utime;

    clock_gettime(CLOCK_MONOTONIC, &func_end);
    clock_gettime(CLOCK_REALTIME, &realClk);
    double accum = ( func_end.tv_sec - func_start.tv_sec )
    + ( func_end.tv_nsec - func_start.tv_nsec )
    / BILLION;
    double timefinished = (realClk.tv_sec + realClk.tv_nsec/BILLION);
    double system_time = (sys_end.tv_sec-sys_start.tv_sec) + (sys_end.tv_usec-sys_start.tv_usec)/MILLION;
    double user_time = (user_end.tv_sec-user_start.tv_sec) + (user_end.tv_usec-user_start.tv_usec)/MILLION;

    dprintf(profiling, "Time: %lf Elapsed:%lf System:%lf User:%lf ", timefinished, accum, user_time, system_time );
    dprintf(profiling, "[%d]", getpid());
    dprintf(profiling, "\n");

  }

  return print_tree || !last_command ? 0 : command_status (last_command);
}
