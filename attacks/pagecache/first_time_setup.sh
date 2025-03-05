#!/bin/bash
set -e

if [ -e .setup_done ]; then
	echo "setup already done"
	exit 3
fi

./clean_cache.sh
max_megabytes_needed=$(free -b | ./parse_free.py | head -n1)
dd bs=1M count=$max_megabytes_needed </dev/urandom >victim/app/random20.img &
dd bs=1M count=$max_megabytes_needed </dev/urandom >attacker/app/random20_2.img &
wait
touch .setup_done
