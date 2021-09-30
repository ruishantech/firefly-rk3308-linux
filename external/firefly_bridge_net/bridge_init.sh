#!/bin/bash
set -e
# set bridge
brctl addbr br0
brctl addif br0 eth0
brctl addif br0 eth1
ifconfig eth0  0.0.0.0
ifconfig eth1  0.0.0.0
ifconfig br0 192.168.2.1:24

if ps -ef|grep -v grep|grep dhcpd ;then
    killall dhcpd
    sleep 1
fi
dhcpd -q
