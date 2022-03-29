.extern start_kernel

    .section .text.init
    .globl _start
	
_start:
    la sp,boot_stack_top		# 设置C语言调用栈环境
    call setup_vm
    call relocate
    
    call mm_init
    
    call setup_vm_final
ppp:
    call task_init
    
	# ------------------

        # set stvec = _traps
        la t0, _traps
        csrrw x0, stvec, t0

    # ------------------

        # set sie[STIE] = 1
        li t0, 0x20
        csrrs x0, sie, t0

    # ------------------
        # set first time interrupt
        li a0, 5300
        mv a1, x0
        mv a2, x0 
        mv a3, x0
        mv a4, x0
        mv a5, x0
        mv a6, x0
        mv a7, x0
        ecall

    # ------------------

        # set sstatus[SIE] = 1
        li t0, 0x02
        csrrs x0, sstatus, t0
        
    j start_kernel          # 跳转到main.c中的start_kernel

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)

    # PA2VA_OFFSET: FFFFF_FDF80_00000_0
    lui t0,0xFFFFF
    slli t0,t0,32

    lui t1,0xFDF80
    slli t1,t1,12

    or t0,t0,t1
    add ra,ra,t0
    add sp,sp,t0

    # set satp with early_pgtbl
	
    addi t2,x0,8
    slli t2,t2,60
    la t1,early_pgtbl
	srli t1,t1,12
    add t1,t1,t2
    csrw satp,t1

    # flush tlb
    sfence.vma zero, zero

    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size  4KB

    .globl boot_stack_top
boot_stack_top:
