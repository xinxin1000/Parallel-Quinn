#!/bin/bash

infile="in-matrix in-vector"
prom_row="matrixvector-row"
prom_col="matrixvector-col"
prom_block="matrixvector-block"

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "两个参数: 脚本名称[row, col, block] 线程数"
    exit 1
fi


if [ $1 == "row" ]; then
    mpirun -np $2 ./${prom_row} $infile
elif [ $1 == "col" ]; then
    mpirun -np $2 ./${prom_col} $infile
elif [ $1 == "block" ]; then
    mpirun -np $2 ./${prom_block} $infile
fi