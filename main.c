#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extmem.h"
#include "main.h"

Relation readTuple(unsigned char *blkPtr, int offset){
    Relation R;
    char str[5];
    for (int k = 0; k < 4; k++){
        str[k] = *(blkPtr + offset*8 + k);
    }
    R.X = atoi(str);
    for (int k = 0; k < 4; k++){
        str[k] = *(blkPtr + offset*8 + 4 + k);
    }
    R.Y = atoi(str);
    return R;
}

void itoa(int n, char s[]){
    char t[5];
    memset(t, '\0', sizeof(t));
    int i = 0, j = 0;
    do{
        t[i++] = '0' + n % 10;
        n = n / 10;
    }while(n != 0);
    while(i > 0){
        s[j++] = t[--i];
    }
}

void writeTuple(unsigned char *blkPtr, int offset, int X, int Y){
    char str[5];
    memset(str, '\0', sizeof(str));
    itoa(X, str);
    for (int k = 0; k < 4; k ++){
        *(blkPtr + offset*8 + k) = str[k];
    }
    memset(str, '\0', sizeof(str));
    itoa(Y, str);
    for (int k = 0; k < 4; k ++){
        *(blkPtr + offset*8 + 4 + k) = str[k];
    }
}

unsigned int nextAddr(unsigned char *blkPtr, int offset){
    char str[5];
    for (int k = 0; k < 4; k++){
        str[k] = *(blkPtr + offset*8 + k);
    }
    unsigned int addr = atoi(str);
    return addr;
}

void writeAddr(unsigned char *blkPtr, int offset, unsigned int addr){
    char str[5];
    memset(str, '\0', sizeof(str));
    itoa(addr, str);
    for (int k = 0; k < 4; k ++){
        *(blkPtr + offset*8 + k) = str[k];
    }
}

void clearBlock(unsigned char *blkPtr, Buffer *buf){
    for (int k = 0; k < buf->blkSize; k++){
        *(blkPtr+k) = '\0';      
    }
}

// 判断是否写满了一个buffer的block，若是，则写入磁盘，再重新申请一个空的buffer块
void judgeWriteBlock(int sum, unsigned char *tmp, unsigned int *cur_addr, Buffer *buf){
    if (sum % 7 == 0 && sum != 0){      
        writeBlockToDisk(tmp, *cur_addr, buf);
        printf("p.s. 结果写入磁盘%d\n", *cur_addr);
        tmp = getNewBlockInBuffer(buf);    
        clearBlock(tmp, buf);
        *cur_addr = (*cur_addr) + 1;
    } 
}

int findTupleFromBlock(unsigned int addr, int target, Buffer *buf, int *cnt, unsigned int *cur_addr, unsigned char *tmp){
    unsigned char *blk;
    if ((blk = readBlockFromDisk(addr, buf)) == NULL){
        perror("Reading Block Failed!\n");
        return -1;
    }
    printf("读入数据块 %d\n", addr);
    int i = 0;
    for (i = 0; i < 7; i ++){   
        // 遍历块中的每个元组
        Relation S = readTuple(blk, i);
        if (S.X == target){
            // 满足查询条件
            printf("(X=%d, Y=%d)\n", S.X, S.Y);
            judgeWriteBlock(*cnt, tmp, cur_addr, buf);
            // 将该元组写入到tmp的buffer块中
            writeTuple(tmp, *cnt % 7, S.X, S.Y); 
            *cnt = (*cnt) + 1;
        }
    } 
    unsigned int next = nextAddr(blk, i);
    freeBlockInBuffer(blk, buf);
    return next;
}

