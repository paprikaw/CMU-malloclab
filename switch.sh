#!/bin/sh
mv mm.c tmp
mv mm-cmu.c mm.c
mv tmp mm-cmu.c
rm tmp
make clean
make