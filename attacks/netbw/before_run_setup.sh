#!/bin/bash

# Change to the directory containing the script
cd "$(dirname "$0")"

make

./bin/netbw_host &
