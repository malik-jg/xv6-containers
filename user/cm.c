#include "user/jsmn.h"
#include "user/user.h"
#include "user/debug.h"
#include "user/cm.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"

char *
read_file(const char *filename){
    int fd = open(filename, 0);
    if(fd < 0){
        printf("CM ERROR: Could not open file %s\n", filename);
        return NULL; //or exit()?
    }

    char *buffer = malloc(JSON_BUFFER_SIZE);
    if(!buffer){
        printf("ERROR: Memory allocation for buffer failed\n");
        close(fd);
        return NULL; //or exit()?
    }

    int n = read(fd, buffer, JSON_BUFFER_SIZE - 1);
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

int
directory_exists(const  char *path){
    struct stat st;
    if(stat(path, &st) == 0){
        if(st.type == T_DIR){
            return 0;
        }
        else{
            printf("ERROR: %s exists but is not a directory\n", path);
            return -1;
        }
    }

    if(mkdir(path) < 0){
        printf("ERROR: Failed to create directory %s\n", path);
        return -1;
    }
    return 0;
}

int 
copy_file(const char *src, const char *dst){
    int fdsrc = open(src, O_RDONLY);
    if(fdsrc < 0){
        printf("ERROR: Failed to open source file %s\n", src);
        return -1;
    }

    int fddst = open(dst, O_WRONLY | O_CREATE | O_TRUNC);
    if(fddst < 0){
        printf("ERROR: Failed to open destination file %s\n", dst);
        close(fdsrc);
        return -1;
    }
    int bytesRead, bytesWritten;
    char buffer[COPY_FILE_BUFFER_SIZE];
    while((bytesRead = read(fdsrc, buffer, COPY_FILE_BUFFER_SIZE)) > 0){
        bytesWritten = write(fddst, buffer, bytesRead);
        if(bytesWritten < bytesRead){
            printf("ERROR: Failed to write to destination file %s\n", dst);
            close(fdsrc);
            close(fddst);
            return -1;
        }   
    }
    if(bytesRead < 0){
        printf("ERROR: Failed to read from source file %s\n", src);
        close(fdsrc);
        close(fddst);
        return -1;
    }
    close(fdsrc);
    close(fddst);
    return 0;
}

int
copy_files_to_container(const char *dirname){
    char *files[] = {COPY_FILES};
    for(int i = 0; i < NUM_COPY_FILES; i++){
        char src[64];
        char dst[64];

        strcpy(src, files[i]);
        strcpy(dst, dirname);

        int len = strlen(dst);
        dst[len] = '/';
        strcpy(dst + len + 1, src);

        if(copy_file(src, dst) < 0){
            printf("ERROR: Failed to copy file %s to %s\n", src, dst);
            return -1;
        }
        printf("Copied %s to %s\n", src, dst);
    }


    return 0;
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
        int dir_status = directory_exists(root_fs);
        if(dir_status < 0){
            printf("ERROR: Failed to check if directory exists\n");
            exit(1);
        }
        int copy_status = copy_files_to_container(root_fs);
        if(copy_status < 0){
            printf("ERROR: Failed to copy files to container\n");
            exit(1);
        }
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
        printf("SUCCESS");
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
main(int argc, char *argv[]){
    printf("Container Manager starting...\n");
    printf("CM awaiting requests...\n");

    int fd = open(REQUEST_FILE, O_WRONLY | O_CREATE);
    if(fd < 0){
        printf("Error: Could not create or access the request file\n");
        exit(1);
    }
    close(fd);

    char buffer[REQUEST_BUFFER_SIZE];
    for(;;){
        fd = open(REQUEST_FILE, O_RDONLY);
        if(fd < 0){
            printf("Error: Could not open request file.\n");
            exit(1);
        }
        int n = read(fd, buffer, sizeof(buffer) - 1);
        close(fd);
        if(n > 0){
            buffer[n] = '\0';
            cm(buffer);
            unlink(REQUEST_FILE);
            break;
        } 
        sleep(1);
    }

    return 0;
}