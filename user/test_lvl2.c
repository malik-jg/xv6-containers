#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_THREADS 5
#define ITERATIONS 10

//makeshift shared resource, need shared resource to be done.
int shared_resource = 0;
int mutex_id;

//shared resource thread to obtain the stuff
void thread_function(void *arg) {
    //process > thread, want to implement individual thread stuff.
    //threadid, 
    int tid = *(int *)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        //lock
        printf("Thread %d: attempting to lock mutex %d\n", tid, mutex_id);
        mutex_lock(mutex_id);

        //critical section
        int temp = shared_resource;
        //printf("pretend work here, just sleeping");
        sleep(50); 
        shared_resource = temp + 1;

        printf("Thread %d: shares resrouce was %d\n to %d\n", tid, shared_resource -1, shared_resource);

        mutex_unlock(mutex_id);
        printf("Thread %d: unlocked mutex %d\n", tid, mutex_id);
    }

    exit(0);
}

int main() {
    mutex_id = mutex_create("shared_mutex");
    if (mutex_id < 0) {
        printf("Failed to create mutex.\n");
        exit(1);
    }
    printf("Mutex created with ID %d\n", mutex_id);

    int pids[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pids[i] = fork();
        if (pids[i] == 0) {
            thread_function(&thread_ids[i]);
        }
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        wait(0);
    }
    printf("Final value of shared_resource: %d (expected: %d)\n", shared_resource, NUM_THREADS * ITERATIONS);
    mutex_delete(mutex_id);
    printf("Mutex %d deleted\n", mutex_id);

    exit(0);
}
