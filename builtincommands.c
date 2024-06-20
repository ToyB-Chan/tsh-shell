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
	
	if (String_EqualsCString(cmd, "cd"))
	{
		CommandCd(shell, params);
		return true;
	}

	if (String_EqualsCString(cmd, "pwd"))
	{
		CommandPwd(shell, params);
		return true;
	}

	ListString_Insert(params, cmd, 0);
	return false;
}

void CommandJob(ShellInfo* shell, ListString* params)
{
	assert(shell);
	CHECK_PRINT_ERROR_RETURN(params->numElements > 0, "no job executable given",);

	String* orgPath = ListString_Remove(params, 0);
	String* resolvedPath = ShellInfo_ResolvePath(shell, orgPath);
	String_Destroy(orgPath);

	CHECK_PRINT_ERROR_RETURN(ShellInfo_IsFile(shell, resolvedPath), "executable does not exist",);
	CHECK_PRINT_ERROR_RETURN(ShellInfo_IsExecutable(shell, resolvedPath), "file is not an executable",);
	ListString_Insert(params, resolvedPath, 0);

	JobInfo* job = JobManager_CreateJob(shell->jobManager, params);
	JobInfo_Execute(job, shell);
	printf("[created job with id %li]\n", job->id);
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
	CHECK_PRINT_ERROR_RETURN(String_Atoi(ListString_Get(params, 0), &id), "job id parameter is not a number",);

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

void CommandCd(ShellInfo* shell, ListString* params)
{
	CHECK_PRINT_ERROR_RETURN(params->numElements > 0, "no directory given",);

	String* path = ListString_Get(params, 0);
	if (String_GetCharAt(path, 0) == '/' && ShellInfo_IsDirectory(shell, path))
	{
		String_Destroy(shell->directory);
		shell->directory = String_Copy(path);
		PRINT_SUCCESS();
		return;
	}
}

void CommandPwd(ShellInfo* shell, ListString* params)
{
	printf("%s\n", String_GetCString(shell->directory));
	PRINT_SUCCESS();
}

void ExecuteFile(ShellInfo* shell, ListString* params)
{
	String* orgPath = ListString_Remove(params, 0);
	String* resolvedPath = ShellInfo_ResolvePath(shell, orgPath);
	String_Destroy(orgPath);

	CHECK_PRINT_ERROR_RETURN(ShellInfo_IsFile(shell, resolvedPath), "executable does not exist",);
	CHECK_PRINT_ERROR_RETURN(ShellInfo_IsExecutable(shell, resolvedPath), "file is not an executable",);
	ListString_Insert(params, resolvedPath, 0);

	int statusCode = 0;
	ShellInfo_Execute(shell, params, &statusCode);
	printf("[status=%i]\n", statusCode);
}
