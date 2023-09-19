#pragma once

#define MOUSE_EVENT_MOVE 1
#define MOUSE_EVENT_LEFT_DOWN 2
#define MOUSE_EVENT_LEFT_UP 3
#define MOUSE_EVENT_DOUBLE_CLICK 4
#define MOUSE_EVENT_RIGHT_DOWN 5
#define MOUSE_EVENT_RIGHT_UP 6
#define WIN_BG_COLOR 0xC6C6C6 //窗口背景色

/**打开一个窗口，返回用于操作窗口的句柄。
 * xsize 窗口宽度
 * ysize 窗口高度
 * title 窗口标题
 */
void *api_openwin(int xsize, int ysize, char *title);

//退出应用程序
void api_exit(void);

/**画实心矩形
 * layer 图层指针
 * x0,y0 矩形左上角在图层上的坐标
 * width,height 矩形的宽度和高度
 * color 矩形的颜色
 */
void api_fill_rectangle(void *layer, int x0, int y0, int width, int height, int color);

/**获取键盘输入
 * is_wait 是否等待
 */
int api_get_sys_msg(int is_wait);

/**向屏幕打印一行ASCII字符串，以0x00结尾。
 * layer 图层指针
 * x,y 字符串左上角在图层上的坐标
 * color 字符颜色
 * str 字符串地址
 * scale 字符放大比例
 */
void api_print_string(void *layer, int x, int y, int color, char *str, int scale);

/**开启或关闭光标
 * window 窗口指针
 * on 1或0 开启或关闭光标
 */
void api_switch_cursor(void *window, int on);

/**移动光标
 * x,y 光标新坐标
 */
void api_move_cursor(void *window, int x, int y);

/**在多行文本框中输出一段字符串（白底黑字）
 * layer 图层指针
 * str 字符串指针
 * x0,y0 文本框左上角坐标
 * width,height 文本框宽度和高度
 * lens 如果不为零记录每行字符数（为0则不记录）
 */
void api_print_text(void *layer, char *str, int x0, int y0, int width, int height, int lens[]);

//将图层指定矩形区域的一种颜色更换为另一种颜色，主要用来改变背景色。
void api_change_color(void *layer, int x0, int y0, int width, int height, int old_color, int new_color);

/**打开一个弹窗，返回弹窗指针。
 * title 弹窗标题
 * msg 弹窗信息
 */
void *api_open_popwin(char *title, char *msg);

//在用户堆中申请size字节内存并清零，成功返回虚拟地址，失败返回0。
void *api_umalloc(unsigned int size);

//释放用户堆内存
void api_ufree(void *vaddr);

//创建文件，正常返回文件指针，如果文件已存在返回0。
void *api_create_file(char *fullname);

//保存文件
void api_save_file(void *file, char *content);

//获取启动后应该打开的文件名称，保存到filename中。
void api_get_should_open_filename(char *filename);

//打开一个文件
void *api_open_file(char *fullname);

//读文件。file_addr是将文件读取到的内存地址。
void api_read_file(void *file, void *file_addr);