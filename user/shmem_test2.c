#include "kernel/types.h"
#include "user/user.h"

int main (void) {
    printf("\nTEST TWO: CHILD AND PARENT INTERACTIONS\n");
    const char * text = "test 2 message";
    char * shmem = (char *) shm_get("shmem1");

    strcpy(shmem, text);
    
    int pid = fork();
    
    if (pid == 0) {
        char* b = shmem;

        if (strcmp(b, text) == 0) {
            printf("SUCCESS! CHILD CAN ACCESS SAME SHARED MEM AS PARENT (WITHOUT SHM_GET)!\n");
        }
        exit(0);
        
    } else {
        
        wait(&pid);  
        
    }
    shm_rem("shmem1");
    printf("TEST TWO COMPLETE\n\n");
    exit(0);
}