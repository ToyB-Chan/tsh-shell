#include "shell.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <assert.h>

int main()
{
	ShellInfo* shell = ShellInfo_New();
	int ret;
	
	// Set the input stream to no blocking
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	assert(flags >= 0);
	ret = fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
	assert(ret >= 0);
	
	// Set the terminal to non-canonical (disables input processing by the terminal, which also blocks the input stream)
	struct termios term;
	ret = tcgetattr(STDIN_FILENO, &term);
	assert(ret == 0);
	term.c_lflag &= ~(ICANON);
	ret = tcsetattr(STDIN_FILENO, TCSANOW, &term);
	assert(ret == 0);

	while(true)
	{
		ShellInfo_Tick(shell);
		usleep(15000);
	}
	
}