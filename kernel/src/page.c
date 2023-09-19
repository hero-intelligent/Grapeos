#include "common.h"
// 页表
#define KERNEL_PAGE_DIR_SIZE 0x1000                                          // 内核页目录大小4K
#define KERNEL_PAGE_TABLE_ADDR (KERNEL_PAGE_DIR_ADDR + KERNEL_PAGE_DIR_SIZE) // 内核页表紧跟在页目录后面，必须4K对齐【必须加括号】
#define KERNEL_PAGE_TABLE_SIZE 0x11000                                       // 内核页表大小68K，映射6M主存+4M显存。
#define PG_P_0 0                                                             // present 0表示该页不在物理内存中
#define PG_P_1 1                                                             // present 1表示该页在物理内存中。
#define PG_RW_R 0                                                            // read/write 0表示可读不可写
#define PG_RW_W 2                                                            // read/write 1表示可读可写
#define PG_US_S 0                                                            // user/supervisor 0表示该页属于supervisor级，只允许特权级别为0、1、2的程序可以访问。
#define PG_US_U 4                                                            // user/supervisor 1表示该页属于user级，任意特权级程序都可以访问。
#define KERNEL_PAGE_ATTR (PG_US_S | PG_RW_W | PG_P_1)                        // 内核页目录项和页表项属性
#define USER_PAGE_ATTR (PG_US_U | PG_RW_W | PG_P_1)                          // 用户页目录项和页表项属性
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

void *wait_free_user_page_dirs[10]; // 需要在内核任务中释放的用户任务页目录。

void add_wait_free_user_page_dir(void *page_dir)
{
    for (int i = 0; i < 10; i++)
    {
        if (wait_free_user_page_dirs[i] == 0)
        {
            wait_free_user_page_dirs[i] = page_dir;
            break;
        }
    }
}

// 释放等待释放的用户任务页目录
void free_wait_free_user_page_dirs(void)
{
    for (int i = 0; i < 10; i++)
    {
        if (wait_free_user_page_dirs[i] != 0)
        {
            kfree(wait_free_user_page_dirs[i]);
            wait_free_user_page_dirs[i] = 0;
        }
    }
}

/*得到虚拟地址vaddr对应的pte指针*/
unsigned int *pte_ptr(unsigned int vaddr)
{
    // 高10位都是1，就指向了页目录本身。高10位+中10位+低12位。
    unsigned int *pte = (unsigned int *)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}

/*得到虚拟地址vaddr对应的pde的指针*/
unsigned int *pde_ptr(unsigned int vaddr)
{
    // 高20位都是1，就指向了页表本身。高20位+低12位。
    unsigned int *pde = (unsigned int *)(0xfffff000 + PDE_IDX(vaddr) * 4);
    return pde;
}

