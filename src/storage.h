#ifndef STORAGE_H
#define STORAGE_H

int kls_init_storage(const char* base_dir, int purge);

void kls_add_word(const char* word);
void kls_next_line();
void kls_add_file(const char* file);
void kls_is_binary();
void kls_file_done();

int kls_is_storage_folder(const char* fname);

void kls_finish_storage(int sync_to_hdd);

char* kls_get_base_dir();
void kls_dump_index_for(const char* word);

#endif
