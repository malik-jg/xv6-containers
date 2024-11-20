#define JSON_BUFFER_SIZE 4096
#define REQUEST_BUFFER_SIZE 256
#define COPY_FILE_BUFFER_SIZE 2048
#define REQUEST_FILE "/cm_request.txt"
#define NUM_COPY_FILES 13
#define COPY_FILES "cat", "echo", "grep", "grind", "init", "kill", "ln", "ls", "mkdir", "rm", "sh", "wc", "zombie"

char *read_file(const char *filename);
char **parse_json(const char *js);
void cm(const char *filename);