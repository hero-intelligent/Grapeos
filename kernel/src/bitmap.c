#include "common.h"

#define BITMAP_MASK 1

/*将位图btmp初始化清零*/
void init_bitmap(struct Bitmap *btmp)
{
    memset(btmp->bits, 0, btmp->size);
}

/*判断bit_idx位是否为1，若为1，则返回true，否则返回false*/
_Bool bitmap_scan_test(struct Bitmap *btmp, unsigned int bit_idx)
{
    unsigned int byte_idx = bit_idx / 8; //向下取整用于索引数组下标
    unsigned int bit_odd = bit_idx % 8;  //取余用于索引数组内的位
    return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}

/*在位图中申请连续cnt个位，成功则返回其起始位下标，失败返回-1。*/
int bitmap_scan(struct Bitmap *btmp, unsigned int cnt)
{
    unsigned int idx_byte = 0; //用于记录空闲位所在的字节。
    while ((0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->size))
    { //0xff表示8个位都是1，没有空闲的位。
        idx_byte++;
    }
    if (idx_byte == btmp->size) //内存池中找不到可用空间
    {
        return -1;
    }
    int idx_bit = 0;
    while ((unsigned char)(BITMAP_MASK << idx_bit) & (btmp->bits[idx_byte])) //找到第1个空闲的位
    {
        idx_bit++;
    }
    int bit_idx_start = idx_byte * 8 + idx_bit; //第1个空闲位在位图内的下标
    if (cnt == 1)
    {
        return bit_idx_start;
    }
    unsigned int bit_left = (btmp->size * 8 - bit_idx_start); //记录还有多少位可以判断
    unsigned int next_bit = bit_idx_start + 1;
    unsigned int count = 1; //用于记录找到的空闲位的个数
    bit_idx_start = -1;     //先将其置为-1，若找不到连续的位就直接返回。
    while (bit_left-- > 0)
    {
        if (!(bitmap_scan_test(btmp, next_bit))) //若next_bit为0
        {
            count++;
        }
        else
        {
            count = 0;
        }
        if (count == cnt)
        {
            bit_idx_start = next_bit - cnt + 1;
            break;
        }
        next_bit++;
    }
    return bit_idx_start;
}

/* 将位图btmp的bit_idx位设置为value，value只能是0或1。 */
void bitmap_set(struct Bitmap *btmp, unsigned int bit_idx, char value)
{
    unsigned int byte_idx = bit_idx / 8; //向下取整用于索引数组下标
    unsigned int bit_odd = bit_idx % 8;  //取余用于索引数组内的位
    if (value)                           //如果value为1
    {
        btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
    }
    else //如果value为0
    {
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
    }
}