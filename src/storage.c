#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include "utils.h"
#include "htable.h"
#include "comp.h"
#include "error_codes.h"

static const char* kls_subdir = "/.kaseklis";
#define OCC_FILE_ITEM_COUNT 100000
#define HT_SIZE 1024 * 1024

static char* base_dir = 0;
static int base_dir_len = 0;
static char* kls_dir = 0;

static char* words_file = 0;
static char* files_file = 0;

static FILE* files_ptr = 0;
static uint32_t files_ptr_written = 0;
static KlsItemCompressed occ_buff[OCC_FILE_ITEM_COUNT];
static uint32_t occ_buff_count = 0;
static uint32_t occ_file_item_offset = 0;
static uint32_t occ_file_count = 0;
static uint32_t file_count = 0;
static struct KlsHtContext* ht = 0;
static int line_number = 0;
static uint64_t total_occ_pos = 0;

static char curr_file_name[FNAME_LEN];
static int curr_file_is_binary;

void get_occ_fname(char* buff, int num)
{
    snprintf(buff, FNAME_LEN, "%s/occ.%d.dat", kls_dir, 
             num);
}

void flush_occ_buff(int force)
{
    if (occ_buff_count == OCC_FILE_ITEM_COUNT || force)
    {
        occ_buff_count = 0;
        char buff[FNAME_LEN];
        get_occ_fname(buff, occ_file_count);
        FILE* f = fopen(buff, "w");
        if (fwrite(occ_buff, sizeof(KlsItemCompressed), 
                   OCC_FILE_ITEM_COUNT, f) != OCC_FILE_ITEM_COUNT)
        {
            LOG("e: couldn't write file %s\n", buff);
            exit(EX_IOERR);
        }

        fclose(f);
        occ_file_count++;
        memset(occ_buff, 0, sizeof(occ_buff));
    }
}

int remove_occ_file(int num)
{
    char occ_file_name[FNAME_LEN];
    get_occ_fname(occ_file_name, num);
    return remove(occ_file_name);
}

int kls_init_storage(const char* base_dir0, int purge)
{
    base_dir = kls_concat(base_dir0, "");
    base_dir_len = strlen(base_dir);
    kls_dir = kls_concat(base_dir, kls_subdir);

    words_file = kls_concat(kls_dir, "/words.dat");
    files_file = kls_concat(kls_dir, "/files.dat");
    
    if (purge) 
    {
        LOG("i: removing index\n");
        remove(words_file);
        remove(files_file);
        int i = 0;
        while (remove_occ_file(i++) == 0);

        ht = (struct KlsHtContext*)malloc(sizeof(struct KlsHtContext));
        kls_create_ht(ht, HT_SIZE);

        mkdir(kls_dir, 0700);
        
        files_ptr = fopen(files_file, "w");

        if (!files_ptr)
        {
            LOG("e: cannot open for write %s", files_file);
            exit(EX_IOERR);
        }

        memset(occ_buff, 0, sizeof(occ_buff));
    }
    return 0;
}

void kls_add_word(const char* word)
{
    uint64_t prev_occ_pos;
    kls_put(ht, word, total_occ_pos, &prev_occ_pos);

    KlsItemCompressed kic;
    kls_compress(occ_file_item_offset, line_number, 
                 prev_occ_pos ? total_occ_pos - prev_occ_pos : 0, &kic);
    occ_buff[occ_buff_count++] = kic;
    flush_occ_buff(0);
    total_occ_pos++;
    occ_file_item_offset++;
}

void kls_add_file(const char* file)
{
    const char* relative_file = file + base_dir_len + 1;
    strcpy(curr_file_name, relative_file);
    // null char + is binary flag
    uint32_t ds = strlen(curr_file_name) + 1 + 1;
    curr_file_is_binary = 0;

    KlsItemCompressed kic;
    kls_compress_file_start(files_ptr_written, &kic);
    files_ptr_written += ds;
    occ_buff[occ_buff_count++] = kic;
    total_occ_pos++;
    flush_occ_buff(0);

    line_number = 0;
    occ_file_item_offset = 1;
}

int kls_is_storage_folder(const char* fname)
{
    int zz = strlen(kls_subdir);
    int zz2 = strlen(fname);
    return zz2 < zz ? 0 : strcmp(fname + (zz2 - zz), kls_subdir) == 0;
}

