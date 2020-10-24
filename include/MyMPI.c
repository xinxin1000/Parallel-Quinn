#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "MyMPI.h"


//************************MISCELLANEOUS FUNCTIONS***********

//获得mpi内置数据类型的字节数
int get_size(MPI_Datatype t) {
    if(t == MPI_BYTE) return sizeof(char);
    if(t == MPI_DOUBLE) return sizeof(double);
    if(t == MPI_FLOAT) return sizeof(float);
    if(t == MPI_INT) return sizeof(int);
    printf("Error: unrecognized datatype");
    fflush(stdout);
    MPI_Abort(MPI_COMM_WORLD, TYPE_ERR);
}


//所有的进程必须都要调用这个函数
void terminate(
    int id,
    char *error_msg
) {
    if(!id) {
        printf("Error: %s\n", error_msg);
        fflush(stdout);
    }
    MPI_Finalize();
    exit(-1);
}

// 分配空间
void* my_malloc(int id, int bytes) {  //输入进程号， 字节数
    void *buffer;
    if((buffer = malloc((size_t)bytes)) == NULL) {
        printf("Error: Malloc failed for process %id\n", id);
        fflush(stdout);
        MPI_Abort(MPI_COMM_WORLD, MALLOC_ERR);
    }
    return buffer;
}



//**********************DATA DISTRIBUTION FUNTIONS*********

//将向量的元素分配给不同进程，记录分配个数与偏移量
void create_mixed_xfer_arrays(
    int id, //
    int p, 
    int n, //输入，元素总个数
    int** count,  //输出，每个进程拥有的向量个数，的数组
    int** disp  //输出，每个进程首元素在总向量中的偏移量
){
    *count = my_malloc(id, p * sizeof(int));
    *disp = my_malloc(id, p * sizeof(int));
    (*count)[0] = BLOCK_SIZE(0, p, n);
    (*disp)[0] = 0;
    for(int i = 1; i < p; ++i) {
        (*disp)[i] = (*disp)[i-1] + (*count)[i-1];
        (*count)[i] = BLOCK_SIZE(i, p, n);
    }
}

//用于alltoAll，记录个数和偏移量，但是此时每个进程从其它进程获得相等个数的元素
void create_uniform_xfer_arrays(
    int id, 
    int p, 
    int n,
    int** count,  //输出，个数数组
    int** disp  //输出，偏移量数组
) {
    *count = my_malloc(id, sizeof(int) * p);
    *disp = my_malloc(id, sizeof(int) * p);
    (*count)[0] = BLOCK_SIZE(id, p, n);
    (*disp)[0] = 0;
    for(int i = 1; i < p; ++i) {
        (*disp)[i] = (*disp)[i-1] + (*count)[i-1];
        (*count)[i] = BLOCK_SIZE(id, p, n);  //注意这里是id，mixed函数中是i
    }
}



//向量聚合, 每个进程拥有向量的一部分元素,最后使每个进程都拥有全部向量
void replicate_block_vector(
    void* ablock, //  输入，每个进程拥有的一部分向量
    int n, //输入，元素总个数
    void* agre,  //输出，聚合后的总向量
    MPI_Datatype dtype,  //
    MPI_Comm comm //
) {
    int id, p;
    int* cnt;  //分配个数数组
    int* disp;  //偏移量数组
    
    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &p);
    create_mixed_xfer_arrays(id, p, n, &cnt, &disp);
    
    MPI_Allgatherv(ablock, cnt[id], dtype, agre, cnt, disp, dtype, comm);
    //printf("%d %d %d\n", id, cnt[id], disp[id]);
    // if(id == 0) {
    //     printf("%d\n", id);
    //     for(int i = 0; i < n; ++i) printf("%6.3f ", ((double*)ablock)[i]);
    //     putchar('\n');
    // }
    
    free(cnt);
    free(disp);    
    //printf("%d %d %d\n", id, cnt[id], disp[id]);
}



//**************************INPUT**************************


