#!/bin/bash

edc=1
eds=0
ede=0
vimArgs=()

args=()
while [ "$OPTIND" -le $# ]; do
    if getopts ":Cse" opt; then
        case $opt in
            C)
                edc=0
                ;;
            s)
                eds=1
                ;;
            e)
                ede=1
                ;;
            \?)
                exit 1
                ;;
        esac
    else
        shift $((OPTIND - 1))
        args+=($1)
        OPTIND=2
    fi
done

if [ ${#args[*]} -lt 2 ]; then
    echo "Usage: $0 <workspace> [-C] [-e] [-s] <path>"
    exit 1
fi

workspace=${args[0]}
source prepare_path.sh $workspace ${args[1]}
source checkVim.sh

if [ $eds -eq 1 ]; then vimArgs+=("Wes \"${path}\""); fi
if [ $ede -eq 1 ]; then vimArgs+=("Wee \"${path}\""); fi

ret=$(whisp call main nodeInfo --workspace=$workspace --app=cms -- $path | jq '.[0]')
if [ $(echo "$ret" | jq -r '.ok') == "false" ]; then
    echo The node $path does not exist.
    exit 1
fi

node=$(echo "$ret" | jq -r '.node')

if [ $(echo "$node" | jq -r '.type') == "D" ]; then
    echo The node $path is directory
    exit 1
fi

contentType=$(echo "$node" | jq -r '.contentType')

if [ $edc -eq 1 ]; then
    case "$contentType" in
        "json" | "html" | "md" | "txt" | "js" | "cmp" | "fra" | "pkg" | "elm" | "style")
            vimArgs+=("Wec \"${path}\"")
            ;;
        "jpg" | "png") 
            echo 'Get node content...'
            content=$(whisp call main node --workspace=$workspace --app=cms -- $path | jq -r '.[0].node.content')
            echo 'Get img url...'
            imgFile=$(echo $content | sed 's#^res:##')
            url=$(whisp call media mediaURL --workspace=$workspace --app=cms -- "$imgFile" | jq -r '.[0]')
            echo '...get and show img'
            curl $(echo "$url" | sed 's|\([^:/]\)//|\1/|g') -sk | feh - &
            echo 'end'
            ;;
        ?)
            echo unknow content type
    esac
fi

if [ -e /dev/fd/4 ]; then
    IFS=$'\n'
    printf "${vimArgs[*]}" >&4
fi

if [ $RunInVIM = 0 ]; then
    cmdString=""
    for item in "${vimArgs[@]}"; do
        cmdString+="exec '$item' | " # 构建字符串
    done
    cmdString="${cmdString% | }"

    vim -c "$cmdString"
    #echo "$cmdString"
fi

