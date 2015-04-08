#!/bin/bash

# Plots all 3 methods (u, a, t3, t4, t5) on a single plot
# x-axis: epsilon values
# y-axis: FR or Jaccard, as setup via parameters
#
# $1: block num of points (gives <alpha, beta>)
# $2: key position (multi-words)
# $3: column for y-axis
# $4: output file-name part
plot_versus_epsilon() {
    block_num=$1
    key="$2"
    column=$3
    output=$4
    ylabel=${output%%-*}
    ylabel=${ylabel^^}

    cat << END | gnuplot
set terminal post eps enhanced font "Helvetica,28"
set datafile separator ","
set key ${key} right
set xrange [0:1.1]
set xlabel "{/Symbol e}"
set yrange [-0.1:1.1]
set ylabel "${ylabel}"
set output "${output}".".eps"
s = 5 * ${block_num}
plot "u.csv" u 5:${column} every ::s+1::s+5 w lp ps 2 lt 1 pt 4 title 'u',\
     "a.csv" u 5:${column} every ::s+1::s+5 w lp ps 2 lt 1 pt 6 title 'a',\
     "t.csv" u 5:${column} every 3::3*s+1::3*s+15 w lp ps 2 lt 1 pt 8 title 't 3',\
     "t.csv" u 5:${column} every 3::3*s+2::3*s+16 w lp ps 2 lt 1 pt 10 title 't 4',\
     "t.csv" u 5:${column} every 3::3*s+3::3*s+17 w lp ps 2 lt 1 pt 12 title 't 5'
END
}

methods="li fi ug v50 v1 v2"
for i in `seq 0 2`; do
    alpha=0.$((2 + 3 * i))
    for j in `seq 0 2`; do
        beta=0.$((2 + 3 * j))
        index=$((3 * i + j)) # index on dataset to represent
        ix=18
        for method in ${methods}; do
            for estimate in star bar; do
                plot_versus_epsilon $index t $ix j-${method}-${estimate}-${alpha}-${beta}
                plot_versus_epsilon $index b $((ix+1)) fr-${method}-${estimate}-${alpha}-${beta}
                ix=$((ix + 7))
            done
        done
    done
done
