#!/bin/bash

# 检查是否提供了参数
if [ $# -lt 1 ]; then
  echo "参数错误。用法: $0 <scenario>"
  exit 1
fi

workspace=$1

if [ $# -ge 2 ]; then
    token=$2
else
    read -r -p "Token: " token
fi

echo "Token: " $token

whisp env token -v $token --workspace=$workspace > /dev/null
result=$(whisp call account login --workspace=$workspace --no-prefix -- $token | jq -r '.[0]')
echo login...$result...

