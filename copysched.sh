#!/usr/pkg/bin/bash

# created 2014-5-2
# Automates copying MINIX scheduler files

cp servers/pm/scheduler.c /usr/src/servers/pm/scheduler.c
cp servers/sched/scheduler.c /usr/src/servers/sched/scheduler.c
cp servers/sched/schedproc.h /usr/src/servers/sched/schedproc.h
cd /usr/src/tools
make install
