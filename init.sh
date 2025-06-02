#!/bin/bash

# === function ===
# log
readonly INFO="INFO"
readonly WARNING="WARNING"
readonly ERROR="ERROR"
function log() {
    echo "[$1] $(date +%Y-%m-%d\ %H:%M:%S): $2"
}

# check_ret
function check_ret() {
    if [ $? -ne 0 ]; then
        log "${ERROR}" "Execution failed, exit."
        exit 1
    fi
}

# === main ===
case "${1}" in
"this")
    # init algorithms-server
    log "${INFO}" "init algorithms-server"

    git submodule update --init
    check_ret

    cd dependencies/json && git checkout tags/v3.11.2 -b v3.11.2 && cd -
    check_ret

    # init submodules in docker
    log "${INFO}" "init submodules in docker"
    docker compose run --rm air2.0-dev bash init.sh submodules
    check_ret

    # build algorithms-server
    log "${INFO}" "init algorithms-server in docker"
    docker compose run --rm air2.0-dev bash build.sh all
    check_ret

    ;;

"submodules")
    # init submodules
    log "${INFO}" "init submodules"

    current_dir=$(pwd)

    # init algorithm-core
    log "${INFO}" "init algorithm-core"

    cd dependencies/airouting2.0/algorithm/algorithm-core/
    check_ret

    git config --global --add safe.directory /home/ubuntu/air2.0/dependencies/airouting2.0/algorithm/algorithm-core
    check_ret

    git config --global --add safe.directory /home/ubuntu/air2.0/dependencies/airouting2.0/algorithm/algorithm-core/dependencies/json
    check_ret

    git checkout main
    check_ret

    git submodule update --init
    check_ret

    cd dependencies/json && git checkout tags/v3.11.2 -b v3.11.2 && cd -
    check_ret

    bash build.sh all

    cd "${current_dir}"
    check_ret

    ;;

*)
    echo "Usage:"
    echo "${0} [this | submodules]"

    ;;

esac

exit 0
