CC = gcc
ARGS = -Wall

all: make

make: simple_shell.c
	$(CC) $(ARGS) -g -o simple_shell simple_shell.c

clean:
	rm -f *.o make *~
