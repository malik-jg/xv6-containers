#include "kernel/types.h"
#include "user/user.h"

int main (void) {
    int pid = fork();

    if (pid == 0) {
        sleep(2);
        char* a = (char* )shm_get("hello");
        printf("CHILD %ld\n", (uint64)a);
        //printf("%d\n", a[0]);

    } else {
        uint64* mem = (uint64*) shm_get("hello");
        char * text = "hello\0";
        printf("PARENT ADDR: %ld\n",(uint64) mem);
        //shm_get("hello2");        
        wait(&pid);
    }
}