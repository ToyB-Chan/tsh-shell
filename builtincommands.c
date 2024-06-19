#include "builtincommands.h"
#include "jobmanager.h"
#include "string.h"
#include "shell.h"

bool ExecuteBuiltinCommand(ShellInfo* shell, ListString* params)
{
	assert(shell);
	assert(params && params->numElements > 0);
	String* cmd = ListString_Remove(params, 0);

	if (String_EqualsCString(cmd, "job"))
	{
		CommandJob(shell, params);
		return true;
	}
	else if (String_EqualsCString(cmd, "list"))
	{
		return true;
	}
	else if (String_EqualsCString(cmd, "info"))
	{
		return true;
	}
	else if (String_EqualsCString(cmd, "wait"))
	{
		return true;
	}
	else if (String_EqualsCString(cmd, "kill"))
	{
		return true;
	}
	else if (String_EqualsCString(cmd, "quit"))
	{
		exit(0);
		return true;
	}
	
	ListString_Insert(params, cmd, 0);
	return false;
}

void CommandJob(ShellInfo* shell, ListString* params)
{
	JobInfo* job = JobManager_CreateJob(shell->jobManager, params);
	JobInfo_Execute(job, shell);
}

void CommandList(ShellInfo* shell, ListString* params)
{

}

void CommandInfo(ShellInfo* shell, ListString* params)
{

}

void CommandWait(ShellInfo* shell, ListString* params)
{

}

void CommandKill(ShellInfo* shell, ListString* params)
{

}

void CommandQuit(ShellInfo* shell, ListString* params)
{

}
