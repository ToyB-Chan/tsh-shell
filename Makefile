CC = gcc
CFLAGS = -Wall -std=gnu11

tsh:
	$(CC) $(CFLAGS) -o tsh jasotto_jobmanager.c jasotto_main.c jasotto_shell.c jasotto_string.c jasotto_signalhandler.c
	$(CC) $(CFLAGS) -o test_printstdout jasotto_test_printstdout.c
	$(CC) $(CFLAGS) -o test_wait jasotto_test_wait.c
	$(CC) $(CFLAGS) -o test_input jasotto_test_input.c

clean:
	rm -f tsh
	rm -f test_printstdout
	rm -f test_wait
	rm -f test_input