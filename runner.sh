#!/bin/bash
set -e

if [ $# -ne 2 ]; then
    echo "Error: Exactly 2 arguments are required."
    echo "Usage: runner.sh <system> <attack>"
    exit 1
fi


system=$1
attack=$2

cd systems/$system
if [ -f ./before_run_setup.sh ]; then
    ./before_run_setup.sh
fi
cd ../..

cd attacks/$attack

if [ -f ./before_run_setup.sh ]; then
    ./before_run_setup.sh
fi

echo "Running victim unmodified..."
../../systems/$system/run.sh victim

if [ -f ./after_run_teardown.sh ]; then
    ./after_run_teardown.sh
fi

if [ -f ./before_run_setup.sh ]; then
    ./before_run_setup.sh
fi

echo "Running victim + attack..."
../../systems/$system/run.sh victim attacker

if [ -f ./after_run_teardown.sh ]; then
    ./after_run_teardown.sh
fi

cd ../..
cd systems/$system
if [ -f ./after_run_teardown.sh ]; then
    ./after_run_teardown.sh
fi
cd ../..
