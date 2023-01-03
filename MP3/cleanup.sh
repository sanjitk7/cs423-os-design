#!/bin/sh
echo "removing and cleaning mp3 kernel module..."
sudo rmmod mp3
sudo rm node
sudo dmesg -C
