#include "common.h"

#define EXPLORER_WIDTH 800
#define EXPLORER_HEIGHT 600
#define INPUT_NAME_BOX_WIDTH 8 * 12 + 1 //加1是为了放光标
#define INPUT_NAME_BOX_HEIGHT 16
#define SELECTED_ITEM_COLOR 0x00CCE8FF //204, 232, 255 鼠标选中文件时的背景色
#define NORMAL_BG_COLOR 0xffffff

static struct File *file_infos;
static int file_num;
static char fullname[13];
static int selected_file_index;

//以下4个变量是选中文件列表项的背景范围
static const int item_x0 = 1;
static int item_y0;
static const int item_width = EXPLORER_WIDTH - 2;
static const int item_height = 16;

//鼠标相关变量
static int mouse_data_stage = 0; //0~2，鼠标数据是3个连续的int。
static int mouse_event = 0;
static int mouse_x = 0;
static int mouse_y = 0;

static int rmenus_idx = -1;   //当前显示哪个右键菜单，-1表示当前没有显示右键菜单。
static int is_input_mode = 0; //是否是在输入文件名模式
static int fullname_idx = 12; //文件名字符索引 0~12
//输入文件名文本框坐标
static const int input_box_x = 1;
static int input_box_y;
static int cursor_x, cursor_y; //光标坐标

static void init(struct Layer *win)
{
    memset(fullname, 0, 13);
    selected_file_index = -1;
    item_y0 = selected_file_index * 16 + 36;

    mouse_data_stage = 0;
    mouse_event = 0;
    mouse_x = 0;
    mouse_y = 0;

    rmenus_idx = -1;
    is_input_mode = 0;
    fullname_idx = 12;
    input_box_y = selected_file_index * 16 + 36;
    cursor_x = fullname_idx * 8 + 1;
    cursor_y = input_box_y;

    close_right_menus();
    switch_cursor(win, 0);
}

/**从硬盘读取文件信息，刷新窗口。返回文件数量。
 * explorer_win 窗口指针
 * file_infos 将从硬盘读取的文件列表信息存储到该数组中。
 */
void refresh(struct Layer *win)
{
    init(win);
    char name[9] = {0};
    char ext[4] = {0};
    file_num = read_root_dir(file_infos);
    fill_rectangle(win, 1, 20, win->xsize - 2, win->ysize - 21, 0xffffff);
    gprintf(win, 1, 20, 0x000000, "name            size");
    for (int i = 0; i < file_num; i++)
    {
        memcpy(name, file_infos[i].name, 8);
        memcpy(ext, file_infos[i].ext, 3);
        gprintf(win, 1, i * 16 + 36, 0x000000, "%s.%s    %d", name, ext, file_infos[i].size);
    }
}

void explorer_exit(void)
{
    kfree(file_infos);
}

