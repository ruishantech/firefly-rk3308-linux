#!/bin/bash

# Target arch
export RK_ARCH=arm64
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=firefly-rk3308-debug-uart4-logo
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=firefly-rk3308b_linux_defconfig
# Kernel dts
export RK_KERNEL_DTS=rk3308b-roc-cc-plus-amic-rgb_7.0inch_emmc
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm64/boot/Image.lz4
# parameter for GPT table
export RK_PARAMETER=parameter-64bit-emmc-qt.txt
# packagefile for make update image
export RK_PACKAGE_FILE=rk3308-package-file
# Buildroot config
export RK_CFG_BUILDROOT=firefly_rk3308_qt_release
# Recovery config
export RK_CFG_RECOVERY=firefly_rk3308_recovery
# ramboot config
export RK_CFG_RAMBOOT=
# Pcba config
export RK_CFG_PCBA=firefly_rk3308_pcba
# Build jobs
export RK_JOBS=12
# target chip
export RK_TARGET_PRODUCT=rk3308
# Set rootfs type, including ext2 ext4 squashfs
export RK_ROOTFS_TYPE=ext2
# rootfs image path
export RK_ROOTFS_IMG=buildroot/output/$RK_CFG_BUILDROOT/images/rootfs.$RK_ROOTFS_TYPE
# Set oem partition type, including ext2 squashfs
export RK_OEM_FS_TYPE=ext2
# Set userdata partition type, including ext2, fat
export RK_USERDATA_FS_TYPE=ext2
# Set flash type. support <emmc, nand, spi_nand, spi_nor>
export RK_STORAGE_TYPE=emmc
#OEM config: /oem/dueros/aispeech-6mic-64bit/aispeech-2mic-64bit/aispeech-4mic-32bit/aispeech-2mic-32bit/aispeech-2mic-kongtiao-32bit/iflytekSDK/CaeDemo_VAD/smart_voice
export RK_OEM_DIR=oem
#userdata config
export RK_USERDATA_DIR=userdata_empty
MIC_NUM=6
#misc image
export RK_MISC=wipe_all-misc.img
#choose enable distro module
export RK_DISTRO_MODULE=
#choose enable Linux A/B
export RK_LINUX_AB_ENABLE=

#build info
export PRODUCT_UI=Qt-rgb_7.0inch
export RK_MODEL=ROC-RK3308B-CC-PLUS
export RK_VERSION=$(echo $RK_MODEL)-$(echo $PRODUCT_UI)-$(date +"%Y%m%d")
export RK_OTA_HOST=
  
#choose rkbin/RKBOOT/ *.ini file to pack loader image
export RK_RKBOOT_INI=RK3308MINIALL_UART4
  
export RK_LOADER_NAME=*_loader_uart4_v*.bin
