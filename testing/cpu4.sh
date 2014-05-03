#!/usr/pkg/bin/bash

# CREATED 5/1/14
# This test runs three cpu intensive programs. Two of which have max tickets
# and one which has min tickets.
# It is expected the programs with max tickets will not prevent the 
# program with min tickets from ever executing.

time > test1 nice -n 80 ./cputest &
time > test2 nice -n -19 ./cputest &
time > test3 nice -n 80 ./cputest &
time > test4 nice -n 80 ./cputest &
