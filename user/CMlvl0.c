#include "user/jsmn.h"
#include "user/user.h"
#include "user/debug.h"
#include "user/cm.h"
//-----------------------COPIED FROM CM.C-------------------------------
char *
read_file(const char *filename){
    int fd = open(filename, 0);
    if(fd < 0){
        //printf("CM ERROR: Could not open file %s\n", filename);
        return NULL;
    }

    char *buffer = malloc(JSON_BUFFER_SIZE);
    if(!buffer){
        //printf("ERROR: Memory allocation for buffer failed\n");
        close(fd);
        return NULL;
    }

    int n = read(fd, buffer, JSON_BUFFER_SIZE - 1);
    if(n < 0){
        //printf("ERROR: Could not read file %s\n", filename);
        free(buffer);
        close(fd);
        return NULL;
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
            //printf("ERROR: Not enough tokens were provided\n");
        }
        else if(number_of_tokens == JSMN_ERROR_INVAL){
            //printf("ERROR: Invalid character inside JSON string\n");
        }
        else if(number_of_tokens == JSMN_ERROR_PART){
            //printf("ERROR: The string is not a full JSON packet, more bytes expected\n");
        }
        return NULL; //or exit()?
    }
    if(number_of_tokens > MAX_TOKENS){
        printf("ERROR: Number of tokens exceeds maximum number of tokens\n");
        return NULL; //or exit()?
    }   

    char **token_values = (char **)malloc(number_of_tokens * sizeof(char *));
    if(!token_values){
        //printf("ERROR: Memory allocation for token_values array failed\n");
        return NULL; //or exit()?
    }

    for(int i = 0; i < number_of_tokens; i++) {
        jsmntok_t token = tokens[i];

        xv6_size_t length = token.end - token.start;
        if(length >= MAX_TOKEN_LENGTH){
            //printf("ERROR: Token length exceeds maximum token length\n");
            return NULL; //or exit()?
        }
        token_values[i] = (char *)malloc(length + 1);
        if(token_values[i] == NULL){
            //printf("ERROR: Memory allocation for token value failed\n");
            return NULL; //or exit()?
        }
        strncpy(token_values[i], js + token.start, length);
        token_values[i][length] = '\0';

        debug_print_int("Stored Token" , i); 
        debug_print_string("Stored Token Value", token_values[i]);
    }

    return token_values;
}
//------------------------------------------------------------------


int 
test1(char * parse){
    const char *filename = "test1.json";
    const char *test_init = "/init";
    const char *test_root_fs = "/fs";
    const int test_max_processes = 8;


    char *json_content = read_file(filename);
    if(json_content == NULL){
        return -1;
    }
    char **token_values = parse_json(json_content);
    if(token_values == NULL){
        free(json_content);
        return -1;
    }
    char* init = token_values[INIT_TOKEN];
    char* root_fs = token_values[ROOT_FS_TOKEN];

    char *max_processes_str = token_values[MAX_PROCESSES_TOKEN];
    int max_processes = atoi(max_processes_str);
    
    if(strcmp(init, test_init) != 0){
        return -1;
    }
    if(strcmp(root_fs, test_root_fs) != 0){
        return -1;
    }
    if(max_processes != test_max_processes){
        return -1;
    }
    

    strcpy(parse, token_values[ENTIRE_TOKEN]);

    return 0;
}


int 
test2(char *parse){
    const char *filename = "test2.json";
    const char *test_init = "/sh";
    const char *test_root_fs = "/c0";
    const int test_max_processes = 4;


    char *json_content = read_file(filename);
    if(json_content == NULL){
        return -1;
    }
    char **token_values = parse_json(json_content);
    if(token_values == NULL){
        free(json_content);
        return -1;
    }
    char* init = token_values[INIT_TOKEN];
    char* root_fs = token_values[ROOT_FS_TOKEN];

    char *max_processes_str = token_values[MAX_PROCESSES_TOKEN];
    int max_processes = atoi(max_processes_str);
    
    if(strcmp(init, test_init) != 0){
        return -1;
    }
    if(strcmp(root_fs, test_root_fs) != 0){
        return -1;
    }
    if(max_processes != test_max_processes){
        return -1;
    }

    strcpy(parse, token_values[ENTIRE_TOKEN]);

    return 0;
}


int 
test3(){
    const char *filename = "test3.json";

    char *json_content = read_file(filename);
    if(json_content == NULL){
        return -1;
    }
    char **token_values = parse_json(json_content);
    if(token_values == NULL){
        free(json_content);
        return 0;
    }
    return -1;
}

void 
result(int status){
    if(status < 0){
        printf("FAILED\n");
    }
    else{
        printf("SUCCESS\n");
    }
}

int 
main(void){
    int status;
    char *parse = malloc(256);
    printf("Running Level 0 Tests\n");
    printf("-------------------------------------------------------------------------------\n");
    printf("Test 1: Parsing test1.json Correctly: ");
    status = test1(parse);
    result(status);
    printf("Test 1 Parsed: %s\n", parse);

    printf("Test 2: Parsing test2.json Correctly: ");
    status = test2(parse);
    result(status);
    printf("Test 2 Parsed: %s\n", parse);

    printf("Test 3: Detect Invalid JSON Character: ");
    status = test3();
    result(status);


    free(parse);
    return 0;
}