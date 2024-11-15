#include "kernel/types.h"
#include "user/user.h"
#include "user/cm.h"

int 
main(int argc, char *argv[]){
    if(argc < 2){
        printf("Usage: dockv6 <command> [args]\n");
        exit(1);
    }

    return 0;
}