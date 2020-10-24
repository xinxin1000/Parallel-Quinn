#include <stdio.h>
#include <stdlib.h>

int main(){
    FILE *infileptr, *outfileptr;

    int m, n;
    int *buf;

    //////////////////////////////////////////////
    // 读取文本文件
    infileptr = fopen("in.txt", "r");
    fscanf(infileptr, "%d %d", &m, &n);

    buf = malloc(sizeof(int) * m * n);
    for(int i = 0; i < m * n; ++i) {
        fscanf(infileptr, "%d", &buf[i]);
    }

    fclose(infileptr);
    

    //////////////////////////////////////////////
    // 将buf存成二进制文件
    outfileptr = fopen("in", "wb");

    fwrite(&m, sizeof(int), 1, outfileptr);
    fwrite(&n, sizeof(int), 1, outfileptr);
    fwrite(buf, sizeof(int), m * n, outfileptr);

    fclose(outfileptr);
    free(buf);

    //////////////////////////////////////////////
    // 读取二进制文件，输出
    infileptr = fopen("in", "r");
    fread(&m, sizeof(int), 1, infileptr);   //读取行数
    fread(&n, sizeof(int), 1, infileptr);   //读取列数

    buf = malloc(sizeof(int) * m * n);
    fread(buf, sizeof(int), m * n, infileptr);

    printf("%d %d\n", m, n);
    for(int i = 0; i < m * n; ++i) {
        printf("%d ", buf[i]);
    }
    putchar('\n');


    fclose(infileptr);
    free(buf);
    return 0;
}