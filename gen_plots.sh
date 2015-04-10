#!/bin/bash

methods="li fi ug v50 v1 v2"

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

generate_versus_epsilon() {
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
}

# Plots t versus alpha
# x-axis: alpha values
# y-axis: FR or Jaccard, as setup via parameters
#
# $1: block num of points (gives <beta, epsilon>)
# $2: key position (multi-words)
# $3: column for y-axis
# $4: output file-name part
plot_versus_alpha() {
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
set xrange [0:0.6]
set xlabel "{/Symbol a}"
set yrange [-0.1:1.1]
set ylabel "${ylabel}"
set output "${output}".".eps"
s = 3 * ${block_num}
plot "t_ab.csv" u 1:${column} every 105::s+1 w lp ps 2 lt 1 pt 4 title 't 3',\
     "t_ab.csv" u 1:${column} every 105::s+2 w lp ps 2 lt 1 pt 6 title 't 4',\
     "t_ab.csv" u 1:${column} every 105::s+3 w lp ps 2 lt 1 pt 8 title 't 5'
END
}

generate_versus_alpha() {
    for i in `seq 0 6`; do
        beta=0.$((2 + i))
        for j in `seq 0 4`; do
            epsilon=`python -c "print 0.1 * (2 + 2*$j)"`
            index=$((5 * i + j))
            ix=18
            for method in ${methods}; do
                for estimate in star bar; do
                    plot_versus_alpha $index t $ix j-ab-a-${method}-${estimate}-${beta}-${epsilon}
                    plot_versus_alpha $index b $((ix + 1)) fr-ab-a-${method}-${estimate}-${beta}-${epsilon}
                    ix=$((ix + 7))
                done
            done
        done
    done
}

# Plots t versus beta
# x-axis: beta values
# y-axis: FR or Jaccard, as setup via parameters
#
# $1: block num of points (gives <alpha, epsilon>)
# $2: key position (multi-words)
# $3: column for y-axis
# $4: output file-name part
plot_versus_beta() {
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
set xrange [0:1]
set xlabel "{/Symbol b}"
set yrange [-0.1:1.1]
set ylabel "${ylabel}"
set output "${output}".".eps"
s = 3 * ${block_num}
plot "t_ab.csv" u 2:${column} every 15::s+1::s+105 w lp ps 2 lt 1 pt 4 title 't 3',\
     "t_ab.csv" u 2:${column} every 15::s+2::s+105 w lp ps 2 lt 1 pt 6 title 't 4',\
     "t_ab.csv" u 2:${column} every 15::s+3::s+105 w lp ps 2 lt 1 pt 8 title 't 5'
END
}

generate_versus_beta() {
    for i in `seq 0 3`; do
        alpha=0.$((2 + i))
        for j in `seq 0 4`; do
            epsilon=`python -c "print 0.1 * (2 + 2*$j)"`
            index=$((35 * i + j))
            ix=18
            for method in ${methods}; do
                for estimate in star bar; do
                    plot_versus_beta $index t $ix j-ab-b-${method}-${estimate}-${alpha}-${epsilon}
                    plot_versus_beta $index b $((ix + 1)) fr-ab-b-${method}-${estimate}-${alpha}-${epsilon}
                    ix=$((ix + 7))
                done
            done
        done
    done
}

# Plots t versus N
# x-axis: N values
# y-axis: FR or Jaccard, as setup via parameters
#
# $1: block num of points (gives <alpha, beta, epsilon>)
# $2: key position (multi-words)
# $3: column for y-axis
# $4: output file-name part
plot_versus_N() {
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
set xrange [0:60000]
set xtics ("10" 10000, "20" 20000, "30" 30000, "40" 40000, "50" 50000)
set xlabel "N"
set yrange [-0.1:1.1]
set ylabel "${ylabel}"
set output "${output}".".eps"
s = 3 * ${block_num}
plot "t_N.csv" u 10:${column} every 135::s+1 w lp ps 2 lt 1 pt 4 title 't 3',\
     "t_N.csv" u 10:${column} every 135::s+2 w lp ps 2 lt 1 pt 6 title 't 4',\
     "t_N.csv" u 10:${column} every 135::s+3 w lp ps 2 lt 1 pt 8 title 't 5'
END
}

generate_versus_N() {
    for i in `seq 0 2`; do
        alpha=0.$((2 + 3 * i))
        for j in `seq 0 2`; do
            beta=0.$((2 + 3 * j))
            for k in `seq 0 4`; do
                epsilon=`python -c "print 0.1 * (2 + 2*$k)"`
                index=$((15 * i + 5 * j + k))
                ix=18
                for method in ${methods}; do
                    for estimate in star bar; do
                        plot_versus_N $index t $ix j-N-${method}-${estimate}-${alpha}-${beta}-${epsilon}
                        plot_versus_N $index b $((ix + 1)) fr-N-${method}-${estimate}-${alpha}-${beta}-${epsilon}
                        ix=$((ix + 7))
                    done
                done
            done
        done
    done
}