// 按行读取矩阵
void read_row_stripped_matrix(
    char* s, //文件名
    void*** subs, //输出，2D 子矩阵,存储的是行首的指针，int**
    void** storage, //输出，子矩阵的实际存储
    MPI_Datatype dtype, //输入，元素类型
    int* m, //输出，行， 注意这里m、n已经是ptr了
    int* n, //输出，列
    MPI_Comm comm //输入，通信域
) {
    
    // 思路：最后一个进程打开文件，复制到自己的内存，然后拷贝给其它进程
    int id, p;
    FILE *infileptr;
    
    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &p);


    int datum_size = get_size(dtype);  //矩阵中元素的数据类型大小

    /*  p-1进程打开文件
        读取行与列  */
    if (id == p-1) {
        infileptr = fopen(s, "r");
        if(infileptr == NULL) *m = 0;
        else {
            fread(m, sizeof(int), 1, infileptr);   //读取行数
            fread(n, sizeof(int), 1, infileptr);   //读取列数
        }
    }
    
    MPI_Bcast(m, 1, MPI_INT, p-1, MPI_COMM_WORLD);  //广播行数
    if(!(*m)) MPI_Abort(MPI_COMM_WORLD, OPEN_FILE_ERR);
    MPI_Bcast(n, 1, MPI_INT, p-1, MPI_COMM_WORLD);  //广播列数
    


    /*  分配进程id所需空间 */
    int local_rows = BLOCK_SIZE(id, p, *m);  //当前进程拥有的行

    *storage =  my_malloc(id, local_rows * (*n) * datum_size);  //当前进程包括的行所需的空间
    *subs =  my_malloc(id, local_rows * PTR_SIZE);
    
    void** ptr1 = *subs; //subs中的首地址
    void* ptr2 = *storage; //storage中的首地址
    for(int i = 0; i < local_rows; ++i) {
        *ptr1 = (void*)ptr2;
        ptr1++;
        ptr2 += (*n) * datum_size;  //注意这里要乘以datum_size!!!, 因为void*,编译器是不知道一个元素大小的！！！
    }


    /*  进程p-1读取数据，并发送给相应进程*/
    int x;
    MPI_Status status;
    if(id == p-1) {
        for(int i = 0; i < p-1; ++i) {
            x = fread(*storage, datum_size, (*n) * BLOCK_SIZE(i, p, *m), infileptr);
            MPI_Send(*storage, (*n) * BLOCK_SIZE(i, p, *m), 
                    dtype, i, DATA_MSG, comm);
        }

        x = fread(*storage, datum_size, (*n) * local_rows, infileptr);
        fclose(infileptr);
    }
    else {
        
        MPI_Recv(*storage, (*n) * local_rows, dtype, p-1, 
                DATA_MSG, comm, &status);
    }
}


void read_col_striped_matrix(char* s, //文件名
    void*** subs, //输出，2D 子矩阵,存储的是行首的指针，int**
    void** storage, //输出，子矩阵的实际存储
    MPI_Datatype dtype, //输入，元素类型
    int* m, //输出，行， 注意这里m、n已经是ptr了
    int* n, //输出，列
    MPI_Comm comm //输入，通信域
) {
        
    // 思路：最后一个进程打开文件，复制到自己的内存，然后拷贝给其它进程
    int id, p;
    int datum_size;
    int local_cols;
    FILE *infileptr;
    void* buffer;  //读取文件的缓存
    int* send_count;
    int* send_disp;
    
    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &p);
    datum_size = get_size(dtype);

    /*  p-1进程打开文件
        读取行与列  */
    if (id == p-1) {
        infileptr = fopen(s, "r");
        if(infileptr == NULL) *m = 0;
        else {
            fread(m, sizeof(int), 1, infileptr);   //读取行数
            fread(n, sizeof(int), 1, infileptr);   //读取列数
        }
    }

    MPI_Bcast(m, 1, MPI_INT, p-1, MPI_COMM_WORLD);  //广播行数
    if(!(*m)) MPI_Abort(MPI_COMM_WORLD, OPEN_FILE_ERR);  //要在广播后，所有进程才能都根据m的值判断是否abort
    MPI_Bcast(n, 1, MPI_INT, p-1, MPI_COMM_WORLD);  //广播列数


    /*  分配进程id所需空间 */
    local_cols = BLOCK_SIZE(id, p, *n);  //当前进程拥有的列数

    *storage =  my_malloc(id, local_cols * (*m) * datum_size);  //当前进程包括的列所需的空间
    *subs =  my_malloc(id, (*m) * PTR_SIZE);
    
    void** ptr1 = *subs; //subs中的首地址
    void* ptr2 = *storage; //storage中的首地址
    for(int i = 0; i < (*m); ++i) {
        *ptr1 = (void*)ptr2;
        ptr1++;
        ptr2 += local_cols * datum_size;  //注意这里要乘以datum_size!!!, 因为void*,编译器是不知道一个元素大小的！！！
    }


    /*  进程p-1读取数据，并发送给相应进程, 注意这里的矩阵还是按行存储的*/
    int x;
    MPI_Status status;
    if(id == p-1) {
        buffer = my_malloc(id, *n * datum_size);
    }
    create_mixed_xfer_arrays(id, p, *n, (void*)&send_count, (void*)&send_disp);

    for(int i = 0; i < *m; ++i) {
        if(id == p-1) {
            fread(buffer, datum_size, *n, infileptr);  //按行读入
        }
            
        MPI_Scatterv(buffer, send_count, send_disp, dtype, 
                    (*storage) + i * local_cols * datum_size, local_cols, dtype, p-1, comm);  //分发，注意接收buffer首地址的变化
    }

    free(send_count);
    free(send_disp);
    if(id == p - 1) free(buffer);  //注意！buffer只有进程p-1才有

}



