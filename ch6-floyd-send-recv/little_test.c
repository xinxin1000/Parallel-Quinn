#include <stdio.h>

int main(int argc, char *argv[]) {
    FILE *infile, *outfile;
    int buf[18] = {4,4,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    outfile = fopen("in", "wb");
    fwrite(buf, sizeof(int), 18, outfile);
    fclose(outfile);
    return 0;
}