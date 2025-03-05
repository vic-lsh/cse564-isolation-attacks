#!/bin/bash

if [ ! -e .setup_done ]; then
	echo "First time setup not done."
	exit 1
fi

if [ -e .ready_to_run ]; then
	echo "System already ready to run!"
	exit 2
fi

# get num of free memory to use
./clean_cache.sh
free -b | ./parse_free.py | tail -n1 > bytes_to_read
mv bytes_to_read victim/app/bytes_to_read

touch .ready_to_run