void read_checkerboard_matrix (
    char* s, //输入，文件名
    void*** subs, //输出，存储的是二维行首指针
    void** storage, //输出，实际存储
    MPI_Datatype dtype, //
    int* m, 
    int* n,
    MPI_Comm grid_comm  //输入，笛卡尔通信域
) {

    // 思路：进程0打开文件，先读取行数和列数，然后读取每一行，对应发给其它进程
    int grid_id, p;
    int datum_size;
    FILE *infileptr;

    int coords[2];
    int grid_size[2];
    int grid_period[2];
    int grid_coord[2];  //进程在cart通信域中的坐标
    int dest_id;
    int local_rows, local_cols;

    void** ptr1;
    void* ptr2;
    void* buffer;
    void* raddr;  //
    void* laddr;  //
    MPI_Status status;


    MPI_Comm_rank(grid_comm, &grid_id);
    MPI_Comm_size(grid_comm, &p);
    datum_size = get_size(dtype);

    //进程0，打开文件，读取行数和列数
    if(grid_id == 0) {
        infileptr = fopen(s, "r");
        if(infileptr == NULL) *m = 0;
        else {
            fread(m, sizeof(int), 1, infileptr);   //读取行数
            fread(n, sizeof(int), 1, infileptr);   //读取列数
        }
    }

    MPI_Bcast(m, 1, MPI_INT, 0, grid_comm);  //广播行数
    if(!(*m)) MPI_Abort(MPI_COMM_WORLD, OPEN_FILE_ERR);  //要在广播后，所有进程才能都根据m的值判断是否abort
    MPI_Bcast(n, 1, MPI_INT, 0, grid_comm);  //广播列数


    // 每个进程计算对应block的大小
    MPI_Cart_get(grid_comm, 2, grid_size, grid_period, grid_coord);
    
    local_rows = BLOCK_SIZE(grid_coord[0], grid_size[0], *m);
    local_cols = BLOCK_SIZE(grid_coord[1], grid_size[1], *n);

    // 分配空间
    *storage = my_malloc(grid_id, datum_size * local_cols * local_rows);
    *subs = (void**)my_malloc(grid_id, local_rows * PTR_SIZE);

    ptr1 = *subs;
    ptr2 = *storage;
    for(int i = 0; i < local_rows; ++i) {
        *ptr1 = ptr2;
        ptr1 += 1;
        ptr2 += local_cols * datum_size;
    }

    // 进程0 每次读取一行，然后分发给其它进程
    if(grid_id == 0) {
        buffer = my_malloc(grid_id, *n * datum_size);
    }

    for(int i = 0; i < grid_size[0]; ++i) {  //每个cart域中的行
        coords[0] = i; //cart域中的行坐标

        for(int j = 0; j < BLOCK_SIZE(i, grid_size[0], *m); ++j){  //cart域中每行对应的，矩阵中的行
            if(grid_id == 0) fread(buffer, datum_size, *n, infileptr);  //进程0负责读一行的数据

            for(int k = 0; k < grid_size[1]; ++k) {  //每个cart域中的列
                coords[1] = k;  //cart域中的列坐标
                MPI_Cart_rank(grid_comm, coords, &dest_id);  //确认在cart域中的进程号
                raddr = buffer + BLOCK_LOW(k, grid_size[1], *n) * datum_size; //进程[i,k]应该读取数据的首地址

                if(grid_id == 0) {
                    if(dest_id == 0) { //向自己拷贝数据
                        laddr = (*subs)[j];  //第j行首地址
                        memcpy(laddr, raddr, local_cols * datum_size);
                    }
                    else { //向其它进程发送
                        MPI_Send(raddr, BLOCK_SIZE(k, grid_size[1], *n), dtype, dest_id, 0, grid_comm);
                    }
                }
                else if(grid_id == dest_id) {  //只有dest_id进程需要接收数据
                    MPI_Recv((*subs)[j], local_cols, dtype, 0, 0, grid_comm, &status);
                }
            }
        }
    }

    if(grid_id == 0) {
        free(buffer);
        fclose(infileptr);
    }
}



