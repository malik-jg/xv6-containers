#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "stat.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "pstat.h"
#include "sleeplock.h"




struct cpu cpus[NCPU];

struct proc proc[NPROC];



struct mutex all_locks[MAX_MAXNUM];

struct proc *initproc;

int             nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

//shared memory struct


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
		p->kstack = KSTACK((int)(p - proc));
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

//straight from stack overflow regarding how to make strcmp used in mutex_create
int 
strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
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
	p -> cid = 0;
	p -> root = 0;
	p -> root_set = 0;
	p -> maxprocs = 0;
	p -> maxprocs_set = 0;
	p -> in_container = 0;
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

	release(&p->lock);
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
int fork(void) {
    int i, pid;
    struct proc *np;
    struct proc *p = myproc();

    if ((np = allocproc()) == 0) {
        return -1;
    }

	if(p -> cid > 0 && p -> maxprocs > 0){
		struct proc *pp;
		int numprocs = 0;
		for(pp = proc; pp < &proc[NPROC]; pp++){
			if(pp -> cid == p -> cid){
				numprocs++;
			}
		}
		if(numprocs	>= p -> maxprocs){
			return -1;
		}
	}

    if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0) {
        freeproc(np);
        release(&np->lock);
        return -1;
    }
    np->sz = p->sz;

    *(np->trapframe) = *(p->trapframe);

	np -> cid = p -> cid;
	np -> root_set = p -> root_set;
	np -> maxprocs = p -> maxprocs;
	np -> maxprocs_set = p -> maxprocs_set;
	np -> in_container = p -> in_container;

	if(p -> root_set){
		np -> root = idup(p -> root);
	}
	else{
		np -> root = 0;
	}
    np->trapframe->a0 = 0;

    for (i = 0; i < NOFILE; i++)
        if (p->ofile[i])
            np->ofile[i] = filedup(p->ofile[i]);
    np->cwd = idup(p->cwd);

	//shmem stuff
	memmove(np->shmems, p->shmems, sizeof(struct proc_shmem) * SHM_MAXNUM);
	np->shmem_count = p->shmem_count;
	np->shmem_alloc = p->shmem_alloc;

	for (int i = 0; i < p->shmem_alloc; i++) {
		if (p->shmems[i].status == 1) {
			shmem_fork(p->shmems[i].name);
			uvmunmap(np->pagetable, p->shmems[i].va, 1, 0);
			mappages(np->pagetable, p->shmems[i].va, PGSIZE, p->shmems[i].pa, PTE_W | PTE_R | PTE_U);
			np->shmems[i].pa = p->shmems[i].pa;
		}
	}

    safestrcpy(np->name, p->name, sizeof(p->name));

    pid = np->pid;

    //Copy mutex table and increment references
    for (int i = 0; i < p->mutex_count; i++) {
       np->mutex_table[i] = p->mutex_table[i];
    	np->mutex_table[i]->referenced_by++;
	}
	np->mutex_count = p->mutex_count;

    release(&np->lock);

    acquire(&wait_lock);
    np->parent = p;
    release(&wait_lock);

    acquire(&np->lock);
    np->state = RUNNABLE;
    release(&np->lock);

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

	//so cv_wait doesnt aboslutely shit itself
	for (int i = 0; i < p->mutex_count; i++) {
        struct mutex *cur_mutex = p->mutex_table[i];
        acquiresleep(&cur_mutex->slock);
        cur_mutex->status = 0;        
        cur_mutex->owner_id = -1;
        wakeup(cur_mutex);            
        releasesleep(&cur_mutex->slock);
    }

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

	p -> cid = 0;
	p -> root = 0;
	p -> root_set = 0;
	p -> maxprocs = 0;
	p -> maxprocs_set = 0;
	p -> in_container = 0;


	acquire(&wait_lock);

	// Give any children to init.
	reparent(p);

	//shmem stuff
	int max = p->shmem_alloc;
	for (int i = 0; i < max; i++) {
		//printf("i: %d EXIT %s\n", i, p->shmems[i].name);
		shm_rem(p->shmems[i].name);
	}
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

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
	struct proc *p;
	struct cpu  *c = mycpu();

	c->proc = 0;
	for (;;) {
		// The most recent process to run may have had interrupts
		// turned off; enable them to avoid a deadlock if all
		// processes are waiting.
		intr_on();

		int found = 0;
		for (p = proc; p < &proc[NPROC]; p++) {
			acquire(&p->lock);
			if (p->state == RUNNABLE) {
				// Switch to chosen process.  It is the process's job
				// to release its lock and then reacquire it
				// before jumping back to us.
				p->state = RUNNING;
				c->proc  = p;
				swtch(&c->context, &p->context);

				// Process is done running for now.
				// It should have changed its p->state before coming back.
				c->proc = 0;
				found   = 1;
			}
			release(&p->lock);
		}
		if (found == 0) {
			// nothing to run; stop running on this core until an interrupt.
			intr_on();
			asm volatile("wfi");
		}
	}
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




