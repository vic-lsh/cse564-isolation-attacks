#!/bin/bash

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

./mk_fs_img.sh vm1
./mk_fs_img.sh vm2


roles=$@
has_attacker=0
for role in "$roles"; do
    if [[ "$role" == "attacker" ]]; then
        has_attacker=1
        break
    fi
done

vmroot=
vm_cmd=
for role in $roles; do
    echo "running $attack/$role"

    vmroot="${vm_map[$role]}"

    log_suffix=$role
    if [[ "$role" == "victim" ]]; then
        if [[ $has_attacker -eq 1 ]]; then
            log_suffix=vicim
        else
            log_suffix=victim_standalone
        fi
    fi

    vm_cmd="/mnt/data/$attack/$role/app/run.sh"

    ./start_firecracker.sh $vmroot         > out_$log_suffix.txt 2>&1  &
    firecracker_pid=$!
    firecracker_pids+=($firecracker_pid)
    ./start_microvm.sh     $vmroot $vm_cmd > microvm_out_$log_suffix.txt 2>&1 &
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

sudo ps aux | grep "/tmp/firecracker_" | grep -v grep | awk '{print $2}' | xargs -r kill -9
# these firecracker scripts can make the terminal behavior funky. do a reset here to fix.
reset

