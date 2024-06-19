#include "shell.h"
#include "string.h"
#include "jobmanager.h"
#include <stdio.h>
#include <sys/types.h>
//#include <sys/wait.h>
#include <stdlib.h>

ShellInfo* ShellInfo_New()
{
	ShellInfo* shell = (ShellInfo*)malloc(sizeof(ShellInfo));
	shell->directory = String_New();
	shell->jobManager = JobManager_New();
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
	FILE* inStream = NULL;
	FILE* outStream = NULL;


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
	char** argv = (char**)calloc(params->numElements, sizeof(char**)); 
	for (int i = 1; i < params->numElements; i++)
	{
		argv[i - 1] = String_GetCString(ListString_Get(params, i));
	}

	bool success = true;
	pid_t pid = fork();
	if (pid == 0)
	{
		execv(filePath, argv);
		success = false;
		exit(1);
	}
	else if (pid < 0)
	{
		return false;
	}

	pid_t tpid = wait(outStatusCode);
	assert(pid == tpid);
	free(argv);

	return success;
}
