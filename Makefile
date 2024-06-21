CC = gcc
CFLAGS = -Wall -std=gnu11 -ggdb

tsh: jobmanager.c main.c shell.c string.c
	$(CC) $(CFLAGS) -o tsh jobmanager.c main.c shell.c string.c
	$(CC) $(CFLAGS) -o test_printstdout test_printstdout.c
	$(CC) $(CFLAGS) -o test_wait test_wait.c
	$(CC) $(CFLAGS) -o test_input test_input.c

clean:
	rm -f tsh
	rm -f test_printstdout
	rm -f test_wait
	rm -f test_input