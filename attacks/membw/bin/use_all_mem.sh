#!/bin/bash
set -e

numProcs=$1

# figure amt of avail memory and use it all
memKB=$(cat /proc/meminfo | grep MemAvailable | cut -c14-25)
#pageSizeKB=4096
fillFactorTop=4
fillFactorBottom=5
#memPages=$((memKB * fillFactorTop / pageSizeKB / fillFactorBottom))
memPages=$numProcs
pageSizeKB=$((memKB * fillFactorTop / numProcs / fillFactorBottom))

tim="5s"

echo "num procs: $memPages"
echo "using ${pageSizeKB}k of mem for $tim on each"

for i in $(seq 1 $memPages); do
	./use_mem.sh ${pageSizeKB}k $tim &
done
wait
