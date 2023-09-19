#include "common.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define TIMER_FLAGS_ALLOC 1 //已配置状态。
#define TIMER_FLAGS_USING 2 //定时器运转中。

struct TimerControl timerctl;

//初始化定时器中断100Hz。
void init_pit(void)
{
    /* 定时器中断频率=定时器主频1.19318MHz/设定的数值。
    设定的数值=1.19318MHz/100Hz=11931.8≈11932=0x2e9c */
    io_out8(PIT_CTRL, 0X34); //
    io_out8(PIT_CNT0, 0x9c); //中断周期的低8位。
    io_out8(PIT_CNT0, 0x2e); //中断周期的高8位。
    timerctl.count = 0;
    task_timer = &timerctl.timers0[0];
    cursor_timer = &timerctl.timers0[1];
    return;
}

void timer_set_fifo(struct Timer *timer, struct Fifo *fifo, int data)
{
    timer->fifo = fifo;
    timer->data = data;
}

//设置定时器timer再过timeout个时钟中断后超时。
void timer_set_time(struct Timer *timer, unsigned int timeout)
{
    timer->timeout = timeout + timerctl.count;
}

void inthandler20(void)
{
    io_out8(PIC0_OCW2, 0x60); //把IRQ0信号接受完了的信息通知给PIC。
    timerctl.count++;
    if (timerctl.count >= task_timer->timeout)
    {
        task_switch();
    }
    if (timerctl.count >= cursor_timer->timeout)
    {
        fifo_put(cursor_timer->fifo, cursor_timer->data);
        if (cursor_timer->data == 0)
        {
            timer_set_fifo(cursor_timer, &sys_fifo, 1);
        }
        else
        {
            timer_set_fifo(cursor_timer, &sys_fifo, 0);
        }
        timer_set_time(cursor_timer, 50);
    }
}