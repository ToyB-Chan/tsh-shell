#include "jobmanager.h"
#include "shell.h"
#include "string.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

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
	job->params = ListString_Copy(params);
	job->status = JS_Pending;

	job->pid = 0;
	job->inFile = NULL;
	job->outFile = NULL;

	job->inBuffer = NULL;
	job->outBuffer = NULL;

	job->exitStatus = 0;
	job->needsCleanup = false;

	ListJobInfo_Add(manager->jobs, job);
	return job;
}

void JobManager_DestroyJob(JobManager* manager, JobInfo* job)
{
	assert(manager);
	assert(job);
	assert(job->status != JS_Running);
	assert(!job->needsCleanup);

	size_t index = ListJobInfo_Find(manager->jobs, job);
	assert(index != INVALID_INDEX);
	ListJobInfo_Remove(manager->jobs, index);

	for (size_t i = 0; i < job->params->numElements; i++)
		String_Destroy(ListString_Get(job->params, i));
	ListString_Destroy(job->params);
	free(job);
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

void JobManager_Tick(JobManager* manager)
{
	for (size_t i = 0; i < manager->jobs->numElements; i++)
	{
		JobInfo* job = ListJobInfo_Get(manager->jobs, i);
		if (!job->needsCleanup)
			continue;

		FILE* pipeOutStream = fdopen(job->outPipe[0], "r");
		assert(pipeOutStream);

		while(1)
		{
			int c = fgetc(pipeOutStream);
			if (c == EOF)
				break;

			if (c == '\n')
			{
				if (job->outFile)
				{

				}
				else
				{
					String* jobinfo = JobInfo_ToShellString(job);
					printf("%s %s", String_GetCString(jobinfo), String_GetCString(job->outBuffer));
					String_Destroy(jobinfo);
					String_Reset(job->outBuffer);
				}

				continue;
			}

			String_AppendChar(job->outBuffer, (char)c);
		}

		fclose(pipeOutStream);
		pipeOutStream = NULL;

		pid_t tpid = waitpid(job->pid, &job->exitStatus, WNOHANG);
		if (tpid == job->pid)
		{
			if (job->status == JS_Running)
				job->status = JS_Finished;

			JobInfo_Cleanup(job);
			printf("[job %li has finished executing]\n[status=%i]\n", job->id, WEXITSTATUS(job->exitStatus));
		}
	}
	
}

void JobInfo_Execute(JobInfo* job, ShellInfo* shell)
{
	assert(job);
	assert(job->pid == 0);
	assert(job->status == JS_Pending);

	assert(pipe(job->inPipe) == 0);
	assert(pipe2(job->outPipe, O_NONBLOCK) == 0);

	job->inBuffer = String_New();
	job->outBuffer = String_New();

	pid_t cpid = fork();
	if (cpid == 0)
	{
		dup2(job->inPipe[0], STDIN_FILENO);
		dup2(job->outPipe[1], STDOUT_FILENO);

		close(job->inPipe[0]);
		close(job->inPipe[1]);
		close(job->outPipe[0]);
		close(job->outPipe[1]);

		String* filePath = ListString_Get(job->params, 0);

		// needs nullptr as last element to mark the end
		char** argv = (char**)calloc(job->params->numElements + 1, sizeof(char*));
		assert(argv);
		for (size_t i = 0; i < job->params->numElements; i++)
		{
			argv[i] = String_GetCString(ListString_Get(job->params, i));
		}

		chdir(String_GetCString(shell->directory));
		execvp(String_GetCString(filePath), argv);
		exit(EXIT_STATUS_COMMAND_NOT_FOUND);
	}
	else if (cpid < 0)
	{
		job->status = JS_Faulted;
	}

	close(job->inPipe[0]);
	close(job->outPipe[1]);

	job->pid = cpid;
	job->status = JS_Running;
	job->needsCleanup = true;
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

	switch (job->status)
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

void JobInfo_Cleanup(JobInfo* job)
{
	assert(job);
	assert(job->status >= JS_Finished);
	assert(job->needsCleanup);

	close(job->inPipe[1]);
	close(job->outPipe[0]);

	if (job->inFile)
		fclose(job->inFile);

	if (job->outFile)
		fclose(job->outFile);

	String_Destroy(job->inBuffer);
	String_Destroy(job->outBuffer);

	job->needsCleanup = false;
}