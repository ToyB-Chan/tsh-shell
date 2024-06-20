#include "builtincommands.h"
#include "jobmanager.h"
#include "string.h"
#include "shell.h"
#include <unistd.h>


bool ExecuteBuiltinCommand(ShellInfo* shell, ListString* params)
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
		CommandJob(shell, params);
		return true;
	}
	
	if (String_EqualsCString(cmd, "list"))
	{
		CommandList(shell, params);
		return true;
	}
	
	if (String_EqualsCString(cmd, "info"))
	{
		CommandInfo(shell, params);
		return true;
	}
	
	if (String_EqualsCString(cmd, "wait"))
	{
		CommandWait(shell, params);
		return true;
	}
	
	if (String_EqualsCString(cmd, "kill"))
	{
		CommandKill(shell, params);
		return true;
	}
	 
	if (String_EqualsCString(cmd, "quit"))
	{
		CommandQuit(shell, params);
		return true;
	}
	
	ListString_Insert(params, cmd, 0);
	return false;
}

void CommandJob(ShellInfo* shell, ListString* params)
{
	assert(shell);
	CHECK_PRINT_ERROR_RETURN(params->numElements > 0, "no job executable given",);
	CHECK_PRINT_ERROR_RETURN(ShellInfo_IsFile(shell, ListString_Get(params, 0)), "executable does not exist",);
	CHECK_PRINT_ERROR_RETURN(ShellInfo_IsExecutable(shell, ListString_Get(params, 0)), "file is not an executable",);

	JobInfo* job = JobManager_CreateJob(shell->jobManager, params);
	JobInfo_Execute(job, shell);
	PRINT_SUCCESS();
}

void CommandList(ShellInfo* shell, ListString* params)
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

void CommandInfo(ShellInfo* shell, ListString* params)
{
	assert(shell);
	assert(params);

	CHECK_PRINT_ERROR_RETURN(params->numElements > 0, "no job id parameter given",);

	int id = -1;
	CHECK_PRINT_ERROR_RETURN(String_Atoi(ListString_Get(params, 0)), "job id parameter is not a number",);

	JobInfo* job = JobManager_FindJobById(shell->jobManager, id);
	CHECK_PRINT_ERROR_RETURN(job, "invalid job id",);

	String* str = JobInfo_ToInfoString(job);
	printf("%s\n", String_GetCString(str));
	String_Destroy(str);
	PRINT_SUCCESS();
}

void CommandWait(ShellInfo* shell, ListString* params)
{

}

void CommandKill(ShellInfo* shell, ListString* params)
{

}

void CommandQuit(ShellInfo* shell, ListString* params)
{
	assert(shell);
	exit(0);
}
