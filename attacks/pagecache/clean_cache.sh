#!/bin/bash
sync && echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null
