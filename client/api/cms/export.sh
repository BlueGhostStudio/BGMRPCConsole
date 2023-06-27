#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Invalid argument. Usage: $0 <workspace>"
    exit 1
fi

workspace=$1

export_dir() {
    printf "export dir $1 ... "
    local dir=$(pwd)/contents${1}
    mkdir $dir 2>/dev/null
    echo "$node" | jq -c 'del(.id,.own,.pid)' > ${dir}__DIR__
    printf "ok\n"
}

export_content() {
    local path=$(pwd)/contents${1}${id}_${name}_${contentType}
    echo "$node" | jq -c 'del(.id,.own,.pid)' > $path

    printf "export content ok\n"
}

export_media() {
    export_content "$1"
    local imgFile=$(echo "$node" | jq -r '.content' | sed 's#^res:##')
    local url=$(whisp call media mediaURL --workspace=$workspace \
        --app=cms -- "$imgFile" | jq -r '.[0]')

    url=$(echo "$url" | sed 's|\([^:/]\)//|\1/|g')
    (cd media; curl -kO "$url")

    printf "export image ok\n"
}

ignore=()

iterator() {
    local cmsPath=$1
    local list=$(whisp call main list --app=cms --workspace=$workspace -- $cmsPath 2 | \
        jq -r '.[0].list')

    local len=$(echo $list | jq -r '.|length')

    local i=0
    for ((; i < len; i++)); do
        local node=$(echo $list | jq -r '.['$i']')
        local id=$(echo $node | jq -r '.id')
        local type=$(echo $node | jq -r '.type')
        local contentType=$(echo $node | jq -r '.contentType')
        local name=$(echo $node | jq -r '.name')
        if [ $type = 'D' ]; then
            local dirPath=${cmsPath}/${name}
            dirPath=${dirPath//\/\//\/}/
            export_dir "$dirPath"
            iterator $dirPath
        else
            printf "get node ${cmsPath}${name} ... "
            node=$(whisp call main node \
                --app=cms --workspace=$workspace -- $id | \
                jq -r '.[0].node')

            if [ $type = 'F' ]; then
                export_content "$cmsPath"
            elif [ $type = 'R' ]; then
                if [ $contentType != 'ref' ]; then
                    if ! printf '%s\n' "${ignore[@]}" | grep -q "^${contentType}$"; then
                        if [[ $contentType =~ ^(jpg|png)$ ]]; then
                            export_media "$cmsPath"
                        elif command -v export_${contentType}.sh > /dev/null 2>&1; then
                            whisp @export_${contentType} "$cmsPath" "$node"
                        fi
                    else
                        printf "skip\n"
                    fi
                else
                    printf "skip\n"
                fi
            fi

        fi
    done
}

mkdir contents 2>/dev/null
mkdir media 2>/dev/null

args=()
while [ "$OPTIND" -le $# ]; do
    if getopts ":i:" opt; then
        case $opt in
            i)
                ignore+=("$OPTARG")
                ;;
            :)
                echo "Option -$OPTARG requires an argument."
                exit 1
                ;;
            \?)
                echo "Invalid option: -$OPTARG"
                exit 1
                ;;
        esac
    else
        shift $((OPTIND - 1))
        args+=($1)
        OPTIND=2
    fi
done

iterator /
