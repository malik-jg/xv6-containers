#include "kernel/types.h"
#include "user/user.h"

int main(void) {
    int pid, priority;

    pid = 0;
    priority = 0;
    int res = prio_set(pid, priority);
    printf("%d \n", res);
    printf("Empty Implementation Works \n");
    return 0;
}