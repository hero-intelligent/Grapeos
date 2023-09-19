#include "common.h"

#define FLAGS_OVERRUN 0x0001 //溢出

//初始化指定的fifo
void init_fifo(struct Fifo *fifo, int size, int *buf, struct Task *task)
{
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->next_r = 0;
    fifo->next_w = 0;
    fifo->task = task; //有数据写入时需要唤醒的任务。
    return;
}

//向缓存区中放入4字节数据。返回0表示正常，返回-1表示溢出。
int fifo_put(struct Fifo *fifo, int data)
{
    if (fifo->free == 0) //溢出
    {
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->next_w] = data;
    fifo->next_w++;
    if (fifo->next_w == fifo->size)
    {
        fifo->next_w = 0;
    }
    fifo->free--;
    if (fifo->task != 0)
    {
        if (fifo->task->status == 1) //如果任务处于休眠状态
        {
            task_add(fifo->task); //将任务唤醒
        }
    }
    return 0;
}

//从缓存区中取出4个字节的数据。如果缓存区为空返回-1。
int fifo_get(struct Fifo *fifo)
{
    if (fifo->free == fifo->size) //缓存区为空
    {
        return -1;
    }
    int data = fifo->buf[fifo->next_r];
    fifo->next_r++;
    if (fifo->next_r == fifo->size)
    {
        fifo->next_r = 0;
    }
    fifo->free++;
    return data;
}

//查看缓存区现有多少数据
int fifo_length(struct Fifo *fifo)
{
    return fifo->size - fifo->free;
}