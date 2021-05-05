#include "extmem.h"
typedef struct relation
{
    unsigned int X;     
    unsigned int Y;
} Relation;

unsigned int startAddr_R = 1;
unsigned int startAddr_S = 17;

unsigned int endAddr_R = 16;
unsigned int endAddr_S = 48;

/**
 * 从缓存块读取下个块的地址（后继地址）
 */ 
unsigned int nextAddr(unsigned char *blkPtr, int offset);

/**
 * 从缓存块读一个元组
 */ 
Relation readTuple(unsigned char *blkPtr, int offset);

/**
 * 写一个元组到缓存块
 */ 
void writeTuple(unsigned char *blkPtr, int offset, int X, int Y);

/**
 * 整型int转换为字符
 */ 
void itoa(int n, char s[]);

/**
 * 写4个字节的地址
 */ 
void writeAddr(unsigned char *blkPtr, int offset, unsigned int addr);

/**
 * 清空一个缓存块
 */ 
void clearBlock(unsigned char *blkPtr, Buffer *buf);

/**
 * 判断是否写满了一个buffer的block，若是，则写入磁盘，再重新申请一个空的buffer块
 */ 
void judgeWriteBlock(int sum, unsigned char *tmp, unsigned int *cur_addr, Buffer *buf);

/**
 * 从block中找到与target相同的元组
 */ 
int findTupleFromBlock(unsigned int addr, int target, Buffer *buf, int *cnt, unsigned int *cur_addr, unsigned char *tmp);

/**
 * 基于线性搜索的关系选择算法
 * 基于ExtMem程序库，使用C语言实现线性搜索算法，选出S.C=50的元组，记录IO读写次数，并将选择结果存放在磁盘上。
 * （模拟实现 select S.C, S.D from S where S.C = 50）
 */
int linearSelection();

/**
 * TPMMS第一阶段内排序：对缓存块冒泡排序
 */
void innerSort(unsigned char *blkPtr[], int blkNum);

/**
 * 两阶段多路归并排序算法(TPMMS):利用内存缓冲区将关系R和S分别排序，并将排序后的结果存放在磁盘上。
 */ 
int TPMMS(unsigned int startAddr, unsigned int endAddr, unsigned int midAddr, unsigned int outAddr);

/**
 * 利用TPMMS中的排序结果为关系S建立索引文件，
 */
int createIndexFile(unsigned int staAddr, unsigned int endAddr, unsigned int idxAddr);

/**
 * 基于索引的关系选择算法: 利用索引文件选出S.C=50的元组，并将选择结果存放在磁盘上。
 * 记录IO读写次数，与(1)中的结果对比。(模拟实现 select S.C, S.D from S where S.C = 50 )
 */
int indexSelection(unsigned int idxAddr, unsigned int idxEnd, int target);

/**
 * 连接R元组
 */
int joinR(unsigned int *idx_r, unsigned int end_r, Relation T, Buffer *buf, int *cnt, unsigned int *cur_addr, unsigned char *tmp);

/**
 * 基于排序的连接操作算法(Sort-Merge-Join): 对关系S和R计算S.C连接R.A ，并统计连接次数，将连接结果存放在磁盘上。 
 * (模拟实现 select S.C, S.D, R.A, R.B from S inner join R on S.C = R.A)
 */ 
int sortMergeJoin(unsigned int sta_R, unsigned int end_R, unsigned int sta_S, unsigned int end_S);

/**
 * 基于排序的两趟扫描算法：实现S和R的集合交操作算法。将结果存放在磁盘上，并统计交操作后的元组个数。
 */ 
int sortIntersection(unsigned int sta_R, unsigned int end_R, unsigned int sta_S, unsigned int end_S);

