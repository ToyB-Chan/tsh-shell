#include "shell.h"
#include "string.h"
#include "jobmanager.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

ShellInfo* ShellInfo_New()
{
	ShellInfo* shell = (ShellInfo*)malloc(sizeof(ShellInfo));
	shell->directory = String_New();
	shell->jobManager = JobManager_New();
	return shell;
}

void ShellInfo_Destroy(ShellInfo* shell)
{
	assert(shell);
	String_Destroy(shell->directory);
	JobManager_Destroy(shell->jobManager);
	free(shell);
}

bool ShellInfo_Execute(ShellInfo* shell, ListString* params, int* outStatusCode)
{
	/*
	for (size_t i = 0; i < params->numElements; i++)
	{
		if (String_EqualsCString(ListString_Get(params, i), "<"))
		{
			if(!(params->numElements - 1 > i + 1))
			{
				// TODO: Error
			} 				

			if (inStream)
			{
				// TODO: Error
			}

			inStream = fopen(String_GetCString(ListString_Get(params, i + 1)), "rw");
			continue;
		}

		if (String_EqualsCString(ListString_Get(params, i), ">"))
		{
			if(!(params->numElements - 1 > i + 1))
			{
				// TODO: Error
			} 				

			if (outStream)
			{
				// TODO: Error
			}

			outStream = fopen(String_GetCString(ListString_Get(params, i + 1)), "rw");
		}
	}
	*/

	String* filePath = ListString_Get(params, 0);

	// we ignore the file path which is always at index 0 but instead (implicitly) add a nullptr to mark the end of our arg values
	char** argv = (char**)calloc(params->numElements, sizeof(char*)); 
	for (int i = 1; i < params->numElements; i++)
	{
		argv[i - 1] = String_GetCString(ListString_Get(params, i));
	}

	int inPipe[2];
	int outPipe[2];
	pipe(inPipe);
	pipe(outPipe);


	bool* success = (bool*)malloc(sizeof(bool));
	*success = true;

	pid_t pid = fork();
	if (pid == 0)
	{
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);
		printf("child: success is %i, flipping...\n", *success);
		*success = !*success;

		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);

		execv(String_GetCString(filePath), argv);
		//printf("we failed!!!\n");
		*success = false;
		exit(1);
	}
	else if (pid < 0)
	{
		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);

		return false;
	}

	printf("parent: success is %i, flipping...\n", *success);
	*success = !*success;
	close(inPipe[0]);
	close(outPipe[1]);

	FILE* outStream = fdopen(outPipe[0], "r");
	while (true)
	{	
		usleep(5000);
		while (true)
		{
			int c = fgetc(outStream);
			if (c == EOF)
				break;
			putc(c, stdout);
		}

		pid_t tpid = waitpid(pid, outStatusCode, WNOHANG);
		if (tpid == pid)
			break;
	}

	fclose(outStream);
	close(inPipe[1]);
	close(outPipe[0]);
	free(argv);

	bool successStack = *success;
	free(success);
	return successStack;
}
