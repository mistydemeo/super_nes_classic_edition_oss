#!/bin/bash
set -e

PLATFORM="clover"
MODE=""

show_help()
{
    printf "\nbuild.sh - Top level build scritps\n"
    echo "Valid Options:"
    echo "  -h  Show help message"
    echo "  -p <platform> platform, e.g. sun5i, sun6i, sun8iw1p1, sun8iw3p1, sun9iw1p1"
    echo "  -m <mode> mode, e.g. ota_test"
    printf "\n\n"
}

build_uboot()
{
    make ARCH=arm CROSS_COMPILE=arm-clover-linux-gnueabihf- distclean
    make ARCH=arm CROSS_COMPILE=arm-clover-linux-gnueabihf- ${PLATFORM}_config
    make ARCH=arm CROSS_COMPILE=arm-clover-linux-gnueabihf- CONFIG_SPL=y spl fes -j16
    make ARCH=arm CROSS_COMPILE=arm-clover-linux-gnueabihf- -B -j16
}

while getopts p:m: OPTION
do
    case $OPTION in
    p)
        PLATFORM=$OPTARG
        ;;
    m)
        MODE=$OPTARG
        ;;
    *) show_help
        exit
        ;;
esac
done

build_uboot
