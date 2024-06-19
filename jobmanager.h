#pragma once

#include "stdint.h"
#include "list.h"
#include <stdatomic.h>
#include <sys/types.h>

typedef struct ListString ListString;
typedef struct String String;
typedef struct ShellInfo ShellInfo;

typedef enum JobState
{
	JS_Pending,
	JS_Running,
	JS_Aborted,
	JS_Faulted,
	JS_Finished
} JobState;

typedef struct JobInfo
{
	size_t id;
	ListString* params;
	atomic_uint state;
	pid_t pid;
	int statusCode;
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

void JobInfo_Execute(JobInfo* job, ShellInfo* shell);
String* JobInfo_ToString(JobInfo* job);