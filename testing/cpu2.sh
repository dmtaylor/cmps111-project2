#!/usr/pkg/bin/bash

# CREATED 5/1/14
# This test runs two cpu intensive programs, one with double tickets.
# It is expected the programs should finish with a ratio of 3:4
# as shown in the proof on piazza.

time > test1 nice -n 20 ./cputest &
time > test2 ./cputest &