/*得到虚拟地址映射到的物理地址，如果没有映射返回0.*/
unsigned int addr_v2p(unsigned int vaddr)
{
    unsigned int *pte = pte_ptr(vaddr);
    if (*pte == 0)
    {
        return 0;
    }
    //(*pte)的值是页表所在的物理页框地址，去掉其低12位的页表属性+虚拟地址vaddr的低12位。
    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

/* 页表中添加虚拟地址_vaddr与物理地址_page_phyaddr的映射 */
void page_table_add(void *_vaddr, void *_page_phyaddr)
{
    unsigned int vaddr = (unsigned int)_vaddr;
    unsigned int page_phyaddr = (unsigned int)_page_phyaddr;
    unsigned int *pde = pde_ptr(vaddr);
    unsigned int *pte = pte_ptr(vaddr);
    int pg_us = PG_US_S;
    if (vaddr < 0xc0000000)
    {
        pg_us = PG_US_U;
    }
    if (*pde == 0) // 如果页目录项不存在，要先创建页目录项，再创建页表项。
    {
        // 页表中用到的页框一律从物理空间分配。
        unsigned int pde_val = (unsigned int)palloc(&pm_pool, 1);
        *pde = (pde_val | pg_us | PG_RW_W | PG_P_1);
        // 刚分配的页表需要先清零一下，避免脏数据。pte高20位指向刚分配的页表的地址。
        memset((void *)((int)pte & 0xfffff000), 0, PG_SIZE);
    }
    if (*pte == 0) // 如果pte不存在则创建
    {
        *pte = (page_phyaddr | pg_us | PG_RW_W | PG_P_1);
    }
}

// 去掉页表中虚拟地址vaddr的映射，只去掉vaddr对应的pte
void page_table_pte_remove(unsigned int vaddr)
{
    unsigned int *pte = pte_ptr(vaddr);
    *pte = 0; // 将页表项pte置0
    do_invlpg((void *)vaddr);
}

/**初始化分页
 * pgram 物理显存地址
 * vgram 虚拟显存地址
 */
void init_page(unsigned int pgram, unsigned int vgram)
{
    // 0.为了避免脏数据，先清空页目录和页表内存。
    memset((void *)KERNEL_PAGE_DIR_ADDR, 0, KERNEL_PAGE_DIR_SIZE + KERNEL_PAGE_TABLE_SIZE);
    // 1.开始创建页目录项PDE
    unsigned int *pde = (unsigned int *)KERNEL_PAGE_DIR_ADDR; // 内核第一个PDE
    *pde = KERNEL_PAGE_TABLE_ADDR + KERNEL_PAGE_ATTR;         // 内核第一个PDE的值 指向第一个PTE
    for (int i = 0; i < 16; i++)
    { // 内核空间64M主存页表(64M=4M*16)
        *(pde + 768 + i) = KERNEL_PAGE_TABLE_ADDR + 0x1000 * i + KERNEL_PAGE_ATTR;
    }
    *(pde + 1023) = KERNEL_PAGE_DIR_ADDR + KERNEL_PAGE_ATTR; // 使最后一个目录项指向目录自己的地址
    // 2.开始创建页表项PTE
    unsigned int *pte = (unsigned int *)KERNEL_PAGE_TABLE_ADDR; // 内核第一个PTE
    // 2.1映射低端1M物理内存，1M=4K*256;
    for (int i = 0; i < 256; i++)
    {
        *(pte + i) = 0x1000 * i + KERNEL_PAGE_ATTR;
    }
    // 2.2映射显存地址
    unsigned int pde_idx_start = vgram >> 22;
    if (pde_idx_start > 768) // 虚拟显存在3G以上地址空间
    {
        unsigned int last_page_addr = KERNEL_PAGE_TABLE_ADDR + KERNEL_PAGE_TABLE_SIZE - 0x1000; // 最后一个页表的地址
        *(pde + pde_idx_start) = last_page_addr + KERNEL_PAGE_ATTR;                             // 映射到最后一个页表
        unsigned int pte_idx = (vgram >> 12) & 0x3ff;                                           // 取显存地址的中间10位
        unsigned int *gpte = (unsigned int *)(pte_idx * 4 + last_page_addr);                    // 显存第一个PTE
        for (int i = 0; i < 768; i++)
        {                                                       // 3M=4K*768 （1024*768分辨率显存共3MB）
            *(gpte + i) = pgram + (i << 12) + KERNEL_PAGE_ATTR; // 左移12位相当于乘以4K。
        }
    }
    open_page(pde);
}

/**创建用户页表及栈 用户空间统一为4M，地址从低到高依次为：用户程序、用户堆、用户栈(4K)。
 * file_size 用户程序文件大小
 * file_addr 用户程序文件在内核空间的地址
 * page_dir 返回页目录内核虚拟地址
 * page_table 返回页表内核虚拟地址
 * user_stack_in_kernel 返回用户栈的内核地址
 * 返回用户页目录物理地址
 */
unsigned int create_user_page(int file_size, void *file_addr, unsigned int *page_dir, unsigned int *page_table, unsigned int *user_stack_in_kernel)
{
    unsigned int *pde_u = kmalloc(PG_SIZE); // 用户页目录
    *page_dir = (unsigned int)pde_u;
    unsigned int pd_phy_addr = addr_v2p((unsigned int)pde_u);   // 用户页目录物理地址
    unsigned int *pde_k = (unsigned int *)KERNEL_PAGE_DIR_ADDR; // 内核页目录
    *pde_u = *pde_k;                                            // 第1个pde中保存低端1M物理内存的映射，必须复制。
    for (int i = 768; i < 1023; i++)
    {
        *(pde_u + i) = *(pde_k + i); // 复制内核空间映射。
    }
    *(pde_u + 1023) = pd_phy_addr + KERNEL_PAGE_ATTR; // 使最后一个目录项指向目录自己的地址
    unsigned int *pte_u = kmalloc(PG_SIZE);
    *page_table = (unsigned int)pte_u;                             // 用户页表，用户程序大小在4M内，1个页表就够了。
    *(pde_u + 1) = addr_v2p((unsigned int)pte_u) + USER_PAGE_ATTR; // 第2个pde指向用户页表
    unsigned int *pte_k = pte_ptr((unsigned int)file_addr);        // 用户程序文件在内核空间的pte
    int pte_num = (file_size + 4095) / 4096;                       // pte数量，也就是文件占多少个页。
    for (int i = 0; i < pte_num; i++)
    {
        *(pte_u + i) = (*(pte_k + i) & 0xfffff000) + USER_PAGE_ATTR;
    }
    void *stack = kmalloc(PG_SIZE); // 为栈分配空间
    *user_stack_in_kernel = (unsigned int)stack;
    unsigned int stack_phy_addr = addr_v2p((unsigned int)stack);
    *(pte_u + 1023) = stack_phy_addr + USER_PAGE_ATTR; // 将栈放到8M空间的顶端
    return pd_phy_addr;
}