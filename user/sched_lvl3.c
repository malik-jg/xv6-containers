#include "kernel/types.h"
#include "user/user.h"

int main (void) {
    
    // int pid = getpid();
    // int prio = prio_get(pid);
    int i = 0;
    while (i < 100) {
        int pid = getpid();
        int prio = prio_get(pid);
        printf("PID: ");
        printf("%d\n", pid);
    
        printf("PRIORITY: ");
        printf("%d\n", prio);

        if (prio == 7) {
            printf("SUCCESS LVL 3: Priority demoted to last bucket \n");
            break;
        }
        i++;
    }
    
    return 0;
}