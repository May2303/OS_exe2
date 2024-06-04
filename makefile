CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: mync3 mync4 ttt

mync3: mync3.c
	$(CC) $(CFLAGS) -o mync3 mync3.c

mync4: mync4.c
	$(CC) $(CFLAGS) -o mync4 mync4.c

ttt: ttt.c
	$(CC) $(CFLAGS) -o ttt ttt.c

clean:
	rm -f mync3 mync4 ttt
