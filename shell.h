#pragma once

#include <stdbool.h>

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