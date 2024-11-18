#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
	int n;
	argint(0, &n);
	exit(n);
	return 0; // not reached
}

uint64
sys_getpid(void)
{
	return myproc()->pid;
}

uint64
sys_fork(void)
{
	return fork();
}

uint64
sys_wait(void)
{
	uint64 p;
	argaddr(0, &p);
	return wait(p);
}

uint64
sys_sbrk(void)
{
	uint64 addr;
	int    n;

	argint(0, &n);
	addr = myproc()->sz;
	if (growproc(n) < 0) return -1;
	return addr;
}

uint64
sys_sleep(void)
{
	int  n;
	uint ticks0;

	argint(0, &n);
	if (n < 0) n = 0;
	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n) {
		if (killed(myproc())) {
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

uint64
sys_kill(void)
{
	int pid;

	argint(0, &pid);
	return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}




uint64
sys_mutex_create(void){
	char name[16]; //buffer 16 bytes 16 chars
	//cannot create
	if (argstr(0, name, sizeof(name)) < 0){
		return -1;
	}

	//return the mutex that is created, will return -1 if cannot create oen
	return mutex_create(name);

}



uint64
sys_mutex_delete(void){
	//will be passed onto the mutex_delete in proc.c
	int muxid;

	argint(0, &muxid);

	if (muxid < 0){
		return -1;
	}


	mutex_delete(muxid);
	//deleted return 0, success
	return 0;
}



uint64
sys_mutex_lock(void){
	int muxid;

	argint(0, &muxid);

	if (muxid < 0){
		return -1;
	}

	mutex_lock(muxid);
	return 0;
}




uint64
sys_mutex_unlock(void){

	int muxid;

	argint(0, &muxid);

	if (muxid < 0){
		return -1;
	}
	//mutex unlock will return -1 if not unlocked
	mutex_unlock(muxid);
	return 0;
}


//do later
int
sys_cv_wait(void) {
    int muxid;
    argint(0, &muxid);

    if (muxid < 0) {
        return -1;
    }

    cv_wait(muxid);
    return 0;
}





int
sys_cv_signal(void) {
    int muxid;

    argint(0, &muxid);

    if (muxid < 0) {
        return -1;
    }

    cv_signal(muxid);
    return 0;
}