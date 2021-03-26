#ifndef UTILS_H
#define UTILS_H

// TODO: line number
#define LOG(...) fprintf(stderr, __VA_ARGS__)

#define FNAME_LEN 4096
#define MAX_WORD_LEN 0xffff
#define READ_WORD_BUFF_SIZE 0xffff



char* kls_concat(const char* s0, const char* s1);


#endif
