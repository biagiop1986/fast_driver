#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

insmod fast_driver.ko

lsmod | head -1
lsmod | grep fast_driver

chmod 666 /dev/sample_acc
