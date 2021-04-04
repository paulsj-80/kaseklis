#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "htable.h"
#include "utils.h"

#define READ_WORD_BUFF_SIZE 4096

void free_item(struct t_kls_ht_item* item, bool completely)
{
    if (item->id)
        kls_ut_free(item->id, strlen(item->id) + 1);
    if (completely)
        kls_ut_free(item, sizeof(struct t_kls_ht_item));
}

t_hash get_hash(struct t_kls_ht_context* ht, const unsigned char *str)
{
    return kls_ut_hash(str) % ht->size;
}

void kls_ht_create(struct t_kls_ht_context* ht, t_hash size)
{
    memset(ht, 0, sizeof(struct t_kls_ht_context));
    t_hash hsize = sizeof(struct t_kls_ht_item_0) * size;
    ht->items = (struct t_kls_ht_item_0*)kls_ut_malloc(hsize);
    LOGI("hash table byte size is %d", hsize);
    ht->size = size;
    memset(ht->items, 0, hsize);
}

void kls_ht_destroy(struct t_kls_ht_context* ht)
{
    for (t_hash i = 0; i < ht->size; i++) 
    {
        struct t_kls_ht_item* j = ht->items[i].item.next;
        free_item(&ht->items[i].item, 0);

        while (j)
        {
            struct t_kls_ht_item* tmp = j->next;
            free_item(j, 1);
            j = tmp;
        }
    }
    kls_ut_free(ht->items, ht->size);
}

void kls_ht_dump(struct t_kls_ht_context* ht, bool omit_empty) 
{
    printf("*** kls_ht_dump ***\n");

    t_hash collisions = 0;

    for (t_hash i = 0; i < ht->size; i++) 
    {
        struct t_kls_ht_item_0* zero_p = ht->items + i;
        struct t_kls_ht_item* p = &ht->items[i].item;
        if (omit_empty && !p->id)
            continue;
        if (zero_p->bucket_size > 1)
            collisions += zero_p->bucket_size - 1;
        printf("%dl: %d\n", i, zero_p->bucket_size);
        while (p)
        {
            printf("  %s: %d\n", p->id, p->occ_pos);
            p = p->next;
        }
    }

    printf("*** *** ***\n");
    printf("size: %d\n", ht->size);
    printf("collisions: %d\n", collisions);
}

void kls_ht_write(struct t_kls_ht_context* ht, 
                  const char* file_name0,
                  const char* file_name1)
{
    FILE* f0 = fopen(file_name0, "w");
    FILE* f1 = fopen(file_name1, "w");

    t_hash zero = 0;

    // first byte is reserved, as there will be many references to
    // null inside f0
    t_hash f1_size = 1;
    KLS_IO_CHECK(fwrite((char*)&zero, 1, 1, f1) == 1,
                 "couldn't write %s", file_name1);

    for (t_hash i = 0; i < ht->size; i++) 
    {
        struct t_kls_ht_item_0* zero_p = ht->items + i;
        if (zero_p->bucket_size == 0)
        {
            KLS_IO_CHECK(fwrite((char*)&zero, sizeof(t_hash), 1, f0) == 1,
                         "couldn't write %s", file_name0);
            continue;
        }
        KLS_IO_CHECK(fwrite((char*)&f1_size, sizeof(t_hash), 1, f0) == 1,
                     "couldn't write %s", file_name0);

        struct t_kls_ht_item* p = &zero_p->item;
        while (p)
        {
            int ssize = strlen(p->id) + 1;
            KLS_IO_CHECK(fwrite(p->id, ssize, 1, f1) == 1,
                         "couldn't write %s", file_name1);
            KLS_IO_CHECK(fwrite((char*)&p->occ_pos, sizeof(t_occ_id), 
                                1, f1) == 1,
                         "couldn't write %s", file_name1);
            f1_size += ssize + sizeof(t_occ_id);
            p = p->next;
        }
        fwrite((char*)&zero, 1, 1, f1);
        f1_size++;
    }

    fclose(f1);
    fclose(f0);
}

