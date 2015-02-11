#!/bin/bash

make && valgrind ./dpsn 0.5 0.5 10 3 2.0 u datasets/debug1.dat
