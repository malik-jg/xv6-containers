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
    if (pid == 0) { // Child
        mutex_lock(muxid);
        printf("Child acquired mutex\n");
        sleep(50);
        mutex_unlock(muxid);
        printf("Child released mutex\n");
        exit(0);
    } else { // Parent
        sleep(10); // Ensure child locks first
        mutex_lock(muxid);
        printf("Parent acquired mutex\n");
        mutex_unlock(muxid);
        printf("Parent released mutex\n");
        wait(0);
    }
}

int main() {
    test_mutex_blocking();
    exit(0);
}
