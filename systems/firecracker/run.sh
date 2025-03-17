#!/bin/bash
set -x
set -e

attack=$(basename $(pwd))
firecracker_dir=../../systems/firecracker

declare -A vm_map
vm_map["attacker"]="vm1"
vm_map["victim"]="vm2"

cd $firecracker_dir

firecracker_pids=()
microvm_pids=()
cleanup() {
    echo ""
    echo "Ctrl+C detected. Killing background processes..."
    for pid in "${firecracker_pids[@]}"; do
        sudo kill -9 $pid
    done
    for pid in "${microvm_pids[@]}"; do
        sudo kill -9 $pid
    done
    exit 0
}

# Set up trap for Ctrl+C (SIGINT)
trap cleanup SIGINT


vmroot=
vm_cmd=
roles=$@
for role in $roles; do
    echo "running $attack/$role"

    vmroot="${vm_map[$role]}"

    vm_cmd="/mnt/data/$attack/$role/app/run.sh"

    ./start_firecracker.sh $vmroot         > out_$role.txt 2>&1  &
    firecracker_pid=$!
    firecracker_pids+=($firecracker_pid)
    ./start_microvm.sh     $vmroot $vm_cmd > microvm_out_$role.txt 2>&1 &
    microvm_pid=$!
    microvm_pids+=($microvm_pid)

done

for pid in "${microvm_pids[@]}"; do
    wait $pid
    echo "Process $pid completed with status $?"
done

for pid in "${firecracker_pids[@]}"; do
    sudo kill -9 $pid
done

sudo pkill -9 -f firecracker
# these firecracker scripts can make the terminal behavior funky. do a reset here to fix.
reset

