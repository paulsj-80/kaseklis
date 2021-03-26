#ifndef HTABLE_H
#define HTABLE_H

#include <stdint.h>

struct KlsHtItem 
{
    struct KlsHtItem* next;
    char* id;
    uint64_t occ_pos;
};

struct KlsHtContext
{
    struct KlsHtItem* items;
    uint32_t size;
    uint64_t collisions;
};

int kls_create_ht(struct KlsHtContext* ht, uint32_t size);
int kls_write_ht(struct KlsHtContext* ht, const char* file_name);
int kls_destroy_ht(struct KlsHtContext* ht);
struct KlsHtItem* kls_get(struct KlsHtContext* ht, const char* id);
int kls_put(struct KlsHtContext* ht, const char* id, uint64_t occ_pos, 
            uint64_t* prev_occ_pos);

void kls_ht_dump(struct KlsHtContext* ht, int omit_empty, 
                 int collision_number_only);


#endif
