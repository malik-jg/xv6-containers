#include "kernel/types.h"
#include "user/user.h"


#define NCHILDREN  8        /* should be > 1 to have contention */
#define CRITSECTSZ 3
#define DATASZ (1024 * 32 / NCHILDREN)

#define LOCK_CREATE(x)  mutex_create(x)
#define LOCK_TAKE(x)    mutex_lock(x)
#define LOCK_RELEASE(x) mutex_unlock(x)
#define LOCK_DEL(x)     mutex_delete(x)

void child(int lock_id, int pipefd, char tosend) {
    int i, j;

    for (i = 0; i < DATASZ; i += CRITSECTSZ) {
        // Critical section begins
        LOCK_TAKE(lock_id);

        for (j = 0; j < CRITSECTSZ; j++) {
            write(pipefd, &tosend, 1); // Write character to pipe
        }

        LOCK_RELEASE(lock_id); // Critical section ends
    }

    exit(0);
}

int main(void) {
    int i, lock_id;
    int pipes[2];
    char data[NCHILDREN], first = 'a';

    for (i = 0; i < NCHILDREN; i++) {
        data[i] = (char)(first + i); // Fill data array with 'a', 'b', ..., 'h'
    }

    if (pipe(pipes) != 0) {
        printf("Pipe creation error\n");
        exit(-1);
    }

    lock_id = LOCK_CREATE("test_lock");
    if (lock_id < 0) {
        printf("Lock creation error\n");
        exit(-1);
    }

    for (i = 0; i < NCHILDREN; i++) {
        if (fork() == 0) {
            child(lock_id, pipes[1], data[i]);
        }
    }

    close(pipes[1]); // Close the write end of the pipe in the parent

    // Read and validate data from the pipe
    while (1) {
        char fst, c;
        int cnt;

        fst = '_';
        for (cnt = 0; cnt < CRITSECTSZ; cnt++) {
            if (read(pipes[0], &c, 1) == 0) {
                goto done; // Exit when no more data
            }

            if (fst == '_') {
                fst = c; // Set the first character
            } else if (fst != c) {
                printf("RACE CONDITION DETECTED: %c != %c\n", fst, c);
            }
        }
    }

done:
    for (i = 0; i < NCHILDREN; i++) {
        if (wait(0) < 0) {
            printf("Wait error\n");
            exit(-1);
        }
    }

    LOCK_DEL(lock_id);

    printf("Test completed successfully.\n");
    exit(0);
}