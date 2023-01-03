#!/bin/sh
echo "removing and cleaning mp2 kernel module..."
sudo rmmod mp2
sudo dmesg -C