void kls_finish_storage(int sync_to_hdd)
{
    if (sync_to_hdd)
        flush_occ_buff(1);
    if (ht)
    {
        kls_ht_dump(ht, 1, 1);
        if (sync_to_hdd)
            kls_write_ht(ht, words_file);
        kls_destroy_ht(ht);
        free(ht);
    }
    if (files_ptr)
        fclose(files_ptr);
}

char* kls_get_base_dir()
{
    return base_dir;
}

struct OccFile
{
    uint32_t index;
    KlsItemCompressed buff[OCC_FILE_ITEM_COUNT];
    struct OccFile* prev;
    struct OccFile* next;
};

struct OccFile* create_occ_file(uint32_t index, struct OccFile* prev, 
                                struct OccFile* next)
{
    struct OccFile* res;
    res = (struct OccFile*)malloc(sizeof(struct OccFile));
    memset(res, 0, sizeof(struct OccFile));
    res->index = index;
    res->prev = prev;
    res->next = next;

    if (prev)
        prev->next = res;
    if (next)
        next->prev = res;

    char fbuff[FNAME_LEN];
    get_occ_fname(fbuff, index);

    FILE* f = fopen(fbuff, "r");
    if (fread(res->buff, sizeof(KlsItemCompressed), OCC_FILE_ITEM_COUNT, 
              f) != OCC_FILE_ITEM_COUNT)
    {
        LOG("e: couldn't read file %s\n", fbuff);
        exit(EX_IOERR);
    }
    fclose(f);

    return res;
}

void destroy_occ_file(struct OccFile* f)
{
    // TODO: write word file
    struct OccFile* tmp = f->next;
    if (f->prev)
        f->prev->next = f->next;
    if (f->next)
        f->next->prev = f->prev;
    free(f);
}

void destroy_occ_file_chain(struct OccFile* last)
{
    assert(last && last->next == 0 || !last);

    while (last)
    {
        struct OccFile* tmp = last->prev;
        destroy_occ_file(last);
        last = tmp;
    }
}

char* get_item_from(struct OccFile** last0, struct KlsItem* item0, 
                    char* files_list, uint64_t occ_pos, int* is_binary)
{
    char* res;
    assert(last0);
    struct OccFile* last = *last0;
    assert(files_list);
    assert(!last || !last->next);

    // cleaning up, removing unneeded OccFiles
    {
        uint64_t occ_file_index = occ_pos / OCC_FILE_ITEM_COUNT;

        while (last && last->index > occ_file_index)
        {
            assert(last->prev && 
                   (last->prev->next == last) || !last->prev);
            struct OccFile* tmp = last->prev;
            destroy_occ_file(last);
            last = tmp;
        }
    }

    struct OccFile* last_last = last;
    int item_set = 0;
    while (1)
    {
        uint64_t occ_file_index = occ_pos / OCC_FILE_ITEM_COUNT;
        uint64_t occ_file_offset = occ_pos % OCC_FILE_ITEM_COUNT;

        struct OccFile* p = last_last;
        struct OccFile* p0 = 0;

        while (p)
        {
            if (p->index <= occ_file_index)
                break;
            p0 = p;
            p = p->prev;
        }

        if (!p || p->index < occ_file_index)
            p = create_occ_file(occ_file_index, p, p0);

        if (!last)
            last = p;

        KlsItemCompressed tmp = p->buff[occ_file_offset];
        struct KlsItem item;
        kls_uncompress_item(tmp, &item);

        if (!item_set)
        {
            assert((uint8_t)item.type > (uint8_t)KLS_ITEM_FILE_START);
            *item0 = item;
            item_set = 1;
        }
        else if (item.type == KLS_ITEM_FILE_START)
        {
            char* zz = files_list + item.kfs.file_index;
            res = kls_concat("./", zz);
            *is_binary = *(files_list + item.kfs.file_index +
                           strlen(zz) + 1);
            break;
        }

        assert(occ_pos >= item.occ.jumpback);
        occ_pos = occ_pos - (item.occ.jumpback > 0 ? 
                             item.occ.jumpback : 1);
        last_last = p;
    }

    *last0 = last;
    return res;
}

char* load_file(const char* name, uint64_t* fs)
{
    FILE* f = fopen(name, "r");
    if (!f)
    {
        LOG("e: couldn't open %s\n", name);
        exit(EX_IOERR);
    }
    fseek(f, 0, SEEK_END);
    uint64_t sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* res = malloc(sz);

    if (fread(res, 1, sz, f) != sz)
    {
        LOG("e: couldn't read file %s\n", name);
        exit(EX_IOERR);
    }
    fclose(f);
    *fs = sz;
    return res;
}

