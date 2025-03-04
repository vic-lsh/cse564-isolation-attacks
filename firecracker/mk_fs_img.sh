#!/bin/bash

set -e

# Create an empty disk image
dd if=/dev/zero of=data.ext4 bs=1M count=50

# Format it
mkfs.ext4 data.ext4

# Mount it to add files
sudo mkdir -p /mnt/data
sudo mount -o loop data.ext4 /mnt/data
sudo cp -r ./data/* /mnt/data/
sudo umount /mnt/data

echo "Completed syncing ./data to data.ext4"
