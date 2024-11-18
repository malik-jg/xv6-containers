#include "kernel/types.h"
#include "user/user.h"

// int 
// main(void) {
//     printf("Hello, world!\n");
//     exit(0);
// }



// #include "kernel/types.h"
// #include "user/user.h"

int
main(void)
{
    int lock_id;
    lock_id = mutex_create("test_lock");
    if (lock_id < 0) {
        printf("Error: Failed to create mutex\n");
        exit(0);
    }

     mutex_lock(lock_id); 

     mutex_unlock(lock_id); 

     mutex_delete(lock_id); 

     exit(0);
}