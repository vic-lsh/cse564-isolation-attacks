#!/bin/bash
set -e

SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

if [ ! -e $SCRIPTPATH/../docker/.setup_done ]; then
	echo "docker setup not done"
	exit 4
fi

total_memory=$(free -b | grep -oP '\d+' | head -n 1)
total_memory_over_22=$(python3 -c "print(int($total_memory / 2.2))")

people=$@
for person in $people; do
	if [ ! -e .ready_to_run ]; then
		echo "System not ready to run"
		exit 2
	fi
	cd $person/app
	# run app process
	sudo docker run --memory=$total_memory_over_22 --cpuset-cpus="0-27" -v $(pwd):/mnt/app --rm --runtime=runsc cs564-runner &
	cd ../..
done
wait
