#include "user/jsmn.h"
#include "user/user.h"
#include "user/debug.h"

#define BUFFER_SIZE 4096

char *
read_file(const char *filename){
    int fd = open(filename, 0);
    if(fd < 0){
        printf("Error: Could not open file %s\n", filename);
        return NULL;
    }

    char *buffer = malloc(BUFFER_SIZE);
    if(!buffer){
        printf("Error: Memory allocation failed\n");
        close(fd);
        return NULL;
    }

    int n = read(fd, buffer, BUFFER_SIZE - 1);
    if(n < 0){
        printf("Error: Could not read file %s\n", filename);
        free(buffer);
        close(fd);
        return NULL;
    }

    buffer[n] = '\0';
    close(fd);
    return buffer;
}
void 
parse_json(const char *js){
    jsmn_parser parser;
    jsmntok_t tokens[100];
    jsmn_init(&parser);

    int n = jsmn_parse(&parser, js, strlen(js), tokens, 100);
    if(n < 0){
        if(n == JSMN_ERROR_NOMEM){
            printf("Error: Not enough tokens were provided\n");
        }
        else if(n == JSMN_ERROR_INVAL){
            printf("Error: Invalid character inside JSON string\n");
        }
        else if(n == JSMN_ERROR_PART){
            printf("Error: The string is not a full JSON packet, more bytes expected\n");
        }
    }

    debug_print_int("Number of Tokens", n);
    debug_print("\n");

    for(int i = 0; i < n; i++){
        jsmntok_t token = tokens[i];
        const char *type_str;

        if(token.type == JSMN_PRIMITIVE){
            type_str = "Primitive";
        }
        else if(token.type == JSMN_OBJECT){
            type_str = "Object";
        }
        else if(token.type == JSMN_ARRAY){
            type_str = "Array";
        }
        else if(token.type == JSMN_STRING){
            type_str = "String";
        }
        else{
            type_str = "Unknown";
        }


        xv6_size_t length = token.end - token.start;
        char value[length + 1];
        strncpy(value, js + token.start, length);
        value[length] = '\0';

        debug_print_int("Token", i);
        debug_print_string("Type", type_str);
        debug_print_string("Value", value);
        debug_print_int("Start", token.start);
        debug_print_int("End", token.end);
        debug_print_int("Size", token.size);
        debug_print("\n");
    }
}

int 
main(int argc, char *argv[]){
    set_debug_mode(argc, argv);
    const char *filename = "example.json";

    debug_print_string("Parsing JSON file", filename);
    debug_print("\n");

    char *json_content = read_file(filename);
    if(!json_content){
        printf("ERROR: Failed to get JSON content\n");
        return -1;
    }
    printf("SUCCESS: read JSON file %s with jmsn\n", filename);
    debug_print("JSON file read successfully. Beginning parse...");

    parse_json(json_content);

    debug_print("JSON content parsed successfully.");

    free(json_content);

    printf("SUCCESS: parsed and tokenized JSON file %s with jmsn\n", filename);
    return 0;
}