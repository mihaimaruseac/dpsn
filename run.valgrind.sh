#!/bin/bash

make && valgrind ./dpsn 0.1 0.2 10 3 0.5 u dspn
