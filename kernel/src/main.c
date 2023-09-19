#include "common.h"

#define LOADER_SAVE_GRAPHICS_ADDRESS 0x1004 // loader中保存显存地址的地址
#define VARTUAL_GRAPHICS_ADDRESS 0xe0000000 // 虚拟显存地址

struct Fifo sys_fifo; // main task fifo

int main(void)
{
    struct Layer *layer_desktop = 0; // 桌面图层
    struct Layer *layer_mouse = 0;   // 鼠标图层
    struct Layer *layer = 0;         // 临时变量
    struct MouseDecoder mdec;
    int fifobuf[128] = {0};
    char key_char = 0;                            // 键盘码对应的字符
    int mx = 0, my = 0;                           // 鼠标坐标
    int mxs = 0, mys = 0;                         // 鼠标在图层内的坐标
    int focus_win_x_var = 0, focus_win_y_var = 0; // 窗口移动变化量
                                                  // 显存地址（QEMU:0xfd000000,Bochs:0xe0000000）
    unsigned int pgram = *((unsigned int *)LOADER_SAVE_GRAPHICS_ADDRESS);
    unsigned int vgram = VARTUAL_GRAPHICS_ADDRESS;
    init_page(pgram, vgram);
    init_gdt();
    init_idt();
    init_pic();
    init_pit();
    init_memory();
    init_task();
    init_fifo(&sys_fifo, 128, fifobuf, main_task);
    init_keyboard(256);
    init_mouse(512, &mdec);
    init_layer((int *)vgram, SCRNX, SCRNY);
    init_cursor();

    layer_desktop = layer_alloc();
    int *buf_desktop = (int *)kmalloc(SCRNX * SCRNY * 4);
    layer_set(layer_desktop, buf_desktop, SCRNX, SCRNY, -1);
    init_desktop(layer_desktop);
    layer_slide(layer_desktop, 0, 0);

    layer_updown(layer_desktop, 0);

    layer_mouse = layer_alloc();
    int *buf_mouse = (int *)kmalloc(16 * 16 * 4); // 鼠标宽高都是16
    layer_set(layer_mouse, buf_mouse, 16, 16, 0x123456);
    init_mouse_cursor(buf_mouse, layer_mouse->col_inv);
    mx = (SCRNX - 16) / 2; // 将鼠标画到屏幕中央。
    my = (SCRNY - 16) / 2;
    layer_slide(layer_mouse, mx, my);
    layer_updown(layer_mouse, 1);

    asm_sti();
    while (1)
    {
        asm_cli();
        int fifo_len = fifo_length(&sys_fifo);
        if (fifo_len == 0)
        {
            asm_sti();
            if ((focus_win_x_var != 0 || focus_win_y_var != 0) && focus_win != 0 && focus_win->type == 1)
            { // 空闲时移动窗口
                layer_slide(focus_win, focus_win->vx0 + focus_win_x_var, focus_win->vy0 + focus_win_y_var);
                focus_win_x_var = 0;
                focus_win_y_var = 0;
            }
            else
            {
                asm_hlt();
            }
        }
        else
        {
            int data = fifo_get(&sys_fifo);
            asm_sti();
            if (data == 0 || data == 1) // 光标闪烁
            {
                cursor_blink(focus_win, data);
            }
            else if (data == 2) // 释放等待释放的用户任务页目录
            {
                free_wait_free_user_page_dirs();
            }
            else if (256 <= data && data <= 511) // 键盘数据
            {
                key_char = keyboark_decode(data);
                if (key_char != 0 && focus_win != 0 && focus_win->type == 1)
                { // 将键盘数据发送给获得焦点的窗口。
                    fifo_put(&focus_win->task->fifo, key_char + 256);
                }
            }
            else if (512 <= data && data <= 767) // 鼠标数据。
            {
                if (mouse_decode(&mdec, data) == 1)
                {
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0)
                    {
                        mx = 0;
                    }
                    if (my < 0)
                    {
                        my = 0;
                    }
                    if (mx > SCRNX - 1)
                    {
                        mx = SCRNX - 1;
                    }
                    if (my > SCRNY - 1)
                    {
                        my = SCRNY - 1;
                    }
                    layer_slide(layer_mouse, mx, my);
                    if ((mdec.btn & 0x01) == 1) // 按下左键
                    {
                        if (focus_win->type != 3) // 有弹窗时只能先关闭弹窗才能做其它操作。
                        {
                            // 按照从上到下的顺序寻找鼠标点击到的图层
                            for (int j = layerctl->top - 1; j >= 0; j--)
                            {
                                layer = layerctl->layers[j];
                                mxs = mx - layer->vx0;
                                mys = my - layer->vy0;
                                if (0 <= mxs && mxs < layer->xsize && 0 <= mys && mys < layer->ysize)
                                {
                                    if (layer->buf[mys * layer->xsize + mxs] != layer->col_inv)
                                    { // 找到了点中的图层
                                        if (layer->type != 2 && layer != focus_win)
                                        { // 由不同的窗口获得了焦点
                                            focus_exchange(focus_win, layer);
                                            focus_win = layer;
                                        }
                                        // 如果点中的是右键菜单，这里不做任何处理，而是由各任务自己处理。
                                        break;
                                    }
                                }
                            }
                        }
                        mxs = mx - focus_win->vx0;
                        mys = my - focus_win->vy0;
                        if (focus_win == layer_desktop)
                        {
                            if (100 < mx && mx < 200 && 100 < my && my < 200 && last_mouse_event == MOUSE_EVENT_DOUBLE_CLICK)
                            { // 如果双击了我的电脑则打开资源管理器。
                                open_explorer();
                            }
                        }
                        else if (focus_win != 0 && (focus_win->type == 1 || focus_win->type == 3))
                        { // 弹窗按模态处理，不关闭则无法做其它操作。
                            if (focus_win->type != 3)
                            {
                                if (focus_win->height != layerctl->top - 1)
                                {
                                    // 将点击到的图层的高度设置为仅次于鼠标图层高度的第2高。
                                    layer_updown(focus_win, layerctl->top - 1);
                                }
                                if (0 <= mxs && mxs < focus_win->xsize - 20 && 0 <= mys && mys < 20)
                                { // 当点击窗口标题栏时进入窗口移动模式（为了性能，实际移动延迟到sys_fifo中没有数据时再执行。）
                                    focus_win_x_var += mdec.x;
                                    focus_win_y_var += mdec.y;
                                }
                            }
                            if (focus_win->xsize - 18 <= mxs && mxs < focus_win->xsize - 2 && 3 <= mys && mys < 18)
                            { // 点击窗口“X”关闭按钮
                                if (focus_win->type == 1)
                                {
                                    exit_task(focus_win->task);
                                }
                                else if (focus_win->type == 3)
                                {
                                    layer_free(focus_win);
                                    focus_win->task->layers[2] = 0;
                                }
                                focus_win = 0;
                            }
                        }
                    }
                    if (focus_win != 0 && focus_win->type == 1)
                    { // 将鼠标数据发送给获得焦点的窗口。
                        mxs = mx - focus_win->vx0;
                        mys = my - focus_win->vy0;
                        if (0 <= mxs && mxs < focus_win->xsize && 0 <= mys && mys < focus_win->ysize)
                        {
                            fifo_put(&focus_win->task->fifo, last_mouse_event + 512);
                            fifo_put(&focus_win->task->fifo, mxs + 512);
                            fifo_put(&focus_win->task->fifo, mys + 512);
                        }
                    }
                }
            }
        }
    }
    return 0;
}