// 读取向量并广播，最后所有进程拥有全部向量
void read_replicated_vector(
    char* s,  //输入，文件目录
    void** v,  //输出，向量
    MPI_Datatype dtype,   //输入，数据类型
    int *n,  //输出，向量长度
    MPI_Comm comm   //输入，通信域
)
{
    // p-1号进程读取向量，并广播给其它进程
    int id, p;
    int datum_size;
    FILE* infileptr;

    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &p);
    datum_size = get_size(dtype);

    if(id == p - 1) {
        infileptr = fopen(s, "r");
        if(infileptr == NULL) *n = 0;
        else fread(n, sizeof(int), 1, infileptr);  //先读取个数n
    }
    MPI_Bcast(n, 1, MPI_INT, p-1, MPI_COMM_WORLD);
    if(! *n) terminate(id, "Cannot open vector file\n");  //元素个数为0，终止各进程

    *v = my_malloc(id, (*n) * datum_size);
    if(id == p-1) {  //读取向量内容
        fread(*v, datum_size, *n, infileptr);
        fclose(infileptr);
    }
    MPI_Bcast(*v, *n, dtype, p-1, MPI_COMM_WORLD);
};


// 读取向量，最后每个进程拥有向量的部分元素
void read_block_vector(
    char* s, //输入，文件名
    void** v, //输出，向量
    MPI_Datatype dtype, //
    int *n, //输出，向量长度
    MPI_Comm comm  
){
    // p-1号进程读取向量，并向其它进程分发元素
    int id, p;
    int datum_size;
    FILE* infileptr;
    int *rec_count;
    int *rec_disp;
    int local_els;
    void* buffer;

    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &p);
    datum_size = get_size(dtype);

    //进程p-1 读取元素个数并广播
    if(id == p - 1) {
        infileptr = fopen(s, "r");
        if(infileptr == NULL) *n = 0;
        else fread(n, sizeof(int), 1, infileptr);  //先读取个数n
    }
    MPI_Bcast(n, 1, MPI_INT, p-1, MPI_COMM_WORLD);
    if(! *n) terminate(id, "Cannot open vector file\n");  //元素个数为0，终止各进程


    //进程p-1 读取全部向量并分发
    //书上写的是每个block读一次，用send和recv
    local_els = BLOCK_SIZE(id, p, *n);
    create_mixed_xfer_arrays(id, p, *n, (void*)&rec_count, (void*)&rec_disp);

    if(id == p - 1) {
        buffer = my_malloc(id, *n * datum_size);
        fread(buffer, datum_size, *n, infileptr);
    }
        
    *v = my_malloc(id, local_els * datum_size);
    MPI_Scatterv(buffer, rec_count, rec_disp, dtype, *v, 
            rec_count[id], dtype, p-1, comm);

    //释放空间
    free(rec_count);
    free(rec_disp);
    if(id == p - 1){
        free(buffer);
        fclose(infileptr);
    } 
}

