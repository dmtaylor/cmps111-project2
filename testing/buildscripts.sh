#!/usr/pkg/bin/bash

# CREATED 5/1/14
# This builds the testing programs that the testing scripts all use
# It also creates a large file for IO tests

cc -c cputest.c
cc -o cputest cputest.o
cc -c iotest.c
cc -o iotest iotest.o

dd if=/dev/urandom of=in bs=1024 count=10000