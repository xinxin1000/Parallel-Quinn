//Floyd's all-pair shortest-path algorithm
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "MyMPI.h"


typedef int dtype; //数据类型
#define MPI_TYPE MPI_INT  //mpi数据类型

int main(int argc, char* argv[]){
    dtype** a;  //当前进程分配到的一些行
    dtype* storage;  
    int m, n;  //总矩阵行、列
    int id, p;
    void compute_shortest_path(int, int, int**, int);

    if(argc != 2) {
        printf("input file should be given!\n");
        exit(-1);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    //读取邻接矩阵
    read_row_stripped_matrix(argv[1], (void***)&a, (void**)&storage, MPI_TYPE,
    &m, &n, MPI_COMM_WORLD);
    if(m != n) terminate(id, "Matrix must be square.\n");

    print_row_stripped_matrix((void**)a, MPI_TYPE, m, n, MPI_COMM_WORLD);
    compute_shortest_path(id, p, a, n);
    print_row_stripped_matrix((void**)a, MPI_TYPE, m, n, MPI_COMM_WORLD);

    MPI_Finalize();
}

void compute_shortest_path(int id, int p, dtype** a, int n) { //a是当前进程包含的行
    //不同进程分到不同行，更新
    int root, offset;  //offset第k行在进程root中的序号
    dtype* tmp = (dtype*) malloc(sizeof(dtype) * n);

    for(int k = 0; k < n; ++k) {
        root = BLOCK_OWNER(k, p, n);
        if(id == root) {  //k在当前进程覆盖的行
            offset = k - BLOCK_LOW(id, p, n);
            for(int j = 0; j < n; ++j) {
                tmp[j] = a[offset][j];
            }
        }

        // 每个进程有第k列，但是第k行需要从root进程广播到其它进程
        MPI_Bcast(tmp, n, MPI_TYPE, root, MPI_COMM_WORLD);

        // 各进程再更新自己的所有值
        for(int i = 0; i < BLOCK_SIZE(id, p, n); ++i) {
            for(int j = 0; j < n; ++j) {
                a[i][j] = MIN(a[i][j], a[i][k] + tmp[j]);
            }
        }
    }
    free(tmp);  //释放空间
}