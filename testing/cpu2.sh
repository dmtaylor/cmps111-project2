#!/usr/pkg/bin/bash
time > test1 nice -n 20 ./test &
time > test2 ./test &
