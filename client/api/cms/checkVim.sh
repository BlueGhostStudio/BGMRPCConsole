#!/bin/sh

PID=$$
RunInVIM=0
while true; do
    PID=$(ps -o ppid= -p $PID 2>/dev/null)
    if [ $? -ne 0 ]; then
        break
    fi

    process_name=$(ps -o comm= -p $PID 2>/dev/null)
    if [ "$process_name" = "vim" ]; then
        RunInVIM=1
        break
    fi
done