int linearSelection(){
    Buffer buf; /* A buffer */
    unsigned char *blk, *tmp; /* A pointer to a block */
    if (!initBuffer(520, 64, &buf)){
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    unsigned int addr = startAddr_S;
    unsigned int answer_addr = 100;
    unsigned int cur_addr = answer_addr;
    int cnt = 0;
    tmp = getNewBlockInBuffer(&buf);    // 存放查询结果
    
    while(addr <= endAddr_S){
        addr = findTupleFromBlock(addr, 50, &buf, &cnt, &cur_addr, tmp);
        if (addr == -1)
            return -1;
    }
    // 最后tmp中还剩的元组写入磁盘
    writeBlockToDisk(tmp, cur_addr, &buf);  
    if (answer_addr == cur_addr){
        printf("查询结束，结果写入磁盘: %d\n", answer_addr);
    }
    else{
        printf("查询结束，结果写入磁盘: %d.blk 到 %d.blk\n", answer_addr, cur_addr);
    }
    printf("满足条件的元组共%d个\n", cnt);
    printf("Buffer的IO读写共 %ld 次\n", buf.numIO);
    freeBuffer(&buf);
    return 0;
}

void innerSort(unsigned char *blkPtr[], int blkNum){
    int len = blkNum * 7;
    // 冒泡排序
    for (int i = 0; i < len - 1; i ++){
        for (int j = 0; j < len - i - 1; j ++){
            // 找到j元组编号对应的buffer块和块内偏移
            int blkidx_t = j / 7;       
            int offset_t = j % 7;
            int blkidx_w = blkidx_t;
            int offset_w = offset_t + 1;
            // 避开每个块的最后一个位置（后继地址）!!!
            if (offset_t == 6){
                blkidx_w = blkidx_t + 1;
                offset_w = 0;
            }
            Relation T = readTuple(blkPtr[blkidx_t], offset_t);
            Relation W = readTuple(blkPtr[blkidx_w], offset_w);
            if (T.X > W.X){
                // 在缓冲区交换元组
                writeTuple(blkPtr[blkidx_t], offset_t, W.X, W.Y);
                writeTuple(blkPtr[blkidx_w], offset_w, T.X, T.Y);
            }
        }
    }
}

int TPMMS(unsigned int startAddr, unsigned int endAddr, unsigned int midAddr, unsigned int outAddr){
    Buffer buf;
    unsigned char *blk[8];
    if (!initBuffer(520, 64, &buf)){
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    unsigned int readAddr = startAddr;
    unsigned int sliceAddr[5];
    int sliceCnt = 0;
    sliceAddr[sliceCnt] = midAddr;
    
    // Phase 1: Inner sort, each slice has at most 8 block ---------------------
    int midIdx = 0;
    while(readAddr <= endAddr){
        if ((blk[midIdx] = readBlockFromDisk(readAddr, &buf)) == NULL){
            perror("Reading Block Failed!\n");
            return -1;
        }
        readAddr = nextAddr(blk[midIdx], 7);
        if (midIdx == 7 || readAddr > endAddr){       // idx=7意味着写完8个buffer块
            innerSort(blk, midIdx+1);                 // 对buffer所有元组进行排序
            for (int i = 0; i <= midIdx; i ++){
                unsigned int a = sliceAddr[sliceCnt]+i;
                if (i < midIdx)                       // 写入后继磁盘块地址
                    writeAddr(blk[i], 7, a+1);          
                else                                // 该slice末尾块的后继地址写0
                    writeAddr(blk[i], 7, 0);
                // -----------------Debug-------------------
                // for (int j = 0; j <= 7; j ++){
                //     Relation T = readTuple(blk[i],j);
                //     printf("(%d, %d)\t", T.X, T.Y);
                // }
                // printf("\n");
                // ------------------------------------------
                writeBlockToDisk(blk[i], a, &buf);  // 保存中间排序结果到磁盘
            }
            sliceAddr[sliceCnt+1] = sliceAddr[sliceCnt] + midIdx + 1;
            sliceCnt ++;                 
            midIdx = 0;
        }
        else
            midIdx ++;
    }
    
    // Phase 2: Outer merge sort---------------------------------------------
    unsigned char *merge = getNewBlockInBuffer(&buf);
    unsigned char *output = getNewBlockInBuffer(&buf);
    clearBlock(merge, &buf);
    clearBlock(output, &buf);
    int tupleIdx[5];
    memset(tupleIdx, 0, sizeof(tupleIdx));
    for (int i = 0; i < 8; i ++)
        clearBlock(blk[i], &buf);
    // 将每个slice的第一块写入buffer，并将其第一个元组写入merge
    for (int i = 0; i < sliceCnt; i ++){        
        readAddr = sliceAddr[i];
        if ((blk[i] = readBlockFromDisk(readAddr, &buf)) == NULL){
            perror("Reading Block Failed!\n");
            return -1;
        }
        Relation T = readTuple(blk[i], tupleIdx[i]);
        writeTuple(merge, i, T.X, T.Y);
        // -------Debug---------
        // Relation W = readTuple(merge,i);
        // printf("(%d, %d)\t", W.X, W.Y);
    }

    int inf = 9999;
    int outIdx = 0;
    // Merge sort begin
    while (1){
        Relation min;
        int minLoc;
        min.X = inf;
        // 在merge块中找最小值
        for (int i = 0; i < sliceCnt; i ++){
            Relation T = readTuple(merge, i);
            if (T.X < min.X){
                min.X = T.X;
                min.Y = T.Y;
                minLoc = i;
            }
        }
        if (min.X == inf)   break;      // 没有最小值，排序完成
        writeTuple(output, outIdx++, min.X, min.Y);
        if (outIdx == 7){               // output的buffer块写满了，需写入输出磁盘
            writeAddr(output, outIdx, outAddr+1);
            writeBlockToDisk(output, outAddr++, &buf);      // !!!output被重新置为可用了，需重新分配
            outIdx = 0;
            output = getNewBlockInBuffer(&buf);
            clearBlock(output, &buf);
        }
        if (tupleIdx[minLoc] == 6){     // 该最小值所在的buffer块已经没有下一个元组
            readAddr = nextAddr(blk[minLoc], 7);
            if (readAddr == 0)          // 对应slice的块全部加载完，写inf
                writeTuple(merge, minLoc, inf, inf);
            else {                      // 从对应的slice读取下一块磁盘文件
                tupleIdx[minLoc] = 0;
                // !!!先free掉当前blk的buffer再读，不然会导致buffer以为自己溢出了
                freeBlockInBuffer(blk[minLoc], &buf);
                if ((blk[minLoc] = readBlockFromDisk(readAddr, &buf)) == NULL){
                    perror("Reading Block Failed!\n");
                    return -1;
                }
                Relation T = readTuple(blk[minLoc], tupleIdx[minLoc]);
                writeTuple(merge, minLoc, T.X, T.Y);
            }
        }
        else{
            Relation T = readTuple(blk[minLoc], ++tupleIdx[minLoc]);
            writeTuple(merge, minLoc, T.X, T.Y);
        }
    }
    return 0;
}

int createIndexFile(unsigned int staAddr, unsigned int endAddr, unsigned int idxAddr){
    Buffer buf;
    if (!initBuffer(520, 64, &buf)){
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    unsigned char *blk;
    unsigned char *tmp = getNewBlockInBuffer(&buf);
    unsigned int readAddr = staAddr;
    unsigned int tmpAddr = idxAddr;
    int idx = 0;
    while(readAddr <= endAddr){
        if ((blk = readBlockFromDisk(readAddr, &buf)) == NULL){
            perror("Reading Block Failed!\n");
            return -1;
        }
        Relation T = readTuple(blk, 0);
        readAddr = nextAddr(blk, 7);
        writeTuple(tmp, idx++, T.X, (int)readAddr-1);
        if (idx == 7 || readAddr > endAddr){
            writeAddr(tmp, 7, tmpAddr+1);
            writeBlockToDisk(tmp, tmpAddr++, &buf);
            tmp = getNewBlockInBuffer(&buf);
            clearBlock(tmp, &buf);
            idx = 0;
        }
        freeBlockInBuffer(blk, &buf);
    }
    freeBlockInBuffer(tmp, &buf);
    return tmpAddr;
}

int indexSelection(unsigned int idxAddr, unsigned int idxEnd, int target){
    Buffer buf;
    if (!initBuffer(520, 64, &buf)){
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    unsigned int readAddr = idxAddr;
    unsigned char *blk, *tmp;
    unsigned int answer_addr = 120;
    unsigned int cur_addr = answer_addr;
    int cnt = 0;
    tmp = getNewBlockInBuffer(&buf);    // 存放查询结果
    
    int flag = 1;
    Relation T, P;
    T.X = T.Y = 0;
    while(readAddr <= idxEnd && flag){
        printf("读入索引块%d\n", readAddr);
        if ((blk = readBlockFromDisk(readAddr, &buf)) == NULL){
            perror("Reading Block Failed!\n");
            return -1;
        }
        int find = 0;
        // 遍历索引块中的记录
        for (int i = 0; i < 7; i ++){
            P.X = T.X;  P.Y = T.Y;
            T = readTuple(blk, i);
            if (T.X >= target && P.X <= target){
                // 前一个记录P的索引项满足条件，其指向的块可能有X=50的元组
                find = 1;
                unsigned int addr = (unsigned int) P.Y;
                if (findTupleFromBlock(addr, 50, &buf, &cnt, &cur_addr, tmp) == -1)
                    return -1;
            }
            else if (T.X > target && P.X > target){     // 全部找完，剩下的索引块不用找了
                writeBlockToDisk(tmp, cur_addr, &buf);  // 最后tmp中还剩的元组写入磁盘
                flag = 0;
                break;
            }
        }
        if (!find)  printf("没有满足条件的元组\n");
        readAddr = nextAddr(blk, 7);
        freeBlockInBuffer(blk, &buf);
    }
    if (answer_addr == cur_addr){
        printf("查询结束，结果写入磁盘: %d\n", answer_addr);
    }
    else{
        printf("查询结束，结果写入磁盘: %d.blk 到 %d.blk\n", answer_addr, cur_addr);
    }
    printf("满足条件的元组共%d个\n", cnt);
    printf("Buffer的IO读写共 %ld 次\n", buf.numIO);
    freeBuffer(&buf);
    return 0;
}

int joinR(unsigned int *idx_r, unsigned int end_r, Relation T, Buffer *buf, int *cnt, unsigned int *cur_addr, unsigned char *tmp){
    unsigned int idx = *idx_r, target = T.X;
    unsigned char *blk;
    int find = 0, end = 0;
    while(idx <= end_r){
        if ((blk = readBlockFromDisk(idx, buf)) == NULL){
            perror("Reading Block Failed!\n");
            return -1;
        }
        for (int i = 0; i < 7; i ++){
            Relation W = readTuple(blk, i);
            if (W.X == target){
                if (!find){         // 找到的第一个位置，保存所在blk编号
                    find = 1;
                    *idx_r = idx;
                }
                int sum = (*cnt) << 1;  // 结果元组的个数，是连接数的两倍
                judgeWriteBlock(sum, tmp, cur_addr, buf);
                writeTuple(tmp, sum % 7, T.X, T.Y);
                judgeWriteBlock(++sum, tmp, cur_addr, buf);
                writeTuple(tmp, sum % 7, W.X, W.Y);
                *cnt = (*cnt) + 1;
            }
            else if (find){         // find=1之后的非target说明找完了
                end = 1;
                break;
            }
            else continue;
        } 
        idx = nextAddr(blk, 7);
        freeBlockInBuffer(blk, buf);
        if ((find & end) == 1)
            break;
    }
    return 0;
}

int sortMergeJoin(unsigned int sta_R, unsigned int end_R, unsigned int sta_S, unsigned int end_S){
    Buffer buf;
    if (!initBuffer(520, 64, &buf)){
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    unsigned int idx_r = sta_R, idx_s = sta_S;
    unsigned char *blk_s, *tmp;
    unsigned int answer_addr = 501;
    unsigned int cur_addr = answer_addr;
    int cnt = 0;
    tmp = getNewBlockInBuffer(&buf);    // 存放查询结果

    while(idx_s <= end_S){
        if ((blk_s = readBlockFromDisk(idx_s, &buf)) == NULL){
            perror("Reading Block Failed!\n");
            return -1;
        }
        for (int i = 0; i < 7; i ++){
            Relation T = readTuple(blk_s, i);
            if (joinR(&idx_r, end_R, T, &buf, &cnt, &cur_addr, tmp) == -1)
                return -1;
        }
        idx_s = nextAddr(blk_s, 7);
        freeBlockInBuffer(blk_s, &buf);
    }
    writeBlockToDisk(tmp, cur_addr, &buf);
    printf("p.s. 结果写入磁盘%d\n\n", cur_addr);
    printf("共连接 %d 次\n", cnt);
    return 0;
}

int sortIntersection(unsigned int sta_R, unsigned int end_R, unsigned int sta_S, unsigned int end_S){
    Buffer buf;
    if (!initBuffer(520, 64, &buf)){
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    unsigned int idx_r = sta_R, idx_s = sta_S;
    unsigned char *blk_r, *blk_s, *tmp;
    unsigned int answer_addr = 601;
    unsigned int cur_addr = answer_addr;
    int cnt = 0;
    tmp = getNewBlockInBuffer(&buf);    // 存放查询结果

    while(idx_s <= end_S){
        if ((blk_s = readBlockFromDisk(idx_s, &buf)) == NULL){
            perror("Reading Block Failed!\n");
            return -1;
        }
        for (int i = 0; i < 7; i ++){
            Relation S = readTuple(blk_s, i);
            int find = 0, end = 0, stop = 0;
            unsigned int idx = idx_r;
            while(idx <= end_R){
                if ((blk_r = readBlockFromDisk(idx, &buf)) == NULL){
                    perror("Reading Block Failed!\n");
                    return -1;
                }
                for (int i = 0; i < 7; i ++){
                    Relation R = readTuple(blk_r, i);
                    if (R.X == S.X && !find){
                        find = 1;
                        idx_r = idx;
                    }
                    if (R.X == S.X && R.Y == S.Y){
                        end = 1;
                        judgeWriteBlock(cnt, tmp, &cur_addr, &buf);
                        writeTuple(tmp, cnt % 7, S.X, S.Y);
                        printf("(X=%d, Y=%d)\n", R.X, R.Y);
                        cnt ++;
                        break;
                    }
                    else if (R.X > S.X)
                        stop = 1;
                    else continue;
                } 
                idx = nextAddr(blk_r, 7);
                freeBlockInBuffer(blk_r, &buf);
                if ((find & end) || stop)
                    break;
            }
        }
        idx_s = nextAddr(blk_s, 7);
        freeBlockInBuffer(blk_s, &buf);
    }
    writeBlockToDisk(tmp, cur_addr, &buf);
    printf("p.s. 结果写入磁盘%d\n\n", cur_addr);
    printf("S和R的交集有 %d 个元组\n", cnt);
    return 0;
}

int main(){
    printf("--------------------------\n");
    printf("基于线性搜索的关系选择算法: S.C = 50\n");
    printf("--------------------------\n");
    if (linearSelection() < 0){
        printf("Error.\n");
        return -1;
    }
    printf("\n\n--------------------------\n");
    printf("两阶段多路归并排序算法（TPMMS）\n");
    printf("--------------------------\n");
    if (TPMMS(startAddr_R, endAddr_R, 201, 301) < 0 || TPMMS(startAddr_S, endAddr_S, 217, 317) < 0){
        printf("Error.\n");
        return -1;
    }
    else{
        printf("分组排序中间结果存放在201.blk到248.blk文件中\n");
        printf("排序后的结果已存放在磁盘301.blk到316.blk（关系R），317.blk到348.blk（关系S）");
    }

    printf("\n\n--------------------------\n");
    printf("基于索引的关系选择算法: S.C = 50\n");
    printf("--------------------------\n");
    unsigned int idxAddr = 401;     // 索引文件起始磁盘地址
    unsigned int idxEnd = createIndexFile(317, 348, idxAddr) - 1;
    printf("索引文件存在%d.blk到%d.blk中\n", idxAddr, idxEnd);
    if (indexSelection(idxAddr, idxEnd, 50) < 0){
        printf("Error.\n");
        return -1;
    }

    printf("\n\n--------------------------\n");
    printf("基于排序的连接操作算法\n");
    printf("--------------------------\n");
    if (sortMergeJoin(301, 316, 317, 348) < 0){
        printf("Error.\n");
        return -1;
    }
    
    printf("\n\n--------------------------\n");
    printf("基于排序的集合的交计算\n");
    printf("--------------------------\n");
    if (sortIntersection(301, 316, 317, 348) < 0){
        printf("Error.\n");
        return -1;
    }
    return 0;
}