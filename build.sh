#!/bin/bash

BUILD_DIR='build'
BUILD_TYPE='Release' # or Debug or RelWithDebInfo or MinSizeRel
INSTALL_PREFIX='/usr/local/'
VERBOSE_MAKEFILE='OFF' # or ON

LIB_OUTPUT_DIR='lib'
EXE_OUTPUT_DIR='bin'

function clear_build() {
    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
    fi
}

function build() {
    # build dir
    if [ ! -d "${BUILD_DIR}" ] && ! mkdir "${BUILD_DIR}"; then
        exit 1
    fi

    if ! cd "${BUILD_DIR}"; then
        exit 1
    fi

    # cmake
    if ! cmake \
        "-DSHELL_BUILD_TYPE=${1}" \
        "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}" \
        "-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}" \
        "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${VERBOSE_MAKEFILE}" \
        "-DEXECUTABLE_OUTPUT_PATH=${EXE_OUTPUT_DIR}" \
        "-DLIBRARY_OUTPUT_PATH=${LIB_OUTPUT_DIR}" \
        --no-warn-unused-cli \
        ..; then
        exit 1
    fi

    # make
    if ! make; then
        exit 1
    fi

    if ! cd -; then
        exit 1
    fi
}

function install() {
    if [ ! -d "${BUILD_DIR}" ]; then
        exit 1
    fi

    if ! cd "${BUILD_DIR}"; then
        exit 1
    fi

    # install
    if ! make install || ! ldconfig; then
        exit 1
    fi

    if ! cd -; then
        exit 1
    fi
}

function test() {
    if [ ! -d "${EXE_OUTPUT_DIR}" ]; then
        exit 1
    fi

    if ! cd "${EXE_OUTPUT_DIR}"; then
        exit 1
    fi

    # run
    if ! ./test; then
        exit 1
    fi

    if ! cd -; then
        exit 1
    fi
}

case "${1}" in
"clear")
    clear_build
    ;;

"build_lib")
    build lib
    ;;

"install")
    install
    ;;

"build_test")
    build test
    ;;

"test")
    test
    ;;

"all")
    # clear_build
    # build lib
    # install
    build test
    test
    ;;

*)
    echo "Usage:"
    echo "${0} [clear | build_lib | install | build_test | test | all]"
    ;;

esac

exit 0
