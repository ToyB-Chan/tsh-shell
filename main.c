#include "list.h"
#include <stdio.h>
#include "string.h"
#include "shell.h"
#include "builtincommands.h"
#include "jobmanager.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

int main()
{
	ShellInfo* shell = ShellInfo_New();

	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	assert(flags >= 0);
	int ret = fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
	assert(ret >= 0);

	
    // Step 3: Ensure terminal settings are set to non-canonical mode (optional but often necessary)
    struct termios term;
    ret = tcgetattr(STDIN_FILENO, &term);
    assert(ret == 0);
    
    term.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    ret = tcsetattr(STDIN_FILENO, TCSANOW, &term);
    assert(ret == 0);

	while(true)
	{
		usleep(50000);
		JobManager_Tick(shell->jobManager);		

		bool cmdReady = false;

		printf("\033[F"); // up one line
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
    		printf("\033[K"); // clear line (effectively removing the shell prompt so we can redraw it)
			printf("tsh@%s> %s", String_GetCString(shell->directory), String_GetCString(shell->inputBuffer));
			fflush(stdout);
		}

		printf("\n");

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
	}
	
}