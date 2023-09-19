#include "common.h"

/*
loader                  0x1000~0x1fff   max 4K
kernel                  0x2000~0x1ffff  max 120K
kernel_page_directory   0x20000~0x20fff 4K
kernel_page_table       0x21000~0x31fff max 68K 最多映射68M内存（64M主存+4M显存）。
physical_address_bitmap         0x40000~0x407ff 2K 管理63M物理内存，最低1M内存不归内存管理程序管理。
kernel_virtual_address_bitmap   0x40800~0x40fff 2K 
*/

//位图
#define PM_BM_ADDR 0x40000   //物理内存位图地址（管理范围：0x100000~0x3fffffff。管理63M物理内存，最低1M内存不归内存管理程序管理。）
#define PM_BM_SIZE 0x800     //物理内存位图大小
#define K_VM_BM_ADDR 0x40800 //内核虚拟内存位图地址（管理范围：0xc0100000~0xc3ffffff。管理63M虚拟内存）
#define K_VM_BM_SIZE 0x800   //内核虚拟内存位图大小

#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP)) //除法的向上取整。

struct Pool pm_pool; //物理内存池
struct Heap k_heap;  //内核堆

//将指定的内存池初始化
void init_pool(struct Pool *pool, unsigned int addr_start, unsigned int size, void *bits)
{
    pool->addr_start = addr_start;
    pool->size = size;
    pool->bitmap.size = size / PG_SIZE / 8;
    pool->bitmap.bits = bits;
    init_bitmap(&pool->bitmap);
}

//初始化内存管理
void init_memory(void)
{
    init_pool(&pm_pool, 0x100000, 0x3f00000, (void *)PM_BM_ADDR); //63M物理内存归内存池管理。
    init_pool(&k_heap.vm_pool, 0xc0100000, 0x3f00000, (void *)K_VM_BM_ADDR);
}

/*在pool指向的内存池中分配pg_cnt个页，成功则返回页框的地址，失败则返回NULL*/
void *palloc(struct Pool *pool, int pg_cnt)
{
    int bit_idx = bitmap_scan(&pool->bitmap, pg_cnt);
    if (bit_idx == -1)
    {
        return NULL;
    }
    for (int i = 0; i < pg_cnt; i++)
    {
        bitmap_set(&pool->bitmap, bit_idx + i, 1); //将此位bit_idx置1，表示已占用。
    }
    unsigned int pg_addr = ((bit_idx * PG_SIZE) + pool->addr_start);
    return (void *)pg_addr;
}

//释放指定内存池中的连续多个内存页
void pfree(struct Pool *pool, void *pg_addr, int pg_cnt)
{
    unsigned int bit_idx = ((unsigned int)pg_addr - pool->addr_start) / PG_SIZE;
    for (int i = 0; i < pg_cnt; i++)
    {
        bitmap_set(&pool->bitmap, bit_idx + i, 0); //将此位bit_idx置0，表示没有占用。
    }
}

//释放内存页
void free_pages(struct Pool *vm_pool, void *pg_addr, int pg_cnt)
{
    pfree(vm_pool, pg_addr, pg_cnt); //先释放虚拟页
    //后释放物理页
    int cnt = pg_cnt;
    unsigned int vaddr = (unsigned int)pg_addr;
    while (cnt--)
    {
        unsigned int pg_phy_addr = addr_v2p(vaddr);
        if (pg_phy_addr == 0)
        { //申请内存时，物理内存不够，程序回滚才会进入这里。
            return;
        }
        pfree(&pm_pool, (void *)pg_phy_addr, 1); //物理页要一页一页的释放。
        page_table_pte_remove(vaddr);
        vaddr += PG_SIZE;
    }
}

