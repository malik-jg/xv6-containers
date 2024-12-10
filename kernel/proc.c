#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// --------------------------------------------------------------------------


// --------------------------------------------------------------------------
// struct prioMap fs_Map = {0};
// struct prioMap* hfs_Map = {0};
// --------------------------------------------------------------------------



struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

// init empty maps for the procs 
struct prioMap fs_Map = {0};
struct prioMap hfs_Map = {0};

int             nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);
// init the enqueue method
extern void enqueue(struct proc *process, int priority);
extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl)
{
	struct proc *p;

	for (p = proc; p < &proc[NPROC]; p++) {
		char *pa = kalloc();
		if (pa == 0) panic("kalloc");
		uint64 va = KSTACK((int)(p - proc));
		kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
	}
}

// initialize the proc table.
void
procinit(void)
{
	struct proc *p;

	initlock(&pid_lock, "nextpid");
	initlock(&wait_lock, "wait_lock");
	for (p = proc; p < &proc[NPROC]; p++) {
		initlock(&p->lock, "proc");
		p->state  = UNUSED;
		// enqueue all procs in the table with prio of 0
		
		p->kstack = KSTACK((int)(p - proc));
		//printf("Calling enqueue\n");
		enqueue(p, 0);
	}
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
	int id = r_tp();
	return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void)
{
	int         id = cpuid();
	struct cpu *c  = &cpus[id];
	return c;
}

// Return the current struct proc *, or zero if none.
struct proc *
myproc(void)
{
	push_off();
	struct cpu  *c = mycpu();
	struct proc *p = c->proc;
	pop_off();
	return p;
}

