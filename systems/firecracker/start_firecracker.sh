#!/bin/bash

set -e

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <vm-rootdir>"
    exit 1
fi

VMROOT=$1

API_SOCKET="/tmp/firecracker_$VMROOT.socket"

# Remove API unix socket
sudo rm -f $API_SOCKET

# Run firecracker
sudo firecracker --api-sock "${API_SOCKET}"
