#!/usr/pkg/bin/bash
time > test1 nice -n 0 ./test &
time > test2 ./test &
