#!/bin/bash

make clean || exit
rm -f debug_* || exit

make || exit
#./dpsn ALPHA BETA K NT EPS <u GAMMA|a GAMMA|t DEPTH> DATASET [SEED]
${TOOL} ./dpsn 0.5 0.5 10 3 1000005.0 t 3 datasets/debug1.dat || exit
ls debug_* &> /dev/null && ./gen_plots.sh debug_* || exit
