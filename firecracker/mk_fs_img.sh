#!/bin/bash

set -e

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <vm-rootdir>"
    exit 1
fi

VMROOT=$1
mkdir -p $VMROOT
cd $VMROOT

MOUNT_DIR=/mnt/data
cleanup() {
    echo "Performing cleanup..."
    if mountpoint -q "$MOUNT_DIR"; then
        echo "Unmounting $MOUNT_DIR"
        sudo umount "$MOUNT_DIR" || echo "Warning: Failed to unmount $MOUNT_DIR"
    fi
}
trap cleanup EXIT

# remake the symlink
HOST_DATA_DIR=data
rm -rf $HOST_DATA_DIR
ln -s ../data $HOST_DATA_DIR

# Create an empty disk image
dd if=/dev/zero of=data.ext4 bs=1M count=50

# Format it
mkfs.ext4 data.ext4

# Mount it to add files
sudo mkdir -p $MOUNT_DIR
sudo mount -o loop data.ext4 $MOUNT_DIR
sudo cp -r ./$HOST_DATA_DIR/* $MOUNT_DIR

echo "Completed syncing ./$VMROOT/$HOST_DATA_DIR to data.ext4"