int mutex_create(char *name) {
    struct proc *process = myproc();

    // Find an unused mutex slot in `all_locks`
	for (int i = 0; i < MAX_MAXNUM; i++) {
    	if (strcmp(all_locks[i].nameM, name) == 0) {
        	printf("HITS HITS HITS \n");
			all_locks[i].referenced_by += 1;
        	return all_locks[i].mutex_id;
    	}
	}

    for (int i = 0; i < MAX_MAXNUM; i++) {

		if (all_locks[i].status == 0){

            all_locks[i].mutex_id = i;
            all_locks[i].status = 1; 
            all_locks[i].owner_id = -1; 
            all_locks[i].referenced_by += 1;
			strncpy(all_locks[i].nameM, name, sizeof(all_locks[i].nameM));
            initsleeplock(&all_locks[i].slock, name);

            // Check if process's mutex table has space
            if (process->mutex_count >= MAX_MAXNUM) {
                printf("Process %d full table \n", process->pid);
                return -1;
            }

            // Add mutex to process's mutex table
            process->mutex_table[process->mutex_count] = &all_locks[i];
            process->mutex_count++;

            //printf("Created mutex %d for process %d. Mutex count: %d\n", i, process->pid, process->mutex_count);
			//printf("%d", mutex_id);
            return all_locks[i].mutex_id; //mutex_id
        }
    }

    printf("Failed to create mutex for process %d\n", process->pid);
    return -1; //no avialable,c an't create
}

void mutex_delete(int muxid) {
    struct proc *process = myproc();
    int flag = -1;

    for (int i = 0; i < process->mutex_count; i++) {
        if (process->mutex_table[i]->mutex_id == muxid) {
            process->mutex_table[i]->referenced_by--;
            flag = i;
        }
        if (flag != -1 && i < process->mutex_count - 1) {
            process->mutex_table[i] = process->mutex_table[i + 1];
        }
    }

    if (flag != -1) {
        process->mutex_count--;
    }

    if (all_locks[muxid].referenced_by == 0) {
        all_locks[muxid].status = 0;
    }
}


//acquire sleep essentially acquires, sleeps and wakesup

