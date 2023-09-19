#include "common.h"

#define LAYER_USE 1 // 表示图层正在使用。

struct LayerControl *layerctl; // 图层管理遍历，全局唯一。

// 初始化图层管理。vgram虚拟显存起始地址，(xsize,ysize)屏幕分辨率。
void init_layer(int *vgram, int xsize, int ysize)
{
    layerctl = (struct LayerControl *)kmalloc(sizeof(struct LayerControl));
    if (layerctl == 0)
    {
        return;
    }
    layerctl->map = kmalloc(xsize * ysize);
    if (layerctl->map == 0)
    {
        return;
    }
    layerctl->vgram = vgram;
    layerctl->xsize = xsize;
    layerctl->ysize = ysize;
    layerctl->top = -1; //-1表示暂时没有图层。
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        layerctl->layers0[i].flags = 0; // 0表示该图层未使用。
    }
}

// 取得新生成的未使用图层。
struct Layer *layer_alloc(void)
{
    struct Layer *layer;
    int i;
    for (i = 0; i < MAX_LAYERS; i++)
    {
        if (layerctl->layers0[i].flags == 0)
        {
            layer = &layerctl->layers0[i];
            layer->flags = LAYER_USE; // 标记为正在使用。
            layer->height = -1;       // 隐藏。
            layer->task = 0;          // 不使用自动关闭功能
            return layer;
        }
    }
    return 0; // 所有的图层都正在使用
}

// 设置图层属性。col_inv为-1时表示没有透明色。
void layer_set(struct Layer *layer, int *buf, int xsize, int ysize, int col_inv)
{
    layer->buf = buf;
    layer->xsize = xsize;
    layer->ysize = ysize;
    layer->col_inv = col_inv;
    return;
}

// 释放图层
void layer_free(struct Layer *layer)
{
    if (layer == 0)
    {
        return;
    }
    if (layer->height >= 0)
    {
        layer_updown(layer, -1); // 如果处于显示状态，则先设定为隐藏。
    }
    layer->flags = 0;  // “未使用”标志。
    kfree(layer->buf); // 释放图形内存
    return;
}

/**刷新屏幕指定的矩形部分。
 * vx0,vy0 矩形左上角坐标
 * width,height 矩形的宽和高
 * h0,h1 刷新时需要判断的图层高度范围
 */
void layer_refreshsub(int vx0, int vy0, int width, int height, int h0, int h1)
{
    int bx0, by0; // 要刷新的矩形在图层内左上角的坐标。
    int bx1, by1; // 要刷新的矩形在图层内右下角的坐标。
    unsigned char *map = layerctl->map, sid;
    int *buf, *gram = layerctl->vgram;
    // int sid4, i, i1, *p, *q, *r, bx2;
    int vx, vy;
    int bx, by;
    struct Layer *layer;
    int vx1 = vx0 + width;
    int vy1 = vy0 + height;

    // 如果刷新的范围超出了屏幕则修正。
    if (vx0 < 0)
    {
        vx0 = 0;
    }
    if (vy0 < 0)
    {
        vy0 = 0;
    }
    if (vx1 > layerctl->xsize)
    {
        vx1 = layerctl->xsize;
    }
    if (vy1 > layerctl->ysize)
    {
        vy1 = layerctl->ysize;
    }

    for (int h = h0; h <= h1; h++)
    {
        layer = layerctl->layers[h];
        buf = layer->buf;
        sid = layer - layerctl->layers0;
        // 屏幕坐标-图层坐标=图层内的坐标
        bx0 = vx0 - layer->vx0;
        by0 = vy0 - layer->vy0;
        bx1 = vx1 - layer->vx0;
        by1 = vy1 - layer->vy0;
        // 图层内的坐标不能超出图层的范围。
        if (bx0 < 0)
        {
            bx0 = 0;
        }
        if (by0 < 0)
        {
            by0 = 0;
        }
        if (bx1 > layer->xsize)
        {
            bx1 = layer->xsize;
        }
        if (by1 > layer->ysize)
        {
            by1 = layer->ysize;
        }

        /* while (1)
        {
            asm_hlt();
        } */

        for (by = by0; by < by1; by++)
        {
            vy = layer->vy0 + by; // 屏幕纵坐标。
            for (bx = bx0; bx < bx1; bx++)
            {
                vx = layer->vx0 + bx; // 屏幕横坐标。
                if (map[vy * layerctl->xsize + vx] == sid)
                {
                    gram[vy * layerctl->xsize + vx] = buf[by * layer->xsize + bx];
                }
            }
        }
    }
    return;
}

