#!/bin/bash


# 检查是否提供了参数
if [ $# -ne 1 ]; then
  echo "参数错误。用法: $0 <scenario>"
  exit 1
fi

workspace=$1

watch_TTY=$(whisp env ${workspace}_cms_watch_TTY)
if [ -z "$watch_TTY" ]; then
    watch_TTY=/dev/null
fi

#echo Set the app attribute of the \"$workspace\" workspace to \"cms\"
#whisp workspace --app=cms --workspace=$workspace > /dev/null
#echo ...ok

echo Enable CMS connection status monitoring....
whisp watch --state -c "watch_state.sh" --workspace="$workspace" > $watch_TTY &
echo ...ok

echo Enable the account module.
whisp account::initial
echo ...ok

echo Enable the todo module.
whisp todo::initial
echo ...ok
