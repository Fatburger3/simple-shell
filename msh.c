/*
 * AUTHOR: Carsen Yates
 * PURPOSE: A simple (unfinished) command line interpreter.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>

// mmmmm these are fun to use
// I get bored easily
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// This flag enables colored error messages and also colors the "msh>" prompt green.
// It is disabled by default in case this breaks functionality or gradability.
#define ENABLE_ANSI_COLORS 1

#define delim " \t"

// Max chars to read on one line.
#define LENGTH 2048

void printfc(char* color, char* message, ...)
{
	// I wrap all my print statements with optional color
	if(ENABLE_ANSI_COLORS)
	{
		printf("%s", color);
	}

	// Found this code online, it basically allows me to grab the
	// variable arguement list and pass it to printf without unpacking it.
	va_list args;
	va_start(args, message);

	vprintf(message, args);

	va_end(args);

	// We are done printing so we clear the ansi color formatting
	if(ENABLE_ANSI_COLORS)
	{
		printf("%s", ANSI_COLOR_RESET);
	}
}

// Formatted error message. Maybe I should just print to standard error?
void printe(char*message)
{
	printfc(ANSI_COLOR_RED, "Error: %s\n", message);
}

void printd(char*message)
{
	printfc(ANSI_COLOR_CYAN, "Debug: %s\n", message);
}

int main(int argc, char** argv)
{
	int stdin_mode = 1;
	FILE* input_stream = stdin;

	switch(argc)
	{
		case 0:
			printe("No input args, this is impossible!");
			return 1;
		case 1:
			// The only arg is the program name, so we read from sdtin
			// We don't need to change the variables for this case,
			// they were set up above.
			break;
		case 2:
			// The only arg should be a file that contains commands
			input_stream = fopen(argv[1], "r");
			stdin_mode = 0;
			break;
		default:
			printe("Too many input args!");
			return 1;
	}

	char input[LENGTH];
	// Loop unconditionally
	start:
	while(1)
	{
		// Print the shell prompt
		if(stdin_mode)
		{
			printfc(ANSI_COLOR_GREEN, "%s", "msh");
			printfc(ANSI_COLOR_CYAN, "%s", "> ");
		}

		// Grab user input
		if(fgets(input, LENGTH, input_stream) == NULL)
		{
			// This is a special case where we hit EOF (CTRL-D), it's different from "exit"
			// because I want a newline before EOF, but not before "exit", because the user
			// entered a newline after exit.
			// I also print "exit" just like bash does.
			if(stdin_mode) printfc(ANSI_COLOR_YELLOW, "%s\n", "exit");
			return 0;
		}

		if(strcmp(input, "\n") == 0)
		{
			// This is to make sure that we don't do anything except print the prompt again
			// when the user presses enter without entering text
			continue;
		}

		char* no_endl = strtok(input, "\n");

		// Now check for redirection
		int sub_in  = 0;
		int sub_out = 0;
		char* sub_out_file;
		char* sub_in_file;

		char* redir_delims = "><";
		char* c = malloc(sizeof(char) * strlen(no_endl));
		strcpy(c, no_endl);
		char* command = strtok(no_endl, redir_delims);

		// Scan for redirection chars
		while(*c)
		{
			if(*c == '>')
			{

				if(sub_out != 0)
				{
					printe("Only one '>' is allowed");
					goto start;
				}
				// We have determined there is output redirection, so open the output file
				sub_out_file = strtok(NULL, redir_delims);
				sub_out++;

			}
			else if(*c == '<')
			{

				if(sub_in != 0)
				{
					printe("Only one '<' is allowed");
					goto start;
				}
				// We have determined there is input redirection, so open the input file
				sub_in_file = strtok(NULL, redir_delims);
				sub_in++;
			}
			c++;
		}


		char* tokens[LENGTH];
		int t;
		int i = 0;

		// grab the first token in input string
		tokens[i] = strtok(command, delim);

		// Grab the rest of the tokens
		while(tokens[i] != NULL)
		{
			tokens[++i] = strtok(NULL, delim);
		}
		// printf("%d\n", i);

		// Truncate the array
		char* args[i];
		// from i-1 to 0
		for(t = 0; t < i; t++)
		{
			args[t] = strdup(tokens[t]);
		}
		args[i]=NULL;

		if(strcmp(args[0],"exit") == 0)
		{
			// Exit the command loop and quit the program
			return 0;
		}
		else if(strcmp(args[0],"pwd") == 0)
		{
			// Print the working directory and return to input prompt
			char *buf;
			char *ptr;
			long size = pathconf(".", _PC_PATH_MAX);
			if ((buf = (char *)malloc((size_t)size)) != NULL)
				ptr = getcwd(buf, (size_t)size);
			printfc(ANSI_COLOR_YELLOW, "%s\n", ptr);
			continue;
		}
		else if(strcmp(args[0],"cd") == 0)
		{
			if(args[1] == NULL)
			{
				args[1] = getenv("HOME");
			}
			// Change directory, then return to input prompt
			if(chdir(args[1]) != 0)
			{
				printe("Could not change directory");
			}
			continue;
		}
		else if(strcmp(args[0],"help") == 0)
		{
			// Print Help text, then return to input prompt
			printfc(ANSI_COLOR_YELLOW, "%s\n", "enter Linux commands, or ‘exit’ to exit");
			continue;
		}
		else if(strcmp(args[0],"today") == 0)
		{
			time_t rawtime;
			struct tm *info;
			char buffer[80];
			time(&rawtime);
			info = localtime(&rawtime);
			strftime(buffer,80,"%B %e, %Y", info);
			printfc(ANSI_COLOR_YELLOW, "%s\n", buffer);
			continue;
		}
		else
		{
			// (try to) Execute the command
			int fv = fork();
			if(fv < 0)
			{
				// There was an error creating the process
				printe("Unable to start process. (fork)");
			}
			else if(fv == 0) // valid child process
			{
				// Redirection
				if(sub_in)
				{
					dup2(open(strtok(sub_in_file, delim), O_RDONLY), 0);
				}
				if(sub_out)
				{
					dup2(open(strtok(sub_out_file, delim), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR), 1);
				}
				if(0 > execvp(args[0], args))
				{
					// The process was not started.
					// This process is now a clone of the original msh process,
					// So here we print an error and then exit,
					// allowing the parent process to take over again
					printe("Unable to start process. (execvp)");
					return 1;
				}
			}
			wait(NULL);
		}
	}
}
