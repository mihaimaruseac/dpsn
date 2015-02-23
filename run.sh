#!/bin/bash

make clean || exit
rm -f debug_* || exit

make || exit
${TOOL} ./dpsn 0.5 0.5 10 3 2.0 t 3 datasets/debug1.dat || exit
./gen_plots.sh debug_* || exit
evince debug_*.eps || exit
