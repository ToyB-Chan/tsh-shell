#include "jobmanager.h"
#include "shell.h"
#include "string.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

DEFINE_LIST(ListJobInfo, JobInfo*);

JobManager* JobManager_New()
{
	JobManager* manager = (JobManager*)malloc(sizeof(JobManager));
	manager->nextJobId = 0;
	manager->jobs = ListJobInfo_New();
	return manager;
}

void JobManager_Destroy(JobManager* manager)
{
	assert(manager);
	assert(manager->jobs->numElements == 0);
	ListJobInfo_Destroy(manager->jobs);
	free(manager);
}

JobInfo* JobManager_CreateJob(JobManager* manager, ListString* params)
{
	assert(manager);
	assert(params);
	JobInfo* job = (JobInfo*)malloc(sizeof(JobInfo));
	job->id = manager->nextJobId++;
	job->params = params;
	job->status = ATOMIC_VAR_INIT(JS_Pending);
	job->pid = 0;
	job->exitStatus = 0;

	return job;
}

void JobManager_DestroyJob(JobManager* manager, JobInfo* job)
{
	assert(manager);
	assert(job);
	assert(atomic_load(&job->status) != JS_Running);
}

JobInfo* JobManager_FindJobById(JobManager* manager, size_t jobId)
{
	assert(manager);
	for (size_t i = 0; i < manager->jobs->numElements; i++)
	{
		if (ListJobInfo_Get(manager->jobs, i)->id == jobId)
			return ListJobInfo_Get(manager->jobs, i);
	}

	return NULL;
}

void JobInfo_Execute(JobInfo* job, ShellInfo* shell)
{
	assert(job);
	assert(job->pid == 0);
	pid_t cpid = fork();
	if (cpid == 0)
	{
		atomic_exchange(&job->status, JS_Running);
		bool success = ShellInfo_Execute(shell, job->params, &job->exitStatus);
		String* jobStr = JobInfo_ToShellString(job);
		if (success)
		{
			atomic_exchange(&job->status, JS_Finished);
			printf("[%s finished execution]\n", String_GetCString(jobStr));
		}
		else
		{
			printf("[%s an error occured during execution of the command]\n", String_GetCString(jobStr));
			atomic_exchange(&job->status, JS_Faulted);
		}

		printf("[status=%i]\n", job->exitStatus);
		String_Destroy(jobStr);
		exit(job->exitStatus);
	}
	else if (cpid < 0)
	{
		atomic_exchange(&job->status, JS_Faulted);
	}

	job->pid = cpid;
}

String* JobInfo_ToInfoString(JobInfo* job)
{
	assert(job);
	String* str = String_New();
	String* temp = NULL; 

	temp = String_Itoa(job->id);
	String_AppendString(str, temp);
	String_Destroy(temp);

	String_AppendCString(str, "(pid\t");

	temp = String_Itoa(job->pid);
	String_AppendString(str, temp);
	String_Destroy(temp);

	String_AppendChar(str, '\t');

	switch (atomic_load(&job->status))
	{
	case JS_Pending:
		String_AppendCString(str, "pending");
		break;

	case JS_Running:
		String_AppendCString(str, "running");
		break;

	case JS_Finished:
		String_AppendCString(str, "finished");
		break;

	case JS_Aborted:
		String_AppendCString(str, "aborted");
		break;

	case JS_Faulted:
		String_AppendCString(str, "faulted");
		break;
	
	default:
		assert(false);
		break;
	}

	String_AppendChar(str, '\t');
	String_AppendCString(str, "status=");

	temp = String_Itoa(job->exitStatus);
	String_AppendString(str, temp);
	String_Destroy(temp);

	String_AppendCString(str, "): ");

	temp = String_Join(job->params, ' ');
	String_AppendString(str, temp);
	String_Destroy(temp);
	return str;
}

String* JobInfo_ToShellString(JobInfo* job)
{
	assert(job);
	String* str = String_New();
	String* temp = NULL; 

	String_AppendCString(str, "[job: ");
	temp = String_Itoa(job->id);
	String_AppendString(str, temp);
	String_Destroy(temp);
	String_AppendCString(str, "]");

	return str;
}