#!/bin/bash
set -e

if [ -e .setup_done ]; then
	echo "setup already done"
	exit 3
fi

touch .setup_done
