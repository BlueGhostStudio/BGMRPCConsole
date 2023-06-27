#!/bin/bash

# 检查是否提供了参数
if [ $# -ne 1 ]; then
  echo "参数错误。用法: $0 <scenario>"
  exit 1
fi

workspace=$1

# 读取标准输入
read input

if [ "$input" -eq 3 ]; then
    token=$(whisp env token --workspace=$workspace)

    if [ -n "$token" ]; then
        login.sh "$workspace" "$token"
    fi
fi
