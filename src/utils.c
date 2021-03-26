#include "utils.h"
#include <stdlib.h>
#include <string.h>

char* kls_concat(const char* s0, const char* s1)
{
    char* res = malloc(strlen(s0) + strlen(s1) + 1);
    strcpy(res, s0);
    strcpy(res + strlen(s0), s1);
    return res;
}
