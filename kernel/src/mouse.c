#include "common.h"

#define KEYCMD_SENDTO_MOUSE 0xd4 //向鼠标发送命令
#define MOUSECMD_ENABLE 0xf4     //允许鼠标设备发送数据包

int mousedata0;
int last_mouse_event;   //记录最近一次鼠标事件
int double_click_phase; //取值0~3，left_down1、left_up1、left_down2、left_up2，0表示初始未开始或归位状态。
unsigned int left_down_time1;
unsigned int left_down_time2;
int is_right_down;

//初始化鼠标（必须先初始化键盘才能初始化鼠标）
void init_mouse(int data0, struct MouseDecoder *mdec)
{
    mousedata0 = data0;
    //激活鼠标
    wait_KBC_sendready();
    io_out8(PORT_KEYCMDSTA, KEYCMD_SENDTO_MOUSE); //先向0x64端口发送0xd4命令，表示向鼠标发送命令。
    wait_KBC_sendready();
    //后向0x60端口发送具体的鼠标命令，0xf4表示允许鼠标设备发送数据包。顺利的话,键盘控制器会返送回ACK(0xfa)
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0;
}

/* PS/2鼠标中断 */
void inthandler2c(void)
{
    io_out8(PIC1_OCW2, 0x64); /* 通知PIC1 IRQ-12的受理已经完成 */
    io_out8(PIC0_OCW2, 0x62); /* 通知PIC0 IRQ-02的受理已经完成 */
    int data = io_in8(PORT_KEYDAT);
    fifo_put(&sys_fifo, data + mousedata0);
}

//返回1表示3个字节到齐了。
int mouse_decode(struct MouseDecoder *mdec, int data)
{
    data -= mousedata0;
    if (mdec->phase == 0) //等待鼠标的0xfa的状态
    {
        if (data == 0xfa) // 0xfa是鼠标接收到控制命令后的应答数据。
        {
            mdec->phase = 1;
        }
        return 0;
    }
    else if (mdec->phase == 1) //等待鼠标的第一字节
    {
        //如果第一字节正确，高4位在0~3范围内，低4位在8~F范围内。
        //正常不需要该判断，这里是为了以防万一。
        if ((data & 0xc8) == 0x08)
        {
            mdec->buf[0] = data;
            mdec->phase = 2;
        }
        return 0;
    }
    else if (mdec->phase == 2) //等待鼠标的第二字节
    {
        mdec->buf[1] = data;
        mdec->phase = 3;
        return 0;
    }
    else if (mdec->phase == 3) //等待鼠标的第三字节
    {
        mdec->buf[2] = data;
        mdec->phase = 1;
        mdec->btn = mdec->buf[0] & 0x07; // 00000111b bit0左键、bit1右键、bit2中键。
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        if ((mdec->buf[0] & 0x10) != 0) // x方向移动为负
        {
            mdec->x |= 0xffffff00; //鼠标实际返回的是9位数的补码，-256~255。
        }
        if ((mdec->buf[0] & 0x20) != 0) // y方向移动为负
        {
            mdec->y |= 0xffffff00;
        }
        mdec->y = -mdec->y; // 鼠标的y方向与屏幕相反

        if (mdec->btn == 0)
        {
            last_mouse_event = MOUSE_EVENT_MOVE;
        }

        if ((mdec->btn & 0x01) == 1) //按下左键
        {
            last_mouse_event = MOUSE_EVENT_LEFT_DOWN;
            if (double_click_phase == 0)
            {
                double_click_phase++;
                left_down_time1 = timerctl.count;
            }
            else if (double_click_phase == 2)
            {
                double_click_phase++;
                left_down_time2 = timerctl.count;
                int interval = left_down_time2 - left_down_time1;
                if (interval < 50 && mdec->x < 4 && mdec->y < 4)
                {
                    last_mouse_event = MOUSE_EVENT_DOUBLE_CLICK;
                }
                else
                {
                    double_click_phase = 1;
                    left_down_time1 = timerctl.count;
                }
            }
        }
        else if ((mdec->btn & 0x01) == 0 && (double_click_phase == 1 || double_click_phase == 3)) //抬起左键
        {
            last_mouse_event = MOUSE_EVENT_LEFT_UP;
            if (double_click_phase == 1)
            {
                double_click_phase++;
            }
            else if (double_click_phase == 3)
            {
                double_click_phase = 0;
            }
        }

        if ((mdec->btn & 0x02) == 2) //按下右键
        {
            last_mouse_event = MOUSE_EVENT_RIGHT_DOWN;
            is_right_down = 1;
        }
        else if ((mdec->btn & 0x02) == 0 && is_right_down == 1) //抬起右键
        {
            last_mouse_event = MOUSE_EVENT_RIGHT_UP;
            is_right_down = 0;
        }

        return 1;
    }
    return -1; //应该不可能到这里来
}