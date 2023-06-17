#!/bin/bash

# 获取当前场景名称
scenario="$1"

# 保存上一个进程 ID 到文件
save_previous_pid() {
    echo "$1" > "/tmp/${scenario}_alive.pid"
}

# 读取上一个进程 ID 文件并获取上一个进程 ID
get_previous_pid() {
    if [ -f "${scenario}_alive.pid" ]; then
        previous_pid=$(cat "${scenario}_alive.pid")
    fi
}

# 杀死上一个进程
kill_previous_process() {
    if [ -n "$previous_pid" ]; then
        kill "$previous_pid"
    fi
}

# 保存当前进程 ID
save_current_pid() {
    current_pid=$$
    save_previous_pid "$current_pid"
}

# 延迟 1 秒后发送 ping
send_ping() {
    sleep 1
    whisp ping "$scenario"
    exit_code=$?  # 获取退出码
    if [ $exit_code -eq 128 ]; then  # 如果退出码为 128，退出整个脚本
        exit 1
    fi
}

# 等待 5 秒
wait_for_pong() {
    sleep 5
}

# 重新连接服务器
reconnect_server() {
    whisp reconnect "$scenario"
}

# 主要逻辑
main() {
    # 恢复上一个进程 ID
    get_previous_pid

  # 杀死上一个进程
  kill_previous_process

  # 保存当前进程 ID
  save_current_pid

  # 发送 ping
  send_ping

  # 等待 pong
  wait_for_pong

  # 重新连接服务器
  reconnect_server
}

# 执行主函数
main

