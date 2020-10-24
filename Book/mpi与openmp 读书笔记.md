# mpi与openmp 读书笔记

并行计算机

* 多计算机（多台计算机通过网络互连，发送消息）
* 集中式多处理器（或对称多处理器SMP），所有cpu共享全局内存

互连网络拓扑：

​    二维、二叉树、超树型、蝶形网络、超立方体

弗林分类法

​    SISD,SIMD,MISD,MIMD

# 第3章 并行算法设计

## Foster设计方法论：

* 划分：域（数据）分解，功能分解，目的是找到可并行的原始任务
* 通信：确定原始任务间的通信模式
* 聚集：按照cpu数量合并原始任务等，目的1 减少通信，2 维持程序可扩展性
* 映射：将任务分配给cpu

## 例1：边界值问题

棒两端温度固定，初始值已知，求温度变化。

将棒分为n段，有限差分法推导得某点温度，与前一时刻，该位置以及左右相邻位置相关。

任务划分：不同位置，最多n-1个原始任务。也可以根据cpu个数合并

$m(\chi(n-1)/p+2\lambda), m为时间步，\chi为一个原始任务时间，p为cpu个数，\lambda为通信时间$
单位通信时间内可发送和接收消息，但只能和另一个任务通信。

## 例2：找最大值

见笔记本

# 第4章

mpi是分布式内存并行编程。

mpi的代码，**每个进程都有一份副本，**代码中可以用进程号来标示只需要特定进程运行的代码段。

```c++
MPI_Init  ;
MPI_Comm_rank  ;
MPI_Comm_size  ;
// do work
MPI_Finalize  ;
```

计时

```
MPI_Wtime();
MPI_Wtick();
```

归约 Reduce

```c++
MPI_Reduce(&local_cnt, &global_cnt, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);  
```

例程：电路



# 第5章 俄式素数筛选

思路： 进程0选出每轮的最小素数，然后广播给其它进程，所有进程标记该素数的倍数，最后未标记的数为素数

按**数据分解（按块）**，实现并行。
注意区分数据局部下标和全局下标。

由于n很大，sqrt(n) 在第一个数据块，所以只需要第一块找到下一个最小素数即可

mpi 广播Bcast：

```c++
MPI_Bcast(&prime, 1, MPI_INT, 0, MPI_COMM_WORLD);
```

复杂度

## 改进

* 删偶数
* 消除广播，每个进程多拥有一个3-sqrt(n) 数组，先串行找到≤sqrt(n) 的素数，然后各自标记即可
* 交换内外层循环，提高cache命中率

# 第6章 Floyd 

思路：每个进程分几行，去更新各自行内的距离

并行的**可行性**：
`a[i,k], a[k,j]` 在循环迭代k中不改变  
`a[i,j] 的更新 与 a[i,k], a[k,j]的更新没有依赖关系`

行的分配，沿用第5章的方法。

矩阵的**输入**，利用进程p-1输入每块的数据（对应不同进程的行），然后点对点发送给其它进程。
输出，所有进程点对点发送给进程0，进程0负责打印所有数据

数据的**存储**，一个int*真正用来存数据，一个用来存每行的首地址的int**

mpi点对点通信：

```c++
MPI_Send(viod *msg, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

MPI_Recv(void *msg, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
```

* tag用来说明消息的不同用途

* MPI_Send有消息缓冲区，缓冲区未清空前，**阻塞**进程；此时将消息复制到系统缓冲区，使MPI_Send 可以返回调用者，可以避免阻塞！

* ```
  status -> MPI_source
  status -> MPI_tag
  status -> MPI_ERROR
  ```

  当函数参数为MPI_ANY_SOURCE时，可以用status来确定到底是从哪个进程接收来的

小心**死锁**问题！

时间复杂度分析

# 第7章 性能分析

1. 加速比: 
   串行程序执行时间 / 并行程序执行时间
   $$  \psi(n, p) = \frac{\sigma(n) + \phi(n)}{\sigma(n) + \phi(n) / p + \kappa(n, p)} $$

   $\sigma 串行时间，\phi 可并行执行的时间， \kappa 通信开销$

2. 效率：
   加速比 / 处理器个数（p）
   处理器利用率的度量

3. **Amdahl定律**：
   利用串行部分在所有程序都串行时，的占比 **f**，得到加速比上界（忽略通信开销）
   展现 p 增加，执行时间减少。

4. Amdahl效应：
   加速比是规模 n 的增函数（通信开销一般比计算开销复杂度低）

5. **Gustafson-Barsis定律**：
   利用串行部分在串并行时的占比 **s**，得到加速比上界（忽略通信开销），又称为**比例加速比**。

6. **Karp-Flatt量度**：
   实验确定的串行比例：e  = （串行 + 通信） / （串行 + 总并行）
   得到加速比与 e 和 p 的关系

7. 等效指标
   串行算法时间，大于，所有处理器通信开销的和（可以求得n下界），才能使并行效率不下降。

8. 可扩展性：
   是在计算每个处理器所需要的内存大小（与p的关系）。

# 第8章 矩阵向量乘法

串行：相当于计算n次内积，总复杂度 `O(n^2)`

并行：域分解

* 按行分解
* 按列分解
* 棋盘式分解

函数：

```c++
MPI_Allgaterv(viud* send_buffer, int send_cnt, MPI_Datatpe send_type, void* receive_buffer, int* receive_cnt, int* receive_disp, MPI_Datatype receive_type, MPI_Comm comm)
```

