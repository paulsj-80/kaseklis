#ifndef STORAGE_H
#define STORAGE_H

#include "utils.h"

void kls_st_init(const char* base_dir, bool purge);
void kls_st_finish(bool sync_to_hdd);

void kls_st_nested_ignored(const char* fname);

char* kls_st_get_base_dir();
void kls_st_add_file(const char* file, bool is_binary);
void kls_st_file_done();

void kls_st_add_word(const char* word);
void kls_st_dump_index_for(const char* word);

#endif