/*

void
acquiresleep(struct sleeplock *lk)
{
	acquire(&lk->lk);
	while (lk->locked) { sleep(lk, &lk->lk); }
	lk->locked = 1;
	lk->pid    = myproc()->pid;
	release(&lk->lk);
}

void
releasesleep(struct sleeplock *lk)
{
	acquire(&lk->lk);
	lk->locked = 0;
	lk->pid    = 0;
	wakeup(lk);
	release(&lk->lk);
}
	for (int i = 0; i < p->mutex_count; i++) {
        struct mutex *cur_mutex = p->mutex_table[i];
        acquiresleep(&cur_mutex->slock);
        cur_mutex->status = 0;        
        cur_mutex->owner_id = -1;
        wakeup(cur_mutex);            
        releasesleep(&cur_mutex->slock);
    }
*/
void
mutex_lock(int muxid) {
    struct proc *process = myproc();
    struct mutex *cur_mutex = &all_locks[muxid];

	 printf("860 \n");

	//printf("750 \n");
    if (muxid < 0 || muxid >= MAX_MAXNUM) {
        panic("Invalid mutex ID in mutex_lock");
    }
	printf("866 \n");
    int found = 0;
    for (int i = 0; i < process->mutex_count; i++) {
        if (process->mutex_table[i]->mutex_id == muxid) {
            found = 1;
            break;
        }
    }
	printf("874 \n");


	//printf("759 \n");
    if (!found) {
        panic("Invalid mutex ID");
    }
	printf("763 \n");
    acquiresleep(&cur_mutex->slock);

    // while (cur_mutex->status == 1) {
	// 	printf("767 \n");
    //     sleep(cur_mutex, &cur_mutex->slock.lk); 
	// 	printf("885");
    // }

	cur_mutex->owner_id = myproc()->pid;
	printf("771 \n");
    cur_mutex->status = 1;
    cur_mutex->owner_id = process->pid;
	printf("774 \n");
	releasesleep(&cur_mutex->slock);
}


void mutex_unlock(int muxid) {
    struct proc *process = myproc();
    struct mutex *cur_mutex = &all_locks[muxid];


	if (muxid < 0 || muxid >= MAX_MAXNUM) {
        panic("Invalid mutex ID");
    }
	printf("804 \n");
    //acquiresleep(&cur_mutex->slock);
	printf("806 \n");
    if (cur_mutex->owner_id != process->pid) {
		//exit(0);
		//release(&cur_mutex->slock);
		exit(0);
        panic("Mutex unlock by non-owner/release an already released mutex");
    }
	

    // Release the mutex
    cur_mutex->status = 0;
    cur_mutex->owner_id = -1;

    // Wake up any threads waiting on the mutex
    wakeup(cur_mutex);

    releasesleep(&cur_mutex->slock);
}






void cv_wait(int muxid) {
    if (muxid < 0 || muxid >= MAX_MAXNUM) {
        panic("Invalid mutex ID in cv_wait");
    }

    struct mutex *cur_mutex = &all_locks[muxid];
    struct proc *p = myproc();

    if (cur_mutex->owner_id != p->pid) {
        panic("cv_wait called by non-owner");
    }
	printf(" 839\n");
	
    acquiresleep(&cur_mutex->slock);
    cur_mutex->status = 0;     
    cur_mutex->owner_id = -1;   

	printf(" 845\n");
	
    //sleep(cur_mutex, &cur_mutex->slock.lk);

	printf(" 839\n");

    cur_mutex->status = 1;       
    cur_mutex->owner_id = p->pid;

	printf(" 842\n"); 

    releasesleep(&cur_mutex->slock);

	printf(" 844\n");
}


void cv_signal(int muxid) {
    if (muxid < 0 || muxid >= MAX_MAXNUM) {
        panic("Invalid mutex ID in cv_signal");
    }

    struct mutex *cur_mutex = &all_locks[muxid];

    acquiresleep(&cur_mutex->slock);
    wakeup(cur_mutex);
    releasesleep(&cur_mutex->slock);
}


