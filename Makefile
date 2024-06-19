CC = gcc
CFLAGS = -Wall -std=c11 -ggdb

tsh: builtincommands.c jobmanager.c main.c shell.c string.c
	$(CC) $(CFLAGS) -o tsh builtincommands.c jobmanager.c main.c shell.c string.c

clean:
	rm -f tsh