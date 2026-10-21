CC = gcc
CFLAGS = -Wall -Wextra -O2

all: player1 player2

player1: player1.c common.h
	$(CC) $(CFLAGS) player1.c -o player1

player2: player2.c common.h
	$(CC) $(CFLAGS) player2.c -o player2

clean:
	rm -f player1 player2
