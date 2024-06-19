#include "jobmanager.h"
#include "shell.h"
#include "string.h"
#include <stdlib.h>
#include <unistd.h>

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
	assert(manager->jobs->numElements = 0);
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
	job->state = ATOMIC_VAR_INIT(JS_Pending);
	job->pid = 0;
	job->statusCode = 0;

	return job;
}

void JobManager_DestroyJob(JobManager* manager, JobInfo* job)
{
	assert(manager);
	assert(job);
	assert(atomic_load(&job->state) != JS_Running);
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
		atomic_exchange(&job->state, JS_Running);
		bool success = ShellInfo_Execute(shell, job->params, &job->statusCode);
		if (success)
			atomic_exchange(&job->state, JS_Finished);
		else
			atomic_exchange(&job->state, JS_Faulted);

		exit(job->statusCode);
	}
	else if (cpid < 0)
	{
		atomic_exchange(&job->state, JS_Faulted);
	}

	job->pid = cpid;
}

String* JobInfo_ToString(JobInfo* job)
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

	switch (atomic_load(&job->state))
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

	temp = String_Itoa(job->statusCode);
	String_AppendString(str, temp);
	String_Destroy(temp);

	String_AppendCString(str, "): ");

	temp = String_Join(job->params, ' ');
	String_AppendString(str, temp);
	String_Destroy(temp);
	return str;
}