#!/bin/bash
set -x #echo on

sudo ip link set down $1
sudo iwconfig $1 mode monitor
sudo ip link set up $1
sudo iwconfig $1 channel $2

