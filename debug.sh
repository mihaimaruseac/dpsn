#!/bin/bash

make clean || exit
rm -f debug_* || exit

make || exit
#./dpsn ALPHA BETA K NT EPS <u GAMMA|a GAMMA|t DEPTH> TTRESH RESOLUTION DATASET [SEED]
${TOOL} ./dpsn 0.5 0.5 1 3 1.0 t 4 1 1 datasets/new_N20000_theta80.dat || exit
ls debug_tree_grid* &> /dev/null && ./gen_debug_tree_grid_plots.sh debug_tree_grid*
ls debug_uniform_grid* &> /dev/null && ./gen_debug_uniform_grid_plots.sh debug_uniform_grid*
