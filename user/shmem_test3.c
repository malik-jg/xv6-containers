#include "kernel/types.h"
#include "user/user.h"

int main (void) {
    printf("\nTEST THREE: FORK(), EXIT(), EXEC()\n");
    shm_get("part1");

    
    int pid = fork();
    
    if (pid == 0) {
        exit(0);
        
    } else {
        wait(&pid); 
        int post_exit = shm_rem("part1");
        if (post_exit == 1) {
            printf("SUCCESS! SHMEM IS DEALLOCATED AFTER EXIT!\n");
        }
    }

    shm_get("part2");
    int pid2 = fork();

    if (pid2 == 0) {
        sleep(2);
        exit(0);
        
    } else {
        int pre_exit = shm_rem("part2");
        if (pre_exit == 0) {
            printf("SUCCESS! SHMEM IS NOT DEALLOCATED BEFORE EXIT!\n");
        }
        wait(&pid2); 
    }

    
    printf("TEST THREE COMPLETE\n\n");
    exit(0);
}