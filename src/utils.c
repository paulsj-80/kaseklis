#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include "exit_codes.h"

FILE* kls_ut_log_file_ptr = 0;
uint64_t kls_full_one = -1;
const char* kls_ut_subdir = "/.kaseklis";
const char* ignored_flag = "/ignored";


char* kls_ut_concat(const char* s0, const char* s1)
{
    char* res = malloc(strlen(s0) + strlen(s1) + 1);
    strcpy(res, s0);
    strcpy(res + strlen(s0), s1);
    return res;
}

char* kls_ut_concat_fnames(const char* s0, const char* s1)
{
    size_t total_len = strlen(s0) + strlen(s1) + 1;
    char* res = malloc(total_len);
    strcpy(res, s0);
    strcpy(res + strlen(s0), s1);

    KLS_CHECK(total_len < FNAME_LEN, KLS_LIMIT_EXCEEDED,
              "file name total length exceeded %s", res);

    return res;
}

char* kls_ut_load_file(const char* name, uint64_t* fs)
{
    FILE* f = fopen(name, "r");
    KLS_IO_CHECK(f, "couldn't open %s", name);
    fseek(f, 0, SEEK_END);
    uint64_t sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* res = malloc(sz);

    KLS_IO_CHECK(fread(res, sz, 1, f) != sz, 
                 "couldn't read %s", name);
    
    fclose(f);
    *fs = sz;
    return res;
}

uint64_t kls_ut_file_size(const char* name)
{
    FILE* f = fopen(name, "r");
    KLS_IO_CHECK(f, "couldn't open %s", name);

    fseek(f, 0, SEEK_END);
    uint64_t sz = ftell(f);
    fclose(f);
    return sz;
}

t_hash kls_ut_hash(const unsigned char *str)
{
    t_hash hash = 5381;
    int c;

    while (c = *str++)
        hash = hash * 33 + c;

    return hash;
}

void kls_ut_init_log_file(const char* fname)
{
    kls_ut_log_file_ptr = fopen(fname, "w");
    KLS_IO_CHECK(kls_ut_log_file_ptr, 
                 "cannot open log file for writing %s", 
                 fname);
    setvbuf(kls_ut_log_file_ptr, NULL, _IONBF, 0);
}
