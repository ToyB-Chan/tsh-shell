#include "shell.h"
#include "string.h"
#include "jobmanager.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

ShellInfo* ShellInfo_New()
{
	ShellInfo* shell = (ShellInfo*)malloc(sizeof(ShellInfo));
	shell->directory = String_New();
	shell->jobManager = JobManager_New();
	shell->inputBuffer = String_New();

	char* buffer = NULL;
	size_t bufferSize = 1024;
	while(1)
	{
		if (buffer)
			free(buffer);

		buffer = (char*)malloc(bufferSize * sizeof(char));
		assert(buffer);

		if (getcwd(buffer, bufferSize))
			break;

		assert(errno == ERANGE);
		bufferSize *= 2;
	}

	String_AppendCString(shell->directory, buffer);
	free(buffer);
	return shell;
}

void ShellInfo_Destroy(ShellInfo* shell)
{
	assert(shell);
	String_Destroy(shell->directory);
	JobManager_Destroy(shell->jobManager);
	String_Destroy(shell->inputBuffer);
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

		// needs nullptr as last element to mark the end
		char** argv = (char**)calloc(params->numElements + 1, sizeof(char*));
		assert(argv);
		for (size_t i = 0; i < params->numElements; i++)
		{
			argv[i] = String_GetCString(ListString_Get(params, i));
		}

		chdir(String_GetCString(shell->directory));
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

bool ShellInfo_IsDirectory(ShellInfo* shell, String* path)
{
	struct stat info;

	if (stat(String_GetCString(path), &info) != 0)
		return false;

	return S_ISDIR(info.st_mode);
}

String* ShellInfo_ResolvePath(ShellInfo* shell, String* path)
{
	assert(shell);
	assert(path);
	assert(String_GetLength(path) > 0);

	if(String_GetCharAt(path, 0) == '/')
		return String_Copy(path);

	String* realPath = String_New();
	String_AppendString(realPath, shell->directory);
	String_AppendChar(realPath, '/');
	String_AppendString(realPath, path);

	if (ShellInfo_IsFile(shell, realPath) && ShellInfo_IsExecutable(shell, realPath))
		return realPath;

	char* pathEnv = getenv("PATH");
	if (!pathEnv)
		return String_Copy(path);

	String* pathEnvStr = String_New();
	String_AppendCString(pathEnvStr, pathEnv);
	ListString* pathEnvList = String_Split(pathEnvStr, ':');

	String_Destroy(pathEnvStr);
	pathEnvStr = NULL;

	for (size_t i = 0; i < pathEnvList->numElements; i++)
	{
		String_Reset(realPath);
		String_AppendString(realPath, ListString_Get(pathEnvList, i));
		String_AppendChar(realPath, '/');
		String_AppendString(realPath, path);

		if (ShellInfo_IsFile(shell, realPath) && ShellInfo_IsExecutable(shell, realPath))
		{
			ListString_Destroy(pathEnvList);
			return realPath;
		}
	}

	ListString_Destroy(pathEnvList);
	String_Destroy(realPath);
	return String_Copy(path);
}
