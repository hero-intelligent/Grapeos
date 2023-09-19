#include "common.h"

#define AR_TSS32 0x0089 // 386以上的TSS
#define TSS_GDT0 5      // 定义从GDT的几号开始分配给TSS

struct TaskControl // 任务控制
{
    int running;                   // 处于活动状态的任务数量
    int now;                       // 这个变量用来记录当前正在运行的是哪个任务，tasks数组的下标。
    struct Task *tasks[MAX_TASKS]; // 处于活动状态的任务数组。
    struct Task tasks0[MAX_TASKS]; // 所以任务的数组。
};

struct Task *main_task;   // 内核主任务
struct Timer *task_timer; // 任务切换定时器
struct TaskControl *taskctl;

// 获得现在活动的任务。
struct Task *task_now(void)
{
    return taskctl->tasks[taskctl->now];
}

// 执行下一个任务
void goto_next_task(struct Task *next_task)
{
    timer_set_time(task_timer, next_task->time_slice);
    farjmp(0, next_task->selector);
}

// 按一定规则自动切换任务。
void task_switch(void)
{
    struct Task *now_task = taskctl->tasks[taskctl->now];
    taskctl->now++; // 下一个任务
    if (taskctl->now == taskctl->running)
    { // 如果已经是最后一个任务，重新轮到第1个任务。
        taskctl->now = 0;
    }
    struct Task *next_task = taskctl->tasks[taskctl->now];
    if (next_task != now_task) // 如果只有一个处于活动状态的任务，则不进行切换。
    {
        goto_next_task(next_task);
    }
    return;
}

/**申请一个新任务
 * 返回任务结构体的地址，返回0表示没有空闲不用的任务，申请失败。
 */
struct Task *task_alloc(void)
{
    struct Task *task;
    int *fifo_buf = kmalloc(1024);
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (taskctl->tasks0[i].status == 0)
        {
            task = &taskctl->tasks0[i];
            task->status = 1;
            task->time_slice = 2; // 默认值
            task->tss.esp0 = 0;
            task->tss.ss0 = 0;
            task->tss.cr3 = 0;
            task->tss.eip = 0;
            task->tss.eflags = 0x00000202; // IF=1
            task->tss.eax = 0;
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.esp = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.cs = 0;
            task->tss.ss = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.iomap = sizeof(struct TSS);
            task->layers[0] = 0;
            task->layers[1] = 0;
            task->layers[2] = 0;
            init_fifo(&task->fifo, 1024, fifo_buf, task);
            task->exit_func = 0;
            task->heap = 0;
            task->app_addr = 0;
            task->page_dir = 0;
            task->page_table = 0;
            task->opened_file = 0;
            memset(task->should_open_filename, 0, 8);
            return task;
        }
    }
    return 0; // 全部正在使用
}

// 添加一个任务到活动任务队列中（添加到tasks数组中）
void task_add(struct Task *task)
{
    if (task == 0 || task->status != 1)
    {
        return;
    }
    disable_interrupt();
    taskctl->tasks[taskctl->running] = task;
    taskctl->running++;
    task->status = 2;
    recover_interrupt();
    return;
}

/**删除一个任务（从tasks数组中删除）
 * task 准备删除的任务
 */
void task_remove(struct Task *task)
{
    if (task == 0 || task->status != 2)
    {
        return;
    }
    disable_interrupt();
    int i;
    // 寻找task所在的位置
    for (i = 0; i < taskctl->running; i++)
    {
        if (taskctl->tasks[i] == task)
        {
            break; // 在这里
        }
    }
    // 被删除任务后面的任务都向前移动1位。
    for (int j = i; j < taskctl->running - 1; j++)
    {
        taskctl->tasks[j] = taskctl->tasks[j + 1];
    }
    task->status = 1;
    taskctl->running--;
    recover_interrupt();
    if (taskctl->now > i)
    {
        taskctl->now--;
    }
    else if (taskctl->now == i) // 如果是删除当前任务自己，需要切换任务。
    {
        if (taskctl->now == taskctl->running)
        { // 如果已经是最后一个任务，重新轮到第1个任务。
            taskctl->now = 0;
        }
        struct Task *next_task = task_now();
        goto_next_task(next_task);
    }
    return;
}

// 休眠任务
void task_sleep(struct Task *task)
{
    task_remove(task);
}

// 释放任务
void task_free(struct Task *task)
{
    task_remove(task);
    task->status = 0;
}

/**开始一个内核新任务
 * entry_func 新任务的入口函数
 * exit_func 退出时需要执行的函数
 * 返回任务指针或0，0表示失败。
 */
struct Task *new_kernel_task(void *entry_func, void *exit_func)
{
    struct Task *task = task_alloc();
    if (task != 0)
    {
        int cr3 = KERNEL_PAGE_DIR_ADDR;
        int eip = (int)entry_func;
        int esp = (unsigned int)kmalloc(PG_SIZE) + PG_SIZE - 4;
        int cs = SELECTOR_K_CODE;
        int ds = SELECTOR_K_DATA;
        task->tss.cr3 = cr3;
        task->tss.eip = eip;
        task->tss.esp = esp;
        task->tss.es = ds;
        task->tss.cs = cs;
        task->tss.ss = ds;
        task->tss.ds = ds;
        task->tss.fs = ds;
        task->tss.gs = ds;
        task->exit_func = exit_func;
        task_add(task);
    }
    return task;
}