int
allocpid()
{
	int pid;

	acquire(&pid_lock);
	pid     = nextpid;
	nextpid = nextpid + 1;
	release(&pid_lock);

	return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc *
allocproc(void)
{
	struct proc *p;

	for (p = proc; p < &proc[NPROC]; p++) {
		acquire(&p->lock);
		if (p->state == UNUSED) {
			goto found;
		} else {
			release(&p->lock);
		}
	}
	return 0;

found:
	p->pid   = allocpid();
	p->state = USED;

	// Allocate a trapframe page.
	if ((p->trapframe = (struct trapframe *)kalloc()) == 0) {
		freeproc(p);
		release(&p->lock);
		return 0;
	}

        // GWU additions: For now, assume that this process (thread)
        // always uses its own page-table.
        p->pagetable_proc = p;
	// An empty user page table.
	p->pagetable = proc_pagetable(p);
	if (p->pagetable == 0) {
		freeproc(p);
		release(&p->lock);
		return 0;
	}

	// Set up new context to start executing at forkret,
	// which returns to user space.
	memset(&p->context, 0, sizeof(p->context));
	p->context.ra = (uint64)forkret;
	p->context.sp = p->kstack + PGSIZE;

	return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
	if (p->trapframe) kfree((void *)p->trapframe);
	p->trapframe = 0;
        p->pagetable_proc = 0;
	if (p->pagetable) proc_freepagetable(p->pagetable, p->sz);
	p->pagetable = 0;
	p->sz        = 0;
	p->pid       = 0;
	p->parent    = 0;
	p->name[0]   = 0;
	p->chan      = 0;
	p->killed    = 0;
	p->xstate    = 0;
	p->state     = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
	pagetable_t pagetable;

	// An empty page table.
	pagetable = uvmcreate();
	if (pagetable == 0) return 0;

	// map the trampoline code (for system call return)
	// at the highest user virtual address.
	// only the supervisor uses it, on the way
	// to/from user space, so not PTE_U.
	if (mappages(pagetable, TRAMPOLINE, PGSIZE, (uint64)trampoline, PTE_R | PTE_X) < 0) {
		uvmfree(pagetable, 0);
		return 0;
	}

	// map the trapframe page just below the trampoline page, for
	// trampoline.S.
	if (mappages(pagetable, TRAPFRAME, PGSIZE, (uint64)(p->trapframe), PTE_R | PTE_W) < 0) {
		uvmunmap(pagetable, TRAMPOLINE, 1, 0);
		uvmfree(pagetable, 0);
		return 0;
	}

	return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
	uvmunmap(pagetable, TRAMPOLINE, 1, 0);
	uvmunmap(pagetable, TRAPFRAME, 1, 0);
	uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02, 0x97, 0x05, 0x00, 0x00, 0x93,
                    0x85, 0x35, 0x02, 0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00, 0x93, 0x08,
                    0x20, 0x00, 0x73, 0x00, 0x00, 0x00, 0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e,
                    0x69, 0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Set up first user process.
void
userinit(void)
{
	struct proc *p;

	p        = allocproc();
	initproc = p;

	// allocate one user page and copy initcode's instructions
	// and data into it.
	uvmfirst(p->pagetable, initcode, sizeof(initcode));
	p->sz = PGSIZE;

	// prepare for the very first "return" from kernel to user.
	p->trapframe->epc = 0;      // user program counter
	p->trapframe->sp  = PGSIZE; // user stack pointer

	safestrcpy(p->name, "initcode", sizeof(p->name));
	p->cwd = namei("/");

	p->state = RUNNABLE;
	// enqueue proc into the HFS with priority 0
	//enqueue(p, 0);
	release(&p->lock);
	enqueue(p, 0);
	//printf("userinit\n");
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
	uint64       sz;
	struct proc *p = myproc();

	sz = p->sz;
	if (n > 0) {
		if ((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) { return -1; }
	} else if (n < 0) {
		sz = uvmdealloc(p->pagetable, sz, sz + n);
	}
	p->sz = sz;
	return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
	int          i, pid;
	struct proc *np;
	struct proc *p = myproc();

	// Allocate process.
	if ((np = allocproc()) == 0) { return -1; }

	// Copy user memory from parent to child.
	if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0) {
		freeproc(np);
		release(&np->lock);
		return -1;
	}
	np->sz = p->sz;

	// copy saved user registers.
	*(np->trapframe) = *(p->trapframe);

	// Cause fork to return 0 in the child.
	np->trapframe->a0 = 0;

	// increment reference counts on open file descriptors.
	for (i = 0; i < NOFILE; i++)
		if (p->ofile[i]) np->ofile[i] = filedup(p->ofile[i]);
	np->cwd = idup(p->cwd);

	safestrcpy(np->name, p->name, sizeof(p->name));

	pid = np->pid;

	release(&np->lock);

	acquire(&wait_lock);
	np->parent = p;
	release(&wait_lock);

	acquire(&np->lock);
	np->state = RUNNABLE;
	// enqueue the child with zero priority
	//enqueue(np, 0);
	release(&np->lock);
	enqueue(np, 0);
	return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
	struct proc *pp;

	for (pp = proc; pp < &proc[NPROC]; pp++) {
		if (pp->parent == p) {
			pp->parent = initproc;
			wakeup(initproc);
		}
	}
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
	struct proc *p = myproc();

	if (p == initproc) panic("init exiting");

	// Close all open files.
	for (int fd = 0; fd < NOFILE; fd++) {
		if (p->ofile[fd]) {
			struct file *f = p->ofile[fd];
			fileclose(f);
			p->ofile[fd] = 0;
		}
	}

	begin_op();
	iput(p->cwd);
	end_op();
	p->cwd = 0;

	acquire(&wait_lock);

	// Give any children to init.
	reparent(p);

	// Parent might be sleeping in wait().
	wakeup(p->parent);

	acquire(&p->lock);

	p->xstate = status;
	p->state  = ZOMBIE;

	release(&wait_lock);

	// Jump into the scheduler, never to return.
	sched();
	panic("zombie exit");

	return;
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
	struct proc *pp;
	int          havekids, pid;
	struct proc *p = myproc();

	acquire(&wait_lock);

	for (;;) {
		// Scan through table looking for exited children.
		havekids = 0;
		for (pp = proc; pp < &proc[NPROC]; pp++) {
			if (pp->parent == p) {
				// make sure the child isn't still in exit() or swtch().
				acquire(&pp->lock);

				havekids = 1;
				if (pp->state == ZOMBIE) {
					// Found one.
					pid = pp->pid;
					if (addr != 0
					    && copyout(p->pagetable, addr, (char *)&pp->xstate, sizeof(pp->xstate))
					         < 0) {
						release(&pp->lock);
						release(&wait_lock);
						return -1;
					}
					freeproc(pp);
					release(&pp->lock);
					release(&wait_lock);
					return pid;
				}
				release(&pp->lock);
			}
		}

		// No point waiting if we don't have any children.
		if (!havekids || killed(p)) {
			release(&wait_lock);
			return -1;
		}

		// Wait for a child to exit.
		sleep(p, &wait_lock); // DOC: wait-sleep
	}
}
// --------------------------------------------------------------------------

// search for the next process in the HFS map
// struct proc *getNextProcessHFS() {
//     for (int i = 0; i < PRIOMAX; i++) {
//         if (hfs_Map -> buckets[i]) {
//             struct prioNode *node = hfs_Map -> buckets[i];
//             struct proc *p = node->process;
//             hfs_Map -> buckets[i] = node->next; 
//             freeproc(node -> process);
//             return p;
//         }≠
//     }
//     return (void*)0; 
// }
// // search for the next process in the FS map
// struct proc* getNextProcessFS() {
//     for (int i = 0; i < PRIOMAX; i++) {
//         if (fs_Map.buckets[i]) {
//             // Return the first process found
//             return fs_Map.buckets[i];
//         }
//     }
//     // If no processes are found, return NULL
//     printf("No processes in the map.\n");
//     return (void*)0;
// }

// struct proc* getNextProcessHFS() {
//     for (int i = 0; i < PRIOMAX; i++) {
//         if (hfs_Map.buckets[i]) {
//             // Return the first process found
//             return hfs_Map.buckets[i];
//         }
//     }
//     // If no processes are found, return NULL
//     printf("No processes in the map.\n");
//     return (void*)0;
// }


// int hfsIsEmpty (void) {
// 	for (int i=0; i<PRIOMAX; i++) {
// 		if (hfs_Map.buckets[i]) {
// 			// return 0 if the map is NOT empty
// 			return 0;
// 		}
// 	}
// 	// return 1 if it is empty
// 	return 1;
// }
// int fsIsEmpty (void) {
// 	for (int i=0; i<PRIOMAX; i++) {
// 		if (fs_Map.buckets[i]) {
// 			// return 0 if the map is NOT empty
// 			return 0;
// 		}
// 	}
// 	// return 1 if it is empty
// 	return 1;
// }
// --------------------------------------------------------------------------
void enqueue(struct proc* process, int priority) {
	// sanity check
    if (priority < 0 || priority >= PRIOMAX) {
        //printf("Invalid priority: %d\n", priority);
        return;
    }

    // assign the priority
    process->priority = priority;

    // insert at the front of the respective bucket
    process->next = hfs_Map.buckets[priority];
    hfs_Map.buckets[priority] = process;
   
	
}
// struct proc* dequeue() {
//     // Iterate through priority levels from highest to lowest
//     for (int priority = 0; priority < PRIOMAX; priority++) {
//         struct proc* current = hfs_Map.buckets[priority];

//         // If the current bucket has processes, dequeue the first one
//         if (current != (void*)0) {
//             hfs_Map.buckets[priority] = current->next; // Update head of the list
//             current->next = (void*)0; // Detach dequeued process
//             return current; // Return the dequeued process
//         }
//     }

//     // If all buckets are empty, return NULL
//     return (void*)0;
// }

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.

// MAYBE MAKE A FUNC TO PUT THE ROUND ROBIN PROC AT THE END OF THE BUCKET
void
scheduler(void)
{
	//struct proc *p;
    struct cpu *c = mycpu();

    c->proc = 0;

    for (;;) {
        
        intr_on();

        int found = 0;
		// FS protocol
		for (int priority = 0; priority < PRIOMAX; priority++) {
            // skip to next bucket if this one is empty
            if (fs_Map.buckets[priority] == (void*)0) {
                continue;
            }

            // grab the process in the bucket
            struct proc* current = fs_Map.buckets[priority];

            // get it lock
            acquire(&current->lock);
            if (current->state == RUNNABLE) {
                // take the process out of the bucket
               
                // change its state to RUNNING
                current->state = RUNNING;
                c->proc = current;
				//printf("Running the Process\n");
				//printf("%d\n", priority);
				// printf("PID: ");
				// printf("%d\n", current -> pid);
				// printf("Priority: ");
				// printf("%d\n", proc -> priority);
                
                swtch(&c->context, &current->context);

             
                c->proc = 0;
                found = 1;

            }
            release(&current->lock);

            // exit innerloop if we found a valid process
            if (found) {
                break;
            }
        }

		// HFS protocol
		//------------------------------------------------------------------------
        // go thru the priority buckets
        for (int priority = 0; priority < PRIOMAX; priority++) {
            // skip to next bucket if this one is empty
            if (hfs_Map.buckets[priority] == (void*)0) {
                continue;
            }

            // grab the process in the bucket
            struct proc* current = hfs_Map.buckets[priority];

            // get it lock
            acquire(&current->lock);
            if (current->state == RUNNABLE) {
                // take the process out of the bucket
                hfs_Map.buckets[priority] = current->next;
                current->next = (void*)0;

                // change its state to RUNNING
                current->state = RUNNING;
                c->proc = current;
				//printf("Running the Process\n");
				//printf("%d\n", priority);
				// printf("PID: ");
				// printf("%d\n", current -> pid);
				// printf("Priority: ");
				// printf("%d\n", proc -> priority);
                
                swtch(&c->context, &current->context);

             
                c->proc = 0;
                found = 1;

                // after process runs, demote the priority
                if (priority < PRIOMAX - 1) {
                    // requeue the process back into the map 
                    enqueue(current, priority + 1);
                } else {
                    // if we are at the end of the buckets, place into the last for round robin
                    enqueue(current, priority);
                }
            }
            release(&current->lock);

            // exit innerloop if we found a valid process
            if (found) {
                break;
            }
        }
		//------------------------------------------------------------------------

        if (!found) {
            intr_on();
            asm volatile("wfi");
        }
    }


//-------------------------------------------------------------------------------------
	// struct proc *p;
	// struct cpu  *c = mycpu();

	// c->proc = 0;
	// for (;;) {
	// 	// The most recent process to run may have had interrupts
	// 	// turned off; enable them to avoid a deadlock if all
	// 	// processes are waiting.
	// 	intr_on();

	// 	int found = 0;
	// 	for (p = proc; p < &proc[NPROC]; p++) {
	// 		acquire(&p->lock);
	// 		if (p->state == RUNNABLE) {
	// 			// Switch to chosen process.  It is the process's job
	// 			// to release its lock and then reacquire it
	// 			// before jumping back to us.
	// 			p->state = RUNNING;
	// 			c->proc  = p;
	// 			swtch(&c->context, &p->context);

	// 			// Process is done running for now.
	// 			// It should have changed its p->state before coming back.
	// 			c->proc = 0;
	// 			found   = 1;
	// 		}
	// 		release(&p->lock);
	// 	}
	// 	if (found == 0) {
	// 		// nothing to run; stop running on this core until an interrupt.
	// 		intr_on();
	// 		asm volatile("wfi");
	// 	}
	// }
//-------------------------------------------------------------------------------------

// 	for (;;) {
// 		// The most recent process to run may have had interrupts
// 		// turned off; enable them to avoid a deadlock if all
// 		// processes are waiting.
// 		//printf("in sched for loop\n");
// 		found = 0;
// 		intr_on();
// 		// if there are procs in the FS map, we must run those first
// 		if (fsIsEmpty() == 0) {
// 			//printf("in fs cond\n");
// 			p = getNextProcessFS();
// 			if (p) {
// 				acquire(&p -> lock);
// 				if (p -> state == RUNNABLE) {
// 					//printf("Proc is Running\n");
// 					p -> state = RUNNING;
// 					c -> proc = p;
// 					swtch(&c -> context, &p -> context);
// 					c -> proc = 0;
// 					found = 1;
// 				}
// 				release(&p -> lock);	
// 			}
// 		} 
// 		//if there are no more FS procs, we can run the HFS procs
// 		if (hfsIsEmpty() == 0) {
// 			//printf("in hfs cond\n");
			
// 			p = getNextProcessHFS();
// 			if (p) {
// 			//printf("got the next process\n");

// 				acquire(&p -> lock);
// 				if (p -> state == RUNNABLE) {
// 					printf("Running state\n");
// 					p -> state = RUNNING;
// 					c -> proc = p;
// 					swtch(&c -> context, &p -> context);
// 					// demote the priority
// 					if (p -> priority < PRIOMAX - 1) {
// 						p -> priority ++;
// 					}
// 					//addToMap(p -> priority, p);

// 					c -> proc = 0;
// 					found = 1;
// 				}
// 				release(&p -> lock);	
// 			}
// 		}
// 		if (found == 0) {
// 			// nothing to run; stop running on this core until an interrupt.
// 			intr_on();
// 			asm volatile("wfi");
// 		}
// 		//printf("end of sched\n");
// 	}
		
// // ---------------------------------------------------------------------------------------
	
// ---------------------------------------------------------------------------------------

	
	
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
	int          intena;
	struct proc *p = myproc();

	if (!holding(&p->lock)) panic("sched p->lock");
	if (mycpu()->noff != 1) panic("sched locks");
	if (p->state == RUNNING) panic("sched running");
	if (intr_get()) panic("sched interruptible");

	intena = mycpu()->intena;
	swtch(&p->context, &mycpu()->context);
	mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
	struct proc *p = myproc();
	acquire(&p->lock);
	p->state = RUNNABLE;
	sched();
	release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
	static int first = 1;

	// Still holding p->lock from scheduler.
	release(&myproc()->lock);

	if (first) {
		// File system initialization must be run in the context of a
		// regular process (e.g., because it calls sleep), and thus cannot
		// be run from main().
		fsinit(ROOTDEV);

		first = 0;
		// ensure other cores see first=0.
		__sync_synchronize();
	}

	usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
	struct proc *p = myproc();

	// Must acquire p->lock in order to
	// change p->state and then call sched.
	// Once we hold p->lock, we can be
	// guaranteed that we won't miss any wakeup
	// (wakeup locks p->lock),
	// so it's okay to release lk.

	acquire(&p->lock); // DOC: sleeplock1
	release(lk);

	// Go to sleep.
	p->chan  = chan;
	p->state = SLEEPING;

	sched();

	// Tidy up.
	p->chan = 0;

	// Reacquire original lock.
	release(&p->lock);
	acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
	struct proc *p;

	for (p = proc; p < &proc[NPROC]; p++) {
		if (p != myproc()) {
			acquire(&p->lock);
			if (p->state == SLEEPING && p->chan == chan) { p->state = RUNNABLE; }
			release(&p->lock);
		}
	}
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
	struct proc *p;

	for (p = proc; p < &proc[NPROC]; p++) {
		acquire(&p->lock);
		if (p->pid == pid) {
			p->killed = 1;
			if (p->state == SLEEPING) {
				// Wake process from sleep().
				p->state = RUNNABLE;
			}
			release(&p->lock);
			return 0;
		}
		release(&p->lock);
	}
	return -1;
}

void
setkilled(struct proc *p)
{
	acquire(&p->lock);
	p->killed = 1;
	release(&p->lock);
}

int
killed(struct proc *p)
{
	int k;

	acquire(&p->lock);
	k = p->killed;
	release(&p->lock);
	return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
	struct proc *p = myproc();
	if (user_dst) {
		return copyout(p->pagetable, dst, src, len);
	} else {
		memmove((char *)dst, src, len);
		return 0;
	}
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
	struct proc *p = myproc();
	if (user_src) {
		return copyin(p->pagetable, dst, src, len);
	} else {
		memmove(dst, (char *)src, len);
		return 0;
	}
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
	static char *states[] = {[UNUSED] "unused",   [USED] "used",      [SLEEPING] "sleep ",
	                         [RUNNABLE] "runble", [RUNNING] "run   ", [ZOMBIE] "zombie"};
	struct proc *p;
	char        *state;

	printf("\n");
	for (p = proc; p < &proc[NPROC]; p++) {
		if (p->state == UNUSED) continue;
		if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
			state = states[p->state];
		else
			state = "???";
		printf("%d %s %s", p->pid, state, p->name);
		printf("\n");
	}
}


int prio_get(int pid) {
	struct proc *p;
	for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);  
		if (p -> pid == pid) {
			release(&p->lock);
			// printf("returning the prio from prio_get \n");
			// printf("%d\n", p -> priority);
			return p -> priority;
		}
		release(&p->lock);
	}
	return -1;
}


// scheduling sys calls
int prio_set(int pid, int priority) {
	// ret 0 on SUCCESS | ret -1 on FAIL
	//printf("Printing priority input: ");
	//printf("%d\n", priority);
	
	// init new struct and get the current process struct
	struct proc *p;
	struct proc *current = myproc();
    // iterate thru the procs, see if we find the matching pid 
    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);  
        // if we find match, set the priority
		if (p->pid == pid) {
            //printf("In cond \n");
			//p->priority = priority; 
            struct proc *prev_proc = p;
            //int is_prev = 0;
            // go up the chain of procs
			while (prev_proc) {
				// if the current proc shares pid, we are good
                //printf("in while \n");
				if (prev_proc == current) {
                    //printf("going to break \n");
					//is_prev = 1;
                    break;
                }
                prev_proc = prev_proc->parent; //iterate
            }
			// update priority if condition is met
            if (priority >= current->priority) {
                // printf("Setting Priority To: ");
				// printf("%d\n", p -> priority);

				p->priority = priority; 
                release(&p->lock);
                return 0; 

            } else {
                //printf("ret -1\n");
				release(&p->lock);
                return -1; 
            }
        }
        release(&p->lock);        
    }
	

    return -1; 
}