#!/bin/sh
set -eu
cd "$(dirname "$0")"
for task in 9 10 17 18 21 25; do
    (cd "Task_$task" && make)
done
python3 -m unittest -v test_process_pipe test_listing_signal
python3 Task_17/test_editor.py
