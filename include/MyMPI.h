#include <mpi.h>
//*****************MACROS**********************************

#define DATA_MSG        0
#define PROMPT_MSG      1
#define RESPONSE_MSG    2

#define OPEN_FILE_ERR  -1
#define MALLOC_ERR     -2
#define TYPE_ERR       -3
#define DIMENSION_ERR  -4

#define BLOCK_LOW(id, p, n) ((id) * (n) / (p))  //id: 进程号，n：筛选的数字个数，p：进程总数； 计算进程分到的数字最小值（起点0） !#define中所有的变量最好都带括号。。。
#define BLOCK_HIGH(id, p, n) ((id + 1) * (n) / (p) - 1)   //进程分到的数字最大值
#define BLOCK_SIZE(id, p, n) (BLOCK_LOW((id) + 1, p, n) - BLOCK_LOW(id, p, n))
#define BLOCK_OWNER(index, p, n) (((index + 1) * (p) - 1) / n)  //序号index 的数字被分到哪个进程

#define MIN(a, b) ((a < b)? a : b)
#define PTR_SIZE (sizeof(void*))



//**************MISCELLANEOUS FUNCTIONS*******************
int get_size(MPI_Datatype);
void terminate(int, char*);
void* my_malloc(int, int);


//************DATA DISTRIBUTION FUNCTIONS*****************
void create_mixed_xfer_arrays(int id, int p, int n, int** count, int** disp);
void create_uniform_xfer_arrays(int id, int p, int n, int** count, int** disp);
void replicate_block_vector(void* ablock, int n, void* agre, MPI_Datatype dtype, MPI_Comm comm);


//**************INPUT FUNCTIONS***************************
void read_row_stripped_matrix(char*, void***, void**, MPI_Datatype, int*, int*, MPI_Comm);
void read_col_striped_matrix(char* s, void*** subs, void** storage, MPI_Datatype dtype, int* m, int* n, MPI_Comm comm);
void read_checkerboard_matrix(char* s, void*** subs, void** storage, MPI_Datatype dtype, int* m, int* n, MPI_Comm grid_comm);
void read_replicated_vector(char*, void**, MPI_Datatype, int*, MPI_Comm);
void read_block_vector(char* s, void** v, MPI_Datatype dtype, int* n, MPI_Comm comm);
void read_checkerboard_vector(char* s, void** v, MPI_Datatype dtype, int* n, MPI_Comm grid_comm, MPI_Comm row_comm, MPI_Comm col_comm);

//**************OUTPUT FUNCTIONS**************************
void print_submatrx(void**, MPI_Datatype, int, int);
void print_subvector(void*, MPI_Datatype dtype, int);
void print_row_stripped_matrix(void**, MPI_Datatype, int, int, MPI_Comm);
void print_col_striped_matrix(void** a, MPI_Datatype dtype, int m, int n, MPI_Comm comm);
void print_checkerboard_matrix ( void** a, MPI_Datatype dtype, int m, int n, MPI_Comm grid_comm);
void print_replicated_vector(void*, MPI_Datatype, int, MPI_Comm);
void print_block_vector(void* v, MPI_Datatype dtype, int n, MPI_Comm comm);
void print_checkerboard_vector(void* v, MPI_Datatype dtype, int n, MPI_Comm grid_comm, MPI_Comm row_comm);