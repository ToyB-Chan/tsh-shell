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
	shell->waitForJob = NULL;
	shell->foregroundJob = NULL;

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
			for (size_t i = 0; i < pathEnvList->numElements; i++)
				String_Destroy(ListString_Get(pathEnvList, i));
			ListString_Destroy(pathEnvList);
			return realPath;
		}
	}

	for (size_t i = 0; i < pathEnvList->numElements; i++)
		String_Destroy(ListString_Get(pathEnvList, i));
	ListString_Destroy(pathEnvList);

	String_Destroy(realPath);
	return String_Copy(path);
}
