#include "user/jsmn.h"
#include "user/user.h"
#include "user/debug.h"
#include "user/cm.h"
#include "kernel/fcntl.h"


char *
read_file(const char *filename){
    int fd = open(filename, 0);
    if(fd < 0){
        printf("CM ERROR: Could not open file %s\n", filename);
        return NULL; //or exit()?
    }

    char *buffer = malloc(BUFFER_SIZE);
    if(!buffer){
        printf("ERROR: Memory allocation for buffer failed\n");
        close(fd);
        return NULL; //or exit()?
    }

    int n = read(fd, buffer, BUFFER_SIZE - 1);
    if(n < 0){
        printf("ERROR: Could not read file %s\n", filename);
        free(buffer);
        close(fd);
        return NULL; //or exit()?
    }

    buffer[n] = '\0';
    close(fd);
    return buffer;
}

char **
parse_json(const char *js){
    jsmn_parser parser;
    jsmntok_t tokens[MAX_TOKENS];
    jsmn_init(&parser);

    int number_of_tokens = jsmn_parse(&parser, js, strlen(js), tokens, MAX_TOKENS);

    if(number_of_tokens < 0){
        if(number_of_tokens == JSMN_ERROR_NOMEM){
            printf("ERROR: Not enough tokens were provided\n");
        }
        else if(number_of_tokens == JSMN_ERROR_INVAL){
            printf("ERROR: Invalid character inside JSON string\n");
        }
        else if(number_of_tokens == JSMN_ERROR_PART){
            printf("ERROR: The string is not a full JSON packet, more bytes expected\n");
        }
        return NULL; //or exit()?
    }
    if(number_of_tokens > MAX_TOKENS){
        printf("ERROR: Number of tokens exceeds maximum number of tokens\n");
        return NULL; //or exit()?
    }   

    char **token_values = (char **)malloc(number_of_tokens * sizeof(char *));
    if(!token_values){
        printf("ERROR: Memory allocation for token_values array failed\n");
        return NULL; //or exit()?
    }

    for(int i = 0; i < number_of_tokens; i++) {
        jsmntok_t token = tokens[i];

        xv6_size_t length = token.end - token.start;
        if(length >= MAX_TOKEN_LENGTH){
            printf("ERROR: Token length exceeds maximum token length\n");
            return NULL; //or exit()?
        }
        token_values[i] = (char *)malloc(length + 1);
        if(token_values[i] == NULL){
            printf("ERROR: Memory allocation for token value failed\n");
            return NULL; //or exit()?
        }
        strncpy(token_values[i], js + token.start, length);
        token_values[i][length] = '\0';

        debug_print_int("Stored Token" , i); 
        debug_print_string("Stored Token Value", token_values[i]);
    }

    return token_values;
}


void 
cm(const char *filename){
    char *json_content = read_file(filename);
    if(!json_content){
        printf("ERROR: Failed to get JSON content\n");
        exit(1);
    }
    char **token_values = parse_json(json_content);
    if(token_values == NULL){
        printf("ERROR: Failed to parse JSON content\n");
        free(json_content);
        exit(1);
    }

    char* init = token_values[2];
    char* root_fs = token_values[4];

    char *max_processes_str = token_values[6];
    int max_processes = atoi(max_processes_str);

    printf("CM creating container with init = %s, root fs = %s, and max num processes = %d\n", init, root_fs, max_processes);

    int pid = fork();

    if(pid == 0){
        int cm_status = cm_create_and_enter();
        if(cm_status < 0){
			printf("ERROR: Failed to create and enter container\n");
			exit(1);
		}
        cm_status = cm_setroot(root_fs, strlen(root_fs));
        if(cm_status < 0){
            printf("ERROR: Failed to set root file system\n");
            exit(1);
        }
        cm_status = cm_maxproc(max_processes);
        if(cm_status < 0){
            printf("ERROR: Failed to set max number of processes\n");
            exit(1);
        }
    }
    wait(0);

    //REMEMBER TO FREE LATER, IDK WHEN, FIGURE OUT LATER
    for(int i = 0; i < MAX_TOKENS && token_values[i] != NULL; i++){
        free(token_values[i]);
    }
    free(token_values);
    free(json_content);

}

int 
main(int argc, char *argv[]) {
    printf("Container Manager Starting\n");
    printf("CM awaiting requests...\n");
    for(;;){
        
    }
    return 0;
}