#include "user/jsmn.h"
#include "user/user.h"
#include "user/debug.h"
#include "user/cm.h"


int 
test1(){
    int cm_root_status = cm_setroot("/test1_container", strlen("/test1_container"));
    return 0;
}


int
test2(){


    return 0;
}

int 
main(void){
    int status;
    printf("Running Level 1 Tests\n");
    printf("Test 1: Changing root of file system in process: ");
    status = test1();
    result(status);
    printf("Test 2: Parsing test2.json Correctly: ");
    status = test2();
    result(status);


    return 0;
}