#include "kernel/types.h"
#include "user/user.h"

int main(void) {
    // call myproc ???
    // do I need to edit the scheduler?
    // make success or fail conditional to interp output
    // test parent child relationship
    // test edge cases 
    int pid = 0;
    int priority = 0;
    int res = prio_set(pid, priority);
    if (res == 0) {
        printf("Priority Set Successfully \n");
    }
    else {
        printf("Failed Priority Set \n");
    }
    // printf("%d \n", res);
    // printf("Empty Implementation Works \n");
    return 0;
}