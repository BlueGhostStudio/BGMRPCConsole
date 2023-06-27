 #!/bin/bash


# 检查是否提供了参数
if [ $# -ne 1 ]; then
  echo "参数错误。用法: $0 <scenario>"
  exit 1
fi

workspace=$1

echo Initial ${workspace}...

watch_TTY=$(whisp env watch_TTY)

if [ -z "$watch_TTY" ]; then
    watch_TTY=/dev/null
fi

echo Enable keepalive watching...
whisp watch --pong -c "watch_keepalive.sh" --workspace="$workspace" > $watch_TTY &
echo Enable completed

echo Enable watching for connection status changes...
whisp watch --state -c "watch_state.sh" --workspace="$workspace" > $watch_TTY &
echo Enable completed
