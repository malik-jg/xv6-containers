#include "debug.h"

int DEBUG = 0;

void 
set_debug_mode(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            DEBUG = 1;
            printf("Debug mode enabled\n");
        }
    }
}

void 
debug_print(const char *message){
    if(DEBUG){
        if(message && message[0] == '\n' && message[1] == '\0'){
            printf("\n");
        }
        else{
            printf("[DEBUG] %s\n", message);
        }
    }
}


void 
debug_print_int(const char *var_name, int value) {
    if(DEBUG){
        printf("[DEBUG] %s: %d\n", var_name, value);
    }
}

void 
debug_print_string(const char *var_name, const char *value) {
    if(DEBUG){
        printf("[DEBUG] %s: %s\n", var_name, value);
    }
}