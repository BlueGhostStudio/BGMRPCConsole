#!/bin/sh

# workspace=$1
# source prepare_path.sh $workspace $2
# source checkVim.sh

# if [ "$INVIM" != "1" ]; then
#     read -p "Are you sure you want to delete $path? (y/n): " confirm
#     if [ "${confirm,,}" != 'y' ]; then
#         exit 1
#     fi
# fi

force=0
args=()
while [ "$OPTIND" -le $# ]; do
    if getopts ":f" opt; then
        case $opt in
            f)
                force=1
                ;;
        esac
    else
        shift $((OPTIND - 1))
        args+=($1)
        OPTIND=2
    fi
done

if [ ${#args[*]} -lt 2 ]; then
    echo "Usage: $0 <workspace> [-f] <path>"
    exit 1
fi

workspace=${args[0]}
source prepare_path.sh $workspace ${args[1]}

if [ $force -eq 0 ]; then
    if [ "$INVIM" != "1" ]; then
        read -p "Confirm deletion of $path? (y/n): " confirm
        if [ "${confirm,,}" != 'y' ]; then
            exit 1
        fi
    else
        if [ -e /dev/fd/4 ]; then
            : "$(echo "$path" | sed 's/[\"\\]/\\&/g')"
            : "${_//$'\n'/\\n}"
            echo "call WhispCmsConfirmRemove(\"$_\")" >&4
        fi
        exit 1
    fi
fi

ret=$(whisp call main removeNode \
    --workspace=$workspace --app=cms -- \
    "$path" | jq -r '.[0]')

if [ $(echo "$ret" | jq -r '.ok') == "false" ]; then
    echo "Unable to remove content. $(echo "$ret" | jq -r '.error')"
fi

