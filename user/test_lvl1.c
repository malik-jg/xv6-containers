#include "kernel/types.h"
#include "user.h"


//mainly adapted from hw2.


#define MAX_MUTEXES 3
#define MAX_PROCS 3

void test_lvl1_mutexes() {
    int mutex_ids[MAX_MUTEXES];
    int pids[MAX_PROCS];

    for (int i = 0; i < MAX_MUTEXES; i++) {
        mutex_ids[i] = mutex_create("mutex_lvl1");
        if (mutex_ids[i] < 0) {
            printf("Failed to create mutex %d\n", i);
            exit(0);
        }
        printf("Created mutex %d with ID %d\n", i, mutex_ids[i]);
    }

    for (int i = 0; i < MAX_PROCS; i++) {
        pids[i] = fork();
        if (pids[i] == 0) { // Child process
            int muxid = mutex_ids[i % MAX_MUTEXES];
            mutex_lock(muxid);
            printf("Child %d locked mutex %d\n", getpid(), muxid);
            mutex_unlock(muxid);
            printf("Critical Section impromptu \n");
            sleep(20); // Simulate critical section
            printf("Child %d unlocked mutex %d\n", getpid(), muxid);
            exit(0);
        }
    }

    for (int i = 0; i < MAX_PROCS; i++) {
        wait(0);
    }

    for (int i = 0; i < MAX_MUTEXES; i++) {
        mutex_lock(mutex_ids[i]);
        printf("Parent locked mutex %d\n", mutex_ids[i]);
        mutex_unlock(mutex_ids[i]);
        printf("Parent unlocked mutex %d\n", mutex_ids[i]);
    }

    printf("Test completed successfully\n");
}

int main() {
    test_lvl1_mutexes();
    exit(0);
}
