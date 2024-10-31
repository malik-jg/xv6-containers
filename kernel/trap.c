#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint            ticks;

extern char trampoline[], uservec[], userret[];

// Before we switch to user-level, we need to install our trapframe
// into the trapframe of the process whose page-table we use, and also
// save the trap frame of the process that owns the page-table. Of
// course, if we use our own page-table (as a normal process), then
// this doesn't make any actual changes.
//
// This assumes that we're going to switch to the page-table within
// the current process' `p->pagetable_proc`.
void
trapframe_update_save(void)
{
	struct proc *p = myproc();
	struct proc *pt = p->pagetable_proc;
	struct cpu  *c = mycpu();

        // If we're going to switch to user-level using a trapframe of
        // another process that owns our page-table, We have to save
        // its trapframe as we're going to "borrow it" for some
        // execution. K thx bye!
	if (p != pt) {
		// If we are not the page-table owning process, make sure to
		// save its previous trapframe contents to later be restored.
		c->saved_pagetable_proc_tf = *pt->trapframe;
                // save this, so we can later consistency check it
		c->pagetable_proc = pt;
		// Save our trap-frame's information into the one for the
		// active page-table process
		*pt->trapframe = *p->trapframe;
	}
}

// We want to save the trapframe information for the current process.
// However, if we are currently executing (i.e. if `myproc` is) a
// process that is using *another's* page-table, then we need to both
// save our own trapframe (from the page-table process' trapframe),
// and restore the page-table process' trapframe.
void
trapframe_update_reset(void)
{
	struct proc *p = myproc();
	struct proc *pt = p->pagetable_proc;
	struct cpu  *c = mycpu();

        // If we're current executing a process that doesn't match the
        // process with the active page-table, we have to
        if (p != pt) {
		if (c->pagetable_proc != pt) {
			panic("Upon return from user-level, we found that the owner of the page-table and trap frame, does not match that which was saved.");
		}
		// copy the trapframe's information into the process
		// it actually belongs to -- the current process
		// making a transition into the kernel!
		*p->trapframe = *pt->trapframe;
		// Restore the trapframe for the process that owns the
		// page-table.
		*pt->trapframe = c->saved_pagetable_proc_tf;
		c->saved_pagetable_proc_tf = (struct trapframe) { 0 };
		c->pagetable_proc = 0;
	}
}

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
	initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
	w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
	int which_dev = 0;

	if ((r_sstatus() & SSTATUS_SPP) != 0) panic("usertrap: not from user mode");

	// send interrupts and exceptions to kerneltrap(),
	// since we're now in the kernel.
	w_stvec((uint64)kernelvec);

	struct proc *p = myproc();

        // make sure that the process' trapframe information is
        // available for use, and that we restore the
        // page-table-owning process' trapframe.
        //
        // The p->trapframe shouldn't be accessed before this.
        trapframe_update_reset();

	// save user program counter.
	p->trapframe->epc = r_sepc();

	if (r_scause() == 8) {
		// system call

		if (killed(p)) exit(-1);

		// sepc points to the ecall instruction,
		// but we want to return to the next instruction.
		p->trapframe->epc += 4;

		// an interrupt will change sepc, scause, and sstatus,
		// so enable only now that we're done with those registers.
		intr_on();

		syscall();
	} else if ((which_dev = devintr()) != 0) {
		// ok
	} else {
		printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
		printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
		setkilled(p);
	}

	if (killed(p)) exit(-1);

	// give up the CPU if this is a timer interrupt.
	if (which_dev == 2) yield();

	usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
	struct proc *p = myproc();

	// we're about to switch the destination of traps from
	// kerneltrap() to usertrap(), so turn off interrupts until
	// we're back in user space, where usertrap() is correct.
	intr_off();

	// send syscalls, interrupts, and exceptions to uservec in trampoline.S
	uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
	w_stvec(trampoline_uservec);

	// set up trapframe values that uservec will need when
	// the process next traps into the kernel.
	p->trapframe->kernel_satp   = r_satp();           // kernel page table
	p->trapframe->kernel_sp     = p->kstack + PGSIZE; // process's kernel stack
	p->trapframe->kernel_trap   = (uint64)usertrap;
	p->trapframe->kernel_hartid = r_tp(); // hartid for cpuid()

	// set up the registers that trampoline.S's sret will use
	// to get to user space.

	// set S Previous Privilege mode to User.
	unsigned long x = r_sstatus();
	x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
	x |= SSTATUS_SPIE; // enable interrupts in user mode
	w_sstatus(x);

	// set S Exception Program Counter to the saved user pc.
	w_sepc(p->trapframe->epc);

        // Make sure that the our process' data is present in the
        // trapframe of the process that owns the page table (in the
        // case of multiple threads). Don't forget that you have to
        // setup the ->pagetable_proc within the process to properly
        // point to the proc that owns the address space.
        trapframe_update_save();
	// tell trampoline.S the user page table to switch to.
	uint64 satp = MAKE_SATP(p->pagetable);

	// jump to userret in trampoline.S at the top of memory, which
	// switches to the user page table, restores user registers,
	// and switches to user mode with sret.
	uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
	((void (*)(uint64))trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void
kerneltrap()
{
	int    which_dev = 0;
	uint64 sepc      = r_sepc();
	uint64 sstatus   = r_sstatus();
	uint64 scause    = r_scause();

	if ((sstatus & SSTATUS_SPP) == 0) panic("kerneltrap: not from supervisor mode");
	if (intr_get() != 0) panic("kerneltrap: interrupts enabled");

	if ((which_dev = devintr()) == 0) {
		// interrupt or trap from an unknown source
		printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
		panic("kerneltrap");
	}

	// give up the CPU if this is a timer interrupt.
	if (which_dev == 2 && myproc() != 0) yield();

	// the yield() may have caused some traps to occur,
	// so restore trap registers for use by kernelvec.S's sepc instruction.
	w_sepc(sepc);
	w_sstatus(sstatus);
}

void
clockintr()
{
	if (cpuid() == 0) {
		acquire(&tickslock);
		ticks++;
		wakeup(&ticks);
		release(&tickslock);
	}

	// ask for the next timer interrupt. this also clears
	// the interrupt request. 1000000 is about a tenth
	// of a second.
	w_stimecmp(r_time() + 1000000);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
	uint64 scause = r_scause();

	if (scause == 0x8000000000000009L) {
		// this is a supervisor external interrupt, via PLIC.

		// irq indicates which device interrupted.
		int irq = plic_claim();

		if (irq == UART0_IRQ) {
			uartintr();
		} else if (irq == VIRTIO0_IRQ) {
			virtio_disk_intr();
		} else if (irq) {
			printf("unexpected interrupt irq=%d\n", irq);
		}

		// the PLIC allows each device to raise at most one
		// interrupt at a time; tell the PLIC the device is
		// now allowed to interrupt again.
		if (irq) plic_complete(irq);

		return 1;
	} else if (scause == 0x8000000000000005L) {
		// timer interrupt.
		clockintr();
		return 2;
	} else {
		return 0;
	}
}
