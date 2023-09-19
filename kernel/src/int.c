#include "common.h"

#define IDT_DESC_CNT 0x31
#define AR_INTGATE32_K 0x8e // 内核中断门属性
#define AR_INTGATE32_U 0xee // 用户中断门属性

static int eflags; // 暂存eflags

// 中断门描述符
struct GATE_DESCRIPTOR
{
    short offset_low;  // 中断处理程序在目标代码段内的偏移量的15~0位
    short selector;    // 中断处理程序目标代码段描述符选择子
    char dw_count;     // 一般8位全置零
    char access_right; // 位7为P，位6~5为DPL，位4为S，位3~0为TYPE。
    short offset_high; // 中断处理程序在目标代码段内的偏移量的31~16位
};

struct GATE_DESCRIPTOR idt[IDT_DESC_CNT]; // 全局唯一

// 禁止外部可屏蔽中断
void disable_interrupt()
{
    eflags = get_eflags();
    if (eflags & 0x200)
    {
        asm_cli();
    }
}

// 恢复外部可屏蔽中断（disable_interrupt的后续操作，与disable_interrupt成对使用）
void recover_interrupt()
{
    if (eflags & 0x200)
    {
        asm_sti();
    }
}

// 设置门描述符
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, char ar)
{
    gd->offset_low = offset & 0xffff;
    gd->selector = selector;
    gd->dw_count = 0;
    gd->access_right = ar;
    gd->offset_high = (offset >> 16) & 0xffff;
}

void init_idt(void)
{
    /* IDT设置 */
    set_gatedesc(idt + 0x20, (int)asm_inthandler20, SELECTOR_K_CODE, AR_INTGATE32_K); // 设置时钟中断。
    set_gatedesc(idt + 0x21, (int)asm_inthandler21, SELECTOR_K_CODE, AR_INTGATE32_K); // 设置键盘中断。
    set_gatedesc(idt + 0x2c, (int)asm_inthandler2c, SELECTOR_K_CODE, AR_INTGATE32_K); // 设置鼠标中断。
    set_gatedesc(idt + 0x30, (int)asm_inthandler30, SELECTOR_K_CODE, AR_INTGATE32_U); // 设置系统调用。
    load_idtr(IDT_DESC_CNT * 8 - 1, (int)idt);
}

// 初始化可编程中断控制器。
void init_pic(void) /* PIC的初始化 */
{
    io_out8(PIC0_ICW1, 0x11); /* 边沿触发、级联模式 */
    io_out8(PIC0_ICW2, 0x20); /* 设置起始中断向量号，IRQ0-7由INT20-27接收 */
    io_out8(PIC0_ICW3, 0x04); /* 设置IR2接从片 */
    io_out8(PIC0_ICW4, 0x01); /* 全嵌套、非缓冲、非自动结束中断、x86模式 */
    io_out8(PIC1_ICW1, 0x11); /* 边沿触发、级联模式 */
    io_out8(PIC1_ICW2, 0x28); /* 设置起始中断向量号，IRQ8-15由INT28-2f接收 */
    io_out8(PIC1_ICW3, 0x02); /* 设置从片连接到主片的IR2引脚*/
    io_out8(PIC1_ICW4, 0x01); /* 全嵌套、非缓冲、非自动结束中断、x86模式 */
    io_out8(PIC0_IMR, 0xf8);  // 允许PIC1、键盘、时钟(11111000)
    io_out8(PIC1_IMR, 0xef);  // 允许鼠标中断(11101111)
    return;
}

/**通用中断处理程序（汇编中调用）
 * esp_user 用户栈指针（特权级变化时该值才有意义）
 * int_num 中断号
 * eax 系统调用功能号
 */
void inthandler_common(int *esp_user, int int_num, int eax)
{
    if (int_num == 0x20)
    {
        inthandler20();
    }
    else if (int_num == 0x21)
    {
        inthandler21();
    }
    else if (int_num == 0x2c)
    {
        inthandler2c();
    }
    else if (int_num == 0x30)
    {
        eax = syscall(esp_user, eax);
    }
}