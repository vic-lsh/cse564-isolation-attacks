#!/bin/bash

# Usage: ./use_mem.sh <amount of mem> <amount of time>
# Can use suffixes e.g. ./use_mem.sh 500m 5s 
head -c $1 /dev/zero | tac | sleep $2 > /dev/null
