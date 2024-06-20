#include "list.h"
#include <stdio.h>
#include "string.h"
#include "shell.h"
#include "builtincommands.h"
#include "jobmanager.h"
#include <unistd.h>
#include <fcntl.h>

int main()
{
	ShellInfo* shell = ShellInfo_New();

	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	assert(flags >= 0);
	int ret = fcntl(STDERR_FILENO, F_SETFL, flags | O_NONBLOCK);
	assert(ret >= 0);

	while(true)
	{
		printf("\033[F"); // up one line
    	printf("\033[K"); // clear line (effectively removing the shell prompt)
		JobManager_Tick(shell->jobManager);

		printf("tsh@%s> %s", String_GetCString(shell->directory), String_GetCString(shell->inputBuffer));
		fflush(stdout);

		bool cmdReady = false;
		while(1)
		{
			int c = getc(stdin);
			if (c == EOF)
				break;

			if (c == '\n')
			{
				cmdReady = true;
				break;
			}

			String_AppendChar(shell->inputBuffer, (char)c);
		}

		if (cmdReady)
		{
			printf("\n");
			ListString* params = String_Split(shell->inputBuffer, ' ');
			String_Reset(shell->inputBuffer);

			bool success = ExecuteBuiltinCommand(shell, params);

			if (success)
			{
				ListString_Destroy(params);
				continue;
			}

			//ExecuteFile(shell, params);
			ListString_Destroy(params);
		}

		usleep(50000);
	}
	
}