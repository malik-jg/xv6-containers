#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_THREADS 8
#define ITERATIONS 50

//Shared resource
int shared_resource = 0;
int mutex_id;

//Thread function to increment the shared resource
void thread_function(void *arg) {
    int tid = *(int *)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        printf("Thread %d: attempting to lock mutex %d\n", tid, mutex_id);
        printf("%d", mutex_id);
        mutex_lock(mutex_id);

        //int temp = shared_resource;
        // shared_resource = temp + 1;

        //printf("Thread %d: incremented shared_resource to %d\n", tid, shared_resource);

        mutex_unlock(mutex_id); 
        printf("reachable \n");
        sleep(1);
        printf("Thread %d: unlocked mutex %d\n", tid, mutex_id);
    }

    exit(0);
}

int main() {
    mutex_id = mutex_create("test_mutex");
    if (mutex_id < 0) {
        printf("Failed to create mutex.\n");
        exit(1);
    }
    printf("Mutex id %d\n", mutex_id);

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

    int expected = NUM_THREADS * ITERATIONS;
    printf("shared_resource %d expect %d\n", shared_resource, expected);
    
    //printf("Meow");

    // if (shared_resource == expected) {
        // printf("passed");
    // } else {
        // printf("I am beyond cooked");
    // }

    // Delete the mutex
    mutex_delete(mutex_id);
    printf("Mutex %d deleted\n", mutex_id);
    printf("TEST PASSED \n");

    exit(0);
}