# Plots t versus max_split
# x-axis: N values
# y-axis: FR or Jaccard, as setup via parameters
#
# $1: block num of points (gives <alpha, beta, epsilon>)
# $2: key position (multi-words)
# $3: column for y-axis
# $4: output file-name part
plot_versus_max_split() {
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
set xrange [1:6]
set xlabel "max_split" noenhanced
set yrange [-0.1:1.1]
set ylabel "${ylabel}"
set output "${output}".".eps"
s = 3 * ${block_num}
plot "t_ms.csv" u 9:${column} every 60::s+1 w lp ps 2 lt 1 pt 4 title 't 3',\
     "t_ms.csv" u 9:${column} every 60::s+2 w lp ps 2 lt 1 pt 6 title 't 4',\
     "t_ms.csv" u 9:${column} every 60::s+3 w lp ps 2 lt 1 pt 8 title 't 5'
END
}

generate_versus_max_split() {
    for i in `seq 0 1`; do
        alpha=0.$((2 + 3 * i))
        for j in `seq 0 1`; do
            beta=0.$((2 + 3 * j))
            for k in `seq 0 4`; do
                epsilon=`python -c "print 0.1 * (2 + 2*$k)"`
                index=$((10 * i + 5 * j + k))
                ix=18
                for method in ${methods}; do
                    for estimate in star bar; do
                        plot_versus_max_split $index t $ix j-MS-${method}-${estimate}-${alpha}-${beta}-${epsilon}
                        plot_versus_max_split $index b $((ix + 1)) fr-MS-${method}-${estimate}-${alpha}-${beta}-${epsilon}
                        ix=$((ix + 7))
                    done
                done
            done
        done
    done
}

################# Custom plots
# Plots t3-beta0.4/0.6-eps0.4/0.8 versus alpha
# x-axis: N values
# y-axis: FR or Jaccard, as setup via parameters
#
# $1: column for y-axis
# $2: output file-name part
plot_t3_b46_e48() {
    column=$1
    output=$2
    ylabel=${output%%-*}
    ylabel=${ylabel^^}

    cat << END | gnuplot
set terminal post eps enhanced font "Helvetica,28"
set datafile separator ","
set key bottom left
set xrange [0.1:0.6]
set xlabel "{/Symbol a}"
set yrange [-0.1:1.1]
set ylabel "${ylabel}"
set output "${output}".".eps"
plot "t_ab.csv" u 1:${column} every 105::35 w lp ps 2 lt 1 pt 4 title '{/Symbol b} 0.4 {/Symbol e} 0.4',\
     "t_ab.csv" u 1:${column} every 105::41 w lp ps 2 lt 1 pt 5 title '{/Symbol b} 0.4 {/Symbol e} 0.8',\
     "t_ab.csv" u 1:${column} every 105::65 w lp ps 2 lt 1 pt 6 title '{/Symbol b} 0.6 {/Symbol e} 0.4',\
     "t_ab.csv" u 1:${column} every 105::71 w lp ps 2 lt 1 pt 7 title '{/Symbol b} 0.6 {/Symbol e} 0.8'
END
}

generate_t3_b46_e48() {
    ix=18
    for method in ${methods}; do
        for estimate in star bar; do
            plot_t3_b46_e48 $ix j-t3-b46-e48-${method}-${estimate}
            plot_t3_b46_e48 $((ix + 1)) fr-t3-b46-e48-${method}-${estimate}
            ix=$((ix + 7))
        done
    done
}


# Plots t3-alpha0.2/0.5-eps0.4/0.8 versus beta
# x-axis: beta values
# y-axis: FR or Jaccard, as setup via parameters
#
# $1: column for y-axis
# $2: output file-name part
plot_t3_a25_e48() {
    column=$1
    output=$2
    ylabel=${output%%-*}
    ylabel=${ylabel^^}

    cat << END | gnuplot
set terminal post eps enhanced font "Helvetica,28"
set datafile separator ","
set key right
set xrange [0:1]
set xlabel "{/Symbol b}"
set yrange [-0.1:1.1]
set ylabel "${ylabel}"
set output "${output}".".eps"
plot "t_ab.csv" u 2:${column} every 15::5::95 w lp ps 2 lt 1 pt 4 title '{/Symbol a} 0.2 {/Symbol e} 0.4',\
     "t_ab.csv" u 2:${column} every 15::11::101 w lp ps 2 lt 1 pt 5 title '{/Symbol a} 0.2 {/Symbol e} 0.8',\
     "t_ab.csv" u 2:${column} every 15::320::410 w lp ps 2 lt 1 pt 6 title '{/Symbol a} 0.5 {/Symbol e} 0.4',\
     "t_ab.csv" u 2:${column} every 15::326::416 w lp ps 2 lt 1 pt 7 title '{/Symbol a} 0.5 {/Symbol e} 0.8'
END
}

