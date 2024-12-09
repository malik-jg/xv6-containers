#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MUX_NAME "test_mutex"

// Helper function to print error messages
void panic(const char *msg) {
    printf("%s\n", msg);
    exit(1);
}

void test_mutex_released_on_fault() {
    int muxid = mutex_create(MUX_NAME);
    if (muxid < 0) {
        panic("Failed to create mutex");
    }

    int pid = fork();
    if (pid < 0) {
        panic("Failed to fork process");
    }

    if (pid == 0) {
        // Child process
        printf("Child: Locking mutex\n");
        mutex_lock(muxid);
        

        printf("Child: Waiting on condition\n");
        cv_wait(muxid); // This should block

        // Simulate a fault (e.g., divide by zero)
        printf("Child: Simulating fault\n");
        //int fault = 1 / 0; // Intentional fault
        //(void)fault;

        // This point should never be reached
        printf("SHOULD NOT REACh HERE");
        mutex_unlock(muxid);
        exit(0);
    } else {
        // Parent process
        sleep(10); // Allow child to acquire lock and enter wait state

        printf("Parent: Checking mutex status after child fault\n");

        // Try to lock the mutex; should succeed if the child released it
        printf("Parent trying to lock mutex");
        mutex_lock(muxid);

        printf("Parent: Successfully locked mutex after child fault\n");
        mutex_unlock(muxid);
        printf("Parent: Mutex unlocked\n");

        // Clean up
        mutex_delete(muxid);
        printf("Test completed successfully\n");
        wait(0); // Wait for the child to clean up
    }
}

int main(int argc, char *argv[]) {
    printf("Starting Level 3 Fault Handling Test\n");
    test_mutex_released_on_fault();
    exit(0);
}
