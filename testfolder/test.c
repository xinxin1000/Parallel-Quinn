#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

//不太熟悉c的void*,练下手

void f(void*** subs, void** storage){
    *subs = malloc(3 * sizeof(int*));
    *storage = malloc(12 * sizeof(int));

    int* tmpptr = (int*)(*storage);
    for(int i = 0; i < 12; ++i) {
        *(tmpptr + i) = i; 
    }
    // *((int*)*storage) = 0;
    // *((int*)*storage + 1) = 1;
    // *((int*)*storage + 2) = 2;
    // *((int*)*storage + 3) = 3;
    // *((int*)*storage + 4) = 4;
    // *((int*)*storage + 5) = 5;

    // **subs = *storage;
    // *(*subs+1) = *storage + 8;
    // *(*subs+2) = *storage + 16;

    void** ptr1 = *subs;
    void* ptr2 = *storage;

    for(int i = 0; i < 3; ++i){  //3行2列
        *ptr1 = ptr2;
        printf("---%p, %p\n", ptr1, ptr1+1);
        ptr1++;
        printf("---%p, %p, %p\n", ptr2, ptr2+4, ptr2 + 4* (sizeof(int)));
        ptr2 += 4 * sizeof(int);
        

    }
}

int main(int argc, char* argv[]) {
    
    int** subs;
    int* storage;
    f(&subs, &storage);
    
    
    for(int i = 0; i < 12; ++i)
        printf("%d ", storage[i]);
        puts("");
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 4; ++j) {
            printf("%d ", subs[i][j]);
        }
    }
    puts("");


    MPI_Init(&argc, &argv);
    int id;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    printf("this is process %d\n", id);
    MPI_Finalize();
    return 0;
}
