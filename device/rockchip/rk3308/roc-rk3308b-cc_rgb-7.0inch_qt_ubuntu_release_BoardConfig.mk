#!/bin/bash

CMD=`realpath $BASH_SOURCE`
CUR_DIR=`dirname $CMD`

source $CUR_DIR/roc-rk3308b-cc_rgb-7.0inch_qt_release_BoardConfig.mk

# Kernel dts
export RK_KERNEL_DTS=rk3308b-roc-cc-amic-rgb_7.0inch_ubuntu_emmc
# parameter for GPT table
export RK_PARAMETER=parameter-64bit-ubuntu.txt
# packagefile for make update image
export RK_PACKAGE_FILE=rk3308-package-file-ubuntu
# rootfs image path
export RK_ROOTFS_IMG=rootfs/rk3308-ubuntu_rootfs.img
