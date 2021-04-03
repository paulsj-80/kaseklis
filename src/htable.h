#ifndef HTABLE_H
#define HTABLE_H

#include <stdint.h>
#include "utils.h"

struct t_kls_ht_item
{
    struct t_kls_ht_item* next;
    char* id;
    t_occ_id occ_pos;
};

struct t_kls_ht_item_0
{
    struct t_kls_ht_item item;
    t_bucket_size bucket_size;
};

struct t_kls_ht_context
{
    struct t_kls_ht_item_0* items;
    t_hash size;

    // stats
    uint64_t put_count;
};

void kls_ht_create(struct t_kls_ht_context* ht, t_hash size);
void kls_ht_destroy(struct t_kls_ht_context* ht);

void kls_ht_dump(struct t_kls_ht_context* ht, bool omit_empty);
void kls_ht_write(struct t_kls_ht_context* ht, 
                  const char* file_name0,
                  const char* file_name1);
void kls_ht_put(struct t_kls_ht_context* ht, const char* id, 
                t_occ_id occ_pos, t_occ_id* prev_occ_pos);
struct t_kls_ht_item* kls_ht_get(struct t_kls_ht_context* ht, 
                                 const char* id);

void kls_ht_dump_stats(struct t_kls_ht_context* ht);

bool kls_ht_get_occ_id(char* word,
                       const char* file_name0,
                       const char* file_name1,
                       t_occ_id* res);



#endif
