#!/bin/bash

# plot_eps ${datafile} ${outbase}
# plot J vs. epsilon
plot_eps () {
    datafile=$1
    outbase=$2

    output=${outbase}_J_vs_e.eps
    alpha=0.5
    beta=0.5

    cat << END | gnuplot
set xtics (0.2, 0.4, 0.6, 0.8, 1)
set xrange [0:1.1]
set xlabel "{/Symbol e}"
set yrange [-0.1:1.1]
set ylabel "J"
set key bottom right
set terminal post eps enhanced font "Helvetica,28"
set output "${output}"
plot \
    "<awk -F, '{if(\$1 == ${alpha} && \$2 == ${beta} && \$4 == 20000){print \$3,\$5}}' ${datafile}"\
        w lp ps 2 lt 1 pt 4 title "Av1",\
    "<awk -F, '{if(\$1 == ${alpha} && \$2 == ${beta} && \$4 == 20000){print \$3,\$6}}' ${datafile}"\
        w lp ps 2 lt 1 pt 6 title "Av2",\
    "<awk -F, '{if(\$1 == ${alpha} && \$2 == ${beta} && \$4 == 20000){print \$3,\$7}}' ${datafile}"\
        w lp ps 2 lt 1 pt 8 title "Rv50",\
    "<awk -F, '{if(\$1 == ${alpha} && \$2 == ${beta} && \$4 == 20000){print \$3,\$8}}' ${datafile}"\
        w lp ps 2 lt 1 pt 5 title "P 0.25",\
    "<awk -F, '{if(\$1 == ${alpha} && \$2 == ${beta} && \$4 == 20000){print \$3,\$9}}' ${datafile}"\
        w lp ps 2 lt 1 pt 7 title "P 0.5",\
    "<awk -F, '{if(\$1 == ${alpha} && \$2 == ${beta} && \$4 == 20000){print \$3,\$10}}' ${datafile}"\
        w lp ps 2 lt 1 pt 9 title "P 0.75",
END
}

# plot_alpha_beta_N ${datafile} ${outbase} ${ycolumn} ${descr} ${out} ${xcolumn} ${xrange} ${xlabel} ${keypos} ${test1lab} ${test1col} ${test1val1} ${test1val2} ${test2col} ${test2val} ${xticks}
# plot J vs. alpha/beta/N (common code)
plot_alpha_beta_N () {
    datafile=$1
    outbase=$2
    ycolumn=$3
    descr=$4
    out=$5
    xcolumn=$6
    xrange=$7
    xlabel=$8
    keypos=$9

    test1lab=${10}
    test1col=${11}
    test1val1=${12}
    test1val2=${13}
    test2col=${14}
    test2val=${15}

    xticks=${16}

    output=${outbase}_J_vs_${out}_${descr}.eps

    cat << END | gnuplot
${xticks}
set xrange ${xrange}
set xlabel "${xlabel}"
set yrange [-0.1:1.1]
set ylabel "J"
set key bottom ${keypos}
set terminal post eps enhanced font "Helvetica,28"
set output "${output}"
plot \
    "<awk -F, '{if(\$${test1col} == ${test1val1} && \$3 == 0.4 && \$${test2col} == ${test2val}){print \$${xcolumn},\$${ycolumn}}}' ${datafile}"\
        w lp ps 2 lt 1 pt 4 title "${test1lab} ${test1val1} {/Symbol e} 0.4",\
    "<awk -F, '{if(\$${test1col} == ${test1val1} && \$3 == 0.8 && \$${test2col} == ${test2val}){print \$${xcolumn},\$${ycolumn}}}' ${datafile}"\
        w lp ps 2 lt 1 pt 5 title "${test1lab} ${test1val1} {/Symbol e} 0.8",\
    "<awk -F, '{if(\$${test1col} == ${test1val2} && \$3 == 0.4 && \$${test2col} == ${test2val}){print \$${xcolumn},\$${ycolumn}}}' ${datafile}"\
        w lp ps 2 lt 1 pt 6 title "${test1lab} ${test1val2} {/Symbol e} 0.4",\
    "<awk -F, '{if(\$${test1col} == ${test1val2} && \$3 == 0.8 && \$${test2col} == ${test2val}){print \$${xcolumn},\$${ycolumn}}}' ${datafile}"\
        w lp ps 2 lt 1 pt 7 title "${test1lab} ${test1val2} {/Symbol e} 0.8"
END
}

# plot_alpha ${datafile} ${outbase} ${column} ${descr} - plot J vs. alpha
plot_alpha () {
    plot_alpha_beta_N $@ a 1 "[0.1:0.6]" "{/Symbol a}" left\
        "{/Symbol b}" 2 0.4 0.6 4 20000\
        " "
}

# plot_beta ${datafile} ${outbase} ${column} ${descr} - plot J vs. beta
plot_beta () {
    plot_alpha_beta_N $@ b 2 "[0:1]" "{/Symbol b}" right\
        "{/Symbol a}" 1 0.2 0.5 4 20000\
        " "
}

# plot_N ${datafile} ${outbase} ${column} ${descr} - plot J vs. N
plot_N () {
    plot_alpha_beta_N $@ N 4 "[0:60000]" "N" right\
        "{/Symbol a}" 1 0.2 0.5 2 0.5\
        "set xtics (\"10K\" 10000, \"20K\" 20000, \"30K\" 30000, \"40K\" 40000, \"50K\" 50000)"
}

# plot ${datafile} ${outbase} - plot all plots
plot () {
    declare -a terms=(_ _ _ _ _ Av1 Av2 Rv50 P025 P050 P075 P100 P125 P150)
    for i in `seq 5 13`; do
        plot_alpha $@ $i ${terms[i]}
        plot_beta $@ $i ${terms[i]}
        plot_N $@ $i ${terms[i]}
    done

    plot_eps $@
}

datafile=$1
outdir=${datafile%/*}/res
[ -d  ${outdir} ] || mkdir -p ${outdir}
outbase=${datafile##*/}
outbase=${outdir}/${outbase%.*}

plot ${datafile} ${outbase}
