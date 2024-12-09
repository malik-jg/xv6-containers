#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_THREADS 3
#define ITERATIONS 5

int main() {
    int muxid = mutex_create("test_mutex");
    if (muxid < 0) {
        printf("Failed to create mutex.\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        printf("Fork failed.\n");
        exit(1);
    }

    if (pid == 0) {
        // Child process: Wait on condition variable
        for (int i = 0; i < ITERATIONS; i++) {
            printf("Child %d waiting on condition...\n", getpid());
            mutex_lock(muxid);
            cv_wait(muxid);
            printf("Child %d woke up from condition.\n", getpid());
            mutex_unlock(muxid);
        }
        exit(0);
    } else {
        //Parent process: Signal condition variable
        sleep(2); 
        for (int i = 0; i < ITERATIONS; i++) {
            mutex_lock(muxid);
            printf("Parent signaling condition (iteration %d)...\n", i);
            cv_signal(muxid);
            mutex_unlock(muxid);
            sleep(1);
        }
        wait(0);
    }

    mutex_delete(muxid);
    printf("Test Level 2 completed successfully.\n");
    exit(0);
}