// 刷新图层map中指定的矩形区域，从高度最低为h0的图层开始往上判断。
void layer_refreshmap(int vx0, int vy0, int width, int height, int h0)
{
    int bx0, by0;
    int bx1, by1;
    unsigned char sid, *map = layerctl->map;
    int *buf;
    int sid4;
    struct Layer *layer;
    int vx1 = vx0 + width;
    int vy1 = vy0 + height;
    if (vx0 < 0)
    {
        vx0 = 0;
    }
    if (vy0 < 0)
    {
        vy0 = 0;
    }
    if (vx1 > layerctl->xsize)
    {
        vx1 = layerctl->xsize;
    }
    if (vy1 > layerctl->ysize)
    {
        vy1 = layerctl->ysize;
    }
    /* if (h0 == 0)
    {
        sid = layer_back - ctl->layers0;
        sid4 = sid | sid << 8 | sid << 16 | sid << 24;
        int map_int_len = ctl->xsize * ctl->ysize / 4;
        int *p = (int *)map;
        for (int i = 0; i < map_int_len; i++)
        {
            p[i] = sid4;
        }
        h0 = 1;
    } */
    for (int h = h0; h <= layerctl->top; h++)
    {
        layer = layerctl->layers[h];
        sid = layer - layerctl->layers0; // 将进行减法计算的地址作为图层号码使用。sid应该就是该图层的数组下标。
        buf = layer->buf;
        bx0 = vx0 - layer->vx0;
        by0 = vy0 - layer->vy0;
        bx1 = vx1 - layer->vx0;
        by1 = vy1 - layer->vy0;
        if (bx0 < 0)
        {
            bx0 = 0;
        }
        if (by0 < 0)
        {
            by0 = 0;
        }
        if (bx1 > layer->xsize)
        {
            bx1 = layer->xsize;
        }
        if (by1 > layer->ysize)
        {
            by1 = layer->ysize;
        }
        if (layer->col_inv == -1)
        { // 无透明色图层专用的高速版

            if ((layer->vx0 & 3) == 0 && (bx0 & 3) == 0 && (bx1 & 3) == 0)
            {                          // 无透明色图层专用的高速版（4字节型）
                bx1 = (bx1 - bx0) / 4; // mov次数
                sid4 = sid | sid << 8 | sid << 16 | sid << 24;
                for (int by = by0; by < by1; by++)
                {
                    int vy = layer->vy0 + by;
                    int vx = layer->vx0 + bx0;
                    int *p = ((int *)&map[vy * layerctl->xsize + vx]);
                    for (int bx = 0; bx < bx1; bx++)
                    {
                        p[bx] = sid4;
                    }
                }
            }
            else
            { // 无透明色图层专用的高速版（1字节型）
                for (int by = by0; by < by1; by++)
                {
                    int vy = layer->vy0 + by;
                    for (int bx = bx0; bx < bx1; bx++)
                    {
                        int vx = layer->vx0 + bx;
                        map[vy * layerctl->xsize + vx] = sid;
                    }
                }
            }
        }
        else
        { // 有透明色图层用的普通版
            for (int by = by0; by < by1; by++)
            {
                int vy = layer->vy0 + by;
                for (int bx = bx0; bx < bx1; bx++)
                {
                    int vx = layer->vx0 + bx;
                    int c = buf[by * layer->xsize + bx];
                    if (c != layer->col_inv)
                    {
                        map[vy * layerctl->xsize + vx] = sid;
                    }
                }
            }
        }
    }
}

// 将图层指定矩形区域的一种颜色更换为另一种颜色，主要用来改变背景色。
void change_color(struct Layer *layer, int x0, int y0, int width, int height, int old_color, int new_color)
{
    int x1 = x0 + width;
    int y1 = y0 + height;
    if (x0 < 0)
    {
        x0 = 0;
    }
    if (y0 < 0)
    {
        y0 = 0;
    }
    if (x1 >= layer->xsize)
    {
        x1 = layer->xsize - 1;
    }
    if (y1 >= layer->ysize)
    {
        y1 = layer->ysize - 1;
    }

    int *buf = layer->buf;
    int xsize = layer->xsize;
    for (int y = y0; y < y1; y++)
    {
        for (int x = x0; x < x1; x++)
        {
            int c = buf[y * xsize + x];
            if (c == old_color)
            {
                buf[y * xsize + x] = new_color;
            }
        }
    }
    layer_refresh(layer, x0, y0, width, height);
}

