prom_row = matrixvector-row
prom_col = matrixvector-col
prom_block = matrixvector-block
source =  ../include/MyMPI.h ../include/MyMPI.c
source_row = matrixvector-row.c
source_col = matrixvector-col.c
source_block = matrixvector-block.c

all : $(prom_row) $(prom_col) $(prom_block)  #makefile默认只生成第一个可执行文件，为了编译多个文件，用伪执行函数

$(prom_row) : $(source) $(source_row)
	mpicc -o $(prom_row) -I ../include $(source) $(source_row)
$(prom_col) : $(source) $(source-col)
	mpicc -o $(prom_col) -I ../include $(source_col) $(source) 
$(prom_block) : $(source) $(source-block)
	mpicc -o $(prom_block) -I ../include $(source_block) $(source) 