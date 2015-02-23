#!/bin/bash

make clean || exit
rm -f debug_* || exit

make || exit
${TOOL} ./dpsn 0.5 0.5 10 3 5.0 t 30 datasets/debug1.dat || exit
ls debug_* &> /dev/null && ./gen_plots.sh debug_* || exit
