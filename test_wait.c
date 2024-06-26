#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

bool SafeAtoi(const char* str, int* outInt)
{
	int n = atoi(str);
	if (n == 0 && str[0] != '0')
		return false;
	
	*outInt = n;
	return true;
}

int main(int argc, char** argv)
{	
	int seconds = 3;
	if(argc < 2)
	{
		printf("No Argument given, defauling to 3 seconds\n");
	}
	else if (!SafeAtoi(argv[1], &seconds))
	{
		printf("Invalid argument given, exiting...\n");
		return 1;
	}

	printf("I'll wait for %i seconds!\n", seconds);
	fflush(stdout);
	for (int i = 0; i < seconds; i++)
	{
		sleep(1);
		printf("Sleeping since %i seconds\n", i + 1);
		fflush(stdout);
	}

	printf("Done!\n");
	fflush(stdout);
	return 0;
}