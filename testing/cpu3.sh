#!/usr/pkg/bin/bash
time > test1 nice -n 5 ./test &
time > test2 nice -n 30 ./test &
time > test3 nice -n 80 ./test &
