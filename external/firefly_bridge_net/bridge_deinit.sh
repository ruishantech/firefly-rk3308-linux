#!/bin/bash
set -e
brctl delif br0 eth0
brctl delif br0 eth1
ifconfig br0 down
brctl delbr br0

# restart dhcpcd
if ps -ef|grep -v grep|grep dhcpcd ;then
    killall dhcpcd
    sleep 1
fi
dhcpcd -f /etc/dhcpcd.conf
