#pragma once

#include <stdbool.h>
#include <stdio.h>

#define EXIT_STATUS_COMMAND_NOT_FOUND 127

#define CHECK_PRINT_ERROR(condition, msg) if (!(condition)) { printf("[%s]\n[status=1]\n", msg); }
#define CHECK_PRINT_ERROR_RETURN(condition, msg, returnVal) if (!(condition)) { printf("[%s]\n[status=1]\n", msg); return returnVal; }
#define PRINT_SUCCESS() printf("[status=0]\n");

typedef struct String String;
typedef struct ListString ListString;
typedef struct JobManager JobManager;
typedef struct JobInfo JobInfo;

typedef struct ShellInfo
{
	String* directory;
	JobManager* jobManager;
	String* inputBuffer;
	size_t cursorPosition;

	JobInfo* waitForJob;
	JobInfo* foregroundJob;
	bool exitRequested;
} ShellInfo;

ShellInfo* ShellInfo_New();
void ShellInfo_Destroy(ShellInfo* shell);
bool ShellInfo_IsFile(ShellInfo* shell, String* path);
bool ShellInfo_IsExecutable(ShellInfo* shell, String* path);
bool ShellInfo_IsDirectory(ShellInfo* shell, String* path);
String* ShellInfo_ResolvePath(ShellInfo* shell, String* path, bool resolveEnvVar);
void ShellInfo_Tick(ShellInfo* shell);
void ShellInfo_UpdateInputBuffer(ShellInfo* shell, bool* outCommandReady);
String* ShellInfo_ExtractInFilePath(ShellInfo* shell, ListString* params, bool* outInvalidInput);
String* ShellInfo_ExtractOutFilePath(ShellInfo* shell, ListString* params, bool* outInvalidInput);

bool ShellInfo_ExecuteBuiltinCommand(ShellInfo* shell, ListString* params);
void ShellInfo_ExecuteFile(ShellInfo* shell, ListString* params);

void ShellInfo_CommandJob(ShellInfo* shell, ListString* params);
void ShellInfo_CommandList(ShellInfo* shell, ListString* params);
void ShellInfo_CommandInfo(ShellInfo* shell, ListString* params);
void ShellInfo_CommandWait(ShellInfo* shell, ListString* params);
void ShellInfo_CommandKill(ShellInfo* shell, ListString* params);
void ShellInfo_CommandExit(ShellInfo* shell, ListString* params);
void ShellInfo_CommandCd(ShellInfo* shell, ListString* params);
void ShellInfo_CommandPwd(ShellInfo* shell, ListString* params);
void ShellInfo_CommandClear(ShellInfo* shell, ListString* params);
void ShellInfo_CommandSend(ShellInfo* shell, ListString* params);

