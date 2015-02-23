#!/bin/bash

plot_one () {
    file=$1
    cat << END | gnuplot
set term svg enhanced mouse size 1000,1000 mouse
set output "${file}".".svg"
plot "${file}" i 0 u 1:2 w p pt 3 ps 0.5 notitle,\
    "${file}" i 0 u (\$1+0.3):(\$2-0.3):3 w labels font "arial,6" tc lt 0 notitle,\
    "${file}" i 1 u 1:2:3:4 w vectors nohead lt 0 notitle,\
    "${file}" i 2 u (\$1-2):(\$2+2):(sprintf("n: %5.2f", \$3)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1+2):(\$2+2):(sprintf("s: %5.2f", \$4)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1-2):(\$2+1):(sprintf("n^*: %5.2f", \$5)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1+2):(\$2+1):(sprintf("s^*: %5.2f", \$7)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1-2):(\$2+0):(sprintf("%5.2f", \$6)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1+2):(\$2+0):(sprintf("%5.2f", \$8)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1-2):(\$2-1):(sprintf("n': %5.2f", \$9)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1+2):(\$2-1):(sprintf("s': %5.2f", \$11)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1-2):(\$2-2):(sprintf("%5.2f", \$10)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1+2):(\$2-2):(sprintf("%5.2f", \$12)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1-2):(\$2-3):(sprintf("-n-: %5.2f", \$13)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1+2):(\$2-3):(sprintf("-s-: %5.2f", \$15)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1-2):(\$2-4):(sprintf("%5.2f", \$14)) w labels font "arial,4" tc lt 0 notitle,\
    "${file}" i 2 u (\$1+2):(\$2-4):(sprintf("%5.2f", \$16)) w labels font "arial,4" tc lt 0 notitle,\

END
}

for i in $@; do
    plot_one $i
done
