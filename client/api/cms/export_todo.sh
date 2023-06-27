#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Invalid argument. Usage: $0 <workspace>"
    exit 1
fi

workspace=$1

echo "export todo"$2 $3
