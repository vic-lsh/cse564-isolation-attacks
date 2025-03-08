# Firecracker

This folder contains the scripts used to study firecracker's isolation properties.

## Getting started

```bash
./download_firecracker.sh         # run once
```

See the runner.sh script at the repository root for how to run attacks.

Use the following files to debug outputs from firecracker and vms:

```
./out_victim.txt            # firecracker output for victim vm
./microvm_out_victim.txt    # vm's output for the victim vm
./out_victim.txt            # firecracker output for attacker vm
./microvm_out_victim.txt    # vm's output for the attacker vm
```
