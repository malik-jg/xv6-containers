#include "kernel/types.h"
#include "user/user.h"

int main (void) {
    printf("\nTEST ONE: SIMPLE SHMEM W/ FORK API\n");
    int pid = fork();

    const char * text = "message";
    
    if (pid == 0) {
        sleep(2);
        char* a =  shm_get("hello");
        
        char* b = a;

        printf("\tCHILD READING MESSAGE: \"%s\"\n", b);
        if (strcmp(b, text) == 0) {
            printf("SUCCESS! CHILD CAN ACCESS SAME SHARED MEM AS PARENT!\n");
        }
        //shm_rem("hello");
        exit(0);
        
    } else {
        char* mem =  shm_get("hello");
        
        strcpy(mem, text);
        printf("\tPARENT SENDING MESSAGE: \"%s\"\n", text);
        
        
        wait(&pid);  
        shm_rem("hello");
    }
    printf("TEST ONE COMPLETE\n\n");
    exit(0);
}