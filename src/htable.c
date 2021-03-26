#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "htable.h"
#include "utils.h"


int kls_create_ht(struct KlsHtContext* ht, uint32_t size)
{
    ht->items = (struct KlsHtItem*)malloc(sizeof(struct KlsHtItem) * size);
    LOG("i: hash table byte size is %lu\n", sizeof(struct KlsHtItem) * size);
    ht->size = size;
    ht->collisions = 0;
    memset(ht->items, 0, sizeof(struct KlsHtItem) * size);
}

int kls_destroy_ht(struct KlsHtContext* ht)
{
    for (uint32_t i = 0; i < ht->size; i++) 
    {
        if (ht->items[i].id)
            free(ht->items[i].id);
        struct KlsHtItem* p = ht->items[i].next;
        while (p)
        {
            struct KlsHtItem* p2 = p;
            p = p->next;
            free(p2->id);
            free(p2);
        }
    }
    free(ht->items);
}

uint32_t kls_hash(const unsigned char *str)
{
    uint32_t hash = 5381;
    int c;

    while (c = *str++)
        hash = hash * 33 + c;
        //hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

uint32_t kls_ht_hash(struct KlsHtContext* ht, const unsigned char *str)
{
    return kls_hash(str) % ht->size;
}

struct KlsHtItem* kls_get(struct KlsHtContext* ht, const char* id) 
{
    uint32_t h = kls_ht_hash(ht, id);

    struct KlsHtItem* p = ht->items + h;

    while (p && (!p->id || strcmp(p->id, id) != 0))
        p = p->next;

    return p->id ? p : 0;
}

void kls_ht_dump(struct KlsHtContext* ht, int omit_empty,
                 int collision_number_only) 
{
    if (!collision_number_only)
    {
        printf("*** kls_ht_dump ***\n");

        for (uint32_t i = 0; i < ht->size; i++) 
        {
            struct KlsHtItem* p = ht->items + i;
            if (omit_empty && !p->id)
                continue;
            printf("%dl:\n", i);
            while (p)
            {
                printf("  %s: %lu\n", p->id, p->occ_pos);
                p = p->next;
            }
        }

        printf("*** *** ***\n");
        printf("size: %d\n", ht->size);
        printf("collisions: %lu\n", ht->collisions);
    }
    else
    {
        printf("htable collisions: %lu\n", ht->collisions);
    }
}

int kls_put(struct KlsHtContext* ht, const char* id, uint64_t occ_pos, 
            uint64_t* prev_occ_pos) 
{
    if (!id || strlen(id) == 0)
        return 1;
    uint32_t h = kls_ht_hash(ht, id);
    struct KlsHtItem* p0 = 0;
    struct KlsHtItem* p = ht->items + h;
    while (p && (!p->id || strcmp(p->id, id) != 0))
    {
        if (p->id)
            p0 = p;
        p = p->next;
    }
    if (p)
    {
        *prev_occ_pos = p->occ_pos;
        p->occ_pos = occ_pos;
    } 
    else 
    {
        if (p0) 
        {
            p = (struct KlsHtItem*)malloc(sizeof(struct KlsHtItem));
            memset(p, 0, sizeof(struct KlsHtItem));
            p->id = malloc(strlen(id) + 1); 
            strcpy(p->id, id);
            p->occ_pos = occ_pos;
            *prev_occ_pos = 0;
            p0->next = p;
            ht->collisions++;
        }
        else
        {
            ht->items[h].occ_pos = occ_pos;
            ht->items[h].id = malloc(strlen(id) + 1); 
            strcpy(ht->items[h].id, id);
            *prev_occ_pos = 0;
        }
    }
    return 0;
}

int kls_write_ht(struct KlsHtContext* ht, const char* file_name)
{
    FILE* f = fopen(file_name, "w");
    for (uint32_t i = 0; i < ht->size; i++) 
    {
        struct KlsHtItem* p = ht->items + i;
        if (!p->id)
            continue;
        while (p)
        {
            fwrite(p->id, 1, strlen(p->id) + 1, f);
            fwrite((char*)&p->occ_pos, 1, sizeof(uint64_t), f);
            p = p->next;
        }
    }
    fclose(f);
}


