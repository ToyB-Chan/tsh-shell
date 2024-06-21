#include "shell.h"
#include "string.h"
#include "jobmanager.h"
#include "signalhandler.h"
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>

#define ANSI_ESCAPE_CODE '\033'

#define ANSI_RESET_LINE "\033[2K\r"
#define ANSI_CLEAR_SCREEN "\033[2J"
#define ANSI_RESET_CURSOR "\033[H"
#define ANSI_MOVE_CURSOR_LEFT "\033[D"
#define ANSI_MOVE_CURSOR_UP "\033[A"
#define ANSI_MOVE_CURSOR_RIGHT "\033[C"
#define ANSI_MOVE_CURSOR_DOWN "\033[B"

#define ASCII_BACKSPACE 8
#define ASCII_DELETE 127

ShellInfo* ShellInfo_New()
{
	ShellInfo* shell = (ShellInfo*)malloc(sizeof(ShellInfo));
	shell->directory = String_New();
	shell->jobManager = JobManager_New();
	shell->inputBuffer = String_New();
	shell->cursorPosition = 0;
	shell->waitForJob = NULL;
	shell->foregroundJob = NULL;
	shell->exitRequested = false;

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

	signal(SIGINT, SignalHandlerAbort);

	return shell;
}

void ShellInfo_Destroy(ShellInfo* shell)
{
	assert(shell);
	String_Destroy(shell->directory);
	JobManager_Destroy(shell->jobManager);
	String_Destroy(shell->inputBuffer);
	signal(SIGINT, NULL);
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

String* ShellInfo_ResolvePath(ShellInfo* shell, String* path, bool useEnvVar)
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

	if (!useEnvVar)
		return String_Copy(path);

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

void ShellInfo_Tick(ShellInfo* shell)
{
	assert(shell);
	assert(!shell->exitRequested);

	printf(ANSI_RESET_LINE); // Remove the shell prompt so jobs can print cleanly
	JobManager_Tick(shell->jobManager, shell);

	// We are currently waiting for a job, don't handle any promt or input
	if (shell->waitForJob != NULL)
	{
		if (shell->waitForJob->status >= JS_Finished)
		{
			shell->waitForJob = NULL;
			printf("[waiting finished]\n");
			PRINT_SUCCESS();
		}
		else if(g_abortRequested)
		{
			shell->waitForJob = NULL;
			CHECK_PRINT_ERROR(false, "waiting aborted");
			g_abortRequested = false;
		}

		return;
	}

	// Handle input from stdin
	bool commandReady;
	ShellInfo_UpdateInputBuffer(shell, &commandReady);

	// We are running a foreground job, don't bother drawing the promt or executing commands but instead redirect the input
	if (shell->foregroundJob != NULL)
	{
		// Draw our input string
		printf("%s", String_GetCString(shell->inputBuffer));
		fflush(stdout);

		if (shell->foregroundJob->status >= JS_Finished)
		{
			assert(shell->foregroundJob->id + 1 == shell->jobManager->nextJobId);
			JobManager_DestroyJob(shell->jobManager, shell->foregroundJob);
			shell->foregroundJob = NULL;
			shell->jobManager->nextJobId--;
		}
		else if(commandReady)
		{
			String_Destroy(shell->foregroundJob->inBuffer);
			shell->foregroundJob->inBuffer = String_Copy(shell->inputBuffer);
			String_AppendChar(shell->foregroundJob->inBuffer, '\n');
			String_Reset(shell->inputBuffer);
			shell->cursorPosition = 0;
		}
		else if(g_abortRequested)
		{
			kill(shell->foregroundJob->pid, SIGKILL);
			printf("[aborted]\n");
			g_abortRequested = false;
		}

		return;
	}

	// Redraw the promt
	printf("tsh@%s> %s", String_GetCString(shell->directory), String_GetCString(shell->inputBuffer));
	fflush(stdout);

	// User commited the command via newline, execute it
	if (commandReady)
	{
		printf("\n");
		ListString* params = String_Split(shell->inputBuffer, ' ');
		String_Reset(shell->inputBuffer);
		shell->cursorPosition = 0;

		bool success = ShellInfo_ExecuteBuiltinCommand(shell, params);

		if (!success)
			ShellInfo_ExecuteFile(shell, params);

		for (size_t i = 0; i < params->numElements; i++)
			String_Destroy(ListString_Get(params, i));
		ListString_Destroy(params);
	}

	if (g_abortRequested)
		g_abortRequested = false;
}

void ShellInfo_UpdateInputBuffer(ShellInfo* shell, bool* outCommandReady)
{
	assert(shell);
	assert(outCommandReady);

	while(1)
	{
		int c = getc(stdin);
		if (c == EOF)
		{
			*outCommandReady = false;
			return;
		}

		if (c == '\n')
		{
			printf(ANSI_MOVE_CURSOR_UP); // Revert the newline by going a line up again
			*outCommandReady = true;
			return;
		}

		if (c == ASCII_BACKSPACE || c == ASCII_DELETE)
		{
			if (String_GetLength(shell->inputBuffer) > 0)
				String_RemoveAt(shell->inputBuffer, String_GetLength(shell->inputBuffer) - 1);
			continue;
		}

		if (c == ANSI_ESCAPE_CODE)
		{
			String* ansiSeqeunce = String_New();
			String_AppendChar(ansiSeqeunce, c);
			String_AppendChar(ansiSeqeunce, (char)getc(stdin));
			String_AppendChar(ansiSeqeunce, (char)getc(stdin));

			if (String_EqualsCString(ansiSeqeunce, ANSI_MOVE_CURSOR_UP) || String_EqualsCString(ansiSeqeunce, ANSI_MOVE_CURSOR_DOWN))
				continue;

			if (String_EqualsCString(ansiSeqeunce, ANSI_MOVE_CURSOR_LEFT) || String_EqualsCString(ansiSeqeunce, ANSI_MOVE_CURSOR_RIGHT))
				continue;
		}

		String_AppendChar(shell->inputBuffer, (char)c);
		shell->cursorPosition++;
	}
}

String* ShellInfo_ExtractInFilePath(ShellInfo* shell, ListString* params, bool* outInvalidInput)
{
	String* path = NULL;
	size_t index = 0; 
	for (size_t i = 0; i < params->numElements; i++)
	{
		String* current = ListString_Get(params, i);
		if (String_EqualsCString(current, "<"))
		{
			if (i + 1 >= params->numElements)
			{
				*outInvalidInput = true;
				return NULL;
			}

			path = ListString_Get(params, i + 1);
			index = i;
			break;
		}
	}

	*outInvalidInput = false;

	if (path)
	{
		String* realPath = ShellInfo_ResolvePath(shell, path, false);
		String_Destroy(ListString_Remove(params, index)); // remove pipe symbol
		String_Destroy(ListString_Remove(params, index)); // remove path
		return realPath;
	}

	return NULL;
}

String* ShellInfo_ExtractOutFilePath(ShellInfo* shell, ListString* params, bool* outInvalidInput)
{
	String* path = NULL;
	size_t index = 0; 
	for (size_t i = 0; i < params->numElements; i++)
	{
		String* current = ListString_Get(params, i);
		if (String_EqualsCString(current, ">"))
		{
			if (i + 1 >= params->numElements)
			{
				*outInvalidInput = true;
				return NULL;
			}

			path = ListString_Get(params, i + 1);
			index = i;
			break;
		}
	}

	*outInvalidInput = false;

	if (path)
	{
		String* realPath = ShellInfo_ResolvePath(shell, path, false);
		String_Destroy(ListString_Remove(params, index)); // remove pipe symbol
		String_Destroy(ListString_Remove(params, index)); // remove path
		return realPath;
	}

	return NULL;
}

bool ShellInfo_ExecuteBuiltinCommand(ShellInfo* shell, ListString* params)
{
	assert(shell);
	assert(params && params->numElements > 0);
	String* cmd = ListString_Remove(params, 0);

	if (String_EqualsCString(cmd, ""))
	{
		return true;
	}

	if (String_EqualsCString(cmd, "job"))
	{
		ShellInfo_CommandJob(shell, params);
		return true;
	}
	
	if (String_EqualsCString(cmd, "list"))
	{
		ShellInfo_CommandList(shell, params);
		return true;
	}
	
	if (String_EqualsCString(cmd, "info"))
	{
		ShellInfo_CommandInfo(shell, params);
		return true;
	}
	
	if (String_EqualsCString(cmd, "wait"))
	{
		ShellInfo_CommandWait(shell, params);
		return true;
	}
	
	if (String_EqualsCString(cmd, "kill"))
	{
		ShellInfo_CommandKill(shell, params);
		return true;
	}
	 
	if (String_EqualsCString(cmd, "exit") || String_EqualsCString(cmd, "quit"))
	{
		ShellInfo_CommandExit(shell, params);
		return true;
	}
	
	if (String_EqualsCString(cmd, "cd"))
	{
		ShellInfo_CommandCd(shell, params);
		return true;
	}

	if (String_EqualsCString(cmd, "pwd"))
	{
		ShellInfo_CommandPwd(shell, params);
		return true;
	}

	if (String_EqualsCString(cmd, "clear"))
	{
		ShellInfo_CommandClear(shell, params);
		return true;
	}

	if (String_EqualsCString(cmd, "send"))
	{
		ShellInfo_CommandSend(shell, params);
		return true;
	}

	ListString_Insert(params, cmd, 0);
	return false;
}

void ShellInfo_ExecuteFile(ShellInfo* shell, ListString* params)
{
	assert(shell);
	assert(shell->foregroundJob == NULL);
	CHECK_PRINT_ERROR_RETURN(params->numElements > 0, "no executable given",);

	String* orgPath = ListString_Remove(params, 0);
	String* resolvedPath = ShellInfo_ResolvePath(shell, orgPath, true);
	String_Destroy(orgPath);

	CHECK_PRINT_ERROR_RETURN(ShellInfo_IsFile(shell, resolvedPath), "executable does not exist",);
	CHECK_PRINT_ERROR_RETURN(ShellInfo_IsExecutable(shell, resolvedPath), "file is not an executable",);
	ListString_Insert(params, resolvedPath, 0);

	bool invalidInput;

	FILE* inFile = NULL;
	invalidInput = false;
	String* inPath = ShellInfo_ExtractInFilePath(shell, params, &invalidInput);
	CHECK_PRINT_ERROR_RETURN(!invalidInput, "no file given to pipe-in",);
	if (inPath)
	{
		CHECK_PRINT_ERROR_RETURN(ShellInfo_IsFile(shell, inPath), "file given to pipe-in does not exist",);
		inFile = fopen(String_GetCString(inPath), "r");
	}

	FILE* outFile = NULL;
	invalidInput = false;
	String* outPath = ShellInfo_ExtractOutFilePath(shell, params, &invalidInput);
	CHECK_PRINT_ERROR_RETURN(!invalidInput, "no file given to pipe-out",);
	if (outPath)
	{
		CHECK_PRINT_ERROR_RETURN(ShellInfo_IsFile(shell, outPath), "file given to pipe-out does not exist",);
		outFile = fopen(String_GetCString(outPath), "w");
	}

	JobInfo* job = JobManager_CreateJob(shell->jobManager, params);
	shell->foregroundJob = job;
	JobInfo_Execute(job, shell, inFile, outFile);
}

void ShellInfo_CommandJob(ShellInfo* shell, ListString* params)
{
	assert(shell);
	CHECK_PRINT_ERROR_RETURN(params->numElements > 0, "no job executable given",);

	String* orgPath = ListString_Remove(params, 0);
	String* resolvedPath = ShellInfo_ResolvePath(shell, orgPath, true);
	String_Destroy(orgPath);

	CHECK_PRINT_ERROR_RETURN(ShellInfo_IsFile(shell, resolvedPath), "executable does not exist",);
	CHECK_PRINT_ERROR_RETURN(ShellInfo_IsExecutable(shell, resolvedPath), "file is not an executable",);
	ListString_Insert(params, resolvedPath, 0);

	bool invalidInput;

	FILE* inFile = NULL;
	invalidInput = false;
	String* inPath = ShellInfo_ExtractInFilePath(shell, params, &invalidInput);
	CHECK_PRINT_ERROR_RETURN(!invalidInput, "no file given to pipe-in",);
	if (inPath)
	{
		CHECK_PRINT_ERROR_RETURN(ShellInfo_IsFile(shell, inPath), "file given to pipe-in does not exist",);
		inFile = fopen(String_GetCString(inPath), "r");
	}

	FILE* outFile = NULL;
	invalidInput = false;
	String* outPath = ShellInfo_ExtractOutFilePath(shell, params, &invalidInput);
	CHECK_PRINT_ERROR_RETURN(!invalidInput, "no file given to pipe-out",);
	if (outPath)
	{
		CHECK_PRINT_ERROR_RETURN(ShellInfo_IsFile(shell, outPath), "file given to pipe-out does not exist",);
		outFile = fopen(String_GetCString(outPath), "w");
	}

	JobInfo* job = JobManager_CreateJob(shell->jobManager, params);
	JobInfo_Execute(job, shell, inFile, outFile);
	printf("[created job with id %li]\n", job->id);
	PRINT_SUCCESS();
}

void ShellInfo_CommandList(ShellInfo* shell, ListString* params)
{
	assert(shell);
	String* str = String_New();
	for (size_t i = 0; i < shell->jobManager->jobs->numElements; i++)
	{
		String* jobStr = JobInfo_ToInfoString(ListJobInfo_Get(shell->jobManager->jobs, i));
		String_AppendString(str, jobStr);
		String_AppendChar(str, '\n');
		String_Destroy(jobStr);
	}

	printf("%s\n", String_GetCString(str));
	String_Destroy(str);
	PRINT_SUCCESS();
}

void ShellInfo_CommandInfo(ShellInfo* shell, ListString* params)
{
	assert(shell);
	assert(params);

	CHECK_PRINT_ERROR_RETURN(params->numElements > 0, "no job id parameter given",);

	int id = -1;
	CHECK_PRINT_ERROR_RETURN(String_Atoi(ListString_Get(params, 0), &id), "job id parameter is not a number",);

	JobInfo* job = JobManager_FindJobById(shell->jobManager, id);
	CHECK_PRINT_ERROR_RETURN(job, "invalid job id",);

	String* str = JobInfo_ToInfoString(job);
	printf("%s\n", String_GetCString(str));
	String_Destroy(str);
	PRINT_SUCCESS();
}

void ShellInfo_CommandWait(ShellInfo* shell, ListString* params)
{
	assert(shell);
	CHECK_PRINT_ERROR_RETURN(params->numElements > 0, "no job id parameter given",);

	int id = -1;
	CHECK_PRINT_ERROR_RETURN(String_Atoi(ListString_Get(params, 0), &id), "job id parameter is not a number",);

	JobInfo* job = JobManager_FindJobById(shell->jobManager, id);
	CHECK_PRINT_ERROR_RETURN(job, "invalid job id",);

	CHECK_PRINT_ERROR_RETURN(job->status <= JS_Running, "job already finished execution",);

	shell->waitForJob = job;
	printf("[waiting for job %li]\n", job->id);
}

void ShellInfo_CommandKill(ShellInfo* shell, ListString* params)
{
	assert(shell);
	CHECK_PRINT_ERROR_RETURN(params->numElements > 0, "no job id parameter given",);

	int id = -1;
	CHECK_PRINT_ERROR_RETURN(String_Atoi(ListString_Get(params, 0), &id), "job id parameter is not a number",);

	JobInfo* job = JobManager_FindJobById(shell->jobManager, id);
	CHECK_PRINT_ERROR_RETURN(job, "invalid job id",);

	CHECK_PRINT_ERROR_RETURN(job->status == JS_Running, "job not running",);

	job->status = JS_Killed;
	kill(job->pid, SIGKILL);
	printf("[killed job %li]\n", job->id);
	PRINT_SUCCESS();
}

void ShellInfo_CommandExit(ShellInfo* shell, ListString* params)
{
	assert(shell);
	shell->exitRequested = true;
	PRINT_SUCCESS();
}

void ShellInfo_CommandCd(ShellInfo* shell, ListString* params)
{
	assert(shell);
	CHECK_PRINT_ERROR_RETURN(params->numElements > 0, "no directory given",);

	String* path = ListString_Get(params, 0);
	if (String_GetCharAt(path, 0) == '/')
	{
		CHECK_PRINT_ERROR_RETURN(ShellInfo_IsDirectory(shell, path), "directory does not exist",);
		String_Destroy(shell->directory);
		shell->directory = String_Copy(path);
		PRINT_SUCCESS();
		return;
	}

	String* newPath = String_Copy(shell->directory);
	ListString* pathParts = String_Split(path, '/');
	for (size_t i = 0; i < pathParts->numElements; i++)
	{
		if (String_EqualsCString(ListString_Get(pathParts, i), "."))
			continue;

		if (String_EqualsCString(ListString_Get(pathParts, i), ".."))
		{
			while(1)
			{
				if (String_EqualsCString(newPath, "/"))
					break;

				String_RemoveAt(newPath, String_GetLength(newPath) - 1);
				if (String_GetCharAt(newPath, String_GetLength(newPath) - 1) == '/')
					break;
			}

			continue;
		}

		if (String_GetCharAt(newPath, String_GetLength(newPath) - 1) != '/')
			String_AppendChar(newPath, '/');
		
		String_AppendCString(newPath, String_GetCString(ListString_Get(pathParts, i)));
	}

	for (size_t i = 0; i < pathParts->numElements; i++)
		String_Destroy(ListString_Get(pathParts, i));
	ListString_Destroy(pathParts);

	if (String_GetCharAt(newPath, String_GetLength(newPath) - 1) == '/')
		String_RemoveAt(newPath, String_GetLength(newPath) - 1);

	if (!ShellInfo_IsDirectory(shell, newPath))
	{
		CHECK_PRINT_ERROR(false, "directory does not exist");
		String_Destroy(newPath);
		return;
	}

	String_Destroy(shell->directory);
	shell->directory = newPath;
	PRINT_SUCCESS();
}

void ShellInfo_CommandPwd(ShellInfo* shell, ListString* params)
{
	assert(shell);
	printf("%s\n", String_GetCString(shell->directory));
	PRINT_SUCCESS();
}

void ShellInfo_CommandClear(ShellInfo* shell, ListString* params)
{
	assert(shell);
	printf(ANSI_CLEAR_SCREEN);
	printf(ANSI_RESET_CURSOR);
	fflush(stdout);
	PRINT_SUCCESS();
}

void ShellInfo_CommandSend(ShellInfo* shell, ListString* params)
{
	assert(shell);
	CHECK_PRINT_ERROR_RETURN(params->numElements > 0, "no job id parameter given",);

	int id = -1;
	CHECK_PRINT_ERROR_RETURN(String_Atoi(ListString_Get(params, 0), &id), "job id parameter is not a number",);

	JobInfo* job = JobManager_FindJobById(shell->jobManager, id);
	CHECK_PRINT_ERROR_RETURN(job, "invalid job id",);
	CHECK_PRINT_ERROR_RETURN(params->numElements > 1, "nothing to send",);

	String* idStr = ListString_Remove(params, 0);
	String* sendStr = String_Join(params, ' ');
	ListString_Insert(params, idStr, 0);
	String_AppendChar(sendStr, '\n');

	String_Destroy(job->inBuffer);
	job->inBuffer = sendStr;
}
