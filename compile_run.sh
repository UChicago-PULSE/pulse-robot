#!/bin/bash

make SIMULATION=native prep
make
make install
cd build/exe/cpu1
./core-cpu1