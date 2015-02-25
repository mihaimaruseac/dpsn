#!/bin/bash

alphas="0.2 0.5 0.9"
betas="0.2 0.5 0.9"
gammas="0.01 0.05 0.2"
depths="3 5"
Ks="5 10"
Nts="3 4"
TNts="3 5"
epsilons="0.2 0.5 0.9"
ttreshes="0.1 0.5"
seeds="42"

datasetdir="datasets"
outputdir="output"

test -d ${outputdir} || mkdir -p ${outputdir} || exit

run_uat () {
    datafile=$1
    outfile=$2
    outfile=${outfile}.out
    meth=$3
    NTargs=$4
    margs=$5

    > ${outfile}
    for alpha in ${alphas}; do
        for beta in ${betas}; do
            for K in ${Ks}; do
                for Nt in ${NTargs}; do
                    for epsilon in ${epsilons}; do
                        for marg in ${margs}; do
                            for ttresh in ${ttreshes}; do
                                for seed in ${seeds}; do
                                    ./dpsn ${alpha} ${beta} $K ${Nt}\
                                        ${epsilon} ${meth} ${marg}\
                                        ${ttresh} ${datafile} ${seed} >> ${outfile}
                                done
                            done
                        done
                    done
                done
            done
        done
    done
}

run () {
    datafile=$1
    outfile_base=${outputdir}/${datafile##*/}
    outfile_base=${outfile_base%.*}

    run_uat ${datafile} ${outfile_base}_u u "${Nts}" "${gammas}"
    run_uat ${datafile} ${outfile_base}_a a "${Nts}" "${gammas}"
    run_uat ${datafile} ${outfile_base}_t t "${TNts}" "${depths}"
}

for i in ${datasetdir}/*.dat; do
    run $i
done
