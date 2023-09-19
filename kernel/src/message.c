#include "common.h"

/**获取操作系统传递来的消息
 * is_wait 是否等待。0：没有系统消息时返回-1，不休眠。1：休眠直到有系统消息。
 * 返回系统消息。
 */
int get_sys_msg(int is_wait)
{
    struct Task *task = task_now();
    struct Fifo *fifo = &task->fifo;
    for (;;)
    {
        asm_cli();
        if (fifo_length(fifo) == 0) //没有消息
        {
            asm_sti();
            if (is_wait)
            {
                task_sleep(task);
            }
            else
            {
                return -1;
            }
        }
        else
        {
            int data = fifo_get(fifo);
            asm_sti();
            return data;
        }
    }
}