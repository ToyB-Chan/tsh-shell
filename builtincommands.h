#pragma once

#include <stdbool.h>

typedef struct ListString ListString;
typedef struct ShellInfo ShellInfo;

bool ExecuteBuiltinCommand(ShellInfo* shell, ListString* params);

void CommandJob(ShellInfo* shell, ListString* params);
void CommandList(ShellInfo* shell, ListString* params);
void CommandInfo(ShellInfo* shell, ListString* params);
void CommandWait(ShellInfo* shell, ListString* params);
void CommandKill(ShellInfo* shell, ListString* params);
void CommandQuit(ShellInfo* shell, ListString* params);

void ExecuteFile(ShellInfo* shell, ListString* params);