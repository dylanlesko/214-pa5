CC = gcc
CFLAGS = -pthread -g -o
CFILES = book.c

HFILES = headers.h
all: malloc

malloc : $(CFILES)
	$(CC) $(CFLAGS) run $(CFILES)
