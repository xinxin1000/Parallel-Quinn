// Sieve of Eratosthenes

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//思路： 进程0选出每轮的最小素数，然后广播给其它进程，所有进程标记该素数的倍数，最后未标记的数为素数

#define BLOCK_LOW(id, p, n) ((id) * (n) / (p))  //id: 进程号，n：筛选的数字个数，p：进程总数； 计算进程分到的数字最小值（起点0） !#define中所有的变量最好都带括号。。。
#define BLOCK_HIGH(id, p, n) ((id + 1) * (n) / (p) - 1)   //进程分到的数字最大值
#define BLOCK_SIZE(id, p, n) (BLOCK_LOW((id) + 1, p, n) - BLOCK_LOW(id, p, n))
#define BLOCK_OWNER(index, p, n) (((index + 1) * (p) - 1) / n)  //序号index 的数字被分到哪个进程

int main(int argc, char* argv[]) {
    
    double elapsed_time;
    int id, p;

    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);
    elapsed_time = -MPI_Wtime();

    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    if(argc != 2) {  //缺少范围参数，直接终止mpi，退出
        if(!id) printf("range of numbers missing for program %s \n", argv[0]);
        MPI_Finalize();
        exit(1);
    }

    int n = atoi(argv[1]);  //筛选1～n，1不用筛，一共n-1个数，注意数值从2开始，下面的low_value，high_value 都要+2

    //所有待检查素数要在进程0中才行
    int proc0_size = (n-1) / p;
    if((proc0_size + 2) * (proc0_size + 2) < n){
        if(!id) printf("Too many processors\n");
        MPI_Finalize();
        exit(1);
    } 

    int low_value = BLOCK_LOW(id, p, n-1) + 2;  //进程id筛选的数值范围
    int high_value = BLOCK_HIGH(id, p, n-1) + 2;
    int size = BLOCK_SIZE(id, p, n-1);

    //printf("(%d) %d %d %d\n", id, low_value, high_value, size);

    // 分配进程id的内存
    // char字节数=1，用char类型做标记
    char* mark = (char*)malloc(size);
    if(mark == NULL) {  //判断内存是够分配
        printf("Cannot allocate enough memory\n");
        MPI_Finalize();
        exit(1);
    }

    for(int i = 0; i < size; ++i) mark[i] = 0;

    int index = 0, prime = 2;  //j进程0中，index: 当前素数的序号，prime：当前的素数
    

    int first;  //当前进程第一个prime倍数的下标
    int mod;
    // 每个循环，进程0选出当前最小的素数，其他进程标记prime的倍数
    do {
        if(prime * prime > low_value){
            first = prime * prime - low_value;  //标记prime平方以上的即可（小于它的肯定已经标记了）
        }
        else {
            mod =  low_value % prime;
            if(mod == 0) {
                first = 0;
            }
            else {
                first = prime - mod;
            }
        }
        for(int i = first; i < size; i += prime) {  //标记
            mark[i] = 1;
        }
        // 进程0确定下一个素数
        if(!id) {
            while(mark[++index]);
            prime = index + 2;
        }

        //进程0向其它进程广播prime
        MPI_Bcast(&prime, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }while (prime * prime <= n);

    // 统计素数总个数
    int local_count = 0;
    int global_count;

    // 输出每个素数
    for(int i = 0; i < size; ++i) {
        if(mark[i] != 1) {
            printf("(%d) %d ", id, i + low_value);
            local_count ++;
        } 
    }
    puts("");

    MPI_Reduce(&local_count, &global_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); //向进程0归约

    MPI_Barrier(MPI_COMM_WORLD);
    elapsed_time += MPI_Wtime();
    MPI_Finalize();

    if(!id) {
        printf("There are %d prime less than %d\n", global_count, n);
        printf("Total elapsed time %.6lf\n", elapsed_time);
    }

    return 0;
}