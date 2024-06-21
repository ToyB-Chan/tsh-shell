#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{	
	printf("I want some input: \n");

	char buffer[1024];
	fread(buffer, sizeof(char), sizeof(buffer) - 1, stdin);
	buffer[sizeof(buffer) - 1] = '\0';

	printf("Your input was: %s\n", buffer);
	printf("Bye!\n");
	return 0;
}