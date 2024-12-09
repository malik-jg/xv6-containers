struct stat;
#include "kernel/types.h"
struct stat;
struct pstat;

// system calls
int   fork(void);
int   exit(int);
int   wait(int *);
int   pipe(int *);
int   write(int, const void *, int);
int   read(int, void *, int);
int   close(int);
int   kill(int);
int   exec(const char *, char **);
int   open(const char *, int);
int   mknod(const char *, short, short);
int   unlink(const char *);
int   fstat(int fd, struct stat *);
int   link(const char *, const char *);
int   mkdir(const char *);
int   chdir(const char *);
int   dup(int);
int   getpid(void);
int   getcid(void);
char *sbrk(int);
int   sleep(int);
int   uptime(void);


int mutex_create(char *name);
void mutex_delete(int muxid);
void mutex_lock(int muxid);
void mutex_unlock(int muxid);
void cv_wait(int muxid);
void cv_signal(int muxid);
int   cm_create_and_enter(void);
int   cm_setroot(char *, int);
int   cm_maxproc(int);
int   procstat(uint64 which, struct pstat *ps);

// ulib.c
int   stat(const char *, struct stat *);
char *strcpy(char *, const char *);
char *strncpy(char *, const char *, int);
void *memmove(void *, const void *, int);
char *strchr(const char *, char c);
int   strcmp(const char *, const char *);
void  fprintf(int, const char *, ...) __attribute__((format(printf, 2, 3)));
void  printf(const char *, ...) __attribute__((format(printf, 1, 2)));
char *gets(char *, int max);
uint  strlen(const char *);
void *memset(void *, int, uint);
int   atoi(const char *);
int   memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
void ps(void);
void strconcat(char *, char *, char *);

uint64 *user_shm_get(char *name);
char * shm_get(char *name);

int shm_rem(char *name);

// umalloc.c
void *malloc(uint);
void  free(void *);
