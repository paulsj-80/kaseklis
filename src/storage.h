#ifndef STORAGE_H
#define STORAGE_H

#include "utils.h"

#define OCC_FILE_ITEM_COUNT (1024 * 128)

// KLS04007
#define HT_SIZE (1024 * 1024)

#define FILES_FILE0_ITEM_SIZE (sizeof(t_file_id) + 1 + sizeof(t_occ_id))


struct t_storage_context
{
    char* base_dir;
    size_t base_dir_len;
    char* kls_dir;

    char* words_file0; // KLS03003
    char* words_file1; // KLS03004
    char* files_file0; // KLS03001
    char* files_file1; // KLS03002
    char* index_log_file; // KLS03000
    char* nested_file;

    FILE* files_ptr0;
    FILE* files_ptr1;
    FILE* nested_ptr;

    t_file_id files_ptr1_written;
    t_occ_id occ_buff[OCC_FILE_ITEM_COUNT]; // KLS03006
    t_occ_id occ_buff_count;
    t_occ_file_id occ_file_count;
    t_file_id file_count;
    struct t_kls_ht_context* ht;
    t_occ_id total_occ_count;
    t_occ_id first_occ_in_file; // KLS04000

    char fname_to_write[FNAME_LEN];

    bool is_binary_to_write; // KLS02011
    char prev_cwd[FNAME_LEN];
    char output_file_prefix[FNAME_LEN];
};

void kls_st_init(struct t_storage_context* sh, const char* base_dir, 
                 char* output_file_prefix, bool purge);
void kls_st_finish(struct t_storage_context* sh, bool sync_to_hdd);

void kls_st_nested_ignored(struct t_storage_context* sh, 
                           const char* fname);

char* kls_st_get_base_dir(struct t_storage_context* sh);
void kls_st_add_file(struct t_storage_context* sh, const char* file, 
                     bool is_binary);
void kls_st_file_done(struct t_storage_context* sh);

void kls_st_add_word(struct t_storage_context* sh, const char* word);
void kls_st_dump_index_for(struct t_storage_context* sh, 
                           const char* word);

#endif