generate_t3_a25_e48() {
    ix=18
    for method in ${methods}; do
        for estimate in star bar; do
            plot_t3_a25_e48 $ix j-t3-a25-e48-${method}-${estimate}
            plot_t3_a25_e48 $((ix + 1)) fr-t3-a25-e48-${method}-${estimate}
            ix=$((ix + 7))
        done
    done
}

# Plots t3-alpha0.2/0.5-beta0.5-eps0.4/0.8 versus N
# x-axis: N values
# y-axis: FR or Jaccard, as setup via parameters
#
# $1: column for y-axis
# $2: output file-name part
plot_t3_a25_b5_e48_N() {
    column=$1
    output=$2
    ylabel=${output%%-*}
    ylabel=${ylabel^^}

    cat << END | gnuplot
set terminal post eps enhanced font "Helvetica,28"
set datafile separator ","
set key bottom right
set xrange [0:60000]
set xtics ("10" 10000, "20" 20000, "30" 30000, "40" 40000, "50" 50000)
set xlabel "N"
set yrange [-0.1:1.1]
set ylabel "${ylabel}"
set output "${output}".".eps"
plot "t_N.csv" u 10:${column} every 135::20 w lp ps 2 lt 1 pt 4 title '{/Symbol a} 0.2 {/Symbol e} 0.4',\
     "t_N.csv" u 10:${column} every 135::26 w lp ps 2 lt 1 pt 5 title '{/Symbol a} 0.2 {/Symbol e} 0.8',\
     "t_N.csv" u 10:${column} every 135::65 w lp ps 2 lt 1 pt 6 title '{/Symbol a} 0.5 {/Symbol e} 0.4',\
     "t_N.csv" u 10:${column} every 135::71 w lp ps 2 lt 1 pt 7 title '{/Symbol a} 0.5 {/Symbol e} 0.8'
END
}

generate_t3_a25_b5_e48_N() {
    ix=18
    for method in ${methods}; do
        for estimate in star bar; do
            plot_t3_a25_b5_e48_N $ix j-t3-a25-b5-e48-N-${method}-${estimate}
            plot_t3_a25_b5_e48_N $((ix + 1)) fr-t3-a25-b5-e48-N-${method}-${estimate}
            ix=$((ix + 7))
        done
    done
}

# Plots t34-alpha0.2-beta0.5-eps0.4/0.8 versus max_split
# x-axis: N values
# y-axis: FR or Jaccard, as setup via parameters
#
# $1: column for y-axis
# $2: output file-name part
plot_t34_a2_b5_e48_max_split() {
    column=$1
    output=$2
    ylabel=${output%%-*}
    ylabel=${ylabel^^}

    cat << END | gnuplot
set terminal post eps enhanced font "Helvetica,28"
set datafile separator ","
set key right
set xrange [1:6]
set xlabel "max_split" noenhanced
set yrange [-0.1:1.1]
set ylabel "${ylabel}"
set output "${output}".".eps"
plot "t_ms.csv" u 9:${column} every 60::20 w lp ps 2 lt 1 pt 4 title 't 3 {/Symbol e} 0.4',\
     "t_ms.csv" u 9:${column} every 60::21 w lp ps 2 lt 1 pt 5 title 't 4 {/Symbol e} 0.4',\
     "t_ms.csv" u 9:${column} every 60::26 w lp ps 2 lt 1 pt 6 title 't 3 {/Symbol e} 0.8',\
     "t_ms.csv" u 9:${column} every 60::27 w lp ps 2 lt 1 pt 7 title 't 4 {/Symbol e} 0.8'
END
}

generate_t34_a2_b5_e48_max_split() {
    ix=18
    for method in ${methods}; do
        for estimate in star bar; do
            plot_t34_a2_b5_e48_max_split $ix j-t34-a2-b5-e48-MS-${method}-${estimate}
            plot_t34_a2_b5_e48_max_split $((ix + 1)) fr-t34-a2-b5-e48-MS-${method}-${estimate}
            ix=$((ix + 7))
        done
    done
}

###################### Comment here lines which are disabled
generate_versus_epsilon
#generate_versus_alpha
#generate_versus_beta
#generate_versus_N
#generate_versus_max_split
generate_t3_b46_e48
generate_t3_a25_e48
generate_t3_a25_b5_e48_N
generate_t34_a2_b5_e48_max_split
