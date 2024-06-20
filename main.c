#include "list.h"
#include <stdio.h>
#include "string.h"
#include "shell.h"
#include "builtincommands.h"
#include "jobmanager.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/wait.h>

int main()
{
	ShellInfo* shell = ShellInfo_New();

	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	assert(flags >= 0);
	int ret = fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
	assert(ret >= 0);

	
	// ensure terminal settings are set to non-canonical mode
	struct termios term;
	ret = tcgetattr(STDIN_FILENO, &term);
	assert(ret == 0);

	term.c_lflag &= ~(ICANON);
	ret = tcsetattr(STDIN_FILENO, TCSANOW, &term);
	assert(ret == 0);

	while(true)
	{
		printf("\033[2K\r"); // clear line (effectively removing the shell prompt so we can redraw it)
		JobManager_Tick(shell->jobManager);		

		if (shell->waitForJob == NULL)
		{
			bool cmdReady = false;
			while(1)
			{
				int c = getc(stdin);
				if (c == EOF)
					break;

				if (c == '\n')
				{
					cmdReady = true;
					printf("\033[A"); // go one line up again
					break;
				}

				// ascii backspace or acsii delete
				if (c == 8 || c == 127)
				{
					if (String_GetLength(shell->inputBuffer) > 0)
						String_RemoveAt(shell->inputBuffer, String_GetLength(shell->inputBuffer) - 1);
					continue;
				}

				String_AppendChar(shell->inputBuffer, (char)c);
			}

			printf("tsh@%s> %s", String_GetCString(shell->directory), String_GetCString(shell->inputBuffer));
			fflush(stdout);

			if (cmdReady)
			{
				printf("\n");
				ListString* params = String_Split(shell->inputBuffer, ' ');
				String_Reset(shell->inputBuffer);

				bool success = ExecuteBuiltinCommand(shell, params);

				if (success)
				{
					for (size_t i = 0; i < params->numElements; i++)
						String_Destroy(ListString_Get(params, i));
					ListString_Destroy(params);
					continue;
				}

				//ExecuteFile(shell, params);

				for (size_t i = 0; i < params->numElements; i++)
					String_Destroy(ListString_Get(params, i));
				ListString_Destroy(params);
			}
		}
		else
		{
			if (shell->waitForJob->status >= JS_Finished)
			{
				shell->waitForJob = NULL;
				printf("[waiting finished]\n");
				PRINT_SUCCESS();
			}
		}

		usleep(15000);
	}
	
}