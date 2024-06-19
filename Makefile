CC = gcc
CFLAGS = -Wall -std=gnu11 -ggdb

tsh: builtincommands.c jobmanager.c main.c shell.c string.c
	$(CC) $(CFLAGS) -o tsh builtincommands.c jobmanager.c main.c shell.c string.c
	$(CC) $(CFLAGS) -o test_printstdout test_printstdout.c
	$(CC) $(CFLAGS) -o test_wait test_wait.c

clean:
	rm -f tsh
	rm -f test_printstdout