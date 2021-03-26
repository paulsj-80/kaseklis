CC=gcc
DEPS=file_proc.h storage.h walker.h utils.h htable.h comp.h
SRC=file_proc.c storage.c walker.c utils.c htable.c comp.c
OBJ=$(patsubst %.c,%.o,$(SRC))
CFLAGS += -g

all: kaseklis test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

kaseklis: $(OBJ) main.o
	$(CC) -o $@ $^ $(CFLAGS)

test: $(OBJ) test.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm kaseklis test *.o -f

.PHONY: clean
