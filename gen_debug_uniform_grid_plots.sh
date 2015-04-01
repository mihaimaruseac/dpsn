#!/bin/bash

plot_one_cube () {
    file=$1
    title=$2
    ix=$3
    cat << END | gnuplot
set term png
set palette model RGB defined ( 0 'red', 1 'yellow', 2 'white' )
set output "${file}_${title}.png"
set title "${title}"
set view map
splot "${file}" i ${ix} matrix w image
END
}

plot_one () {
    file=$1
    plot_one_cube ${file} "n" 0
    plot_one_cube ${file} "s" 1
    plot_one_cube ${file} "rho" 2
    plot_one_cube ${file} "n_star" 3
    plot_one_cube ${file} "s_star" 4
    plot_one_cube ${file} "rho_star" 5
    plot_one_cube ${file} "n_bar" 6
    plot_one_cube ${file} "s_bar" 7
    plot_one_cube ${file} "rho_bar" 8
    plot_one_cube ${file} "shape" 9
    plot_one_cube ${file} "shape_star" 10
    plot_one_cube ${file} "shape_bar" 11
    plot_one_cube ${file} "shape_delta_bar" 14
}

for i in $@; do
    plot_one $i
done

