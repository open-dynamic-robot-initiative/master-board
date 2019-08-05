#!/bin/bash
set -x #echo on

sudo ifconfig $1 down
sudo iwconfig $1 mode monitor
sudo ifconfig $1 up
sudo iwconfig $1 channel $2

