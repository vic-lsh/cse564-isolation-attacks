#!/bin/bash
set -e

workloads=$@
for workload in $workloads; do
	if [ ! -e .ready_to_run ]; then
		echo "System not ready to run"
		exit 2
	fi
	cd $workload/app
	# run app process
	numactl --localalloc --cpunodebind=0 ./run.sh&
	cd ../..
done
wait
