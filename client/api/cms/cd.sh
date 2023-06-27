#!/bin/bash


# 检查是否提供了参数
if [ $# -lt 1 ]; then
  echo "参数错误。用法: $0 <workspace>"
  exit 1
fi

workspace=$1

source prepare_path.sh $workspace $2

result=$(whisp call main nodeInfo --workspace=$workspace --app=cms -- $path)

ok=$(echo $result | jq -r '.[0].ok')
if [ "$ok" = "true" ]; then
    type=$(echo $result | jq -r '.[0].node.type')
    if [ "$type" = "D" ]; then
        path=$(echo "$path" | sed -e 's|/[^/]\+/\.\.||g' -e 's|^/\.\.||')
        if [ -z "$path" ]; then
            path="/"
        fi
        whisp env wd -v $path --workspace=$workspace > /dev/null
    else
        echo Specified path is not a directory.
    fi
else
    echo Specified path does not exist.
fi
