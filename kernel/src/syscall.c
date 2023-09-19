#include "common.h"

/**系统调用
 * esp_user 用户栈指针
 * api_num 功能号
 */
int syscall(int *esp_user, int api_num)
{
    int *p = esp_user; //*(p + 0)是api_xxx函数的返回地址
    if (api_num == 1)
    {
        return (int)open_window(*(p + 1), *(p + 2), (char *)(*(p + 3)));
    }
    else if (api_num == 2)
    {
        exit_current_task();
    }
    else if (api_num == 3)
    {
        fill_rectangle((void *)(*(p + 1)), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6));
    }
    else if (api_num == 4)
    {
        return get_sys_msg(*(p + 1));
    }
    else if (api_num == 5)
    {
        print_string((void *)(*(p + 1)), *(p + 2), *(p + 3), *(p + 4), (void *)*(p + 5), *(p + 6));
    }
    else if (api_num == 6)
    {
        switch_cursor((void *)*(p + 1), *(p + 2));
    }
    else if (api_num == 7)
    {
        move_cursor((void *)*(p + 1), *(p + 2), *(p + 3));
    }
    else if (api_num == 8)
    {
        print_text((void *)*(p + 1), (char *)*(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6), (int *)*(p + 7));
    }
    else if (api_num == 9)
    {
        change_color((void *)*(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6), *(p + 7));
    }
    else if (api_num == 10)
    {
        return (int)open_popwin((char *)*(p + 1), (char *)*(p + 2));
    }
    else if (api_num == 13)
    {
        return (int)umalloc(*(p + 1));
    }
    else if (api_num == 14)
    {
        ufree((void *)*(p + 1));
    }
    else if (api_num == 15)
    {
        return (int)create_file((void *)*(p + 1));
    }
    else if (api_num == 16)
    {
        save_file((void *)*(p + 1), (void *)*(p + 2));
    }
    else if (api_num == 18)
    {
        get_should_open_filename((char *)(*(p + 1)));
    }
    else if (api_num == 19)
    {
        return (int)open_file((char *)(*(p + 1)));
    }
    else if (api_num == 20)
    {
        read_file((void *)(*(p + 1)), (void *)(*(p + 2)));
    }
    return 0;
}