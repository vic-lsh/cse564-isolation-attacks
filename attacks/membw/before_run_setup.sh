#!/bin/bash

if [ ! -e .setup_done ]; then
	echo "First time setup not done."
	exit 1
fi

if [ -e .ready_to_run ]; then
	echo "System already ready to run!"
	exit 2
fi

cp bin/* attacker/app/
cp bin/* victim/app/

touch .ready_to_run
