#!/bin/bash


# 检查是否提供了参数
if [ $# -lt 1 ]; then
  echo "参数错误。用法: $0 <workspace>"
  exit 1
fi

workspace=$1

tableJqPath=$(dirname "$(readlink -f "$0")")/../default/table.jq

source prepare_path.sh $workspace $2

echo PATH: $path

json=$(whisp call main list --workspace=$workspace --app=cms -- $path | jq -r '.[0].list')
echo "$json" | jq -r --argjson title '["id", "name", "title", "data", "own", "seq", "hide", "type", "contentType"]' \
    -f "$tableJqPath" | column -t -s$'\t' -o '   '

if [ -e /dev/fd/4 ]; then
    id=$(whisp call main nodeInfo --workspace=$workspace --app=cms -- $path | jq -r '.[0].id')
    json=$(echo "$json" \
        | jq -r '[.[]|{id,name,type}]' \
        | jq -r '@json' \
        | jq -aRS)
    echo "call WhispCMSList(${json}, \"$id\")" >&4
fi
