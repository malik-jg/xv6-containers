#ifndef DEBUG_H
#define DEBUG_H
#include "kernel/types.h"
#include "user/user.h"

extern int DEBUG;

void set_debug_mode(int argc, char *argv[]);
void debug_print(const char *message);
void debug_print_int(const char *var_name, int value);
void debug_print_string(const char *var_name, const char *value);



#endif