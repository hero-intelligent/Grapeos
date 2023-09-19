#include <api.h>
#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define MAX_TEXT_LEN 3500
#define MAX_ROWS 35
#define BUTTON_HOVER_COLOR 0XE5F3FF //229, 243, 255 鼠标悬停在按钮上时的按钮背景色
#define BUTTON_COLOR 0XE1E1E1

//将name和ext合并成fullname
void merge_name_and_ext(char *name, char *ext, char *fullname)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        if (name[i] == 0)
        {
            break;
        }
        fullname[i] = name[i];
    }
    fullname[i] = '.';
    for (int j = 0; j < 3; j++)
    {
        i++;
        fullname[i] = ext[j];
    }
}

int main()
{
    char fullname[13] = {0};                //文件全名
    char name[9] = {0};                     //文件名
    char ext[4] = "TXT";                    //后缀名
    int lens[MAX_ROWS] = {0};               //正文每行字符数
    int txt_idx = 0;                        //正文字符索引
    int row = 0, column = 0;                //正文光标行和列的索引
    int columns = (WIN_WIDTH - 2) / 8;      //正文文本框列数（一行能放多少个字符）
    int text_cx = 1, text_cy = 40;          //正文输入框光标坐标
    void *file = 0;                         //文件指针
    int is_show_name_inputbox = 0;          //是否显示文件名输入框
    int inputbox_idx = 0;                   //当前输入框，0为正文输入框，1为文件名输入框。
    int name_idx = 0;                       //文件名字符索引
    int save_btn_color = BUTTON_COLOR;      //save按钮当前的颜色
    int mouse_data_stage = 0;               //0~2，鼠标数据是3个连续的int。
    int mouse_event = 0;                    //鼠标事件
    int mouse_x = 0, mouse_y = 0;           //鼠标坐标
    char *text = api_umalloc(MAX_TEXT_LEN); //存放正文内容的空间

    api_get_should_open_filename(name);
    void *window = api_openwin(WIN_WIDTH, WIN_HEIGHT, "notepad");
    api_fill_rectangle(window, 1, 40, WIN_WIDTH - 2, WIN_HEIGHT - 41, 0xffffff); //画正文输入框
    api_print_string(window, 2, 22, 0x000000, "file name:", 1);
    api_print_string(window, 146, 22, 0x000000, ".TXT", 1);
    api_fill_rectangle(window, 200, 21, 32, 18, BUTTON_COLOR); //画保存按钮
    api_print_string(window, 200, 22, 0x000000, "save", 1);
    api_switch_cursor(window, 1);
    api_move_cursor(window, text_cx, text_cy);

    if (name[0] == 0)
    {
        is_show_name_inputbox = 1;
        api_fill_rectangle(window, 82, 22, 65, 16, 0xffffff); //画文件名输入框
    }
    else
    {
        is_show_name_inputbox = 0;
        api_print_string(window, 82, 22, 0x000000, name, 1); //显示文件名
        merge_name_and_ext(name, ext, fullname);
        file = api_open_file(fullname);
        api_read_file(file, text);
        if (text[0] != 0)
        {
            api_print_text(window, text, 1, 40, WIN_WIDTH - 2, WIN_HEIGHT - 41, lens);
            for (int i = 0; i < MAX_TEXT_LEN; i++)
            { //查找最后一个字符的索引
                if (text[i] != 0)
                {
                    txt_idx++;
                }
            }
            //将光标放到最后一个字符的后面
            for (int i = MAX_ROWS - 1; i >= 0; i--)
            {
                if (lens[i] != 0)
                {
                    row = i;
                    column = lens[i];
                    break;
                }
            }
            text_cx = column * 8 + 1;
            text_cy = row * 16 + 40;
            api_move_cursor(window, text_cx, text_cy);
        }
    }
    for (;;)
    {
        int msg = api_get_sys_msg(1);
        if (256 <= msg && msg <= 511) //键盘
        {
            int key = msg - 256;
            if (inputbox_idx == 1) //输入到文件名输入框
            {
                if (1 <= name_idx && name_idx <= 8 && key == '\b')
                {
                    name_idx--;
                    name[name_idx] = 0;
                }
                else if (0 <= name_idx && name_idx <= 7 && key != '\b')
                {
                    if ('a' <= key && key <= 'z')
                    {
                        key -= 0x20;
                    }
                    name[name_idx] = key;
                    name_idx++;
                }
                api_move_cursor(window, 82 + name_idx * 8, 22);       //将光标移动到文件名后面
                api_fill_rectangle(window, 82, 22, 65, 16, 0xffffff); //刷新输入框背景
                api_print_string(window, 82, 22, 0x000000, name, 1);  //显示文件名
            }
            else //输入到正文输入框
            {
                if (key > 0 && txt_idx < MAX_TEXT_LEN)
                {
                    if (key == '\b' && txt_idx > 0)
                    {
                        txt_idx--;
                        if (text[txt_idx] == '\n') //回车换行的情况
                        {
                            row--;
                            column = lens[row];
                            if (column < 0)
                            {
                                column = 0;
                            }
                        }
                        else //文字满一行自动换行的情况
                        {
                            column--;
                            if (column < 0)
                            {
                                row--;
                                column = columns; //光标在最后一个字符的后面
                            }
                        }
                        text[txt_idx] = 0;
                    }
                    else if (key != '\b')
                    {
                        text[txt_idx] = key;
                        txt_idx++;
                        if (key == '\n') //回车换行
                        {
                            column = columns;
                        }
                        else
                        {
                            column++;
                        }
                        if (column >= columns)
                        {
                            row++;
                            column = 0;
                        }
                    }
                    text_cx = column * 8 + 1;
                    text_cy = row * 16 + 40;
                    api_move_cursor(window, text_cx, text_cy);
                    api_print_text(window, text, 1, 40, WIN_WIDTH - 2, WIN_HEIGHT - 41, lens);
                }
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

                if (82 <= mouse_x && mouse_x <= 145 && 22 <= mouse_y && mouse_y <= 37)
                {
                    if (mouse_event == MOUSE_EVENT_LEFT_DOWN && is_show_name_inputbox == 1) //当鼠标点击文件名输入框时
                    {
                        inputbox_idx = 1;
                        api_move_cursor(window, 82 + name_idx * 8, 22);
                    }
                }
                else if (0 <= mouse_x && mouse_x < WIN_WIDTH && 40 <= mouse_y && mouse_y < WIN_HEIGHT)
                {
                    if (mouse_event == MOUSE_EVENT_LEFT_DOWN) //当鼠标点击正文输入框时
                    {
                        inputbox_idx = 0;
                        api_move_cursor(window, text_cx, text_cy);
                    }
                }

                //鼠标在save按钮上时
                if (200 <= mouse_x && mouse_x < 232 && 21 <= mouse_y && mouse_y < 39)
                {
                    if (mouse_event == MOUSE_EVENT_MOVE) //鼠标在save按钮上时背景变色。
                    {
                        api_change_color(window, 200, 21, 231, 38, save_btn_color, BUTTON_HOVER_COLOR);
                        save_btn_color = BUTTON_HOVER_COLOR;
                    }
                    else if (mouse_event == MOUSE_EVENT_LEFT_DOWN) //点击save按钮
                    {
                        if (name[0] == 0) //提示输入文件名
                        {
                            api_open_popwin("Message", "Please input file name!");
                            api_move_cursor(window, 82 + name_idx * 8, 22);
                        }
                        else
                        {
                            if (file == 0)
                            {
                                merge_name_and_ext(name, ext, fullname);
                                file = api_create_file(fullname);
                                if (file == 0)
                                {
                                    api_open_popwin("Message", "File name already exists.");
                                }
                                else
                                {
                                    is_show_name_inputbox = 0;
                                    inputbox_idx = 0;
                                    api_move_cursor(window, text_cx, text_cy);
                                    api_fill_rectangle(window, 82, 22, 65, 16, WIN_BG_COLOR); //将文件名输入框去除
                                    api_print_string(window, 82, 22, 0x000000, name, 1);      //重新显示文件名
                                }
                            }
                            if (file != 0)
                            {
                                api_save_file(file, text);
                                api_open_popwin("Message", "Save OK.");
                            }
                        }
                    }
                }
                else
                {
                    if (save_btn_color != BUTTON_COLOR) //save菜单恢复原色
                    {
                        api_change_color(window, 200, 21, 231, 38, save_btn_color, BUTTON_COLOR);
                        save_btn_color = BUTTON_COLOR;
                    }
                }
            }
        }
    }
    return 0;
}