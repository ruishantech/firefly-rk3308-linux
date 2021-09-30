#!/bin/bash
set -e
SRC=$1
DST=$2
SIZE=`du -sk --apparent-size $SRC | cut --fields=1`
inode_counti=`find $SRC | wc -l`
inode_counti=$[inode_counti+512]
EXTRA_SIZE=$[inode_counti*4]

MAX_RETRY=10
RETRY=0
while true;do
    SIZE=$[SIZE+EXTRA_SIZE]
    echo "genext2fs -b $SIZE -N $inode_counti -d $SRC $DST"
    genext2fs -b $SIZE -N $inode_counti -d $SRC $DST && break

    RETRY=$[RETRY+1]
    [ ! $RETRY -lt $MAX_RETRY ] && { echo "Failed to make e2fs image! "; exit; }
    echo "Retring with increased size....($RETRY/$MAX_RETRY)"
done

#get size of oem from parameter.
filename=device/rockchip/$RK_TARGET_PRODUCT/$RK_PARAMETER
size_oem=`awk -F ',' '{for(i=1;i<=NF;i++){if($i~/oem/) print $i}}' $filename | awk -F '(' '{print $1}'`

tune2fs -c 1 -i 0 $DST -L oem

if [ -z "$size_oem" ]; then
    resize2fs $DST
else
    ((size_oem=`echo $size_oem | cut -d "@" -f1`))
    ((size_oem=size_oem/2))
    echo `pwd`
    echo size_oem is $size_oem
    resize2fs $DST $size_oem
fi

e2fsck -fy $DST
