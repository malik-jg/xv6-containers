#define JSON_BUFFER_SIZE 4096
#define REQUEST_BUFFER_SIZE 256
#define COPY_FILE_BUFFER_SIZE 2048
#define CONTAINER_NAME_SIZE 256
#define REQUEST_FILE "/cm_request.txt"
#define CONTAINER_FILE "/container.txt"
#define INIT_FILE "/init.txt"
#define NUM_COPY_FILES 13
#define SIMPLE_NUM_COPY_FILES 2
#define COPY_FILES "cat", "echo", "grep", "grind", "init", "kill", "ln", "ls", "mkdir", "rm", "sh", "wc", "zombie"
#define SIMPLE_COPY_FILES "ls", "sh"

char *read_file(const char *filename);
char **parse_json(const char *js);
int cm(const char *filename);
int io(const char *container);