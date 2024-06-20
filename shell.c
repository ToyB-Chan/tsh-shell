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

	int inPipe[2];
	int outPipe[2];
	pipe(inPipe);
	pipe(outPipe);

	pid_t pid = fork();
	if (pid == 0)
	{
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);

		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);

		String* filePath = ListString_Get(params, 0);

		// we ignore the file path which is always at index 0
		char** argv = (char**)calloc(params->numElements, sizeof(char*));
		assert(argv);

		for (size_t i = 1; i < params->numElements; i++)
		{
			printf("arg: %s", String_GetCString(ListString_Get(params, i)));
			argv[i - 1] = String_GetCString(ListString_Get(params, i));
		}

		argv[params->numElements - 1] = NULL; // redundant

		execvp(String_GetCString(filePath), argv);
		exit(EXIT_STATUS_COMMAND_NOT_FOUND);
	}
	else if (pid < 0)
	{
		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);
		return false;
	}

	close(inPipe[0]);
	close(outPipe[1]);

	FILE* outStream = fdopen(outPipe[0], "r");
	int status = 0;
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

		pid_t tpid = waitpid(pid, &status, WNOHANG);
		if (tpid == pid)
			break;
	}

	fclose(outStream);
	close(inPipe[1]);
	close(outPipe[0]);

	*outStatusCode = WEXITSTATUS(status);
	return *outStatusCode != EXIT_STATUS_COMMAND_NOT_FOUND;
}

bool ShellInfo_IsFile(ShellInfo* shell, String* path)
{
	return access(String_GetCString(path), F_OK) == 0;
}

bool ShellInfo_IsExecutable(ShellInfo* shell, String* path)
{
	return access(String_GetCString(path), X_OK) == 0;
}
