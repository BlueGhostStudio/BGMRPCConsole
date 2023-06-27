#!/bin/bash


# 检查是否提供了参数
if [ $# -ne 1 ]; then
  echo "参数错误。用法: $0 <scenario>"
  exit 1
fi

workspace=$1

echo initial todo $workspace
