#!/bin/sh

#this script is used to usb hot plug for ec20 module，when ec20 module inserts into 
#usb interface, it will decide whether firefly-call4g needs to be run.

net_card=`ifconfig "wwan0"|awk '{print $1}'|head -n 1`
num=`ps|grep "firefly-call4g"|wc -l`

if [ -n "$net_card" ]
then
        if [ "$num" == 1 ]
        then
                nohup firefly-call4g &>/dev/null &
        fi
        exit 0
fi