bool kls_ht_put(struct t_kls_ht_context* ht, const char* id, 
                t_occ_id occ_pos, t_occ_id* prev_occ_pos,
                t_occ_id replace_if_lower_than)
{
    KLS_ASSERT(id && strlen(id) > 0, "bad id to hash");

    bool res = 0;

    ht->put_count++;

    t_hash h = get_hash(ht, id);
    struct t_kls_ht_item_0* zero_p = ht->items + h;

    struct t_kls_ht_item* p0 = 0;
    struct t_kls_ht_item* p = &zero_p->item;
    while (p && (!p->id || strcmp(p->id, id) != 0))
    {
        if (p->id)
            p0 = p;
        p = p->next;
    }
    if (p)
    {
        if (p->occ_pos < replace_if_lower_than)
        {
            *prev_occ_pos = p->occ_pos;
            p->occ_pos = occ_pos;
            res = 1;
        }
    } 
    else 
    {
        if (p0) 
        {
            p = (struct t_kls_ht_item*)kls_ut_malloc(
                        sizeof(struct t_kls_ht_item));
            memset(p, 0, sizeof(struct t_kls_ht_item));
            p->id = kls_ut_malloc(strlen(id) + 1); 
            strcpy(p->id, id);
            p->occ_pos = occ_pos;
            *prev_occ_pos = 0;
            p0->next = p;
        }
        else
        {
            KLS_ASSERT(zero_p->item.id == 0, 
                       "hash array not initialized or corrupted");
            zero_p->item.occ_pos = occ_pos;
            zero_p->item.id = kls_ut_malloc(strlen(id) + 1); 
            strcpy(zero_p->item.id, id);
            *prev_occ_pos = 0;
        }
        zero_p->bucket_size++;
        KLS_CHECK_LIMIT(zero_p->bucket_size, t_bucket_size);
        res = 1;
    }
    return res;
}

struct t_kls_ht_item* kls_ht_get(struct t_kls_ht_context* ht, 
                                 const char* id)
{
    t_hash h = get_hash(ht, id);

    struct t_kls_ht_item* p = &ht->items[h].item;

    while (p && (!p->id || strcmp(p->id, id) != 0))
        p = p->next;

    if (!p)
        return 0;

    return p->id ? p : 0;
}

void kls_ht_dump_stats(struct t_kls_ht_context* ht)
{
    int bsize = sizeof(t_bucket_size) * 8 + 1;
    t_bucket_size oldest_bit = 1 << (sizeof(t_bucket_size) * 8 - 1);
    t_bucket_size bucket_stats[bsize];
    memset(bucket_stats, 0, sizeof(bucket_stats));
    // this shouldn't be exceeded
    uint64_t word_count = 0;
    
    for (t_hash i = 0; i < ht->size; i++) 
    {
        struct t_kls_ht_item_0* zero_p = ht->items + i;
        t_bucket_size zz = zero_p->bucket_size;
        word_count += zz;
        if (zz > oldest_bit)
        {
            bucket_stats[bsize - 1]++;
        }
        else
        {
            int b0 = 0;
            while (zz > 0)
            {
                zz = zz >> 1;
                b0++;
            }
            bucket_stats[b0]++;
        }
    }

    LOGI("bucket count by size:");
    LOGI("  =0: %d", bucket_stats[0]);

    for (uint32_t i = 1; i < bsize - 1; i++)
    {
        LOGI("  <%u: %d", 1 << i, bucket_stats[i]);
    }
    LOGI("  <%u: %d", -1, bucket_stats[bsize - 1]);

    LOGI("put_count: %lu", ht->put_count);
    LOGI("word_count: %lu", word_count);
}


bool kls_ht_get_occ_id(char* word,
                       const char* file_name0,
                       const char* file_name1,
                       t_occ_id* res,
                       t_hash ht_size)
{
    FILE* f0 = fopen(file_name0, "r");
    KLS_IO_CHECK(f0, "cannot open for read %s", file_name0);

    t_hash h = kls_ut_hash(word) % ht_size;
    fseek(f0, h * sizeof(t_hash), SEEK_SET);

    t_word_id offset;
    KLS_IO_CHECK(fread((char*)&offset, sizeof(offset), 1, f0) == 1,
                 "couldn't read from %s", file_name0);
    fclose(f0);

    if (offset == 0)
        return 0;

    FILE* f1 = fopen(file_name1, "r");
    KLS_IO_CHECK(f1, "cannot open for read %s", file_name1);
    
    fseek(f1, offset, SEEK_SET);

    int part = 0;
    bool found = 0;
    bool stop = 0;

    char buff[READ_WORD_BUFF_SIZE];
    char buff2[MAX_WORD_LEN + 1];
    uint8_t buff3[sizeof(t_occ_id)];
    size_t c;
    int item_pos = 0;

    while (!found && !stop && 
           (c = fread(buff, 1, READ_WORD_BUFF_SIZE, f1)) > 0)
    {
        for (int i = 0; i < c; i++) 
        {
            char cc = buff[i];

            if (part == 0)
            {
                if (item_pos == 0 && cc == 0)
                {
                    stop = 1;
                    break;
                }

                buff2[item_pos++] = cc;
                if (!cc) 
                {
                    part = 1;
                    item_pos = 0;
                }
            }
            else
            {
                buff3[item_pos++] = cc;
                if (item_pos == sizeof(t_occ_id))
                {
                    if (strcmp(buff2, word) == 0) 
                    {
                        found = 1;
                        break;
                    }
                    part = 0;
                    item_pos = 0;
                }
            }
        }
    }

    fclose(f1);

    if (found)
    {
        *res = *((t_occ_id*)buff3);
        return 1;
    }    
    return 0;
}

