#include "kernel/types.h"
#include "user.h"

#define MAX_PROC 5

void test_mutex_blocking() {
    int muxid = mutex_create("test_mutex");
    if (muxid != 0){
        printf("FAIL");
    }

    int muxid2 = mutex_create("test_mutex2");
    if (muxid2 != 1){
        printf("FAIL \n");
    }

    int muxid3 = mutex_create("test_mutex2");
    printf("%d", muxid2);
    printf("%d", muxid3);
    if (muxid2 != muxid3){
        printf("same name test doesn't work \n");
    }

    if (muxid < 0) {
        printf("Mutex creation failed\n");
        exit(0);
    }


    int muxid4 = mutex_create("holder");
    int temp = muxid4;
    

    mutex_delete(muxid4);

    int muxid5 = mutex_create("meow");


    if (temp != muxid5){
        printf("delete doesn't work");
    }


    //printf("21 \n");

    mutex_lock(muxid);
    //printf("21 \n");
    mutex_unlock(muxid);
    //printf("21 \n");


    mutex_lock(muxid2);
   // printf("30 \n");
    mutex_unlock(muxid2);
    //printf("32 \n");

    if (muxid2 == muxid3){
        printf("SUCCESS IN LEVEL 0 CREATING\n");
    }
}

int main() {
    test_mutex_blocking();
    exit(0);
}
