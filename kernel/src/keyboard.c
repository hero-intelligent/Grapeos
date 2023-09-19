#include "common.h"

#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60 //向键盘发送配置命令
#define KBC_MODE 0x47          //正常开启键盘和鼠标中断

// struct Fifo *keyfifo;
int keydata0;
int key_shift = 0; //Shift键是否按下

//没有按下Shift键时的键盘表。
char keytable0[0x80] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x08, 0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0x0a, 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0x5c, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5c, 0, 0};

//按下Shift键时键盘表。
char keytable1[0x80] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x08, 0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0x0a, 0, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, '_', 0, 0, 0, 0, 0, 0, 0, 0, 0, '|', 0, 0};

/* 等待键盘控制电路准备完毕 */
void wait_KBC_sendready(void)
{
    for (;;)
    {
        if ((io_in8(PORT_KEYCMDSTA) & KEYSTA_SEND_NOTREADY) == 0)
        { //从端口0x0064读取的数据，如果倒数第2位是0，表示键盘控制电路可以接受CPU指令了。
            return;
        }
    }
}

/* 初始化键盘控制电路 */
void init_keyboard(int data0)
{
    keydata0 = data0;
    //键盘控制器的初始化。
    wait_KBC_sendready();
    io_out8(PORT_KEYCMDSTA, KEYCMD_WRITE_MODE); //先向0x64端口发送0x60命令，表示向键盘发送配置命令。
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE); //后向0x60端口发送具体的配置值，0x47表示正常开启键盘和鼠标中断。
    return;
}

/* 来自PS/2键盘的中断 */
void inthandler21(void)
{
    io_out8(PIC0_OCW2, 0x61); /* 通知PIC"IRQ-01已经受理完毕" */
    int data = io_in8(PORT_KEYDAT);
    fifo_put(&sys_fifo, data + keydata0);
}

/**通过键盘码获取对应的字符
 * key_code 键盘码（小于0x80）
 * is_shift 是否同时按下了shift键
 * 返回对应的字符
 */
char get_key_char(int key_code, int is_shift)
{
    if (is_shift == 0)
    {
        return keytable0[key_code];
    }
    else
    {
        return keytable1[key_code];
    }
}

/**通过键盘码获取对应的字符
 * key_code 键盘码（小于0x80）
 * 返回对应的字符或0
 */
char keyboark_decode(int key_code)
{
    key_code -= keydata0;
    if (key_code == 0x2a) //左Shift按下
    {
        key_shift |= 1;
    }
    else if (key_code == 0x36) //右Shift按下（在QEMU中，右Shift没有中断，VirtulBox中可以。）
    {
        key_shift |= 2;
    }
    else if (key_code == 0xaa) //左Shift抬起
    {
        key_shift &= ~1;
    }
    else if (key_code == 0xb6) //右Shift抬起（在QEMU中，右Shift没有中断，VirtulBox中可以。）
    {
        key_shift &= ~2;
    }
    else if (key_code < 0x80) //当作一般可显示字符处理
    {
        return get_key_char(key_code, key_shift);
    }
    return 0;
}