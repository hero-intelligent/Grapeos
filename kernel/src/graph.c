#include "common.h"

#define DESKTOP_COLOR 0x008484  //桌面颜色
#define TASK_BAR_COLOR 0xC6C6C6 //任务栏颜色

/**画实心矩形
 * layer 图层指针
 * x0,y0 矩形左上角在图层上的坐标
 * width,height 矩形的宽度和高度
 * color 矩形的颜色
 */
void fill_rectangle(struct Layer *layer, int x0, int y0, int width, int height, int color)
{
    if (x0 < 0)
    {
        x0 = 0;
    }
    if (y0 < 0)
    {
        y0 = 0;
    }
    int x1 = x0 + width;
    int y1 = y0 + height;
    if (x1 > layer->xsize)
    {
        x1 = layer->xsize;
    }
    if (y1 > layer->ysize)
    {
        y1 = layer->ysize;
    }
    int *gram = layer->buf;
    int xsize = layer->xsize;
    for (int y = y0; y < y1; y++)
    {
        for (int x = x0; x < x1; x++)
        {
            gram[y * xsize + x] = color;
        }
    }
    layer_refresh(layer, x0, y0, width, height);
}

/**打印字符上的一个点
 * pixel 第一个像素的地址
 * scale 放大倍数
 * xsize 图层宽度
 * color 字符颜色
 */
void print_font_dot(int *pixel, int scale, int xsize, int color)
{
    for (int i = 0; i < scale; i++)
    {
        for (int j = 0; j < scale; j++)
        {
            pixel[i * xsize + j] = color;
        }
    }
}

/**向屏幕打印单个ASCII字符。每个字符默认宽8高16，可按比例放大。
 * layer 图层指针
 * x,y 字符左上角坐标
 * color 字符颜色
 * font 字符点阵位图起始地址
 * scale 放大比例
 */
void print_font(struct Layer *layer, int x, int y, int color, char *font, int scale)
{
    int *gram = layer->buf;
    int xsize = layer->xsize;
    for (int i = 0; i < 16; i++) // 16行8列点阵字符，每次循环显示1行8个像素。
    {
        char data = font[i];                             //每一行8个像素对应点阵位图中的一个字节
        int *pixel = gram + (y + i * scale) * xsize + x; //每行第一个像素的地址
        if ((data & 0x80) != 0)
        {
            print_font_dot(pixel + scale * 0, scale, xsize, color);
        }
        if ((data & 0x40) != 0)
        {
            print_font_dot(pixel + scale * 1, scale, xsize, color);
        }
        if ((data & 0x20) != 0)
        {
            print_font_dot(pixel + scale * 2, scale, xsize, color);
        }
        if ((data & 0x10) != 0)
        {
            print_font_dot(pixel + scale * 3, scale, xsize, color);
        }
        if ((data & 0x08) != 0)
        {
            print_font_dot(pixel + scale * 4, scale, xsize, color);
        }
        if ((data & 0x04) != 0)
        {
            print_font_dot(pixel + scale * 5, scale, xsize, color);
        }
        if ((data & 0x02) != 0)
        {
            print_font_dot(pixel + scale * 6, scale, xsize, color);
        }
        if ((data & 0x01) != 0)
        {
            print_font_dot(pixel + scale * 7, scale, xsize, color);
        }
    }
}

/**向屏幕打印一行ASCII字符串，以0x00结尾。
 * layer 图层指针
 * x,y 字符串左上角在图层上的坐标
 * color 字符颜色
 * str 字符串地址
 * scale 字符放大比例
 */
void print_string(struct Layer *layer, int x, int y, int color, char *str, int scale)
{
    int x0 = x;
    for (; *str != 0; str++)
    {
        print_font(layer, x, y, color, font_ascii + *str * 16, scale);
        x += 8 * scale; //指向下一个字符的左上角。
    }
    layer_refresh(layer, x0, y, x - x0, 16 * scale);
}

//图形版的printf。返回拼接后的字符串长度。
int gprintf(struct Layer *layer, int x, int y, int color, const char *format, ...)
{
    va_list args;
    va_start(args, format); //使args指向format
    char buf[1024] = {0};   //用于存储拼接后的字符串
    unsigned int retval = vsprintf(buf, format, args);
    va_end(args);
    print_string(layer, x, y, color, buf, 1);
    return retval;
}