char* map_va(uint64 memory, char* name) {
	
	
	//CHECK IF AN OPENING EXISTS
	struct proc * process = myproc();

	for (int i = 0; i < process->shmem_alloc; i++) {
		if (process->shmems[i].status == 0) {
			uvmunmap(myproc()->pagetable, process->shmems[i].va, 1, 0);
			mappages(myproc()->pagetable, process->shmems[i].va, PGSIZE, memory, PTE_W | PTE_R | PTE_U);
			strncpy(myproc()->shmems[i].name, name, strlen(name));
			myproc()->shmems[i].status = 1;
			myproc()->shmems[i].pa = memory;
			return (char *)process->shmems[i].va;
		}
	}
	//open new to put shmem
	uint64 virtual = (uint64)(PGROUNDUP(myproc()->sz));
	//mapping pages
	//printf("VA of %s: %ld\n", name, virtual);
	if (mappages(myproc()->pagetable, virtual, PGSIZE, memory, PTE_W | PTE_R | PTE_U) > 0) {
		printf("FAILURE IN MAPPAGES\n");
	} 
	myproc()->shmems[myproc()->shmem_count].va = virtual;
	myproc()->shmems[myproc()->shmem_count].pa = memory;
	myproc()->shmems[myproc()->shmem_count].status = 1;
	strncpy(myproc()->shmems[myproc()->shmem_count].name, name, strlen(name));
	myproc()->shmem_count++;
	myproc()->shmem_alloc++;
	
	//account for new size
	myproc()->sz = (uint64)virtual + PGSIZE;
	return (char *)virtual;
}

void proc_free(char* name) {
	int len = strlen(name);
	struct proc * process = myproc();
	//similar process as before to track down the mem in the proc
	int flag = 0;
	for (int i = 0; i < process->shmem_alloc; i++) {
		int len2 = strlen(process->shmems[i].name);
		if (len == len2) {
			if (flag == 0 && strncmp(name, process->shmems[i].name, len) == 0) {		
				process->shmems[i].status = 0;

				//if its not the last
				if (i != process->shmem_alloc - 1) {
					uvmunmap(myproc()->pagetable, process->shmems[i].va, 1, 0);
					process->shmem_count--;
					void* temp_mem = kalloc();
					process->shmems[i].pa = (uint64) temp_mem;
					memset(process->shmems[i].name, 0, 16);
					//should map junk for now
					
					mappages(myproc()->pagetable, process->shmems[i].va, PGSIZE, (uint64) temp_mem, PTE_W | PTE_R | PTE_U);

				} else {
					flag = 1;
					uvmunmap(myproc()->pagetable, process->shmems[i].va, 1, 0);
					process->shmem_count--;
					process->shmem_alloc--;
					memset(process->shmems[i].name, 0, 16);
					myproc()->sz = myproc()->sz - PGSIZE;
					/*
					To whoever may one day read this: I apologize for the ugliest code ever, 
					I just literally could not figure out any other way to do this. Thank you
					for understanding.
					*/
					for (int j = i - 1; j >= 0; j--) {
						//unmap all empty pages before it
						if (process->shmems[j].status == 0) {
							uvmunmap(myproc()->pagetable, process->shmems[j].va, 1, 0);			
							process->shmem_alloc--;
							kfree((void*) process->shmems[j].pa);
							myproc()->sz = myproc()->sz - PGSIZE;
						} else {
							break;
						}
					}
				}

				
				

			}
			//if we have to remove one, we move all of the next ones a spot to the left
			if (flag && i != SHM_MAXNUM - 1) {
				//move each entry to the right one
				process->shmems[i] = process->shmems[i + 1];
			}
		}
	}
}


//move pages to the left now
				/*if (process->shmem_count - i > 0 && i != myproc()->shmem_count) {
					for (int j = i; j < process->shmem_count; j++) {
						uint64 current_va = process->shmems[j].va;
						process->shmems[j].va = process->shmems[j].va - PGSIZE;
						struct proc_shmem next = process->shmems[j + 1];
						//printf("MAPPING %ld to %ld\n", next.va, current_va);
						mappages(myproc()->pagetable, current_va, PGSIZE, next.pa, PTE_W | PTE_R | PTE_U);
						//process->shmems[j + 1].va = process->shmems[j + 1].va - PGSIZE;
					}
					process->shmems[process->shmem_count].va -= PGSIZE;
				}*/
int nextcid = 1;

int
cm_create_and_enter(void){
	struct proc *p = myproc();

	if(p -> in_container){
		printf("ERROR: Already in container\n");
		return -1;
	}
	

	p -> in_container = 1;
	p -> cid = nextcid++;

	return 0;
}

