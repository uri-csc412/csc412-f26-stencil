CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS := $(SAN)
CFLAGS  += $(SAN)

all: clean build-example build-tests run-tests

arena.o: arena.c arena.h
	$(CC) $(CFLAGS) -c arena.c

tests.o: tests.c arena.h
	$(CC) $(CFLAGS) -c tests.c

build-example:
	$(CC) $(CFLAGS) -o arena_example main.c arena.c

build-tests: tests.o arena.o
	$(CC) $(CFLAGS) -o tests $^ $(LDFLAGS)

run-tests:
	./tests

run-example:
	./arena_example

clean:
	rm -f *.o tests
	rm -f *.o arena_example
