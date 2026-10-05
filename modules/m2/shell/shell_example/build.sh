#! /usr/bin/env bash

gcc -Wall -Wextra -pedantic -g -O3 example.c -o example
gcc -Wall -Wextra -pedantic -g -O3 example_fork.c -o example_fork