uint64_t* init_endlines(char* ff, uint64_t size)
{
    uint32_t cnt = 0;
    for (uint64_t i = 0; i < size; i++)
        if (ff[i] == '\n')
            cnt++;

    uint64_t* res = (uint64_t*)malloc(sizeof(uint64_t) * cnt);
    uint64_t cr = 0;
    for (uint64_t i = 0; i < size; i++)
        if (ff[i] == '\n')
        {
            ff[i] = 0;
            res[cr++] = i;
        }

    return res;
}

void dump_index_for(const char* word, uint64_t occ_pos)
{
    uint64_t files_list_fs_ignored;
    char* files_list = load_file(files_file, &files_list_fs_ignored);

    char* curr_file = 0;
    uint64_t curr_file_size = 0;
    char* curr_fname = 0;
    uint64_t* endlines = 0;
    int is_binary = 0;
    struct OccFile* last = 0;
    struct KlsItem item;
    uint64_t occ_in_file = 0;

    while (1)
    {
        char* fname = get_item_from(&last, &item, files_list, occ_pos,
                                    &is_binary);
        if (!curr_fname || strcmp(curr_fname, fname) != 0)
        {
            if (curr_file) 
                free(curr_file);
            if (endlines)
                free(endlines);
            if (curr_fname)
                free(curr_fname);
            curr_fname = fname;

            occ_in_file = 0;

            if (is_binary)
            {
                curr_file = 0;
                endlines = 0;
            } 
            else
            {
                curr_file = load_file(curr_fname, &curr_file_size);
                endlines = init_endlines(curr_file, curr_file_size);
            }
        }
        occ_in_file++;

        if (is_binary)
        {
            if (occ_in_file == 1)
                printf("Binary file %s matches\n", fname);
        }
        else
        {
            uint64_t fpos = item.occ.line_number == 0 ? 0 : 
                endlines[item.occ.line_number - 1] + 1;

            if (fpos < curr_file_size)
            {
                char* line = curr_file + fpos;
                if (strlen(line) > MAX_DISPLAYABLE_LINE_LENGTH)
                    printf("LONG %s:%lu\n", fname, item.occ.line_number);
                else
                    printf("%s:%lu %s\n", fname, item.occ.line_number, 
                           line);
            }
            else
            {
                LOG("w: file %s appears to be changed since indexing", 
                    curr_fname);
            }
        }
            
        if (!item.occ.prev_occurence)
            break;

        assert(occ_pos > item.occ.prev_occurence);
        occ_pos -= item.occ.prev_occurence;
    }

    if (curr_file) 
        free(curr_file);
    if (endlines)
        free(endlines);
    if (curr_fname)
        free(curr_fname);
    
    destroy_occ_file_chain(last);
    free(files_list);
}

void kls_dump_index_for(const char* word)
{
    char buff[READ_WORD_BUFF_SIZE];
    char buff2[MAX_WORD_LEN + 1];
    uint8_t buff3[sizeof(KlsItemCompressed)];

    FILE* f = fopen(words_file, "r");
    int c;
    uint32_t item_pos = 0;

    int part = 0;
    int found = 0;

    while (!found && (c = fread(buff, 1, READ_WORD_BUFF_SIZE, f)) > 0)
    {
        for (int i = 0; i < c; i++) 
        {
            char cc = buff[i];

            if (part == 0)
            {
                buff2[item_pos++] = cc;
                if (!cc) {
                    part = 1;
                    item_pos = 0;
                }
            }
            else
            {
                buff3[item_pos++] = cc;
                if (item_pos == sizeof(KlsItemCompressed))
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
    fclose(f);

    if (found)
    {
        uint64_t occ_pos = *((uint64_t*)buff3);
        dump_index_for(word, occ_pos);
    }
}

void kls_next_line()
{
    line_number++;
}

void kls_is_binary()
{
    curr_file_is_binary = 1;
}

void kls_file_done()
{
    uint32_t ds = strlen(curr_file_name) + 1;
    fwrite(curr_file_name, 1, ds, files_ptr);
    fwrite((char*)&curr_file_is_binary, 1, 1, files_ptr);
}

