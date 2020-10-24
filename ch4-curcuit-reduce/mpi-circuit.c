#include <stdio.h>
#include <mpi.h>

//Circuit satisfiability
//16个输入，0或1，通过特定逻辑运算，求所有得到1的输入组合

//1 按域分解，每个进程负责检查i，i+p， i+2p...的输入是否符合
//2 MPI_Reduce 统计总数
//3 计时，测试性能

int main(int argc, char *argv[]){
    int i;
    int id;  // process id
    int p; //number of processes
    int local_cnt = 0;
    int global_cnt = 0;  //注意了，只有id=0 的进程里的这个值是有意义的，但每个进程都维护了一个副本
    int check_curcuit(int, int);
    double elapsed_time;


    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);  //所有进程准备好之后，同步，再计时
    elapsed_time = -MPI_Wtime();  //从过去某点到现在的时间
    //MPI_Wtick()  //返回时间精度

    MPI_Comm_rank(MPI_COMM_WORLD, &id);  
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    for(i = id; i < 65536; i += p) {
        local_cnt += check_curcuit(id, i);
    }

    // 统计总数，归约
    MPI_Reduce(&local_cnt, &global_cnt, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    elapsed_time += MPI_Wtime();

    printf("Process %d is done, time %lf， timer accuracy %.9lf\n", id, elapsed_time, MPI_Wtick());
    fflush(stdout); //即使程序崩溃，输出缓存也会被清空并输出

    MPI_Finalize();

    if(id == 0) {
        printf("There are %d different solutions\n", global_cnt);
    }
    return 0;
}

// 获得n的第i位
#define EXTRACT_BIT(n, i) (((n >> i) & 1) ? 1 : 0)

int check_curcuit(int id, int n) {
    int v[16];
    for(int i = 0; i < 16; ++i){
        v[i] = EXTRACT_BIT(n, i);
    }

    if((v[0] || v[1]) && (!v[1] || !v[3]) && (v[2] || v[3])
    && (!v[3] || !v[4]) && (v[4] || !v[5])
    && (v[5] || !v[6]) && (v[5] || v[6])
    && (v[6] || !v[15]) && (v[7] || !v[8])
    && (!v[7] || !v[13]) && (v[8] || v[9])
    && (v[8] || !v[9]) && (!v[9] || !v[10])
    && (v[9] || v[11]) && (v[10] || v[11])
    && (v[12] || v[13]) && (v[13] || !v[14])
    && (v[14] || v[15])) {
        printf("(%d) %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", 
        id, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10],
        v[11], v[12], v[13], v[14], v[15]);
        //写在一个printf语句里，输出才不会被其它线程截胡
        fflush(stdout);
        return 1;
    }
    return 0;
}
