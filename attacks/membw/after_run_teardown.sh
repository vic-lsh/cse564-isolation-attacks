#!/bin/bash
set -e
if [ ! -e .ready_to_run ]; then
	echo "System not ready to run!"
	exit 2
fi

rm .ready_to_run
