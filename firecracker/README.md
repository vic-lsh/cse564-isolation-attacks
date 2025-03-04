# Firecracker

This folder contains the scripts used to study firecracker's isolation properties.

## Getting started

The data/ subdirectory contains data that is copied into each VM. This can be
used, for example, to include attack programs in each VM.

```bash
./download_firecracker.sh         # run once

# then, for each vm (vm1, vm2, ...):
mk_linux_img.sh     <vm-dir>      # once per vm
mk_fs_img.sh        <vm-dir>      # when data/ changes

# run everytime one starts a new vm
start_firecracker.sh <vm-dir>     # run in one terminal
start_microvm.sh     <vm-dir>     # run in one terminal
```
