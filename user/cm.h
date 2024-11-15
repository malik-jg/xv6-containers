#define BUFFER_SIZE 4096

char *read_file(const char *filename);
char **parse_json(const char *js);
void cm(const char *filename);