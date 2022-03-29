//arch/riscv/kernel/proc.c
#include "proc.h"    //之后修改成为proc.h

extern void __dummy();
extern void dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);
void schedule(void);

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组，所有的线程都保存在此

void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle

    /* YOUR CODE HERE */
    idle = (struct task_struct*)kalloc();
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    current = idle;
    task[0] = idle;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址， `sp` 设置为 该线程申请的物理页的高地址

    /* YOUR CODE HERE */
    for(int i=1;i<NR_TASKS;i++){
        task[i] = (struct task_struct*)kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        task[i]->pid = i;
        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = (char *)task[i] + PGSIZE;
		// printk("The address of thread %d is %x\n",i,(uint64)task[i]);
		// printk("The sp of thread %d is %x\n",i,(uint64)task[i]->thread.sp);
    }

    printk("...proc_init done!\n");
}

void dummy() {
	//printk("dummy\n");
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            // printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
            // [PID = 28] is running! thread space begin at 0xffffffe007fa2000

            printk("[PID = %d] is running! thread space begin at 0x%lx\n",current->pid,current);
        }
    }
}

#ifdef SJF
void schedule(void) {
    /* YOUR CODE HERE */
    /*遍历线程指针数组task(不包括 idle ，即 task[0] )，在所有运行状态 (TASK_RUNNING) 下的线程运行剩余时间最少的线程作为下一个执行的线程。
如果所有运行状态下的线程运行剩余时间都为0，则对 task[1] ~ task[NR_TASKS-1] 的运行剩余时间重新赋值 (使用 rand()) ，之后再重新进行调度。*/
    int i=1;
	int flag=0;
    for(i=1;i<NR_TASKS;i++){
        if(task[i]->counter != 0){
            flag = 1;
        }
    }
    if(flag == 0){
        //说明所有的线程剩余时间均为0
        for(i=1;i<NR_TASKS;i++){
            task[i]->counter = rand();
            printk("SET [PID = %d COUNT = %d]\n",i,task[i]->counter);
        }
        printk("\n");
    }

    i=1;
    int mid=1;      //具有最短的时间的线程id
    int m = 11;
    // while(task[i]->state != TASK_RUNNING){  //将非 RUNNING 状态的进程跳过
    //     i++;
    //     mid++;
    // }
    for(i=mid;i<NR_TASKS;i++){
        if(task[i]->counter < m && task[i]->state == TASK_RUNNING && task[i]->counter != 0){
            mid = i;
            m = task[i]->counter;
        }
    }

    printk("switch to [PID = %d COUNT = %d]\n",mid,task[mid]->counter);
    switch_to(task[mid]);   //跳转到最短作业       //debug
    // printk("end switch to");
}
#endif

#ifdef PRIORITY
void schedule(void) {
    /* YOUR CODE HERE */
	int i=1;
	int flag=0;
    for(i=1;i<NR_TASKS;i++){
        if(task[i]->counter != 0){
            flag = 1;
        }
    }
    if(flag == 0){
        //说明所有的线程剩余时间均为0
        for(i=1;i<NR_TASKS;i++){
            task[i]->counter = rand();
            printk("SET [PID = %d PRIORITY = %d COUNT = %d]\n",i,task[i]->priority,task[i]->counter);
        }
        printk("\n");
    }
	
	int next=1;      //具有maximum priority的线程id
    int m = 0;
	for(i=1;i<NR_TASKS;i++){
        if(task[i]->priority > m && task[i]->state == TASK_RUNNING && task[i]->counter != 0){
            next = i;
            m = task[i]->priority;
        }
    }
	
	
	printk("switch to [PID = %d PRIORITY = %d COUNT = %d ]\n",next,task[next]->priority,task[next]->counter);
    switch_to(task[next]);   //跳转到最短作业       //debug
}
#endif

void do_timer(void) {
    /* 1. 如果当前线程是 idle 线程 或者 当前线程运行剩余时间为0 进行调度 */
    /* 2. 如果当前线程不是 idle 且 运行剩余时间不为0 则对当前线程的运行剩余时间减1 直接返回 */

    /* YOUR CODE HERE */
    // printk("hhh\n");
    if(current -> pid == idle -> pid){
        printk("idle process is running! \n");
        schedule();
    }else{
        current->counter --;
        if(current->counter > 0){
			return;
		}
        else {
			schedule();
		}
    }
}

void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    /*判断下一个执行的线程 next 与当前的线程 current 是否为同一个线程，
    如果是同一个线程，则无需做任何处理，否则调用 __switch_to 进行线程切换。*/
    if(current->pid == next->pid){
        return;
    }else{
        // printk("switch to\n");

        struct task_struct* cur = current;
        current = next;
        //printk("The answer of address is %x\n",(uint64)&(current->thread.sp) - (uint64)(current));
        __switch_to(cur,next);
    }
    
}