void read_checkerboard_vector(
    char* s,  //输入，文件
    void** v, //输入，向量
    MPI_Datatype dtype,
    int* n,  //输出向量长度
    MPI_Comm grid_comm,
    MPI_Comm row_comm, 
    MPI_Comm col_comm
) {
    
    //cart域中的进程0，负责打开文件，
    //先读入长度，并广播给cart域中所有进程，
    //然后读入全部向量，广播给cart域中第一列
    //第一列负责scatter对应部分给cart域中这一行

    int grid_id, p;
    int datum_size;
    FILE* infileptr;

    void* buffer;
    int grid_size[2];
    int grid_period[2];
    int grid_coord[2];  //进程在cart通信域中的坐标
    int local_col;

    int* cnt, *disp;  //用于scatter的数组

    MPI_Comm_rank(grid_comm, &grid_id);
    MPI_Comm_size(grid_comm, &p);
    datum_size = get_size(dtype);
    MPI_Cart_get(grid_comm, 2, grid_size, grid_period, grid_coord);   

    //step1：进程0，打开文件，读取行数
    if(grid_id == 0) {
        infileptr = fopen(s, "r");
       
        if(infileptr == NULL) *n = 0;
        else fread(n, sizeof(int), 1, infileptr);  //先读取个数n
    }

    MPI_Bcast(n, 1, MPI_INT, 0, grid_comm);  //广播向量元素个数
    if(!(*n)) MPI_Abort(MPI_COMM_WORLD, OPEN_FILE_ERR);  //要在广播后，所有进程才能都根据m的值判断是否abort
    

    //step2：进程0，读取全部向量，并向第一列广播
    //(其实感觉第一行用send recv，然后每列广播，空间更少)
    if(grid_coord[1] == 0) {  //第0列都需要分配空间
    
        buffer = my_malloc(grid_id, *n * datum_size);

        if(grid_coord[0] == 0) {  //[0,0]负责读取
            fread(buffer, datum_size, *n, infileptr);
            fclose(infileptr);  //关文件
        }
        MPI_Bcast(buffer, *n, dtype, 0, col_comm);
    }

    //step3: 每行的进程分发对应向量元素给这一行的其它进程
    local_col = BLOCK_SIZE(grid_coord[1], grid_size[1], *n);
    *v = my_malloc(grid_id, local_col * datum_size);  //分配空间

    if(grid_size[1] > 1) {
        create_mixed_xfer_arrays(grid_coord[1], grid_size[1], *n, &cnt, &disp);
        MPI_Scatterv(buffer, cnt, disp, dtype, 
                    *v, cnt[grid_coord[1]], dtype, 0, row_comm);
    }
    else {  //如果cart域只有一列，不能按行做scatter，只能memcpy
        memcpy(*v, buffer, *n * datum_size);
    }

    if(grid_coord[1] == 0) {  //第一列释放buffer
        free(buffer);
    }
}


//******************OUTPUT************************


// 打印子矩阵
void print_submatrix(
    void** a,   //可以二维下标读取的矩阵
    MPI_Datatype dtype, 
    int rows, 
    int cols
){
    int i, j;
    for(i = 0; i < rows; ++i) {
        for(j = 0; j < cols; ++j) {
            if(dtype == MPI_DOUBLE) {
                printf("%6.3f ", ((double**)a)[i][j]);
            }
            else if(dtype == MPI_FLOAT) {
                printf("%6.3f ", ((float**)a)[i][j]);
            }
            else if(dtype == MPI_INT) {
                printf("%6d ", ((int**)a)[i][j]);
            }
        }
        putchar('\n');
    }
}

//打印向量
void print_subvector(
    void* a, // 输入，向量首地址
    MPI_Datatype dtype, // 输入，数据类型
    int n // 输入，向量长度
){
    for(int i = 0; i < n; ++i) {
        if(dtype == MPI_DOUBLE) {
            printf("%6.3f ", ((double*)a)[i]);
        }
        else if(dtype == MPI_FLOAT) {
            printf("%6.3f ", ((float*)a)[i]);
        }
        else if(dtype == MPI_INT) {
            printf("%6d ", ((int*)a)[i]);
        }
    }
}


