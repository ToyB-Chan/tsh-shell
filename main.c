#include "list.h"
#include <stdio.h>
#include "string.h"
#include "shell.h"
#include "builtincommands.h"

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

		if (success)
			continue;

		ExecuteFile(shell, params);
	}
	
}