void explorer_work(void)
{
    struct Layer *explorer_win = open_window(EXPLORER_WIDTH, EXPLORER_HEIGHT, "explorer");
    file_infos = kmalloc(sizeof(struct File) * 16);
    refresh(explorer_win);

    //右键菜单
    struct RightMenuData rmenus_data[2];
    rmenus_data[0].menu_num = 2; //菜单1
    rmenus_data[0].menus[0] = "new file";
    rmenus_data[0].menus[1] = "refresh";
    rmenus_data[1].menu_num = 2; //菜单2
    rmenus_data[1].menus[0] = "rename";
    rmenus_data[1].menus[1] = "delete";

    for (;;)
    {
        int msg = get_sys_msg(1);
        if (256 <= msg && msg <= 511) //键盘
        {
            int key = msg - 256;
            if (is_input_mode && key > 0)
            {
                if (key == '\n') //回车保存输入的文件名
                {
                    is_input_mode = 0;
                    switch_cursor(explorer_win, 0);
                    struct File *file = &file_infos[selected_file_index];
                    alter_file_name(file, fullname);
                    refresh(explorer_win);
                    continue;
                }
                else if (key == '\b' && 1 <= fullname_idx && fullname_idx <= 12)
                {
                    fullname_idx--;
                    fullname[fullname_idx] = 0;
                }
                else if (key != '\b' && 0 <= fullname_idx && fullname_idx <= 11)
                {
                    if ('a' <= key && key <= 'z')
                    {
                        key -= 0x20;
                    }
                    fullname[fullname_idx] = key;
                    fullname_idx++;
                }
                cursor_x = fullname_idx * 8 + 1;
                move_cursor(explorer_win, cursor_x, cursor_y);
                fill_rectangle(explorer_win, input_box_x, input_box_y, INPUT_NAME_BOX_WIDTH, INPUT_NAME_BOX_HEIGHT, 0xffffff);
                print_string(explorer_win, input_box_x, input_box_y, 0x000000, fullname, 1);
            }
        }
        else if (512 <= msg && msg <= 512 + 1023) //鼠标
        {
            msg -= 512;
            if (mouse_data_stage == 0 && msg < 7)
            {
                mouse_event = msg;
                mouse_data_stage++;
            }
            else if (mouse_data_stage == 1)
            {
                mouse_x = msg;
                mouse_data_stage++;
            }
            else if (mouse_data_stage == 2)
            {
                mouse_y = msg;
                mouse_data_stage = 0;

                if (rmenus_idx >= 0) //当有右键菜单时
                {
                    //以下4个变量是右键菜单在本窗口中的坐标范围
                    int rmx0 = rmenus_data[rmenus_idx].x;
                    int rmy0 = rmenus_data[rmenus_idx].y;
                    int rmx1 = rmx0 + RMENU_WIDTH;
                    int rmy1 = rmenus_data[rmenus_idx].menu_num * 16 + rmy0;
                    if (rmx0 <= mouse_x && mouse_x < rmx1 && rmy0 <= mouse_y && mouse_y < rmy1) //当鼠标在右键菜单上时
                    {
                        int ridx = (mouse_y - rmy0) / 16;    //鼠标在哪个菜单项上方。
                        if (mouse_event == MOUSE_EVENT_MOVE) //当鼠标在右键菜单上移动时，相应的菜单项要高亮。
                        {
                            highlight_right_menu(ridx);
                            continue;
                        }
                        else if (mouse_event == MOUSE_EVENT_LEFT_DOWN) //鼠标点击某个菜单项
                        {
                            if (rmenus_idx == 0 && ridx == 0) //新建文件
                            {
                                void *file = create_file("NEW FILE.TXT");
                                if (file == 0)
                                {
                                    open_popwin("message", "File name already exist.");
                                    continue;
                                }
                                else
                                {
                                    refresh(explorer_win);
                                    continue;
                                }
                            }
                            else if (rmenus_idx == 0 && ridx == 1) //刷新
                            {
                                refresh(explorer_win);
                                continue;
                            }
                            else if (rmenus_idx == 1 && ridx == 0) //重命名
                            {
                                is_input_mode = 1;
                                fullname_idx = 12;
                                input_box_y = selected_file_index * 16 + 36;
                                cursor_x = fullname_idx * 8 + 1;
                                cursor_y = input_box_y;
                                memcpy(fullname, file_infos[selected_file_index].name, 8);
                                fullname[8] = '.';
                                memcpy(fullname + 9, file_infos[selected_file_index].ext, 3);
                                switch_cursor(explorer_win, 1);
                                move_cursor(explorer_win, cursor_x, cursor_y);
                                fill_rectangle(explorer_win, input_box_x, input_box_y, INPUT_NAME_BOX_WIDTH, INPUT_NAME_BOX_HEIGHT, 0xffffff);
                                print_string(explorer_win, input_box_x, input_box_y, 0x000000, fullname, 1);
                                continue;
                            }
                            else if (rmenus_idx == 1 && ridx == 1) //删除
                            {
                                delete_file(&file_infos[selected_file_index]);
                                refresh(explorer_win);
                                continue;
                            }
                        }
                    }
                }
                int item_index = (mouse_y - 20) / 16 - 1;     //鼠标在哪个文件列表项上
                if (item_index >= 0 && item_index < file_num) //当鼠标在某个文件上时。
                {
                    if (mouse_event == MOUSE_EVENT_LEFT_DOWN) //当鼠标单击某个文件时。
                    {
                        if (selected_file_index != item_index)
                        {
                            if (selected_file_index >= 0)
                            { //将之前选中的文件恢复背景色。
                                item_y0 = selected_file_index * 16 + 36;
                                change_color(explorer_win, item_x0, item_y0, item_width, item_height, SELECTED_ITEM_COLOR, NORMAL_BG_COLOR);
                            }
                            selected_file_index = item_index;
                            item_y0 = selected_file_index * 16 + 36;
                            change_color(explorer_win, item_x0, item_y0, item_width, item_height, NORMAL_BG_COLOR, SELECTED_ITEM_COLOR);
                        }
                    }
                    else if (mouse_event == MOUSE_EVENT_DOUBLE_CLICK) //当鼠标双击某个文件时。
                    {
                        struct File *selected_file = &file_infos[selected_file_index];
                        if (memcmp("EXE", selected_file->ext, 3) == 0)
                        { //如果双击后缀名为EXE的文件，则运行该文件。
                            struct Task *task = new_user_task(selected_file, 0);
                            if (task == 0)
                            {
                                open_popwin("message", "new user task fail.");
                            }
                        }
                        else if (memcmp("TXT", selected_file->ext, 3) == 0)
                        { //如果双击后缀名为TXT的文件，则用记事本打开该文件。
                            struct File *notepad = 0;
                            for (int i = 0; i < 16; i++)
                            {
                                notepad = &file_infos[i];
                                if (memcmp(notepad->name, "NOTEPAD ", 8) == 0 && memcmp(notepad->ext, "EXE", 3) == 0)
                                {
                                    struct Task *task = new_user_task(notepad, (char *)selected_file->name);
                                    if (task == 0)
                                    {
                                        open_popwin("message", "new user task fail.");
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    else if (mouse_event == MOUSE_EVENT_RIGHT_DOWN) //当鼠标右键某个文件时弹出对应的右键菜单
                    {
                        rmenus_idx = 1;
                        rmenus_data[rmenus_idx].x = mouse_x;
                        rmenus_data[rmenus_idx].y = mouse_y;
                        open_right_menus(&rmenus_data[rmenus_idx]);
                    }
                }
                else if (item_index >= file_num) //当鼠标在文件列表以下的空白区域时。
                {
                    if (mouse_event == MOUSE_EVENT_LEFT_DOWN)
                    {
                        rmenus_idx = -1;
                        close_right_menus();
                    }
                    else if (mouse_event == MOUSE_EVENT_RIGHT_DOWN) //当鼠标右键空白区域时弹出对应的右键菜单
                    {
                        rmenus_idx = 0;
                        rmenus_data[rmenus_idx].x = mouse_x;
                        rmenus_data[rmenus_idx].y = mouse_y;
                        open_right_menus(&rmenus_data[rmenus_idx]);
                    }
                }
                else if (item_index < 0) //当鼠标在第1个文件以上的区域时
                {
                    if (mouse_event == MOUSE_EVENT_LEFT_DOWN)
                    {
                        rmenus_idx = -1;
                        close_right_menus();
                    }
                    else if (mouse_event == MOUSE_EVENT_RIGHT_DOWN)
                    {
                        rmenus_idx = -1;
                        close_right_menus();
                    }
                }
            }
        }
    }
}

void open_explorer(void)
{
    new_kernel_task(explorer_work, explorer_exit);
}