// 打印二维按行分解的矩阵
void print_row_stripped_matrix(
    void** a, //输入，二维矩阵
    MPI_Datatype dtype,   //输入，矩阵元素类型
    int m,   //输入，行
    int n,   //输入，列
    MPI_Comm comm //输入，通信域
) {

    // 思路：进程0收到所有进程的数据，打印
    int id, p;
    int local_rows;
    int datum_size;
    int max_block_size;
    void** b;
    void* bstorage;
    int x;  //dummy variable, 感觉是用来同步的
    MPI_Status status;

    
    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &p);

    local_rows = BLOCK_SIZE(id, p, m);  //当前进程拥有的行

    if(!id) {
        print_submatrix(a, dtype, local_rows, n);

        if(p > 1) {  //总进程数大于1
            datum_size = get_size(dtype);  //矩阵中元素的数据类型大小
            max_block_size = BLOCK_SIZE(p-1, p, m);  //一个进程中最多拥有的行数

            //给进程0分配足够的空间
            bstorage = my_malloc(0, max_block_size * n * datum_size);  
            b = (void**) my_malloc(0, max_block_size * PTR_SIZE);

            b[0] = bstorage;
            for(int i = 1; i < max_block_size; ++i) {
                b[i] = b[i-1] + n * datum_size;
            } //Quinn大佬又换了种指针转换的方法:)


            for(int i = 1; i < p; ++i) {
                MPI_Send(&x, 1, MPI_INT, i, PROMPT_MSG, comm);
                MPI_Recv(bstorage, n * BLOCK_SIZE(i, p, m), dtype, i, RESPONSE_MSG, comm, &status);
                print_submatrix(b, dtype, BLOCK_SIZE(i, p, m), n);
            }
            free(b);
            free(bstorage);
        }
        putchar('\n');
    }
    else {
        MPI_Recv(&x, 1, MPI_INT, 0, PROMPT_MSG, comm, &status);
        MPI_Send(*a, local_rows * n, dtype, 0, RESPONSE_MSG, comm);
    }

}


// 打印按列分解的矩阵
void print_col_striped_matrix(
    void** a, //输入，二维矩阵
    MPI_Datatype dtype,   //输入，矩阵元素类型
    int m,   //输入，行
    int n,   //输入，列
    MPI_Comm comm //输入，通信域
) {
    //统一将每行发送给进程0， 由进程0打印输出
    int id, p;
    int datum_size;
    int* rec_count;
    int* rec_disp;
    void* buffer;

    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &p);
    datum_size = get_size(dtype);

    create_mixed_xfer_arrays(id, p, n, (void*)&rec_count, (void*)&rec_disp);

    if(!id) buffer = my_malloc(id, n * datum_size);  //给进程0分配空间

    //按每一行，收集所有进程的数据到进程0
    for(int i = 0; i < m; ++i) {
        MPI_Gatherv(a[i], rec_count[id], dtype, buffer + rec_disp[id], 
                    rec_count, rec_disp, dtype, 0, comm);
        //只是进程0负责输出
        if(!id) {
            print_subvector(buffer, dtype, n);
            putchar('\n');
        }
    }

    free(rec_count);
    free(rec_disp);
    if(!id){
        free(buffer);
        putchar('\n');
    } 
}