/**在多行文本框中输出一段字符串（白底黑字）
 * layer 图层指针
 * str 字符串指针
 * x0,y0 文本框左上角坐标
 * width,height 文本框宽度和高度
 * lens 如果不为零记录每行字符数（为0则不记录）
 */
void print_text(struct Layer *layer, char *str, int x0, int y0, int width, int height, int lens[])
{
    int bg_color = 0xffffff;   //白底
    int font_color = 0x000000; //黑字
    int rows = height / 16;    //文本框总行数
    int columns = width / 8;   //文本框总列数
    int row = 0;
    int column = 0;
    int x = x0;
    int y = y0;
    fill_rectangle(layer, x0, y0, width, height, bg_color); //先刷白背景。
    if (str == 0)
        return;
    for (; *str != 0; str++)
    {
        if (*str == '\n') //回车换行
        {
            if (lens != 0)
            {
                lens[row] = column;
            }
            row++;
            column = 0;
            continue;
        }
        if (column >= columns) //一行放不下自动换行
        {
            if (lens != 0)
            {
                lens[row] = columns;
            }
            row++;
            column = 0;
        }
        if (row >= rows)
        {
            break; //超过最大行数，后面的不再输出。
        }
        x = column * 8 + x0;
        y = row * 16 + y0;
        print_font(layer, x, y, font_color, font_ascii + *str * 16, 1);
        column++; //指向下一个字符
    }
    if (*str == 0)
    {
        lens[row] = column; //记录最后一行的字符数
    }
    layer_refresh(layer, x0, y0, width, height);
}

/**初始化鼠标图形
 * mouse 鼠标显存缓存区（1KB），鼠标图形是16*16像素，共256个像素，每个像素对应4个字节。
 * col_inv 透明色
 */
void init_mouse_cursor(int *mouse, int col_inv)
{
    static char cursor[16][16] = {
        "**************..",
        "*OOOOOOOOOOO*...",
        "*OOOOOOOOOO*....",
        "*OOOOOOOOO*.....",
        "*OOOOOOOO*......",
        "*OOOOOOO*.......",
        "*OOOOOOO*.......",
        "*OOOOOOOO*......",
        "*OOOO**OOO*.....",
        "*OOO*..*OOO*....",
        "*OO*....*OOO*...",
        "*O*......*OOO*..",
        "**........*OOO*.",
        "*..........*OOO*",
        "............*OO*",
        ".............***"};
    int x, y;
    for (y = 0; y < 16; y++)
    {
        for (x = 0; x < 16; x++)
        {
            if (cursor[y][x] == '*')
            {
                mouse[y * 16 + x] = 0x000000; //黑边
            }
            else if (cursor[y][x] == 'O') //注意这里是大写字母O，不是数字0。
            {
                mouse[y * 16 + x] = 0xFFFFFF;
            }
            else if (cursor[y][x] == '.')
            {
                mouse[y * 16 + x] = col_inv;
            }
        }
    }
}

void init_desktop(struct Layer *layer)
{
    //画桌面背景
    fill_rectangle(layer, 0, 0, layer->xsize, layer->ysize, DESKTOP_COLOR);
    //画任务栏
    fill_rectangle(layer, 0, layer->ysize - 32, layer->xsize, 32, TASK_BAR_COLOR);
    //画我的电脑图标
    fill_rectangle(layer, 100, 100, 100, 80, 0xffffff);
    fill_rectangle(layer, 110, 110, 80, 60, 0x000084);
    fill_rectangle(layer, 145, 180, 10, 10, 0xffffff);
    fill_rectangle(layer, 100, 190, 100, 10, 0xffffff);
    print_string(layer, 110, 205, 0xffffff, "My Computer", 1);
    //写放大版的GrapeOS字符串
    int scale = 8;
    int x = (layer->xsize - 8 * scale * 7) / 2;
    int y = (layer->ysize - 16 * scale) / 2;
    print_string(layer, x, y, 0xffffff, "GrapeOS", scale);
    //左下角显示假开始按钮
    fill_rectangle(layer, 2, layer->ysize - 30, 44, 28, 0xd7d7d7);
    print_string(layer, 4, layer->ysize - 24, 0x000000, "Start", 1);
    //右下角显示假时间
    print_string(layer, layer->xsize - 50, layer->ysize - 24, 0x000000, "12:34", 1);
}