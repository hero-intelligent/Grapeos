#include "common.h"

#define WIN_BG_COLOR 0xC6C6C6            //窗口背景色
#define TITLE_BAR_FOCUS_COLOR 0x000084   //获得焦点时的标题栏颜色
#define TITLE_BAR_UNFOCUS_COLOR 0x848484 //没有获得焦点时的标题栏颜色
#define TITLE_BAR_HEIGHT 20              //标题栏高度

// int popwin_count = 0;      //当前屏幕弹出窗总数量
// struct Layer *modal_pop_win = 0; //模态弹窗指针
struct Layer *focus_win = 0;    //获得焦点的窗口或桌面指针
struct Timer *cursor_timer = 0; //光标闪烁定时器

/**改变标题栏颜色
 * layer 图层指针
 * is_focus 是否获得焦点
 */
void change_title_bar_color(struct Layer *layer, int is_focus)
{
    if (layer != 0 && (layer->type == 1 || layer->type == 3))
    {
        if (is_focus)
        {
            change_color(layer, 0, 0, layer->xsize, TITLE_BAR_HEIGHT, TITLE_BAR_UNFOCUS_COLOR, TITLE_BAR_FOCUS_COLOR);
        }
        else
        {
            change_color(layer, 0, 0, layer->xsize, TITLE_BAR_HEIGHT, TITLE_BAR_FOCUS_COLOR, TITLE_BAR_UNFOCUS_COLOR);
        }
    }
}

//焦点在不同的窗口之间转移
void focus_exchange(struct Layer *old_focus_layer, struct Layer *new_focus_layer)
{
    cursor_blink(old_focus_layer, 0);           //失去焦点的窗口关闭光标闪烁
    change_title_bar_color(old_focus_layer, 0); //失去焦点的窗口标题栏变暗
    cursor_blink(new_focus_layer, 1);           //获得焦点的窗口开启光标闪烁
    change_title_bar_color(new_focus_layer, 1); //获得焦点的窗口标题栏变亮
}

/**画标题栏
 * layer 图层指针
 * title 标题栏文字
 */
void draw_title_bar(struct Layer *layer, char *title)
{
    //窗口关闭按钮宽16高15，距顶边3，距右边2。
    static char closebtn[15][16] = {
        "OOOOOOOOOOOOOOOO",
        "OOOOOOOOOOOOOOOO",
        "OOOOOOOOOOOOOOOO",
        "OOOXXOOOOOOXXOOO",
        "OOOOXXOOOOXXOOOO",
        "OOOOOXXOOXXOOOOO",
        "OOOOOOXXXXOOOOOO",
        "OOOOOOOXXOOOOOOO",
        "OOOOOOXXXXOOOOOO",
        "OOOOOXXOOXXOOOOO",
        "OOOOXXOOOOXXOOOO",
        "OOOXXOOOOOOXXOOO",
        "OOOOOOOOOOOOOOOO",
        "OOOOOOOOOOOOOOOO",
        "OOOOOOOOOOOOOOOO"};
    int x, y;
    int c;
    int xsize = layer->xsize;
    int *buf = layer->buf;
    fill_rectangle(layer, 1, 1, xsize - 2, 19, TITLE_BAR_FOCUS_COLOR); //画标题栏
    print_string(layer, 10, 2, 0xffffff, title, 1);                    //写标题文字
    //以下画关闭按钮
    for (y = 0; y < 15; y++)
    {
        for (x = 0; x < 16; x++)
        {
            c = closebtn[y][x];
            if (c == 'X')
            {
                c = 0x000000;
            }
            else if (c == 'O')
            {
                c = 0xC6C6C6;
            }
            buf[(3 + y) * xsize + (xsize - 18 + x)] = c;
        }
    }
}

/**画窗口
 * layer 图层指针
 * title 标题栏文字
 */
void draw_window(struct Layer *layer, char *title)
{
    fill_rectangle(layer, 0, 0, layer->xsize, layer->ysize, WIN_BG_COLOR);
    draw_title_bar(layer, title);
}

//打开一个窗口，返回窗口地址。
void *open_window(int width, int height, char *title)
{
    struct Task *current_task = task_now();
    if (current_task->layers[0] != 0)
    { //每个任务只能申请一个窗口。
        return 0;
    }

    if (width < 20)
    {
        width = 20;
    }
    else if (width > SCRNX)
    {
        width = SCRNX;
    }

    if (height < 20)
    {
        height = 20;
    }
    else if (height > SCRNX)
    {
        height = SCRNX;
    }

    struct Layer *window = layer_alloc();
    window->type = 1;
    int *win_buf = kmalloc(width * height * 4);
    layer_set(window, win_buf, width, height, -1);
    draw_window(window, title);
    //在屏幕上居中显示窗口
    int wx = (SCRNX - width) / 2;
    int wy = (SCRNY - height) / 2;
    layer_slide(window, wx, wy);
    layer_updown(window, layerctl->top);
    current_task->layers[0] = window;
    window->task = current_task;
    focus_exchange(focus_win, 0);
    focus_win = window;
    return window;
}

/**打开一个弹窗，返回弹窗指针。
 * title 弹窗标题
 * msg 弹窗信息
 */
void *open_popwin(char *title, char *msg)
{
    int width = 256;
    int height = 68;
    struct Layer *popwin = layer_alloc();
    popwin->type = 3;
    int *win_buf = kmalloc(width * height * 4);
    layer_set(popwin, win_buf, width, height, -1);
    draw_window(popwin, title);
    print_string(popwin, 2, 22, 0x000000, msg, 1);

    struct Task *current_task = task_now();
    current_task->layers[2] = popwin;
    popwin->task = current_task;

    struct Layer *main_win = current_task->layers[0];
    int wx = (main_win->xsize - width) / 2 + main_win->vx0;
    int wy = (main_win->ysize - height) / 2 + main_win->vy0;
    layer_slide(popwin, wx, wy); //将弹出窗置于主窗口中心。
    layer_updown(popwin, layerctl->top);

    focus_exchange(focus_win, 0);
    focus_win = popwin;
    return popwin;
}

//初始化光标
void init_cursor(void)
{
    // cursor_timer = timer_alloc();
    timer_set_fifo(cursor_timer, &sys_fifo, 1);
    timer_set_time(cursor_timer, 50);
}

/**光标闪烁（输入框都是白底黑光标）
 * window 窗口指针
 * isvisible 1或0 控制光标亮灭
 */
void cursor_blink(struct Layer *window, int isvisible)
{
    if (window == 0)
        return;
    if (window->type != 1)
        return;
    if (window->cursor.on == 0)
        return;
    int color = 0xffffff;
    if (isvisible)
    {
        color = 0x000000;
    }
    int x = window->cursor.x0, y;
    int y0 = window->cursor.y0;
    int y1 = y0 + 16;
    for (y = y0; y < y1; y++)
    {
        window->buf[y * window->xsize + x] = color;
    }
    layer_refresh(window, x, y0, 1, 16);
}

/**开启或关闭光标
 * window 窗口指针
 * on 1或0 开启或关闭光标
 */
void switch_cursor(struct Layer *window, int on)
{
    if (on == 0) //关闭光标
    {
        cursor_blink(window, 0);
    }
    window->cursor.on = on;
}

/**移动光标
 * x,y 光标新坐标
 */
void move_cursor(struct Layer *window, int x, int y)
{
    cursor_blink(window, 0);
    window->cursor.x0 = x;
    window->cursor.y0 = y;
}