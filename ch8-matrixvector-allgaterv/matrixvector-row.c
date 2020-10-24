#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "MyMPI.h"

// 矩阵向量乘法，按行分解

typedef double dtype;
#define mpitype MPI_DOUBLE

int main(int argc, char* argv[]) {
    int id, p;
    dtype** a;  //id进程拥有的部分矩阵
    dtype* storage;  //id进程拥有的部分矩阵，实际存储
    dtype* b;   //待乘向量
    dtype* c;   //所有结果向量
    dtype* c_block;  //id进程计算出的部分结果向量
    int m, n;   //总矩阵的行和列
    int rows;   //id进程拥有的行数
    int nprime;    //向量长度

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    if(argc != 3) {
        printf("input file for matrix and vector should be given!\n");
        exit(-1);
    }

    //读取矩阵
    read_row_stripped_matrix(argv[1], (void*)&a, (void*)&storage, mpitype, &m, &n, MPI_COMM_WORLD);
    rows = BLOCK_SIZE(id, p, m);
    print_row_stripped_matrix((void**)a, mpitype, m, n, MPI_COMM_WORLD);

    //读取向量，所有进程都拥有该副本
    read_replicated_vector(argv[2], (void*)&b, mpitype, &nprime, MPI_COMM_WORLD);
    print_replicated_vector(b, mpitype, nprime, MPI_COMM_WORLD);

    if(n != nprime) {  //维度不匹配
        if(!id) printf("matrix size and vector length don't match!\n");
        MPI_Abort(MPI_COMM_WORLD, DIMENSION_ERR);
    }

    //为计算结果分配空间
    c_block = (dtype*)malloc(rows * sizeof(dtype));
    c = (dtype*)malloc(m * sizeof(dtype));

    // 计算内积
    for(int i = 0; i < rows; ++i) {
        c_block[i] = 0;
        for(int j = 0; j < n; ++j) {
            c_block[i] += a[i][j] * b[j];
        }
        //printf("%d %6.3f\n", id, c_block[i]);
    }

    replicate_block_vector((void*)c_block, m, (void*)c, mpitype, MPI_COMM_WORLD);
    print_replicated_vector(c, mpitype, m, MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}