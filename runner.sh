#!/bin/bash
set -e

system=$1
attack=$2

cd attacks/$attack
./before_run_setup.sh

echo "Running victim unmodified..."
../../systems/$system/run.sh victim

./after_run_teardown.sh

./before_run_setup.sh

echo "Running victim + attack..."
../../systems/$system/run.sh victim attacker

./after_run_teardown.sh
