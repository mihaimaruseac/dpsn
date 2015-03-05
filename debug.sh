#!/bin/bash

make clean || exit
rm -f debug_* || exit

make || exit
#./dpsn ALPHA BETA K NT EPS <u GAMMA|a GAMMA|t DEPTH> TTRESH DATASET [SEED]
${TOOL} ./dpsn 0.5 0.5 10 3 .50 u .3 0.1 54 datasets/debug1.dat || exit
ls debug_* &> /dev/null && ./gen_plots.sh debug_* || exit
