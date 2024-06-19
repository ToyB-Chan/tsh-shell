#include <stdio.h>
#include <unistd.h>

int main()
{
	printf("I'll wait!\n");
	for (int i = 0; i < 3; i++)
	{
		printf("Sleeping since %i seconds\n", i + 1);
		sleep(1);
	}

	printf("Done!\n");
	return 0;
}