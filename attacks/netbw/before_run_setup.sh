#!/bin/bash

# Change to the directory containing the script
cd "$(dirname "$0")"

make
cp bin/netbw_vm attacker/app
cp bin/netbw_vm victim/app

./bin/netbw_host &