// 修改图层高度。
void layer_updown(struct Layer *layer, int height)
{
    int h, old = layer->height; // 存储修改前的高度。
    // 如果指定的高度过高或过低，则进行修正。
    if (height > layerctl->top + 1)
    {
        height = layerctl->top + 1;
    }
    if (height < -1)
    {
        height = -1;
    }
    layer->height = height; // 设定高度。
    // 下面主要是进行layers[]的重新排列。
    if (old > height) // 比以前低。
    {
        if (height >= 0)
        {
            // 把old和height中间的层往上提一层。
            for (h = old; h > height; h--)
            {
                layerctl->layers[h] = layerctl->layers[h - 1];
                layerctl->layers[h]->height = h;
            }
            layerctl->layers[height] = layer;
            layer_refreshmap(layer->vx0, layer->vy0, layer->xsize, layer->ysize, height + 1);
            layer_refreshsub(layer->vx0, layer->vy0, layer->xsize, layer->ysize, height + 1, old);
        }
        else // 隐藏
        {
            if (layerctl->top > old)
            {
                // 把old上面的图层都降一层。
                for (h = old; h < layerctl->top; h++)
                {
                    layerctl->layers[h] = layerctl->layers[h + 1];
                    layerctl->layers[h]->height = h;
                }
            }
            layerctl->top--; // 由于显示中的图层减少了一个，所以最上面的图层高度下降。
            layer_refreshmap(layer->vx0, layer->vy0, layer->xsize, layer->ysize, 0);
            layer_refreshsub(layer->vx0, layer->vy0, layer->xsize, layer->ysize, 0, old - 1);
        }
    }
    else if (old < height) // 比以前高。
    {
        if (old >= 0)
        {
            // 把old和height中间的层下拉一层。
            for (h = old; h < height; h++)
            {
                layerctl->layers[h] = layerctl->layers[h + 1];
                layerctl->layers[h]->height = h;
            }
            layerctl->layers[height] = layer;
        }
        else // 由隐藏状态转为显示状态。
        {
            // 将height及以上的层上提一层。
            for (h = layerctl->top; h >= height; h--)
            {
                layerctl->layers[h + 1] = layerctl->layers[h];
                layerctl->layers[h + 1]->height = h + 1;
            }
            layerctl->layers[height] = layer;
            layerctl->top++; // 由于已显示的图层增加了1个，所以最上面的图层高度加1。
        }
        layer_refreshmap(layer->vx0, layer->vy0, layer->xsize, layer->ysize, height);

        layer_refreshsub(layer->vx0, layer->vy0, layer->xsize, layer->ysize, height, height);
    }
    return;
}

// 移动图层到指定位置（不改变高度）。
void layer_slide(struct Layer *layer, int vx0, int vy0)
{
    int old_vx0 = layer->vx0;
    int old_vy0 = layer->vy0;
    layer->vx0 = vx0;
    layer->vy0 = vy0;
    if (layer->height >= 0) // 如果是显示状态。
    {
        layer_refreshmap(old_vx0, old_vy0, old_vx0 + layer->xsize, old_vy0 + layer->ysize, 0);
        layer_refreshmap(vx0, vy0, vx0 + layer->xsize, vy0 + layer->ysize, layer->height);
        // 刷新该图层移动前所占的屏幕空间。
        layer_refreshsub(old_vx0, old_vy0, old_vx0 + layer->xsize, old_vy0 + layer->ysize, 0, layer->height - 1);
        // 刷新该图层移动后所占的屏幕空间。
        layer_refreshsub(vx0, vy0, vx0 + layer->xsize, vy0 + layer->ysize, layer->height, layer->height);
    }
    return;
}

// 刷新图层指定矩形部分的屏幕。
void layer_refresh(struct Layer *layer, int bx0, int by0, int width, int height)
{
    if (layer->height >= 0) // 如果正在显示，则按新图层的信息刷新画面。
    {
        layer_refreshsub(layer->vx0 + bx0, layer->vy0 + by0, width, height, layer->height, layer->height);
    }
    return;
}