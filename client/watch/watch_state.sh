#!/bin/bash

# 检查是否提供了参数
if [ $# -ne 1 ]; then
  echo "参数错误。用法: $0 <scenario>"
  exit 1
fi

scenario=$1

# 读取标准输入
read input

# 检查输入是否为3
if [ "$input" = "3" ]; then
  # 调用 BGMRPCC 命令
  whisp ping $scenario
fi
