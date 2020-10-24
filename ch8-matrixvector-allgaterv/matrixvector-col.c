#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "MyMPI.h"

// 矩阵向量乘法，按列分解

typedef double dtype;
#define mpitype MPI_DOUBLE

int main(int argc, char* argv[]) {
    int id, p;
    dtype** a;  //id进程拥有的部分矩阵
    dtype* storage;  //id进程拥有的部分矩阵，实际存储
    dtype* b;   //id进程的列，对应的待乘向量元素
    dtype* c;   //所有结果向量
    dtype* c_part_out;  //id进程得到其它进程的结果
    dtype* c_part_in;  //id进程计算出的部分结果
    int m, n;   //总矩阵的行和列
    int nprime;  
    int local_els;   //id进程拥有的列数（也是有用的b向量元素）
    int *cnt_out;
    int *cnt_in;
    int *disp_out;
    int *disp_in;

    if(argc != 3) {
        printf("input file for matrix and vector should be given!\n");
        exit(-1);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    //按列读取矩阵
    read_col_striped_matrix(argv[1], (void*)&a, (void*)&storage, mpitype, &m, &n, MPI_COMM_WORLD);
    print_col_striped_matrix((void*)a, mpitype, m, n, MPI_COMM_WORLD);

    //按块读取向量, 每个进程只有一部分向量的元素
    read_block_vector(argv[2], (void*)&b, mpitype, &nprime, MPI_COMM_WORLD); 

    if(n != nprime) {  //维度不匹配
        if(!id) printf("matrix size and vector length don't match!\n");
        MPI_Abort(MPI_COMM_WORLD, DIMENSION_ERR);
    }
    print_block_vector(b, mpitype, nprime, MPI_COMM_WORLD);


    //将进程拥有的列与拥有的b向量中的元素相乘，得到部分结果
    c_part_out = (dtype*)my_malloc(id, m * sizeof(dtype));
    local_els = BLOCK_SIZE(id, p, n);

    for(int i = 0; i < m; ++i) {
        c_part_out[i] = 0.0;
        for(int j = 0; j < local_els; ++j) {
            c_part_out[i] += a[i][j] * b[j];
        } 
    }


    //发送与接收向量c的部分结果, 每个进程只负责汇总local_els个c中的元素
    create_mixed_xfer_arrays(id, p, m, (void*)&cnt_out, (void*)&disp_out);
    create_uniform_xfer_arrays(id, p, m, (void*)&cnt_in, (void*)&disp_in);

    c_part_in = (dtype*)my_malloc(id, p * BLOCK_SIZE(id, p, m) * sizeof(dtype));  //这里收集了进程id负责求和的所有暂时的计算结果
    MPI_Alltoallv(c_part_out, cnt_out, disp_out, mpitype,
                c_part_in, cnt_in, disp_in, mpitype, MPI_COMM_WORLD);
    
    
    // 计算最终的c
    c = (dtype*)my_malloc(id, BLOCK_SIZE(id, p, m) * sizeof(dtype));

    for(int i = 0; i < BLOCK_SIZE(id, p, m); ++i) {
        c[i] = 0;
        for(int j = 0; j < p; ++j) {
            c[i] += c_part_in[i + j * BLOCK_SIZE(id, p, m)];
        }
        //printf("%d %6.3f\n", id, c[i]);
    }

    //输出结果
    print_block_vector(c, mpitype, m, MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}