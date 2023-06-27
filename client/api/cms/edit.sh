#!/bin/sh

# 检查是否提供了参数
if [ $# -lt 2 ]; then
  echo "参数错误。用法: $0 <workspace> <node>"
  exit 1
fi

workspace=$1
source prepare_path.sh $workspace $2

ret=$(whisp call main node --workspace=$workspace --app=cms -- $path | jq '.[0]' | cat -)

ok=$(echo $ret | jq -r '.ok')

if [ "$ok" == "false" ]; then
    echo The node $2 does not exist.
    exit 1
fi

node=$(echo "$ret" | jq -r '.node')
contentType=$(echo "$node" | jq -r '.contentType')
type=$(echo "$node" | jq -r '.type')
title=$(echo "$node" | jq -r '.name')

if [ "$type" == "D" ]; then
    echo "The node is directory"
    exit 1
fi

#echo $ret

edit=-1 # 0 text edit 1 img view
case $contentType in
    "json" | "html" | "md" | "txt" | "js")
        extName=$contentType
        edit=0;
    ;;
    "cmp" | "fra" | "pkg" | "elm")
        extName="html"
        edit=0
    ;;
    "jpg" | "png")
        edit=1
    ;;
esac

case $edit in
    0)
        content_tmp_file=$(mktemp "$title"_content_XXX."$extName" -p /tmp)
        echo "$node" | jq -r '.content' > "$content_tmp_file"
        summary_tmp_file=$(mktemp "$title"_summary_XXX -p /tmp)
        echo "$node" | jq -r '.summary' > "$summary_tmp_file"

        initial_content_hash=$(md5sum $content_tmp_file)
        initial_summary_hash=$(md5sum $summary_tmp_file)

        vim -p -- $summary_tmp_file $content_tmp_file

        final_content_hash=$(md5sum $content_tmp_file)
        final_summary_hash=$(md5sum $summary_tmp_file)

        updateJson='{}'
        modified=0
        if [ "$initial_content_hash" != "$final_content_hash" ]; then
            updateJson=$(echo "$updateJson" | jq -r --arg content "$(cat $content_tmp_file)" '.content = $content')
            modified=1
        fi
        if [ "$initial_summary_hash" != "$final_summary_hash" ]; then
            updateJson=$(echo "$updateJson" | jq -r --arg summary "$(cat $summary_tmp_file)" '.summary = $summary')
            modified=1
        fi

        if [ $modified -eq 1 ]; then
            result=$(whisp call main updateNode --workspace=$workspace --app=cms -- "$path" "$updateJson" | jq -r '.[0]')
            if [ $(echo "$result" | jq -r '.ok') = "false" ]; then
                echo "can't save content"
            fi
        fi

        rm $content_tmp_file
        rm $summary_tmp_file
        ;;
    1)
        imgFile=$(echo "$node" | jq -r '.content' | sed 's#^res:##')
        url=$(whisp call media mediaURL --workspace=$workspace --app=cms -- "$imgFile" | jq -r '.[0]')
        curl $(echo "$url" | sed 's|\([^:/]\)//|\1/|g') -sk | feh -
        ;;
    ?)
        echo unknow content type
        ;;
esac
