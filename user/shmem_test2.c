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

    const char * part1 = "part 1 message";
    const char * part2 = "part 2 message";
    const char * part3 = "part 3 message";
    int pid2 = fork();

    if (pid2 == 0) {
        sleep(2);
        char* tester1 = (char*) shm_get("1tester");
        char* tester2 = (char*) shm_get("2tester");
        char* tester3 = (char*) shm_get("3tester");

        strcpy(tester1, part1);
        strcpy(tester2, part2);
        strcpy(tester3, part3);

        

        printf("GOING TO EXIT\n");
        exit(0);

    } else {
        
        char* tester1 = (char*) shm_get("1tester");
        char* tester2 = (char*) shm_get("2tester");
        char* tester3 = (char*) shm_get("3tester");

        wait(&pid2);

        printf("1: %s\n", tester1);
        printf("2: %s\n", tester2);
        printf("3: %s\n", tester3);

        if (strcmp(tester1, part1) == 0 && strcmp(tester2, part2) == 0 && strcmp(tester3, part3) == 0) {
            printf("SUCCESS! WORKS FOR THREE MESSAGES\n");
        }
    }



    printf("TEST TWO COMPLETE\n\n");
    exit(0);
}