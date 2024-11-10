#include "kernel/types.h"
#include "user/user.h"

int main(void) {
    int pid, priority;

    pid = 0;
    priority = 0;
    prio_set(pid, priority);
    printf("Empty Implementation Works \n");
    return 0;
}