//打印棋盘格矩阵
void print_checkerboard_matrix (
    void** a, //二维矩阵首地址
    MPI_Datatype dtype, 
    int m,   //行
    int n,   //列
    MPI_Comm grid_comm 
) {
    // 思路：进程0读取每行然后打印
    int grid_id, p;
    int datum_size;
    FILE *infileptr;

    int coords[2];
    int grid_size[2];
    int grid_period[2];
    int grid_coord[2];  //进程在cart通信域中的坐标
    int src_id;
    int local_rows, local_cols;
    int els;

    void** ptr1;
    void* ptr2;
    void* buffer;
    void* raddr;  //
    void* laddr;  //
    int prompt;
    MPI_Status status;
    
    MPI_Comm_rank(grid_comm, &grid_id);
    MPI_Comm_size(grid_comm, &p);
    datum_size = get_size(dtype);

    MPI_Cart_get(grid_comm, 2, grid_size, grid_period, grid_coord);
    local_cols = BLOCK_SIZE(grid_coord[1], grid_size[1], n);

    if(!grid_id) {
        buffer = malloc(n * datum_size);  
        free(buffer);      
    }

    for(int i = 0; i < grid_size[0]; ++i) {  //cart域中的每行
        coords[0] = i;

        for(int j = 0; j < BLOCK_SIZE(i, grid_size[0], m); ++j) { //cart域中该行的每一数据行

            for(int k = 0; k < grid_size[1]; ++k) { //cart域中的每列
                coords[1] = k;
                MPI_Cart_rank(grid_comm, coords, &src_id);
                els = BLOCK_SIZE(k, grid_size[1], n);
                laddr = buffer + BLOCK_LOW(k, grid_size[1], n) * datum_size;

                if(!grid_id) {  //进程0负责收集该行的数据
                    if(src_id == 0) {
                        memcpy(laddr, a[j], els * datum_size);
                    }
                    else {
                        //MPI_Send(&prompt, 1, MPI_INT, src_id, PROMPT_MSG, grid_comm);
                        MPI_Recv(laddr, els, dtype, src_id, RESPONSE_MSG, grid_comm, &status);
                    }
                }
                else if(src_id == grid_id){
                    //MPI_Recv(&prompt, 1, MPI_INT, 0, PROMPT_MSG, grid_comm, &status);
                    MPI_Send(a[j], els, dtype, 0, RESPONSE_MSG, grid_comm);
                }
            }
    
            if(!grid_id) {
                print_subvector(buffer, dtype, n);
                putchar('\n');
            }
        }
    }

    if(!grid_id) {
        
        free(buffer);  // 这一行会引起double free or corruption 不懂
        putchar('\n');
    }
    
}



// 打印向量，每个进程拥有向量的部分
void print_block_vector(
    void* v, // 输入
    MPI_Datatype dtype, 
    int n,
    MPI_Comm comm
) {
    //思路：通信域中的0号向量用来单点接收其它进程的元素，并打印
    int id, p;
    int datum_size;
    void *tmp;
    int prompt;
    MPI_Status status;

    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &p);
    datum_size = get_size(dtype);

    if(!id) {
        print_subvector(v, dtype, BLOCK_SIZE(id, p, n));
        if(p > 1) {
            tmp = my_malloc(id, BLOCK_SIZE(p-1, p, n) * datum_size);
            for(int i = 1; i < p; ++i) {
                MPI_Send(&prompt, 1, MPI_INT, i, PROMPT_MSG, comm);
                MPI_Recv(tmp, BLOCK_SIZE(i, p, n), dtype, i, RESPONSE_MSG, comm, &status);                
                print_subvector(tmp, dtype, BLOCK_SIZE(i, p, n));
            }
            free(tmp);
        }
        printf("\n\n");
    }
    else {
        MPI_Recv(&prompt, 1, MPI_INT, 0, PROMPT_MSG, comm, &status);
        MPI_Send(v, BLOCK_SIZE(id, p, n), dtype, 0, RESPONSE_MSG, comm);
    }
}


// 打印广播的向量，左右进程都有全部向量副本
void print_replicated_vector(
    void* v,  // 输入，向量首地址
    MPI_Datatype dtype, // 输入，元素类型
    int n, // 输入，向量长度
    MPI_Comm comm  // 输入，通信域
){
    int id;
    MPI_Comm_rank(comm, &id);
    if(id == 0) {
        print_subvector(v, dtype, n);
        printf("\n\n");
    }
}

void print_checkerboard_vector(
    void* v,  //输入，向量首地址
    MPI_Datatype dtype,
    int n,
    MPI_Comm grid_comm,
    MPI_Comm row_comm
) {
    int grid_size[2];
    int grid_period[2];
    int coord[2];

    //主要是用来验证读入是否正确吧
    MPI_Cart_get(grid_comm, 2, grid_size, grid_period, coord);
    if(coord[0] == 0) 
        print_block_vector(v, dtype, n, row_comm);
}