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

	
    // ensure terminal settings are set to non-canonical mode
    struct termios term;
    ret = tcgetattr(STDIN_FILENO, &term);
    assert(ret == 0);
    
    term.c_lflag &= ~(ICANON);
    ret = tcsetattr(STDIN_FILENO, TCSANOW, &term);
    assert(ret == 0);

	printf("tsh@%s> ", String_GetCString(shell->directory));
	fflush(stdout);

	while(true)
	{
		usleep(5000);
		JobManager_Tick(shell->jobManager);		

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

			printf("tsh@%s> ", String_GetCString(shell->directory));
			fflush(stdout);
		}
	}
	
}