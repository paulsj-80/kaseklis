#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include "exit_codes.h"

FILE* kls_ut_log_file_ptr = 0;
uint64_t kls_full_one = -1;
// KLS01002
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
    char* res = 0;
    FILE* f = fopen(name, "r");
    KLS_IO_CHECK(f, "couldn't open %s", name);
    fseek(f, 0, SEEK_END);
    uint64_t sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz > 0)
    {
        res = kls_ut_malloc(sz);
        size_t rs = fread(res, sz, 1, f);
        KLS_IO_CHECK(rs == 1, "couldn't read %s", name);
    }
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

bool kls_ut_is_letter(char c)
{
    // KLS02000, KLS02001
    return 
        ((c >= 'a' && c <= 'z') || 
         (c >= 'A' && c <= 'Z') || (c == '_')) ? 1 : 0;
}

bool kls_ut_is_number(char c)
{
    // KLS02002
    return (c >= '0' && c <= '9');
}

bool kls_ut_is_word(char* w)
{
    // KLS02003
    size_t wl = strlen(w);
    if (!kls_ut_is_letter(*w))
        return 0;
    w++;
    for (size_t i = 1; i < wl; i++)
    {
        if (!kls_ut_is_letter(*w) && !kls_ut_is_number(*w))
            return 0;
        w++;
    }
    return 1;
}

uint64_t allocated = 0;
uint64_t peak_allocated = 0;


char* kls_ut_malloc(size_t size)
{
    allocated += size;
    KLS_CHECK(allocated < MAX_DYNAMIC_MEMORY_SIZE, KLS_LIMIT_EXCEEDED,
              "allowed memory is full, adjust MAX_DYNAMIC_MEMORY_SIZE");
    peak_allocated = peak_allocated > allocated ? peak_allocated : 
        allocated;
    return malloc(size);
}

void kls_ut_free(void* ptr, size_t size)
{
    allocated -= size;
    free(ptr);
}

int not_letter_or_number[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

int is_letter[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

