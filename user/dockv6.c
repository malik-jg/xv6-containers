#include "kernel/types.h"
#include "user/user.h"
#include "user/cm.h"
#include "kernel/fcntl.h"

int 
main(int argc, char *argv[]){
    if(argc < 2 || strcmp(argv[1], "--help") == 0){
        printf("Usage: dockv6 <command> [args]\n");
        printf("Commands:\n");
        printf("  create <filename>  - Create a new container with the specified filename\n");
        printf("  io <container>     - Perform I/O operations on the specified container\n");
        printf("Options:\n");
        printf("  --help             - Display this help message\n");
        exit(0);
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
        if(argc < 3){
            printf("Usage: dockv6 io <container>\n");
            exit(1);
        }
        char container[CONTAINER_NAME_SIZE];
        strcpy(container, argv[2]);
        int fd = open(CONTAINER_FILE, O_WRONLY);
        if(fd < 0){
            printf("Error: Could not open container file\n");
            exit(1);
        }
        write(fd, container, strlen(container));
        close(fd);
    }
    else{
        printf("Unsupported command: %s\n", argv[1]);
    }
    exit(0);
    return 0;
}