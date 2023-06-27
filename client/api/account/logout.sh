#!/bin/bash

# 检查是否提供了参数
if [ $# -nt 1 ]; then
  echo "参数错误。用法: $0 <workspace>"
  exit 1
fi

workspace=$1

whisp env token -v "" --workspace=$workspace > /dev/null
whisp call account logout --workspace=$workspace --no-prefix > /dev/null
