#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/riscv.h"

int main (void) {
    printf("\nTEST TWO: CHILD AND PARENT INTERACTIONS\n");
    const char * text = "test 2 message";
    char * shmem =  shm_get("shmem1");
    
    int pid = fork();
    
    if (pid == 0) {
        sleep(2);
        char* b = shmem;

        if (strcmp(b, text) == 0) {
            printf("SUCCESS! CHILD CAN ACCESS SAME SHARED MEM AS PARENT (WITHOUT SHM_GET)!\n");
        } else {
            printf("FAILURE! CHILD CAN ACCESS SAME SHARED MEM AS PARENT (WITHOUT SHM_GET)!\n");
        }
        exit(0);
        
    } else {
        strcpy(shmem, text);
        wait(&pid);  
        
    }
    shm_rem("shmem1");

    const char * part1 = "part 1 message";
    const char * part2 = "part 2 message";
    const char * part3 = "part 3 message";
    int pid2 = fork();

    if (pid2 == 0) {
        sleep(2);
        char* tester1 =  shm_get("1tester");
        char* tester2 =  shm_get("2tester");
        char* tester3 =  shm_get("3tester");

        strcpy(tester1, part1);
        strcpy(tester2, part2);
        strcpy(tester3, part3);

        exit(0);

    } else {
        
        char* tester1 =  shm_get("1tester");
        char* tester2 =  shm_get("2tester");
        char* tester3 =  shm_get("3tester");

        if (strcmp(tester1, part1) != 0 && strcmp(tester2, part2) != 0 && strcmp(tester3, part3) != 0) {
            printf("\tMEMORY BLANK BEFORE CHILD WRITE!\n");
        }
        else {
            printf("\tFAILURE! MEMORY POPULATED BEFORE CHILD WRITE!\n");
        }

        wait(&pid2);


        if (strcmp(tester1, part1) == 0 && strcmp(tester2, part2) == 0 && strcmp(tester3, part3) == 0) {
            printf("SUCCESS! WORKS FOR THREE MESSAGES\n");
        } else {
            printf("FAILURE! DOES NOT WORK FOR THREE MESSAGES\n");
        }
        shm_rem("1tester");
        shm_rem("2tester");
        shm_rem("3tester");
    }

    //TODO: ADD THREE MESSAGES BEFORE FORK


    
    //TODO: TEST THE MAX NUM
    char* array[SHM_MAXNUM];
    char names[SHM_MAXNUM][2];
    const char* size_message = "HOPEFULLY NOT TOO BIG";

    //doesnt work for bigger than 128
    for (uint64 i = 0; i < SHM_MAXNUM - 1; i++) {
        //create all but one process
        char next[2] = "";
        next[0] = (char)(((uint64)'a')+ i);
        names[i][0] = (char)(((uint64)'a')+ i);

        array[i] = shm_get(next);

        strcpy(array[i], size_message);
    }

    array[SHM_MAXNUM - 1] = shm_get("testing large");
    array[SHM_MAXNUM] = shm_get("testing too big");


    
    printf("\tBIGGEST MEM ADDRESS: %ld\n", (uint64) (array[SHM_MAXNUM - 1]));
    printf("\tBIGGER THAN BIGGEST: %ld\n", (uint64) (array[SHM_MAXNUM]));

    if ((uint64) (array[SHM_MAXNUM]) == (uint64) (array[SHM_MAXNUM - 1]) + PGSIZE) {
        printf("FAILURE! ALLOCATES PAGES BEYOND BIGGEST!\n");
    } else {
        printf("SUCCESS! DOESN'T ALLOCATE PAGES BEYOND BIGGEST!\n");
    }

    int flag = 0;
    for (int i = 0; i < SHM_MAXNUM - 2; i+=2) {
        shm_rem(names[i]);
        if (strcmp(array[i], size_message) == 0 || strcmp(array[i + 1], size_message) != 0) {
            flag = 1;
            printf("FAILURE WITH PATTERNED REMOVAL\n");
        }
    }

    if (flag == 0) {
        printf("SUCCESS! NO FAILURE WITH PATTERNED REMOVAL!\n");
    }
    


    


    printf("TEST TWO COMPLETE\n\n");
    exit(0);
}