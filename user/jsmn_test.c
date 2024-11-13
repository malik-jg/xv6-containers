#include "user/jsmn.h"
#include "user/user.h"

#define BUFFER_SIZE 8192

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
    printf("Number of Tokens: %d\n", n);

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
        strcpy(value, js + token.start);
        value[length] = '\0';

        printf("Token %d:\n", i);
        printf("Type: %s\n", type_str);
        printf("Value: '%s'\n", value);
        printf("Start: %d\n", token.start);
        printf("End: %d\n", token.end);
        printf("Size: %d\n\n", token.size);
    }
}

int 
main(void){
    const char *filename = "example.json";
    printf("Parsing JSON file: %s\n", filename);
    char *json_content = read_file(filename);
    if(!json_content){
        return 1;
    }

    parse_json(json_content);
    free(json_content);

    return 0;
}