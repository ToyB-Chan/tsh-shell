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

typedef struct ShellInfo
{
	String* directory;
	JobManager* jobManager;
} ShellInfo;


ShellInfo* ShellInfo_New();
void ShellInfo_Destroy(ShellInfo* shell);
bool ShellInfo_Execute(ShellInfo* shell, ListString* params, int* outStatusCode);
bool ShellInfo_IsFile(ShellInfo* shell, String* path);
bool ShellInfo_IsExecutable(ShellInfo* shell, String* path);
bool ShellInfo_IsDirectory(ShellInfo* shell, String* path);
String* ShellInfo_ResolvePath(ShellInfo* shell, String* path);