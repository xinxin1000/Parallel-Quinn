#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "MyMPI.h"

// 矩阵向量乘法，按列分解

typedef double dtype;
#define mpitype MPI_DOUBLE

int main(int argc, char* argv[]) {
    dtype** a;  //二维矩阵，存每行首地址
    dtype* storage;  //实际存储，一位向量
    dtype* b; //进程对应的向量元素
    dtype* c_part; //每个进程的部分结果
    dtype* c;  //cart域中一行的求和结果

    int id, p;
    int m, n, nprime;   //总矩阵的行和列
    int local_rows, local_cols;  //进程拥有的矩阵中的行和列数
    
    MPI_Comm cart_comm;
    int size[2]; //cart域的行数和列数
    int periodic[2];  //cart域是否周期性
    int coord[2];  //进程在cart域中的行列坐标

    MPI_Comm row_comm;
    MPI_Comm col_comm;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    size[0] = size[1] = 0;
    MPI_Dims_create(p, 2, size);  //找到最合适的cart域行数和列数
    periodic[0] = periodic[1] = 0;
    MPI_Cart_create(MPI_COMM_WORLD, 2, size, periodic, 1, &cart_comm);
    MPI_Cart_get(cart_comm, 2, size, periodic, coord);

    MPI_Comm_split(cart_comm, coord[0], coord[1], &row_comm);
    MPI_Comm_split(cart_comm, coord[1], coord[0], &col_comm);

    //steo1 ： 先读入矩阵
    read_checkerboard_matrix(argv[1], (void*)&a, (void*)&storage, mpitype, &m, &n, cart_comm);
    //调试用，可以看到不同进程拥有的元素
    // for(int i = 0; i < BLOCK_SIZE(coord[0], size[0], m); ++i) {
    //     for(int j = 0; j < BLOCK_SIZE(coord[1], size[1], n); ++j) {
    //         printf("%d %d %6.3f--- ", coord[0], coord[1], a[i][j]);
    //     }
    //     putchar('\n');
    // }
    // putchar('\n');
    print_checkerboard_matrix((void*)a, mpitype, m, n, cart_comm);

    //step2 ： 再读入向量并分发给每行的分组
    read_checkerboard_vector(argv[2], (void*)&b, mpitype, &nprime, cart_comm, row_comm, col_comm);
    if(n != nprime) {
        if(!id) printf("matrix size and vector length don't match!\n");
        MPI_Abort(MPI_COMM_WORLD, DIMENSION_ERR);
    }
    print_checkerboard_vector(b, mpitype, n, cart_comm, row_comm);

    //调试用
    // for(int j = 0; j < BLOCK_SIZE(coord[1], size[1], n); ++j) {
    //     printf("%d %d %6.3f--- ", coord[0], coord[1], b[j]);
    // }
    // putchar('\n');

    //step3 : 每个进程自己算结果
    local_rows = BLOCK_SIZE(coord[0], size[0], m);
    local_cols = BLOCK_SIZE(coord[1], size[1], n);
    c_part = my_malloc(id, local_rows * sizeof(dtype));
    
    for(int i = 0; i < local_rows; ++i) {
        c_part[i] = 0;
        for(int j = 0; j < local_cols; ++j) {
            c_part[i] += a[i][j] * b[j];
        }
        //printf("%d %d %6.3f\n", coord[0], coord[1], c_part[i]);
    }

    //cart域中同行的进程，可以做一次reduce,求和到处在第一列的进程
    if(coord[1] == 0) {  //只有归约的根进程需要真的申请内存
        c = my_malloc(id, local_rows * sizeof(dtype));
    }
    MPI_Reduce(c_part, c, local_rows, mpitype, MPI_SUM, 0, row_comm);


    //step 4 : 然后第一列进行结果输出
    if(coord[1] == 0){
        // 调试用
        // for(int i = 0; i < local_rows; ++i) 
        //     printf("%d %d %6.3f\n", coord[0], coord[1], c[i]);
        // puts("");
        print_block_vector(c, mpitype, m, col_comm);
    }

    MPI_Finalize();
        
    return 0;
}