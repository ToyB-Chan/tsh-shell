#pragma once

#include "stdint.h"
#include "list.h"
#include <stdatomic.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct ListString ListString;
typedef struct String String;
typedef struct ShellInfo ShellInfo;

typedef enum JobStatus
{
	JS_Pending = 0,
	JS_Running = 1,
	JS_Finished = 2,
	JS_Killed = 3,
	JS_Faulted = 4,
} JobStatus;

typedef struct JobInfo
{
	size_t id;
	ListString* params;
	JobStatus status;

	pid_t pid;
	int inPipe[2];
	int outPipe[2];
	FILE* inFile;
	FILE* outFile;

	String* inBuffer;
	String* outBuffer;
	
	int exitStatus;
	bool needsCleanup;
} JobInfo;

DECLARE_LIST(ListJobInfo, JobInfo*);
typedef struct JobManager
{
	size_t nextJobId;
	ListJobInfo* jobs;
} JobManager;

JobManager* JobManager_New();
void JobManager_Destroy(JobManager* manager);
JobInfo* JobManager_CreateJob(JobManager* manager, ListString* params);
void JobManager_DestroyJob(JobManager* manager, JobInfo* job);
JobInfo* JobManager_FindJobById(JobManager* manager, size_t jobId);
void JobManager_Tick(JobManager* manager);

void JobInfo_Execute(JobInfo* job, ShellInfo* shell);
String* JobInfo_ToInfoString(JobInfo* job);
void JobInfo_Cleanup(JobInfo* job);
int JobInfo_GetExitCode(JobInfo* job);