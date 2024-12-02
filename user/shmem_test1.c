#include "kernel/types.h"
#include "user/user.h"

int main (void) {
    int pid = fork();

    if (pid == 0) {
        sleep(2);
        char* a = (char* )shm_get("hello");
        printf("CHILD %ld\n", (uint64)a);

        printf("READING FROM CHILD: %d\n", a[0]);

        //printf("%d\n", a[0]);

    } else {
        char* mem = (char*) shm_get("hello");
        const char * text = "hello";
        printf("PARENT ADDR: %ld\n",(uint64) mem);
        strcpy(mem, text);
        shm_get("hello2");        
        wait(&pid);
    }
}