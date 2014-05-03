#!/usr/pkg/bin/bash

# CREATED 5/1/14
# This test runs two cpu intensive programs each with equal numbers of tickets
# It is expected the programs should finish in equal amounts of time.

time > test1 ./cputest &
time > test2 ./iotest &
