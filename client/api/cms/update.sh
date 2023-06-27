#!/bin/sh

# 检查是否提供了参数
if [ $# -lt 2 ]; then
  echo "Invalid argument. Usage: $0 <workspace> <node>"
  exit 1
fi

workspace=$1
source prepare_path.sh $workspace $2

ret=$(whisp call main nodeInfo --workspace=$workspace \
    --app=cms -- "$path" | jq '.[0]' | cat -)

ok=$(echo "$ret" | jq -r '.ok')

if [ "$ok" == "false" ]; then
    echo The node $2 does not exist.
    exit 1
fi

node=$(echo "$ret" | jq -r '.node | del(.summary, .extData)')

updatedJson=$(cmsUpdate "$node" 3>&1 1>&2 2>&3)

if [ -n "$updatedJson" ]; then
    nodeID=$(echo "$updatedJson" | jq -r '.id')
    result=$(whisp call main updateNode --workspace=$workspace \
        --app=cms -- "$nodeID" "$updatedJson" | jq -r '.[0]')
    if [ $(echo "$result" | jq -r '.ok') = "false" ]; then
        echo "Can't save content. $(echo "$result" | jq -r '.error')"
    fi
fi
