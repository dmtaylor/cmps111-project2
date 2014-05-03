#!/usr/pkg/bin/bash

# CREATED 5/1/14
# This test runs three cpu intensive programs each with double the tickets
# of the previous program.
# It is expected the programs should finish with a ratio of 7:10:12 per
# the proof on piazza.

time > test1 nice -n 5 ./cputest &
time > test2 nice -n 30 ./cputest &
time > test3 nice -n 80 ./cputest &
