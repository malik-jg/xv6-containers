#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
	if (cpuid() == 0) {
		consoleinit();
		printfinit();
		printf("\n");
		printf("xv6 kernel is booting\n");
		printf("\n");
		kinit();            // physical page allocator
		kvminit();          // create kernel page table
		kvminithart();      // turn on paging
		//printf("Before Proc Init \n");
		procinit();         // process table
		//printf("After Proc Init \n");
		//printf("Before trapinit \n");
		
		trapinit();         // trap vectors
		//printf("Before trapinithart \n");

		trapinithart();     // install kernel trap vector
		plicinit();         // set up interrupt controller
		plicinithart();     // ask PLIC for device interrupts
		binit();            // buffer cache
		iinit();            // inode table
		fileinit();         // file table
		virtio_disk_init(); // emulated hard disk
		//printf("Before userinit \n");
		userinit();         // first user process
		//printf("After userinit \n");
		
		__sync_synchronize();
		started = 1;
	} else {
		while (started == 0);
		__sync_synchronize();
		printf("hart %d starting\n", cpuid());
		kvminithart();  // turn on paging
		trapinithart(); // install kernel trap vector
		plicinithart(); // ask PLIC for device interrupts
	}
	//printf("Made it to sched \n");
	scheduler();
}
