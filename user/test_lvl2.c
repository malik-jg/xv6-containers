#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// #define NUM_THREADS 8
// #define ITERATIONS 50

// //Shared resource
// int shared_resource = 0;
// int mutex_id;

// //Thread function to increment the shared resource
// void thread_function(void *arg) {
//     int tid = *(int *)arg;

//     for (int i = 0; i < ITERATIONS; i++) {
//         //printf("Thread %d: attempting to lock mutex %d\n", tid, mutex_id);
//         //printf("%d", mutex_id);
//         mutex_lock(mutex_id);

//         //int temp = shared_resource;
//         // shared_resource = temp + 1;

//         //printf("Thread %d: incremented shared_resource to %d\n", tid, shared_resource);
//         sleep(1);
//         mutex_unlock(mutex_id); 
//         //printf("reachable \n");
        
//         //printf("Thread %d: unlocked mutex %d\n", tid, mutex_id);
//     }

//     exit(0);
// }

// int main() {
//     mutex_id = mutex_create("test_mutex");
//     if (mutex_id < 0) {
//         //printf("Failed to create mutex.\n");
//         exit(1);
//     }
//     //printf("Mutex id %d\n", mutex_id);

//     int pids[NUM_THREADS];
//     int thread_ids[NUM_THREADS];
//     for (int i = 0; i < NUM_THREADS; i++) {
//         thread_ids[i] = i;
//         pids[i] = fork();
//         if (pids[i] == 0) {
//             thread_function(&thread_ids[i]); 
//         }
//     }

//     for (int i = 0; i < NUM_THREADS; i++) {
//         wait(0);
//     }

//     int expected = NUM_THREADS * ITERATIONS;
//     //printf("shared_resource %d expect %d\n", shared_resource, expected);
    
//     //printf("Meow");

//     // if (shared_resource == expected) {
//         // printf("passed");
//     // } else {
//         // printf("I am beyond cooked");
//     // }

//     // Delete the mutex
//     mutex_delete(mutex_id);
//     //printf("Mutex %d deleted\n", mutex_id);
//     printf("LVL 2 PASSED \n");

//     exit(0);
// }

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
