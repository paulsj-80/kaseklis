#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "exit_codes.h"

// KLS03000
extern FILE* kls_ut_log_file_ptr;

// KLS07000
#define LOGI(_msg, ...) fprintf(stderr, "INFO %s:%d " _msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); if (kls_ut_log_file_ptr) fprintf(kls_ut_log_file_ptr, "INFO %s:%d " _msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);
#define LOGW(_msg, ...) fprintf(stderr, "WARN %s:%d " _msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); if (kls_ut_log_file_ptr) fprintf(kls_ut_log_file_ptr, "WARN %s:%d " _msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);
#define LOGE(_msg, ...) fprintf(stderr, "ERROR %s:%d " _msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); if (kls_ut_log_file_ptr) fprintf(kls_ut_log_file_ptr, "ERROR %s:%d " _msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);

#define KLS_CHECK(_expr, _exit_code, _msg, ...) if (!(_expr)) { LOGE("check fail: " _msg, ##__VA_ARGS__); exit(_exit_code); }
#define KLS_ASSERT(_expr, _msg, ...) KLS_CHECK(_expr, KLS_ASSERT_FAIL, _msg, ##__VA_ARGS__)

// KLS05012
#define KLS_IO_CHECK(_expr, _msg, ...) if (!(_expr)) { LOGE("assert fail: " _msg, ##__VA_ARGS__); exit(EX_IOERR); }


// KLS05012
#define FNAME_LEN 4096

#define MAX_INDEXABLE_FILE_SIZE (1024 * 1024)

// KLS01003, KLS05009, KLS06003
#define MAX_WORD_LEN 30

// KLS01004
#define MAX_DISPLAYABLE_LINE_LENGTH 4096

// 16 Gb by default
#define MAX_DYNAMIC_MEMORY_SIZE (1024l * 1024l * 1024l * 16l)

// KLS05000
// those have to be unsigned, mandatory
typedef uint32_t t_occ_id;
typedef uint32_t t_word_id;
typedef uint32_t t_file_id;
typedef uint32_t t_occ_file_id;
typedef uint32_t t_hash;
typedef uint32_t t_bucket_size;

typedef int bool;

extern uint64_t kls_full_one;

// KLS05006
#define KLS_CHECK_LIMIT(_var, _vtype) KLS_CHECK(_var < *((_vtype*)&kls_full_one), KLS_LIMIT_EXCEEDED, "limits exceeded for %s", #_var)

#define PATH_POSTPRINT_CHECK(_buff, _ref_name, _rc) {KLS_CHECK(_rc != FNAME_LEN, KLS_LIMIT_EXCEEDED, "file name too long %s %s", _buff, _ref_name); KLS_CHECK(_rc > 0, KLS_UNKNOWN_ERROR, "formatting error for %s", _ref_name);}

#define PATH_POSTPRINT_CHECK_U(_buff, _ref_u, _rc) {KLS_CHECK(_rc != FNAME_LEN, KLS_LIMIT_EXCEEDED, "file name too long %s %u", _buff, _ref_u); KLS_CHECK(_rc > 0, KLS_UNKNOWN_ERROR, "formatting error for %u", _ref_u);}




char* kls_ut_concat(const char* s0, const char* s1);
char* kls_ut_concat_fnames(const char* s0, const char* s1);
char* kls_ut_load_file(const char* name, uint64_t* fs);
uint64_t kls_ut_file_size(const char* name);

t_hash kls_ut_hash(const unsigned char *str);

void kls_ut_init_log_file(const char* fname);


extern const char* kls_ut_subdir;
extern const char* ignored_flag;


bool kls_ut_is_letter(char c);
bool kls_ut_is_number(char c);
bool kls_ut_is_word(char* w);


extern uint64_t allocated;
extern uint64_t peak_allocated;

// no safety checks; use them only where it makes sense for collecting
// stats and ensuring memory is not overfilled
char* kls_ut_malloc(size_t size);
void kls_ut_free(void* ptr, size_t size);

#endif
