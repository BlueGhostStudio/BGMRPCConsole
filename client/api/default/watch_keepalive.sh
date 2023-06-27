#!/bin/bash

workspace="$1"

keepalive_interval=$(whisp env keepalive_interval)
if [ -z "$keepalive_interval" ]; then
    keepalive_interval=5
fi

connection_timeout=$(whisp env connection_timeout)
if [ -z "$connection_timeout" ]; then
    connection_timeout=10
fi

keepalive_pid=$(whisp env keepalive_pid -w $workspace)

if [ -n "$keepalive_pid" ]; then
    kill "$keepalive_pid"
fi

whisp env keepalive_pid -v $$ -w $workspace > /dev/null

sleep $keepalive_interval

whisp ping "$workspace"
exit_code=$?
if [ $exit_code -eq 128 ]; then
    exit 1
fi

sleep $connection_timeout

notify-send "Workspace: ${workspace} - Server Connection Timeout" "Attempting to reconnect..." -a "whisp"
whisp env keepalive_pid -v "" -w $workspace
whisp reconnect "$workspace"

