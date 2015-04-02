#!/bin/bash

alphas="0.2 0.5 0.9"
betas="0.2 0.5 0.9"
gammas="0.01 0.05 0.2"
depths="3"
Ks="10"
Nts="3"
TNts="3"
epsilons="0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9"
ttreshes="0.01"
seeds="42 142 100"
resolutions="10"

datasetdir="datasets"
outputdir="output/output.r6.april"

test -d ${outputdir} || mkdir -p ${outputdir} || exit

run_uat () {
    datafile=$1
    outfile=$2
    outbase=${outfile}
    outfile=${outfile}.out
    meth=$3
    NTargs=$4
    margs=$5

    > ${outfile}
    for marg in ${margs}; do
        for alpha in ${alphas}; do
            for beta in ${betas}; do
                for K in ${Ks}; do
                    for Nt in ${NTargs}; do
                        for epsilon in ${epsilons}; do
                            for ttresh in ${ttreshes}; do
                                for seed in ${seeds}; do
                                    for resolution in ${resolutions}; do
                                        ./dpsn ${alpha} ${beta} $K ${Nt}\
                                            ${epsilon} ${meth} ${marg}\
                                            ${ttresh} ${resolution}\
                                            ${datafile} ${seed} >> ${outfile}
                                        test -f debug_* && for f in debug_*; do
                                            mv $f ${outbase}_${f}_${alpha}_${beta}_$K_${Nt}_${epsilon}_${meth}_${marg}_${ttresh}_${resolution}_${seed}.gnpl
                                        done
                                    done
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

for i in ${datasetdir}/debug_N20000*.dat; do
    run $i
done
