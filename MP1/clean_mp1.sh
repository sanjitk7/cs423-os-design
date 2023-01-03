#!/bin/sh
echo "removing and cleaning mp1 kernel module..."
sudo rmmod mp1
sudo dmesg -C
