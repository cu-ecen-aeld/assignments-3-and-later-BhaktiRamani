CROSS_COMPILE ?=
CC ?= $(CROSS_COMPILE)gcc


CFLAGS ?= -g -Wall -Werror -pthread
LDFLAGS ?=

default : aesdsocket

all: aesdsocket

aesdsocket : aesdsocket.o
	$(CC) $(CFLAGS) aesdsocket.o -o aesdsocket

aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c

clean:
	-rm -f *.o aesdsocket
