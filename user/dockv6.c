#include "kernel/types.h"
#include "user/user.h"
#include "user/cm.h"
#include "kernel/fcntl.h"

int 
main(int argc, char *argv[]){
    if(argc < 2){
        printf("Usage: dockv6 <command> [args]\n");
        exit(1);
    }

    if(strcmp(argv[1], "create") == 0){
        if (argc < 3) {
            printf("Usage: dockv6 create <filename>\n");
            exit(1);
        }

        char request[REQUEST_BUFFER_SIZE];
        strcpy(request, argv[2]);
        int fd = open(REQUEST_FILE, O_WRONLY);
        if(fd < 0){
            printf("Error: Could not open request file\n");
            exit(1);
        }
        write(fd, request, strlen(request));
        close(fd);
    } 
    else if(strcmp(argv[1], "io") == 0){

    }
    else{
        printf("Unsupported command: %s\n", argv[1]);
    }

    return 0;
}