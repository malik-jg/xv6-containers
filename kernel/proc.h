
#include "sleeplock.h"

struct mutex {
	int mutex_id;  //mutex id
	int status; //locked or not, 0 unlocked, 1 locked
	int owner_id; //parent mutexid
	//struct spinlock lk; //spinlock for mutexes
	int referenced_by;   //ref or no ref	
	struct sleeplock slock;
	char 			nameM[16];
};

// Saved registers for kernel context switches.
struct context {
	uint64 ra;
	uint64 sp;

	// callee-saved
	uint64 s0;
	uint64 s1;
	uint64 s2;
	uint64 s3;
	uint64 s4;
	uint64 s5;
	uint64 s6;
	uint64 s7;
	uint64 s8;
	uint64 s9;
	uint64 s10;
	uint64 s11;
};

// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
	/*   0 */ uint64 kernel_satp;   // kernel page table
	/*   8 */ uint64 kernel_sp;     // top of process's kernel stack
	/*  16 */ uint64 kernel_trap;   // usertrap()
	/*  24 */ uint64 epc;           // saved user program counter
	/*  32 */ uint64 kernel_hartid; // saved kernel tp
	/*  40 */ uint64 ra;
	/*  48 */ uint64 sp;
	/*  56 */ uint64 gp;
	/*  64 */ uint64 tp;
	/*  72 */ uint64 t0;
	/*  80 */ uint64 t1;
	/*  88 */ uint64 t2;
	/*  96 */ uint64 s0;
	/* 104 */ uint64 s1;
	/* 112 */ uint64 a0;
	/* 120 */ uint64 a1;
	/* 128 */ uint64 a2;
	/* 136 */ uint64 a3;
	/* 144 */ uint64 a4;
	/* 152 */ uint64 a5;
	/* 160 */ uint64 a6;
	/* 168 */ uint64 a7;
	/* 176 */ uint64 s2;
	/* 184 */ uint64 s3;
	/* 192 */ uint64 s4;
	/* 200 */ uint64 s5;
	/* 208 */ uint64 s6;
	/* 216 */ uint64 s7;
	/* 224 */ uint64 s8;
	/* 232 */ uint64 s9;
	/* 240 */ uint64 s10;
	/* 248 */ uint64 s11;
	/* 256 */ uint64 t3;
	/* 264 */ uint64 t4;
	/* 272 */ uint64 t5;
	/* 280 */ uint64 t6;
};

// Per-CPU state.
struct cpu {
	struct proc   *proc;    // The process running on this cpu, or null.
	struct context context; // swtch() here to enter scheduler().
	int            noff;    // Depth of push_off() nesting.
	int            intena;  // Were interrupts enabled before push_off()?

	// If a thread (proc) uses another's page-tables, when it
	// switches to user-level it has to use the trapframe of the
	// page-table's process. Thus, we must save it and restore it
	// on exit from the kernel, and entry back into the kernel.
	// This is where we save it! These are updated by the
	// trapframe_update_* functions.
	struct proc *pagetable_proc;
	struct trapframe saved_pagetable_proc_tf;
};

struct proc_shmem {
	uint64 va;
	uint64 pa;
	char name[16];
	int status;
};

extern struct cpu cpus[NCPU];

enum procstate
{
	UNUSED,
	USED,
	SLEEPING,
	RUNNABLE,
	RUNNING,
	ZOMBIE
};
// --------------------------------------------------------------------------

// priority map for HFS
#define PRIOMAX 8
// node for hash map
// struct prioNode {
// 	struct proc* process;
//     struct prioNode *next;
// };

// hashmap for priorities
struct prioMap {
    struct proc *buckets[PRIOMAX];
};
// --------------------------------------------------------------------------

// Per-process state
struct proc {
	struct spinlock lock;

	// p->lock must be held when using these:
	enum procstate state;  // Process state
	void          *chan;   // If non-zero, sleeping on chan
	int            killed; // If non-zero, have been killed
	int            xstate; // Exit status to be returned to parent's wait
	int            pid;    // Process ID
	int 		   priority; // Process Priority
	struct proc   *next;
	// For Containers
	int 		   cid;	  			         // Container ID
	struct inode   *root;		     		 // Root directory of container
	struct inode   *fs_root;		 		 // Root directory of file system		
	int 		   root_set;		 		 // Flag to check if container root has been set
	int 		   maxprocs;		         // Maximum number of processes in container
	int 	       maxprocs_set;		     // Flag to check if maxprocs has been set
	int 		   in_container;	         // Flag to check if process is in container
	
	// wait_lock must be held when using this:
	struct proc *parent; // Parent process

	// these are private to the process, so p->lock need not be held.
	uint64            kstack;        // Virtual address of kernel stack
	uint64            sz;            // Size of process memory (bytes)
	pagetable_t       pagetable;     // User page table
	struct trapframe *trapframe;     // data page for trampoline.S
	// GWU updates: Each protection domain (page-table) has a
	// single trapframe for its initial thread mapped into it. If
	// we want multiple threads, then all threads need to be
	// taught to *share* the trapframe of the thread that owns
	// (created) the address space. See the corresponding
	// functions to update these structures: `trapframe_update_*`
	//
	// The process that owns the page-table/protection domain that
	// this process actually uses (i.e. if this process is
	// actually a thread that shares that protection domain..
	struct proc      *pagetable_proc;
	struct context    context;       // swtch() here to run process
	struct file      *ofile[NOFILE]; // Open files
	struct inode     *cwd;           // Current directory
	char             name[16];      // Process name (debugging)


	//struct mutex_table *mutex_table;
	// struct mutex_table{
    // 	struct mutex *mutex[MAX_MAXNUM]; 
    // 	//currently held mutexes
	// 	int mutex_count;
	// };
	//struct sleeplock;




	// struct mutex_table{
	// 	struct mutex *mutex[MAX_MAXNUM];
	// };



	struct mutex *mutex_table[MAX_MAXNUM];

	int mutex_count;


	struct proc_shmem shmems[SHM_MAXNUM];
	int 			 shmem_count;
	int				 shmem_alloc;
};
