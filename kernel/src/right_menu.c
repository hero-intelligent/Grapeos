#include "common.h"

#define RMENU_BCOLOR 0xeeeeee //右键菜单背景色
#define RMENU_HCOLOR 0xffffff //右键菜单高亮色

/**打开右键菜单，返回图层指针。
 * menu_num 菜单项数量
 * menus 所有菜单项字符串数组
 * x,y
 */
void *open_right_menus(struct RightMenuData *menu_data)
{
    close_right_menus(); //如果有已经打开的右键菜单一定要先关闭
    struct Layer *right_menus = layer_alloc();
    right_menus->type = 2;
    right_menus->rmenu_data = menu_data;
    menu_data->highlight_idx = -1;
    int height = menu_data->menu_num * 16 + 4;
    int *layer_buf = kmalloc(RMENU_WIDTH * height * 4);
    layer_set(right_menus, layer_buf, RMENU_WIDTH, height, -1);
    fill_rectangle(right_menus, 0, 0, RMENU_WIDTH, height, 0x000000);             //画背景黑边
    fill_rectangle(right_menus, 1, 1, RMENU_WIDTH - 2, height - 2, RMENU_BCOLOR); //画背景
    for (int i = 0; i < menu_data->menu_num; i++)                                 //画菜单项
    {
        print_string(right_menus, 2, i * 16 + 2, 0x000000, menu_data->menus[i], 1);
    }
    struct Task *current_task = task_now();
    current_task->layers[1] = right_menus;
    right_menus->task = current_task;
    struct Layer *farther = current_task->layers[0]; //本右键菜单所在的父图层
    layer_slide(right_menus, farther->vx0 + menu_data->x, farther->vy0 + menu_data->y);
    layer_updown(right_menus, layerctl->top);
    return right_menus;
}

void close_right_menus(void)
{
    struct Task *current_task = task_now();
    struct Layer *right_menus = current_task->layers[1];
    if (right_menus == 0)
        return;
    layer_free(right_menus);
    current_task->layers[1] = 0;
}

/**高亮右键菜单中指定的一项
 * idx 要高亮指定项的索引
 */
void highlight_right_menu(int idx)
{
    struct Task *current_task = task_now();
    struct Layer *right_menus = current_task->layers[1];
    if (right_menus == 0)
        return;
    int old_idx = right_menus->rmenu_data->highlight_idx;
    if (old_idx >= 0)
    { //如果之前有高亮的菜单项，先取消高亮。
        change_color(right_menus, 2, old_idx * 16 + 2, RMENU_WIDTH - 4, 16, RMENU_HCOLOR, RMENU_BCOLOR);
    }
    change_color(right_menus, 2, idx * 16 + 2, RMENU_WIDTH - 4, 16, RMENU_BCOLOR, RMENU_HCOLOR);
    right_menus->rmenu_data->highlight_idx = idx;
}