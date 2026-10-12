CC       = gcc
CFLAGS   = -D _DEFAULT_SOURCE  -Wall -Wextra -pedantic -std=c11 -g -O3
LDFLAGS  = -pthread

PROGRAM       = main
SRCS          = main.c
OBJS          = $(SRCS:.c=.o)
TEST_PROGRAM  = tests/test_log_files

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# shaun's silly personal thing
remake: clean all run tests

run: $(PROGRAM) cleanlogs
	./run.sh

tests: $(TEST_PROGRAM)
	./$(TEST_PROGRAM)  # run the test suite below ;)

$(TEST_PROGRAM): tests/test_log_files.c
	rm -f $(TEST_PROGRAM)
	$(CC) $(CFLAGS) -o $(TEST_PROGRAM) $^

clean: cleanlogs
	rm -f $(PROGRAM) $(OBJS) $(TEST_PROGRAM)

cleanlogs:
	rm -rf logFolder

.PHONY: all remake run tests clean cleanlogs