/**开始一个用户新任务
 * file 要执行的exe文件
 * should_open_filename 用户任务启动后需要打开的文件
 * 返回任务指针或0，0表示失败。
 */
struct Task *new_user_task(struct File *file, char *should_open_filename)
{
    void *file_addr = kmalloc(file->size);
    read_file(file, file_addr);
    unsigned int page_dir = 0;
    unsigned int page_table = 0;
    unsigned int user_stack_in_kernel = 0;
    unsigned int pd_phy_addr = create_user_page(file->size, file_addr, &page_dir, &page_table, &user_stack_in_kernel);
    struct Heap *heap = kmalloc(sizeof(struct Heap));
    unsigned int heap_start_addr = ((file->size + 0xfff) & 0xfffff000) + 0x400000; // 4KB向上取整
    // 堆大小=8M-堆起始地址-栈大小(4K);
    unsigned int heap_size = 0x800000 - heap_start_addr - 0x1000;
    void *bits = kmalloc(heap_size / PG_SIZE / 8);
    init_pool(&heap->vm_pool, heap_start_addr, heap_size, bits);
    struct Task *task = task_alloc();
    if (task != 0)
    {
        int esp0 = (unsigned int)kmalloc(PG_SIZE) + PG_SIZE;
        int ss0 = SELECTOR_K_DATA;
        int cr3 = pd_phy_addr;
        int eip = 0x400000; // 4M
        int esp = 0x800000; // 8M
        int cs = SELECTOR_U_CODE;
        int ds = SELECTOR_U_DATA;
        task->tss.esp0 = esp0;
        task->tss.ss0 = ss0;
        task->tss.cr3 = cr3;
        task->tss.eip = eip;
        task->tss.esp = esp;
        task->tss.es = ds;
        task->tss.cs = cs;
        task->tss.ss = ds;
        task->tss.ds = ds;
        task->tss.fs = ds;
        task->tss.gs = ds;
        task->heap = heap;
        task->app_addr = file_addr; // kfree(file_addr);不能在这里释放file_addr，应该在退出用户任务时释放。
        task->page_dir = (void *)page_dir;
        task->page_table = (void *)page_table;
        task->user_stack_in_kernel = (void *)user_stack_in_kernel;
        if (should_open_filename != 0)
        {
            memcpy(task->should_open_filename, should_open_filename, 8);
        }
        task_add(task);
    }
    return task;
}

// 退出任务
void exit_task(struct Task *task)
{
    struct Task *now_task = task_now();
    if (task->exit_func != 0)
    {
        (*task->exit_func)();
    }
    for (int i = 0; i < MAX_LAYERS_PER_TASK; i++)
    {
        layer_free(task->layers[i]);
    }
    kfree(task->fifo.buf);
    close_file();
    if (task->app_addr == 0) // 内核任务
    {
        int stack = task->tss.esp & 0xfffff000;
        kfree((void *)stack);
    }
    else // 用户任务
    {
        /* for (int i = 0; i < HEAP_BLOCK_MAX_NUM; i++)
        {
            void *vaddr = task->heap->block_record[i].vaddr;
            if (vaddr != 0)
            {
                free(vaddr, task->heap); //释放用户任务申请的但没有释放的内存
            }
        } */
        // 释放用户堆里没有释放的物理内存
        int user_heap_pte_start_index = (task->heap->vm_pool.addr_start - 0x400000) / 0x1000;
        int user_heap_pte_count = task->heap->vm_pool.size / 0x1000;
        for (int i = 0; i < user_heap_pte_count; i++)
        {
            int phy_addr = (*(task->page_table + user_heap_pte_start_index + i)) & 0xfffff000;
            if (phy_addr != 0)
            {
                pfree(&pm_pool, (void *)phy_addr, 1);
            }
        }
        kfree((void *)task->tss.esp0);
        kfree(task->heap->vm_pool.bitmap.bits);
        kfree(task->heap);
        kfree(task->app_addr);             // 释放用户程序所占的内存
        kfree(task->user_stack_in_kernel); // 释放栈
        kfree(task->page_table);           // 释放页表
        if (task != now_task)
        {
            kfree(task->page_dir);
        }
        else
        { // 最后只剩用户任务的页目录不能在这里释放，因为当前正在使用，需要在内核任务中释放。
            add_wait_free_user_page_dir(task->page_dir);
            fifo_put(&main_task->fifo, 2);
        }
    }
    task_free(task);
}

// 退出当前的任务
void exit_current_task(void)
{
    struct Task *task = task_now();
    exit_task(task);
}

// 初始化任务模块
void init_task(void)
{
    taskctl = (struct TaskControl *)kmalloc(sizeof(struct TaskControl));
    for (int i = 0; i < MAX_TASKS; i++)
    {
        taskctl->tasks0[i].status = 0;
        taskctl->tasks0[i].selector = (TSS_GDT0 + i) * 8;
        set_segmdesc(gdt + TSS_GDT0 + i, sizeof(struct TSS) - 1, (int)&taskctl->tasks0[i].tss, AR_TSS32);
    }

    // 将内核当前运行的程序抽象为第1个任务。
    main_task = task_alloc();
    main_task->tss.cr3 = KERNEL_PAGE_DIR_ADDR;

    task_add(main_task);
    load_tr(main_task->selector);
    timer_set_time(task_timer, main_task->time_slice);
}

// 获取启动后应该打开的文件名称，保存到filename中。
void get_should_open_filename(char *filename)
{
    struct Task *task = task_now();
    memcpy(filename, task->should_open_filename, 8);
}