// arch/riscv/kernel/vm.c
#include "defs.h"
#include "printk.h"
#include "mm.h"
#include <string.h>

extern char _stext[];
extern char _etext[];
extern char _srodata[];
extern char _erodata[];
extern char _sdata[];
extern char _edata[];

/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long early_pgtbl[512] __attribute__((__aligned__(0x1000)));

#ifdef DATA_STRUCT
//自定义数据结构
unsigned long page1[512] __attribute__((__aligned__(0x1000)));
unsigned long page2[512] __attribute__((__aligned__(0x1000)));

unsigned long array1[512][512] __attribute__((__aligned__(0x1000)));
unsigned long array2[512][512] __attribute__((__aligned__(0x1000)));
#endif

void setup_vm(void)
{
	#ifdef DATA_STRUCT
	memset(page1, 0x0, PGSIZE);
	memset(page2, 0x0, PGSIZE);

	memset(array1, 0x0, 512*PGSIZE);
	memset(array2, 0x0, 512*PGSIZE);
	
	early_pgtbl[2] = (((uint64)page1 >> 12) << 10) + 1;
	for(int i=0;i<512;i++){
		page1[i] = (((uint64)array1[i] >> 12) << 10) + 1;
	    for(int j=0;j<512;j++){
	        array1[i][j] = ((i*512 + j + (0x80000000 >> 12)) << 10) + 15; //  [ /PPN:44bits/    /perm:10bits/  ]
	    }
	}

	early_pgtbl[384] = (((uint64)page2 >> 12) << 10) + 1;
	for(int i=0;i<512;i++){
		page2[i] = (((uint64)array2[i] >> 12) << 10) + 1;
	    for(int j=0;j<512;j++){
	        array2[i][j] = ((i*512 + j + (0x80000000 >> 12)) << 10) + 15; //  [ /PPN:44bits/    /perm:10bits/  ]
	    }
	}
	#endif
	
	/* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略(25)
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    memset(early_pgtbl, 0x0, PGSIZE);
    early_pgtbl[2] = (0x80000000 >> 2) + 15;
	early_pgtbl[384] = (0x80000000 >> 2) + 15;
}

/* 创建多级页表映射关系 */
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
{
	/*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小     :      对应的4096
    perm 为映射的读写权限:      R:1,W:2,X:4

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    // printk("create_mapping start!\n");

	uint64 va_i, vpn0, vpn1, vpn2;
	// va~va+sz*block  -->  pa~pa+sz*block(映射)
	// printk("sz = %ld\n",sz);

	for (int i = 0; i < (sz + 4095) / 4096; i++)
	{
		va_i = va + (i << 12);
		//分割虚拟地址得到对应的表项序号
		vpn2 = (va_i << 25) >> 55;
		vpn1 = (va_i << 34) >> 55;
		vpn0 = (va_i << 43) >> 55;

		//三级页表
		//第一级
		if (pgtbl[vpn2] % 2 == 0)
		{						
			uint64 tmp = kalloc() - PA2VA_OFFSET;						//说明 V = 0
			
			pgtbl[vpn2] = ((tmp >> 12) << 10) + 1; //  [ |PPN|:44bits |perm|=000:Pointer to next level of page table  /valid = 1/]
			// printk("tmp = %lx\n",pgtbl[vpn2]);
		}
		uint64 *pgt1 = (uint64 *)((pgtbl[vpn2] >> 10) << 12);
		//第二级
		if (pgt1[vpn1] % 2 == 0)
		{											  
			uint64 tmp = kalloc() - PA2VA_OFFSET;						//说明 V = 0
			
			pgt1[vpn1] = ((tmp >> 12) << 10) + 1; //  [ |PPN|:44bits |perm|=000:Pointer to next level of page table  /valid = 1/]
			// printk("pgt1[vpn1] = %lx\n",pgt1[vpn1]);
		}
		uint64 *pgt0 = (uint64 *)(((pgt1[vpn1] >> 10) << 12)+PA2VA_OFFSET);
		//第三级
		// printk("%")
		pgt0[vpn0] = (((pa >> 12) + i) << 10) + (perm); //  [ /PPN:44bits/    /perm:10bits/  ]
		// printk("pgt0[vpn0] = %lx\n",pgt0[vpn0]);
	}
	// printk("create_mapping end!\n")
	//1111 1111 1111 1111 1111 1111 1110 0000 0000 0000 0010 0000 0000 1100 0010 1100

	// if(pgtbl[384]%2){
	// 	uint64 * tmp1 = (uint64 *)((pgtbl[384]>>10)<<12);
	// 	if(tmp1[1]%2){
	// 		uint64 * tmp2=(uint64 *)((tmp1[1]>>10)<<12);
	// 		if(tmp2[0]%2) {
	// 			uint64 temp=((tmp2[0]>>10)<<12);
	// 			printk("%lx\n",temp);
	// 		}
	// 	}
	// }
}



/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final(void)
{
	memset(swapper_pg_dir, 0x0, PGSIZE);

	// No OpenSBI mapping required

	printk("_etext = %lx\n",_etext);
	printk("_stext = %lx\n",_stext);
	printk("_erodata = %lx\n",_erodata);
	printk("_srodata = %lx\n",_srodata);
	printk("_edata = %lx\n",_edata);
	printk("_sdata = %lx\n",_sdata);
	// mapping kernel text X|-|R|V
	create_mapping(swapper_pg_dir, _stext , _stext - PA2VA_OFFSET, _etext - _stext, 11);

	// mapping kernel rodata -|-|R|V
	create_mapping(swapper_pg_dir, _srodata , _srodata - PA2VA_OFFSET, _erodata - _srodata, 3);

	// mapping other memory -|W|R|V
	create_mapping(swapper_pg_dir, _sdata , _sdata - PA2VA_OFFSET, PHY_END + PA2VA_OFFSET - (uint64)_sdata, 7);

	// set satp with swapper_pg_dir
	uint64 tmp = (uint64)swapper_pg_dir - PA2VA_OFFSET;
	// printk("ppn in satp = %lx\n",tmp);

	__asm__ volatile(
		"addi t2,x0,8\n"
    	"slli t2,t2,60\n"
		"mv t3, %[tmp]\n"
		"srli t3,t3,12\n"
    	"add t1,t2,t3\n"
    	"csrw satp,t1\n"
		: 
		: [tmp] "r" (tmp)
		: "memory");

	// printk("Finish of asm!\n");
	// flush TLB
	asm volatile("sfence.vma zero, zero");
	// printk("Finish of setup_vm_final!\n");
	
	return;
}
