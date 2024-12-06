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
        } else {
            printf("FAILURE! SHMEM IS NOT DEALLOCATED AFTER EXIT!\n");
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
        } else {
            printf("FAILURE! SHMEM IS DEALLOCATED BEFORE EXIT!\n");
        }
        wait(&pid2); 
    }

    int no_such_mem = shm_rem("no_memory");

    if (no_such_mem == -1) {
        printf("SUCCESS! SHM_REM ERROR WHEN UNDECLARED MEMORY!\n");
    }

    char * mem1 = (char*) shm_get("memory1");
    shm_get("memory2");
    char * mem3 = (char*) shm_get("memory3");

    const char * test3 = "please read this";

    shm_rem("memory2");

    int pid3 = fork();

    if (pid3 == 0) {
        strcpy(mem1, test3);
        strcpy(mem3, test3);

        exit(0);
    } else {
        if (strcmp(mem1, test3) != 0 && strcmp(mem3, test3) != 0) {
            printf("\tMEMORY BLANK BEFORE CHILD WRITE!\n");
        }
        else {
            printf("FAILURE! MEMORY POPULATED BEFORE CHILD WRITE!\n");
        }
        wait(&pid3);
        if (strcmp(mem1, test3) == 0 && strcmp(mem3, test3) == 0) {
            printf("SUCCESS! MEM CAN READ AFTER OUT-OF-ORDER-REMOVAL\n");
        } else {
            printf("FAILURE! MEM CAN'T READ AFTER OUT-OF-ORDER-REMOVAL\n"); 
        }
    }

    shm_rem("memory3");
    shm_rem("memory1");

    printf("SUCCESS! NO KERNEL ERROR WHEN REM OCCURS OUT OF ORDER!\n");
    
    printf("TEST THREE COMPLETE\n\n");
    exit(0);
}