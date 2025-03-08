#!/bin/bash

# Change to the directory containing the script
cd "$(dirname "$0")"

timeout 10s ../../bin/forkbomb &
pgid=$!

sleep 10.5

pkill -9 -g $pgid