int
cm_setroot(char *path, int path_length){
	struct proc *p = myproc();
	
	if(p -> root_set){
		printf("ERROR: Root already set\n");
		return -1;
	}
	
	if(path_length >= MAXPATH){
		printf("ERROR: Path too long\n");
		return -1;
	}

	if(path[0] != '/'){
		printf("ERROR: Path must start with /\n");
		return -1;
	}
	begin_op();
	struct inode *ip = namei(path);
	if(ip == 0){
		end_op();
		printf("ERROR: Failed to namei\n");
		return -1;
	}
	ilock(ip);
	if(ip -> type != T_DIR){
		iunlockput(ip);
		end_op();
		printf("ERROR: Not a directory\n");
		return -1;
	}
	iunlock(ip);
	iput(p->cwd);  
	end_op();
	acquire(&p -> lock);
	p -> cwd = ip;
	p -> root = ip;
	p -> root_set = 1;
	release(&p -> lock);
	//------------------------change shell's root------------------------
	struct proc *pp;
	for(pp = proc; pp < &proc[NPROC]; pp++){
		acquire(&pp -> lock);
		if(pp -> state != UNUSED){
			if(pp -> parent != NULL2 && pp -> parent -> pid == 1 && strncmp(pp -> name, "sh", 2) == 0){
				pp -> fs_root = pp -> cwd;
				pp -> cwd = p -> cwd;
				pp -> root = p -> root;
				pp -> root_set = p -> root_set;
				printf("CHANGED");
			}
		}
		release(&pp -> lock);
	}

	//-------------------------------------------------------------------
	return 0;
}

int
cm_maxproc(int maxprocs){
	struct proc *p = myproc();
	struct proc *pp;

	if(maxprocs < 1){
		printf("ERROR: Maxprocs must be greater than 1\n");
		return -1;
	}

	for(pp = p; pp != 0; pp = pp -> parent){
		if(pp -> maxprocs_set == 1){
			return -1;
		}
	}
	
	p -> maxprocs = maxprocs;
	p -> maxprocs_set = 1;
	return 0;
}
/**
 * Goes through the process table and looks for the one that
 * matches the 'which' argument. Copies its information into the kernel 
 * copy of the pstat struct
 * @param which, which of the active processes in the system the caller wants to get information about
 * @param ps, the kernel copy of pstat struct
 * @return 0 if successful, -1 if some error, 1 every process in the system has been iterated through
 */
int 
procstat(uint64 which, struct pstat *ps){
	if(which < 0 || which > NPROC){
		return -1;
	}
	if(!ps){
		return -1;
	}
	int process_count = 0;
	struct proc *p;
	for(p = proc; p < &proc[NPROC]; p++){
		acquire(&p -> lock);
		if(p -> state != UNUSED){
			if(process_count == which){
				ps -> pid = p -> pid;
				safestrcpy(ps -> name, p -> name, sizeof(ps -> name));
				acquire(&wait_lock);
				if(p -> parent){
					ps -> ppid = p -> parent -> pid;
				}
				else{
					ps -> ppid = p -> pid;
				}
				release(&wait_lock);
				if(p -> state == USED){
					ps -> state = 'U';
				}
				else if(p -> state == SLEEPING){
					ps -> state = 'S';
				}
				else if(p -> state == RUNNING){
					ps -> state = 'N';
				}
				else if(p -> state == RUNNABLE){
					ps -> state = 'R';
				}
				else if(p -> state == ZOMBIE){
					ps -> state = 'Z';
				}
				printf("cwd: %d | cid: %d | maxprocs: %d | root_set: %d | maxprocs_set %d | in_container: %d | ", p -> cwd -> inum, p -> cid, p -> maxprocs, p -> root_set, p -> maxprocs_set, p -> in_container);
				release(&p -> lock);
				return 0;
			}
			process_count++;
		}
		release(&p -> lock);
	}
	return 1;
}