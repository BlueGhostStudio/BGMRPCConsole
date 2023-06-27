#!/bin/bash

# 检查是否提供了参数
if [ $# -ne 1 ]; then
  echo "参数错误。用法: $0 <scenario>"
  exit 1
fi

workspace=$1

# 读取标准输入
read input

# 检查输入是否为3
#if [ "$input" = "0" ]; then
#    notify-send "Workspace: ${workspace} - CONNECTION" "Unable to establish connection." -a "whisp"
#elif [ "$input" = "3" ]; then
#    notify-send "Workspace: ${workspace} - CONNECTION" "CONNECTED" -a "whisp"
  # 调用 BGMRPCC 命令
#  whisp ping $workspace
#fi

case $input in
    0)
        keepalive_pid=$(whisp env keepalive_pid -w $workspace)

        if [ -n "$keepalive_pid" ]; then
            kill "$keepalive_pid"
            whisp env keepalive_pid -v "" --workspace=$workspace
        fi

        notify-send "Workspace: ${workspace} - CONNECTION" "Disconnected." -a "whisp"
    ;;
    1)
        notify-send "Workspace: ${workspace} - CONNECTION" "Performing a host name lookup..." -a "whisp"
    ;;
    2)
        notify-send "Workspace: ${workspace} - CONNECTION" "Connecting..." -a "whisp"
    ;;
    3)
        notify-send "Workspace: ${workspace} - CONNECTION" "CONNECTED" -a "whisp"
        # 调用 BGMRPCC 命令
        whisp ping $workspace
    ;;
    4)
        notify-send "Workspace: ${workspace} - CONNECTION" "Bound to address and port." -a "whisp"
        # 调用 BGMRPCC 命令
        whisp ping $workspace
    ;;
    5)
        notify-send "Workspace: ${workspace} - CONNECTION" "Closing in progress..." -a "whisp"
        # 调用 BGMRPCC 命令
        whisp ping $workspace
    ;;
esac
