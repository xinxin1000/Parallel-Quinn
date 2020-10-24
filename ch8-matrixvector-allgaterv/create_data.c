#include <stdio.h>
#include <stdlib.h>

typedef double dtype;

int main(){
    // 将buf存成二进制文件
    FILE *fout, *fin;
    int m = 5, n = 6;
    fout = fopen("in-matrix", "wb");
    fwrite(&m, sizeof(int), 1, fout);
    fwrite(&n, sizeof(int), 1, fout);
    dtype buf[30] = {
               2, 1, 3, 4, 0, 1,
               5, -1, 2, -2, 4, 1,
               0, 3, 4, 1, 2, 1,
               2, 3, 1, -3, 0, 1,
               1, 1, 1, 1, 1, 1};
    fwrite(buf, sizeof(dtype), m * n, fout);
    fclose(fout);

    fout = fopen("in-vector", "wb");
    fwrite(&n, sizeof(int), 1, fout);
    double vec[6] = {3, 1, 4, 0, 3, 1};
    fwrite(vec, sizeof(dtype), 6, fout);
    fclose(fout);

    //////////////////////////////////////////////
    // 读取二进制文件，输出
    fin = fopen("in-matrix", "r");
    dtype* mat = malloc(30 * sizeof(dtype));
    fread(&m, sizeof(int), 1, fin);   //读取行数
    fread(&n, sizeof(int), 1, fin);   //读取列数
    fread(mat, sizeof(dtype), m * n, fin);
    for(int i = 0; i < m * n; ++i) {
        printf("%.3f ", mat[i]);
    }
    putchar('\n');
    fclose(fin);

    fin = fopen("in-vector", "r");
    fread(&n, sizeof(int), 1, fin);
    printf("%d\n", n);
    dtype* vecin = malloc(sizeof(double) * n);
    fread(vecin, sizeof(dtype), n, fin);
    for(int i = 0; i < n; ++i) {
        printf("%.3f ", vecin[i]);
    }
    putchar('\n');
    fclose(fin);
    return 0;
}