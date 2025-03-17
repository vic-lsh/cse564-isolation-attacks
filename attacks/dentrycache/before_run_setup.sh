#!/bin/bash

set -e 

echo "dentrycache before_run"

make
cp bin/dentry_attack attacker/app
cp bin/dentry_victim victim/app
