#!/bin/bash

alphas="0.2 0.5"
betas="0.5"
K=10
Nt=3
epsilons="0.4 0.8"
depth=3
ttresh=1
resolution=1
seeds="112288 148445 779888 288613 942277 57111 99852 19158 45622 31234"

datasetdir="datasets"
outputdir="output/ext5"
test -d ${outputdir} || mkdir -p ${outputdir} || exit

run () {
    datafile=$1
    outbase=${outputdir}/${datafile##*/}
    outbase=${outbase%.*}
    outfile=${outbase}.out

    > ${outfile}
    for epsilon in ${epsilons}; do
        for alpha in ${alphas}; do
            for beta in ${betas}; do
                for seed in ${seeds}; do
                    ./dpsn ${alpha} ${beta} $K ${Nt} ${epsilon} t ${depth} ${ttresh} ${resolution} ${datafile} ${seed} >> ${outfile}
                done
            done
        done
    done
}

for i in ${datasetdir}/sz_*.dat; do
    run $i
done
