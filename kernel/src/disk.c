#include "common.h"

//【该方法IDE硬盘正常，但SATA硬盘会表现为一直忙，无限循环卡在这里。】
void wait_disk_ready()
{
    while (1)
    {
        int data = io_in8(0x1f7);
        if ((data & 0x88) == 0x08) //第3位为1表示硬盘控制器已准备好数据传输，第7位为1表示硬盘忙。
        {
            return;
        }
    }
}

//向硬盘控制器写入起始扇区地址及要读写的扇区数
void select_sector(int lba)
{
    //第1步：设置要读写的扇区数
    io_out8(0x1f2, 1);
    //第2步：将LBA地址存入0x1f3~0x1f6
    io_out8(0x1f3, lba);
    io_out8(0x1f4, lba >> 8);
    io_out8(0x1f5, lba >> 16);
    io_out8(0x1f6, (((lba >> 24) & 0x0f) | 0xe0)); //设置7~4位为1110，表示LBA模式，主盘；3~0位是LBA的27~24位。
}

/**读一个扇区（经测试，一次读多个扇区会有各种问题，需要分开一个扇区一个扇区的读。）
 * lba LBA28扇区号
 * memory_addrress 将数据写入的内存地址
 */
void read_disk_one_sector(int lba, unsigned int memory_addrress)
{
    //等硬盘不忙了再发送命令，否则可能会卡在后面的检测硬盘状态的读取命令上。
    while (io_in8(0x1f7) & 0x80)
        ;
    select_sector(lba);
    //第3步：向0x1f7端口写入读扇区命令0x20
    io_out8(0x1f7, 0x20);
    //第4步：检测硬盘状态
    wait_disk_ready();
    //第5步：从0x1f0端口读数据
    for (int i = 0; i < 256; i++) //每次读取2字节，需要读的次数为256。
    {
        short data = (short)io_in16(0x1f0);
        *((short *)memory_addrress) = data;
        memory_addrress += 2;
    }
}

void write_disk_one_sertor(int lba, unsigned int memory_addrress)
{
    //等硬盘不忙了再发送命令，否则可能会卡在后面的检测硬盘状态的读取命令上。
    while (io_in8(0x1f7) & 0x80)
        ;
    select_sector(lba);
    //第3步：向0x1f7端口写入写扇区命令0x30
    io_out8(0x1f7, 0x30);
    //第4步：检测硬盘状态
    wait_disk_ready();
    //第5步：向0x1f0端口写数据
    for (int i = 0; i < 256; i++) //每次写入2字节，需要写的次数为256。
    {
        short data = *((short *)memory_addrress);
        io_out16(0x1f0, data);
        memory_addrress += 2;
    }
}

/**读取硬盘函数（IDE主通道主盘）
 * lba 起始LBA28扇区号
 * sector_count 读入的扇区数
 * memory_addrress 将数据写入的内存地址
 */
void read_disk(int lba, int sector_count, unsigned int memory_addrress)
{
    for (int i = 0; i < sector_count; i++)
    {
        read_disk_one_sector(lba, memory_addrress);
        lba++;
        memory_addrress += 512;
    }
}

/**写入硬盘函数（IDE主通道主盘）
 * lba 起始LBA28扇区号
 * sector_count 写入的扇区数
 * memory_addrress 将数据写入的内存地址
 */
void write_disk(int lba, int sector_count, unsigned int memory_addrress)
{
    for (int i = 0; i < sector_count; i++)
    {
        write_disk_one_sertor(lba, memory_addrress);
        lba++;
        memory_addrress += 512;
    }
}