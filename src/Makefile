CC=gcc
DEPS=file_proc.h storage.h walker.h utils.h htable.h
SRC=file_proc.c storage.c walker.c utils.c htable.c
OBJ=$(patsubst %.c,%.o,$(SRC))
CFLAGS += -g -O2
SHELL=/bin/bash

all: kaseklis test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

kaseklis: $(OBJ) main.o
	$(CC) -o $@ $^ $(CFLAGS)

test: $(OBJ) test.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm kaseklis test *.o -f

install: kaseklis
	./do-install.sh

.PHONY: clean install
