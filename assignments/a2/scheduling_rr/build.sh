#!/bin/bash

# compiler and then our flags
CXX=gcc
CXXFLAGS="-Wall -Wextra -pedantic"

$CXX $CXXFLAGS -o main main.c
