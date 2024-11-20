#include "kernel/types.h"
#include "user/user.h"

int main(void) {
    // call myproc ???
    // do I need to edit the scheduler?
    // make success or fail conditional to interp output
    // test parent child relationship
    // test edge cases 
    int pid;
    int priority = 0;
    //int prio1 = 1;
    int res = 0;
    for (int i = 0; i < 8; i++) {
        //printf("%s ", "Here");
        pid = fork();

        if (pid == 0) {
            res = prio_set(pid, priority) ;
             
        }
        else{
            continue;
            
        }
        if (res == 0) {
            printf("Priority Set Successfully \n");
        }
        else {
            printf("Failed Priority Set \n");
        }
        printf("iteration");
    }
    // int res = 0;
    // if (pid == 0) {
    //     res = prio_set(pid, priority);
    // }
    //int res = prio_set(pid, priority);
    //printf("%d", res);
    // if (res == 0) {
    //     printf("Priority Set Successfully \n");
    // }
    // else {
    //     printf("Failed Priority Set \n");
    // }
    // printf("%d \n", res);
    // printf("Empty Implementation Works \n");
    return 0;
}