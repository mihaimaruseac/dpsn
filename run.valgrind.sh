#!/bin/bash

make && valgrind ./dpsn 0.5 0.5 10 3 2.0 a 0.2 datasets/debug1.dat
