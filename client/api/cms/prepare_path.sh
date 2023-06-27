#!/bin/bash

workspace=$1

workdir=$(whisp env wd --workspace=$workspace)

if [ -z "$workdir" ]; then
  workdir="/"
fi

normalize_path() {
  path=$(echo "$1" | sed 's#\/\+#/#g')
}

if [ -n "$2" ]; then
  if [[ $2 == /* ]]; then
    normalize_path $2
  elif [[ $2 =~ ^[0-9]+$ ]]; then
    path=$2
  else
    normalize_path "${workdir}/$2"
  fi
else
  normalize_path $workdir
fi
