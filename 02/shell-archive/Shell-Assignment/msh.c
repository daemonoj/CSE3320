/*

Name: Ahmad Omar Javed
ID: 1001877250

*/

// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017, 2020 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
    // so we need to define what delimits our tokens.
    // In this case  white space
    // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 10     // Mav shell only supports ten arguments

void showPids(pid_t *pidlist, int size);

void showCommands(char **cmdlist, int size);

void addPid(pid_t *pidlist, pid_t pid, int size);

void addCommand(char **cmdlist, char *cmd, int size);


int main()
{
	int i, n;

	char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );


	//list of pids to store last 15 pids for processes created.
	pid_t pidlist[15] = {0};

	// list of strings to store last 15 commands by the user
	// setting everything to empty string to avoid memory errors later.
	// this also helps in comparing if the liist item is empty string. 
	char* cmdlist[15] = {"","","","","","","","","","","","","","",""};

	while( 1 )
	{
		// Print out the msh prompt
		printf ("\nmsh> ");


		// get current working directory.
		// This is inside the while loop because
		// user might have changed the cwd in the last command. 
		char *cwd = getcwd(NULL, 0);

		// Read the command from the commandline.  The
		// maximum command that will be read is MAX_COMMAND_SIZE
		// This while command will wait here until the user
		// inputs something since fgets returns NULL when there
		// is no input
		while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

		/* Parse input */
		// token is of size 11 because 
		// msh supports 10 arguments +  the command that is being run
		char *token[MAX_NUM_ARGUMENTS + 1];

	    int token_count = 0;
                               
		// Pointer to point to the token
		// parsed by strsep
		char *argument_ptr;                                         
                               
		char *working_str  = strdup( cmd_str );                

		// we are going to move the working_str pointer so
		// keep track of its original value so we can deallocate
		// the correct amount at the end
		char *working_root = working_str;

		//Check if user is trying to run an old command
		// If so change the working_str to the old command.
		if (strncmp(working_str, "!", 1) == 0)
		{
			n = atoi(working_str + sizeof(char));
			// check if the command exists in history or not
			// load the old command to working_str
			// print relevant message if the command isn't in history
			if(n > 15 || n < 0)
			{
				printf("Command not in history!\n");
				addCommand(cmdlist, cmd_str, 15);
				continue;
			}
			if (strcmp(cmdlist[n-1],"") != 0)
			{
				cmd_str =strdup(cmdlist[n-1]);
				working_str = strdup(cmdlist[n-1]);
			}
			else
			{
				printf("Command not in history!\n");
				addCommand(cmdlist, cmd_str, 15);
				continue;
			}
		}
		// Tokenize the input strings with whitespace used as the delimiter
		while ( ( (argument_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
		(token_count<MAX_NUM_ARGUMENTS))
		{	
			token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
			if( strlen( token[token_count] ) == 0 )
			{
				token[token_count] = NULL;
			}
			token_count++;
		}

		// If user doesnt input anything dont execute rest of the code
		// continue from the start of the loop.
		if (token[0] == NULL)
		{
			//printf("Phew.. That was close.\n");
			continue;
		}

		/* Check if the command is showpids.
		* Call function showPids.
		* add showpids command to cmdList
		* continue loop to restart from beginning
		* and ask user commands
		*/
		if (strcmp(token[0],"showpids") == 0)
		{
			showPids(pidlist, 15);
			addCommand(cmdlist, cmd_str, 15);
			continue;
		}

		/*
		* Check if the user command is cd
		* use chdir() function to change the directory
		* if the directory doesnt exists print the error
		* store the command to command list.
		* continue loop to restart from beginning
		* and ask user commands
		*/
		if (strcmp(token[0], "cd") == 0)
		{
			int ret = chdir(token[1]);
			if (ret != 0)
			{
				perror("msh: cd: ");
			}
			addCommand(cmdlist, cmd_str, 15);
			continue;
		}

		//check if the input command is exit or quit.
		if(strcmp(token[0],"exit") == 0 || strcmp(token[0],"quit") == 0)
		{
			return 0 ;
		}

		//=================================
		/* Check if the command is history. 
		* call showCommands to display the commands in history
		* add the command to command list
		* continue the loop
		*/

		if (strcmp(token[0], "history") == 0)
		{
			addCommand(cmdlist, cmd_str, 15);
			showCommands(cmdlist, 15);
			continue;
		}

		// fork()
		pid_t pid = fork( );

		// check if fork gives an error
		// if pid is -1 it means there was an error while forking.
		if (pid == -1) 
		{ 
			printf("Error while forking.\n");
         	exit(0);
     	}
     	// if fork() returns 0 it means we are child process.
		// if it is child process, we need to run the commands 
     	// provided by the user
		else if( pid == 0 )
		{
			//run the command using execvp
			int ret = execvp(token[0], token);

			//if the exec returns -1, it means that there was some error in
			//running the command
			//display the error message in this case
			if( ret == -1)
			{
				printf("%s: Command Not found\n", token[0]);
			}
			//exit the child
			exit(0);
		}
		// if it is parent process, we need to add the pid to pidlist
		// add the command to command list
		else 
		{
			int status;
			//wait for the child to exit
			wait( & status );
			//call the addPid() to add the child pid to the pidlist
			// call the addCommand() to add the command to command list
			addPid(pidlist, pid, 15);
			addCommand(cmdlist, cmd_str, 15);
		}
		free(working_root);    
	}
	return 0;
}

/*
Function: showPids
Arguments:
	pid_t *pidlist commandlist where last 15 commands are stored
	int size: number of commands that can be stored in command list
Return type: void
Displays the stored pids from the pid list.
*/
void showPids(pid_t *pidlist, int size)
{  
	int i;
	//iterate through the pid list
	for (i = 0; i < size; i++)
	{
		// check if there is no pid to be displayed
		// if so end the loop
		if ((int)pidlist[i] == 0)
		{
			break;
		}
		// print the pid if present in pidlist
		printf("%2d: %d\n", i, (int)pidlist[i]);
	}
	return;
}


/*
Function: showCommands
Arguments:
	char **cmdlist: commandlist where last 15 commands are stored
	int size: number of commands that can be stored in command list
Return type: void
Displays the stored commands from the command list.
*/
void showCommands(char **cmdlist, int size)
{
	int i;
	//iterate through the cmd list
	for (i = 0; i < size; i++)
	{
		// check if there is no command to be displayed
		// if so end the loop
		if (strcmp(cmdlist[i],"")==0)
		{
			break;
		}
		//print the command stored.
		printf("%2d: %s", i+1, cmdlist[i]);
	}
	return;
}

/*
Function: addPid
Arguments: 
	pid_t *pidlist: pid list where child pids are stored
	pid_t pid: child pid that is to be stored
	int size: number of pids that can be stored in pidlist
Return type: void
Stores the given pid into the pid list.
*/
void addPid(pid_t *pidlist, pid_t pid, int size)
{
	int i;
	// Case 1: Theres empty space in pidlist
	// Iterate though the pidlist to find such space.
	for (i = 0; i < size; i++)
	{
		// We know that empty space is where the array has 0
		// since by default we have assigned all elements to 0
		// if any element is 0 it means its empty.
		// if empty space is found
		if ((int)pidlist[i] == 0)
		{
			// store the pid at the empty space
			// end the function
			pidlist[i] = pid;
			return;
		}
	}
	// Case 2: If there is no empty space in the pidlist
	// iterate through the list and move each pid up by 1
	// create space at the end of the list
	for (i = 0; i < size - 1; i++)
	{
		pidlist[i] = pidlist[i+1];
	}
	// store the given pid at the end of the list where space was created
	pidlist[size - 1] = pid;
	return;
}


/*
Function: addCommand
Arguments: 
	char **cmdlist: command list where history is stored
	char* cmd: command that is to be stored
	int size: number of commands that can be stored in command list
Return type: void
Stores the given command into the command list.
*/
void addCommand(char **cmdlist, char *cmd, int size)
{
	int i;

	// Case 1: Command list still has some empty spaces
	// Iterate through the command list
	for (i = 0; i < size; i++)
	{
		// Check if any of the spaces is still empty
		// We know that if the cmdlist contains empty string it is empty
		// because we have assignmed all elements to empty string by default
		// If any such space is found
		if (strcmp(cmdlist[i],"") == 0)
		{
			// allocate memory to the array element
			// copy the command to the array.
			// close the function
			cmdlist[i] = (char*)(malloc(MAX_COMMAND_SIZE));
			strcpy(cmdlist[i], cmd);
			return;
		}
	}
	// Case 2: If all the spaces in command list are full.
	// Iterate through the whole array
	// move each element 1 up to create space for the command 
	// at the end of array
	for (i = 0; i < size - 1; i++)
	{
		strcpy(cmdlist[i], cmdlist[i+1]);
	}
	//copy the new command at the end of the array which is now empty.
	strcpy(cmdlist[i],cmd);
	return;
}
