#include "shell.h"
#include "signalhandler.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <assert.h>

#define UPDATE_TIME_MS 15

int main()
{
	// Set the input stream to non-blocking
	{
		int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
		assert(flags >= 0);
	
		int ret = fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
		assert(ret >= 0);
	}
	
	// Set the terminal to non-canonical mode (disables input processing by the terminal, which also blocks the input stream)
	// See: https://man7.org/linux/man-pages/man3/termios.3.html
	{
		int ret;
		struct termios term;

		ret = tcgetattr(STDIN_FILENO, &term);
		assert(ret == 0);

		term.c_lflag &= ~(ICANON);
		ret = tcsetattr(STDIN_FILENO, TCSANOW, &term);
		assert(ret == 0);
	}

	ShellInfo* shell = ShellInfo_New();
	signal(SIGABRT, SignalHandlerAbort)

	while(true)
	{
		ShellInfo_Tick(shell);
		g_abortRequested = false;
		usleep(UPDATE_TIME_MS * 1000);

		if (shell->exitRequested)
			break;
	}
	
	ShellInfo_Destroy(shell);
	shell = NULL;
}