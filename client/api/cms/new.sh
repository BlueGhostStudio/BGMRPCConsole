#!/bin/sh

workspace=$1
source prepare_path.sh $workspace $2
#source checkVim.sh

ret=$(whisp call main nodeInfo --workspace=$workspace \
    --app=cms -- "$path" | jq '.[0]')

if [ $(echo "$ret" | jq -r '.ok') == "false" ]; then
    echo The node $2 does not exist.
    exit 1
fi

if [ $(echo "$ret" | jq -r '.node.type') != "D" ]; then
    echo The node $2 is not directory
fi

newNodeJson=$(cmsUpdate 3>&1 1>&2 2>&3)

if [ -n "$newNodeJson" ]; then
    ret=$(whisp call main newNode \
        --workspace=$workspace --app=cms -- \
        "$path" "$newNodeJson" | jq -r '.[0]')

    if [ $(echo "$ret" | jq -r '.ok') == "false" ]; then
        echo "Unable to create new content. $(echo "$ret" | jq -r '.error')"
    fi
fi

