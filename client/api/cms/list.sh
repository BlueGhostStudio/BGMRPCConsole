#!/bin/bash


# 检查是否提供了参数
if [ $# -lt 1 ]; then
  echo "参数错误。用法: $0 <workspace>"
  exit 1
fi

workspace=$1

source prepare_path.sh $workspace $2

echo PATH: $path

whisp call main list --workspace=$workspace --app=cms -- $path \
    | jq -r '.[0].list' \
    | jq  -r --argjson title \
        '["id", "name", "title", "data", "own", "seq", "hide", "type", "contentType"]' \
        -f ../default/table.jq | column -t -s$'\t' -o'   '
