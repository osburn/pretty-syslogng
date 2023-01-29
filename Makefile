CC=gcc
CFLAGS=-Wall -Wextra 
# CFLAGS=-Wall -Wextra -g3 -ggdb3


pretty: pretty.o
	$(CC) -o pretty pretty.o
	strip pretty

clean:
	rm -f ./*.o pretty