/* 从内存池中申请pg_cnt个页内存并清零，成功则返回其虚拟地址，失败则返回NULL */
void *alloc_pages(struct Pool *vm_pool, int pg_cnt)
{
    void *vaddr_start = palloc(vm_pool, pg_cnt);
    if (vaddr_start == NULL)
    {
        return NULL;
    }
    int cnt = pg_cnt;
    unsigned int vaddr = (unsigned int)vaddr_start;
    while (cnt--)
    {
        //物理页要一页一页申请，这样才能体现出分页的其中一个优点：虚拟地址连续而物理地址可以不连续，减少内存碎片，充分利用内存。
        void *page_phyaddr = palloc(&pm_pool, 1);
        if (page_phyaddr == NULL)
        { //失败时要将曾经已申请的虚拟地址和物理页全部回滚。
            free_pages(vm_pool, vaddr_start, pg_cnt);
            return NULL;
        }
        page_table_add((void *)vaddr, page_phyaddr); //在页表中做映射
        vaddr += PG_SIZE;
    }
    memset(vaddr_start, 0, pg_cnt * PG_SIZE); //将申请到的内存清0。
    return vaddr_start;
}

/* 从内核内存池中申请pg_cnt个页内存并清零，成功则返回其虚拟地址，失败则返回NULL */
void *alloc_kernel_pages(int pg_cnt)
{
    return alloc_pages(&k_heap.vm_pool, pg_cnt);
}

void free_kernel_pages(void *pg_addr, int pg_cnt)
{
    free_pages(&k_heap.vm_pool, pg_addr, pg_cnt);
}

//新增堆记录，新增成功返回vaddr，新增失败返回NULL。
void *heap_record_add(struct Heap *heap, unsigned int *vaddr, unsigned int pg_cnt)
{
    if (vaddr == NULL)
    {
        return NULL;
    }
    for (int i = 0; i < HEAP_BLOCK_MAX_NUM; i++)
    {
        if (heap->block_record[i].vaddr == NULL) //找1条空白记录用来记录新数据。
        {
            heap->block_record[i].vaddr = vaddr;
            heap->block_record[i].pg_cnt = pg_cnt;
            return vaddr;
        }
    }
    return NULL; //没有空闲的记录项来记录新数据
}

//查找堆记录，没有找到返回NULL。
struct MemBlock *heap_record_find(struct Heap *heap, unsigned int *vaddr)
{
    for (int i = 0; i < HEAP_BLOCK_MAX_NUM; i++)
    {
        if (heap->block_record[i].vaddr == vaddr)
        {
            return &(heap->block_record[i]); //找到了。
        }
    }
    return NULL; //没找到。
}

//删除堆记录
void heap_record_del(struct MemBlock *mem_block)
{
    mem_block->pg_cnt = 0;
    mem_block->vaddr = NULL;
}

//在堆中申请size字节内存并清零，成功返回虚拟地址，失败返回NULL。
void *malloc(unsigned int size, struct Heap *heap)
{
    if (size <= 0 || size > pm_pool.size)
    {
        return NULL;
    }
    unsigned int pg_cnt = DIV_ROUND_UP(size, PG_SIZE); //向上取整需要的页框数
    unsigned int *vaddr = alloc_pages(&heap->vm_pool, pg_cnt);
    if (vaddr != NULL)
    {
        void *ret = heap_record_add(heap, vaddr, pg_cnt);
        if (ret == NULL) //堆记录已满，无法分配新内存，需要回滚。
        {
            free_pages(&heap->vm_pool, vaddr, pg_cnt);
            return NULL;
        }
    }
    return vaddr;
}

//在内核堆中申请size字节内存并清零，成功返回虚拟地址，失败返回NULL。
void *kmalloc(unsigned int size)
{
    return malloc(size, &k_heap);
}

//在用户堆中申请size字节内存并清零，成功返回虚拟地址，失败返回NULL。
void *umalloc(unsigned int size)
{
    struct Task *task = task_now();
    return malloc(size, task->heap);
}

//释放堆内存
void free(void *vaddr, struct Heap *heap)
{
    if (vaddr == 0)
    {
        return;
    }
    struct MemBlock *memBlock = heap_record_find(heap, vaddr);
    if (memBlock == NULL)
    {
        return;
    }
    free_pages(&heap->vm_pool, vaddr, memBlock->pg_cnt);
    heap_record_del(memBlock);
}

//释放内核堆内存
void kfree(void *vaddr)
{
    free(vaddr, &k_heap);
}

//释放用户堆内存（仅供用户任务调用）
void ufree(void *vaddr)
{
    struct Task *task = task_now();
    free(vaddr, task->heap);
}