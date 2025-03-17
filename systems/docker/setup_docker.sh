#!/bin/bash
set -e

# this script assumes you have docker installed...
# compile dockerfile with everything we need

if [ -e .setup_done ]; then
	echo "setup already done"
	exit 3
fi

sudo docker build -t cs564-runner .

touch .setup_done
