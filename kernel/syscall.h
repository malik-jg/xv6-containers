// System call numbers
#define SYS_fork 1
#define SYS_exit 2
#define SYS_wait 3
#define SYS_pipe 4
#define SYS_read 5
#define SYS_kill 6
#define SYS_exec 7
#define SYS_fstat 8
#define SYS_chdir 9
#define SYS_dup 10
#define SYS_getpid 11
#define SYS_sbrk 12
#define SYS_sleep 13
#define SYS_uptime 14
#define SYS_open 15
#define SYS_write 16
#define SYS_mknod 17
#define SYS_unlink 18
#define SYS_link 19
#define SYS_mkdir 20
#define SYS_close 21
#define SYS_prio_set 22
#define SYS_prio_get 23

#define SYS_user_shm_get 22
#define SYS_shm_rem 23

#define SYS_cm_create_and_enter 24
#define SYS_cm_setroot 25
#define SYS_cm_maxproc 26
#define SYS_procstat 27
#define SYS_getcid 28

#define SYS_mutex_create 29
#define SYS_mutex_delete 30
#define SYS_mutex_lock 31
#define SYS_mutex_unlock 32
#define SYS_cv_wait 33
#define SYS_cv_signal 34