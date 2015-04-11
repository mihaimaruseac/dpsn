#!/bin/bash

plot_one_cube () {
    file=$1
    title=$2
    ix=$3
    cat << END | gnuplot
set terminal post eps enhanced font "Helvetica,28"
set size square
set palette model RGB defined ( 0 'white', 1 'black' )
set output "${file}_${title}.eps"
set title "Shape"
set xtics 0,20,100
set ytics 0,20,100
set view map
unset key
f(x,y) = 20 + 100 * exp(-((x-60)**2 + (y-15)**2)/500)
set contour base
set cntrparam levels discrete 80
splot "${file}" i ${ix} matrix w image, f(x, y) w l lt 0
END
}

plot_one () {
    file=$1
    #plot_one_cube ${file} "n" 0
    #plot_one_cube ${file} "s" 1
    #plot_one_cube ${file} "rho" 2
    #plot_one_cube ${file} "n_star" 3
    #plot_one_cube ${file} "s_star" 4
    #plot_one_cube ${file} "rho_star" 5
    #plot_one_cube ${file} "n_bar" 6
    #plot_one_cube ${file} "s_bar" 7
    #plot_one_cube ${file} "rho_bar" 8
    #plot_one_cube ${file} "shape" 9
    #plot_one_cube ${file} "Shape" 10
    #plot_one_cube ${file} "shape_bar" 11
    #plot_one_cube ${file} "shape_delta_bar" 14
    #plot_one_cube ${file} "voting_above_star" 15
    #plot_one_cube ${file} "voting_below_star" 16
    #plot_one_cube ${file} "voting_above_bar" 17
    #plot_one_cube ${file} "voting_below_bar" 18
    #plot_one_cube ${file} "voting_real_star" 19
    #plot_one_cube ${file} "voting_real_bar" 20
    #plot_one_cube ${file} "voting_star" 21
    plot_one_cube ${file} "shape-t" 22
}

for i in $@; do
    plot_one $i
done

