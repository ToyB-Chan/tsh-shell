#include "list.h"
#include <stdio.h>
#include "string.h"
#include "shell.h"
#include "builtincommands.h"

DECLARE_LIST(ListInt, int);
DEFINE_LIST(ListInt, int);

void printList(ListInt* list)
{
	printf("list start\n");
	for (int i = 0; i < list->numElements; i++)
		printf("index: %i, value: %i\n", i, ListInt_Get(list, i));
	printf("list end\n");
}

int main()
{

	ShellInfo* shell = ShellInfo_New();

	while(true)
	{
		printf("tsh> ");

		String* paramString = String_New();
		while(1)
		{
			int c = getc(stdin);
			if (c == EOF || c == '\n')
				break;

			String_AppendChar(paramString, (char)c);
		}

		ListString* params = String_Split(paramString, ' ');
		String_Destroy(paramString);
		paramString = NULL;

		bool success = ExecuteBuiltinCommand(shell, params);

		if (!success)
			printf("[builtin command not found!]\n[status=1]\n");
	}
	
}