#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{	
	printf("I want some input: ");
	fflush(stdout);

	char buffer[1024];
	int i = 0;
	for (; i < sizeof(buffer) - 1; i++)
	{
		int c = getc(stdin);
		if (c == EOF || c == '\n')
			break;

		buffer[i] = (char)c;
	}

	buffer[i + 1] = '\0';

	printf("\nYour input was: %s\n", buffer);
	printf("Bye!\n");
	fflush(stdout);
	return 0;
}