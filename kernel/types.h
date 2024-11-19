typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef unsigned long  uint64;

typedef uint64 pde_t;

#define MAX_MAXNUM 20


int mutex_create(char *name);
void mutex_delete(int muxid);
void mutex_lock(int muxid);
void mutex_unlock(int muxid);
void cv_wait(int muxid);
void cv_signal(int muxid);


struct mutex {
	int mutex_id;  //mutex id
	int status; //locked or not, 0 unlocked, 1 locked
	int owner_id; //parent mutexid
	struct spinlock lk; //spinlock for mutexes
    int referenced_by;   //ref or no ref
};


