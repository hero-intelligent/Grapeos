#include "common.h"

#define LIMIT_GDT GDT_DESC_CNT * 8 - 1
#define AR_CODE32_ER_K 0x4098 //Execute-Only
#define AR_DATA32_RW_K 0x4092 //Read/Write
#define AR_CODE32_ER_U 0x40f8 //Execute-Only
#define AR_DATA32_RW_U 0x40f2 //Read/Write

struct SegmentDescriptor gdt[GDT_DESC_CNT]; //全局唯一

//设置段描述符
void set_segmdesc(struct SegmentDescriptor *sd, unsigned int limit, int base, int ar)
{
    if (limit > 0xfffff)
    {
        ar |= 0x8000; /* G_bit = 1 */
        limit >>= 12; //除以4K
    }
    sd->limit_low = limit & 0xffff;
    sd->base_low = base & 0xffff;
    sd->base_mid = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high = ((ar >> 8) & 0xf0) | ((limit >> 16) & 0x0f);
    sd->base_high = (base >> 24) & 0xff;
    return;
}

void init_gdt(void)
{
    /* // GDT初始化
    for (int i = 0; i < GDT_DESC_CNT; i++)
    {
        set_segmdesc(gdt + i, 0, 0, 0);//gdt为全局变量默认全为0，不需要再初始化。
    } */
    set_segmdesc(gdt + 1, 0xffffffff, 0, AR_CODE32_ER_K);
    set_segmdesc(gdt + 2, 0xffffffff, 0, AR_DATA32_RW_K);
    set_segmdesc(gdt + 3, 0xffffffff, 0, AR_CODE32_ER_U);
    set_segmdesc(gdt + 4, 0xffffffff, 0, AR_DATA32_RW_U);
    load_gdtr(LIMIT_GDT, (int)gdt);
}