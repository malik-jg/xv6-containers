#include "kernel/types.h"
#include "user.h"

#define MAX_PROC 5

void test_mutex_blocking() {
    int muxid = mutex_create("test_mutex");
    if (muxid < 0) {
        printf("Mutex creation failed\n");
        exit(0);
    }

    int pid = fork();
    if (pid == 0) {
        mutex_lock(muxid);
        printf("Child acquired mutex\n");
        //arbitrary number
        sleep(10);
        mutex_unlock(muxid);
        printf("Child released mutex\n");
        exit(0);
    } else {
        wait(0);
        mutex_lock(muxid);
        printf("Parent acquired mutex\n");
        mutex_unlock(muxid);
        printf("Parent released mutex\n");
        printf("Test Works \n \n");
        exit(0);
    }
}


int main() {
    test_mutex_blocking();
    
    exit(0);
}
