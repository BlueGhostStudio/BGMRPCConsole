#!/bin/bash

# 检查是否提供了参数
if [ $# -ne 1 ]; then
  echo "参数错误。用法: $0 <scenario>"
  exit 1
fi

workspace=$1

read input

if [ "$input" -eq 3 ]; then
    echo Associate with the remote CMS object.
    whisp call main join --app=cms | jq -r '.[0]'

#    token=$(whisp env token --workspace=$workspace)
#
#    if [ -n "$token" ]; then
#        login.sh "$workspace" "$token"
#    fi
fi

#token=$(whisp env token --workspace=$workspace)
#echo "token -----> " $token

#if [ -z "$token" ]; then
#    echo login to cms
#fi
