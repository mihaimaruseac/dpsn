#!/bin/bash

make clean || exit
rm -f debug_* || exit

make || exit
#./dpsn ALPHA BETA K NT EPS <u GAMMA|a GAMMA|t DEPTH> TTRESH DATASET [SEED]
${TOOL} ./dpsn 0.5 0.5 10 3 .50 t 3 0.1 50 datasets/debug1.dat || exit
ls debug_tree_grid* &> /dev/null && ./gen_debug_tree_grid_plots.sh debug_tree_grid*
ls debug_uniform_grid* &> /dev/null && ./gen_debug_uniform_grid_plots.sh debug_uniform_grid*
