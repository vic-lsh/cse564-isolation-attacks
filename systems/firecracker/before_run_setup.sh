#!/bin/bash

set -e

echo "setting up firecracker workloads"

./mk_fs_img.sh vm1
./mk_fs_img.sh vm2
