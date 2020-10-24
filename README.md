# Parallel-Quinn

Michael J. Quinn的书，**MPI与OpenMP并行程序设计C语言版**， 按章节整理的代码。      

代码基本和书上相同，按自己理解修改了一些小问题，让例子都可以跑通。  

环境我装的是mpich3。

ch4和ch5运行命令
```shell
mpicc -o 可执行文件名  .c文件名
mpirun -np 进程数 可执行文件名
```

ch6开始，需要用到自己写的库，放在了include目录下。所以为ch6和ch8写了makefile，以及运行时的命令放在run.sh里面了。因为输入数据是二进制格式，所以先写了一个create_data.c,用这个造矩阵数据，再执行矩阵向量乘。整体流程:

```shell
gcc -o create_data create_data.c
./create_data

make
bash run.sh
```

(悄悄的说)
* 为防失忆，可执行文件就没放到.gitignore里>.<
* .vscode 主要是c_cpp_properties.json配的includePath比较有用，可以自动提示mpi的函数参数。tasks.json大概可以build，但是launch.json的gdb暂时没有在mpi下配成功。