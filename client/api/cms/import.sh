#!/bin/bash

level=-1
tail=()

update_node() {
    local ret=$(whisp call main updateNode --app=cms -- "$1" "$2" \
        | jq -r '.[0]')
    local ok=$(echo $ret | jq -r '.ok')

    if [ "$ok" = "true" ]; then
        printf "ok\n"
    else
        printf "fail: $(echo $ret | jq -r '.error')\n"
    fi
}


new_node() {
    local ret=$(whisp call main newNode --app=cms -- "$1" "$2" \
        | jq -r '.[0]')
    local ok=$(echo $ret | jq -r '.ok')
    
    if [ "$ok" = "true" ]; then
        printf "ok\n"
    else
        printf "fail: $(echo $ret | jq -r '.error')\n"
    fi
}

indent() {
    local i=0
    for ((; i < level + 1; i++)); do
        if [ ${tail[i]} = 0 ]; then
            printf "%3s" "|"
        else
            printf -- "   "
        fi
    done
}

import_dir() {
    #local cmsPath=/$(realpath --relative-to=contents $dir)
    indent
    printf -- "- D ${cmsPath}, "
    local isExists=$(whisp call main exists --app=cms -- $cmsPath | jq -r '.[0]')
    if [ "$isExists" = "true" ]; then
        printf "update ... "
        local node=$(cat "$dir"/__DIR__)
        update_node "$cmsPath" "$node"
    else
        printf "new ... "
        if [ -f "$dir"/__DIR__ ]; then
            local node=$(cat "$dir"/__DIR__)
        else
            local node="{ \"name\": \"$(basename $dir)\", \"type\": \"D\" }"
        fi
        new_node "$cmsPPath" "$node"
    fi
}

import_content() {
    #local contentPath=${cmsPath}${name}
    indent
    printf -- "- C ${contentPath}, "

    local isExists=$(whisp call main exists --app=cms -- $contentPath | jq -r '.[0]')

    #echo import content ... $contentPath $isExists

    if [ "$isExists" = "true" ]; then
        printf "update ... "

        update_node "$contentPath" "$node"
    else
        printf "new content ... "

        new_node "$cmsPPath" "$node"
    fi
}

progress="##########"

import_media() {
    indent
    printf -- "- M ${contentPath},  "
    #local contentPath=${cmsPath}${name}
    local content=$(echo $node | jq -r '.content')
    local imgFile=$(echo "$content" | sed 's#^res:##')

    #indent
    printf "post "
    local imgID=$(whisp call media requestPostMedia --app=cms -- "" "$imgFile" | jq -r '.[0]')
    local imgPath=media/${imgFile}
    local chunk_size=1024
    local len=$(stat -c %s $imgPath)
    local i=0
    local cp=0
    for ((; i < len; i += chunk_size)); do
        local base64=$(dd if="$imgPath" bs=1 skip=$i count=$chunk_size 2>/dev/null | base64 -w 0)
        local ok=$(whisp call media writeMediaData --app=cms -- "$imgID" "$base64" | jq -r '.[0]')
        if [ "$ok" = "true" ]; then
            local p=$(( i * 10 / len ))
            if [ $p != $cp ]; then
                printf "${progress:0:(p - cp)}"
                #printf "${p},${cp}"
                cp=$p
            fi
        else
            printf "x"
        fi
    done
    printf "${progress:0:(10 - cp)}"
    printf " ok,  "
    whisp call media writeMediaDataEnd --app=cms -- "$imgID" > /dev/null

    #indent
    local isExists=$(whisp call main exists --app=cms -- $contentPath | jq -r '.[0]')
    if [ "$isExists" = "true" ]; then
        printf "update ... "

        update_node "$contentPath" "$node"
    else
        printf "new image ... "

        new_node "$cmsPPath" "$node"
    fi
}

iterator() {
    local dir=$1
    local item

#    local cmsPath=$(realpath --relative-to=contents $dir)
#    if [ "$cmsPath" = '.' ]; then
#        cmsPath=/
#    else
#        cmsPath=/${cmsPath}
#    fi

#    if [ -f "$dir"/__DIR__ ]; then
#        import_dir
#    fi

    local cmsPath
    if [ "$dir" != "contents" ]; then
        cmsPath=/$(realpath --relative-to=contents $dir)/
        import_dir
    else
        cmsPath=/
    fi

    ((level++))
    tail[level]=0

    local cmsPPath=$cmsPath

    local index=0
    local len=$(($(ls -I __DIR__ -l ${dir} | wc -l) - 1))
    for item in ${dir}/*; do
        #echo "(${level},${index},${len})"
        local itemName=$(basename "$item")
        if [ $index -eq $((len - 1)) ]; then
            tail[level]=1
        fi

        if [ -d "$item" ]; then
            ((index++))
            iterator "$item"
            ((level--))
        elif [ -f "$item" ] && [ $itemName != "__DIR__" ]; then
            ((index++))
            local node=$(cat "$item")
            local type=$(echo $node | jq -r '.type')
            local contentType=$(echo $node | jq -r '.contentType')
            local name=$(echo $node | jq -r '.name')
            local contentPath=${cmsPath}${name}

            if [ $type = 'F' ]; then
                import_content
            elif [ $type = 'R' ]; then
                if [ $contentType != 'ref' ]; then
                    if [[ $contentType =~ ^(jpg|png)$ ]]; then
                        import_media
                    fi
                fi
            fi
        fi
    done
}

iterator contents

