#include <stdio.h>
#include <unistd.h>

int main()
{
	printf("I'll wait!\n");
	fflush(stdout);
	for (int i = 0; i < 3; i++)
	{
		sleep(1);
		printf("Sleeping since %i seconds\n", i + 1);
		fflush(stdout);
	}

	printf("Done!\n");
	fflush(stdout);
	return 0;
}