#!/bin/bash


if [ -z "$1" ]; then
  echo "Error: Missing IP Address."
  echo "Usage: $0 <Beaglebone IP Address>"
  exit 1
fi


make SIMULATION=arm-cortexa8_neon-linux-gnueabi prep
make
make install
scp -r build/exe/cpu1 debian@"$1":~/