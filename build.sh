#!/bin/sh

TARGETS="Windows Web Linux"

mkdir build/
for TARGET in $TARGETS; do
    mkdir build/$TARGET
    make TARGET=$TARGET all 
    cp hello* build/$TARGET
    cp libtea* build/$TARGET
    make TARGET=$TARGET clean
done
