#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "htable.h"
#include "comp.h"
#include "exit_codes.h"


#define OCC_FILE_ITEM_COUNT 1024 * 128
#define HT_SIZE 1024 * 1024

#define FILES_FILE0_ITEM_SIZE (sizeof(t_file_id) + 1 + sizeof(t_occ_id))

static char* base_dir = 0;
static size_t base_dir_len = 0;
static char* kls_dir = 0;

static char* words_file0 = 0;
static char* words_file1 = 0;
static char* files_file0 = 0;
static char* files_file1 = 0;
static char* index_log_file = 0;
static char* nested_file = 0;

static FILE* files_ptr0 = 0;
static FILE* files_ptr1 = 0;
static FILE* nested_ptr = 0;

static t_file_id files_ptr1_written = 0;
static t_occ_id occ_buff[OCC_FILE_ITEM_COUNT];
static t_occ_id occ_buff_count = 0;
static t_occ_file_id occ_file_count = 0;
static t_file_id file_count = 0;
static struct t_kls_ht_context * ht = 0;
static t_occ_id total_occ_pos = 0;

void get_occ_fname(char* buff, t_occ_file_id num)
{
    int rc = snprintf(buff, FNAME_LEN, "%s/occ.%u.dat", kls_dir, 
                      num);
    PATH_POSTPRINT_CHECK_U(buff, num, rc);
}

int remove_occ_file(int num)
{
    char occ_file_name[FNAME_LEN];
    get_occ_fname(occ_file_name, num);
    return remove(occ_file_name);
}

void kls_st_init(const char* base_dir0, bool purge)
{
    base_dir = kls_ut_concat_fnames(base_dir0, "");
    base_dir_len = strlen(base_dir);
    kls_dir = kls_ut_concat_fnames(base_dir, kls_ut_subdir);

    words_file0 = kls_ut_concat_fnames(kls_dir, "/words0.dat");
    words_file1 = kls_ut_concat_fnames(kls_dir, "/words1.dat");

    files_file0 = kls_ut_concat_fnames(kls_dir, "/files0.dat");
    files_file1 = kls_ut_concat_fnames(kls_dir, "/files1.dat");

    index_log_file = kls_ut_concat_fnames(kls_dir, "/index.log");

    nested_file = kls_ut_concat_fnames(kls_dir, "/nested.txt");
    
    if (purge) 
    {
        LOGI("purge");
        remove(words_file0);
        remove(words_file1);
        remove(files_file0);
        remove(files_file1);
        remove(index_log_file);
        remove(nested_file);
        size_t i = 0;
        while (remove_occ_file(i++) == 0);

        mkdir(kls_dir, 0700);

        kls_ut_init_log_file(index_log_file);

        files_ptr0 = fopen(files_file0, "w");
        KLS_IO_CHECK(files_ptr0, "cannot open for write %s", 
                     files_file0);
        files_ptr1 = fopen(files_file1, "w");
        KLS_IO_CHECK(files_ptr1, "cannot open for write %s", 
                     files_file1);
        nested_ptr = fopen(nested_file, "w");
        KLS_IO_CHECK(nested_ptr, "cannot open for write %s", 
                     nested_file);

        memset(occ_buff, 0, sizeof(occ_buff));

        ht = (struct t_kls_ht_context*)malloc(
                     sizeof(struct t_kls_ht_context));
        kls_ht_create(ht, HT_SIZE);
    }
}

void flush_occ_buff(int force)
{
    if ((occ_buff_count == OCC_FILE_ITEM_COUNT || force) && 
        occ_buff_count > 0)
    {
        occ_buff_count = 0;
        char buff[FNAME_LEN];
        get_occ_fname(buff, occ_file_count);
        FILE* f = fopen(buff, "w");
        if (fwrite(occ_buff, sizeof(t_occ_id), 
                   OCC_FILE_ITEM_COUNT, f) != OCC_FILE_ITEM_COUNT)
        {
            LOGE("couldn't write file %s", buff);
            exit(EX_IOERR);
        }

        fclose(f);
        occ_file_count++;
        memset(occ_buff, 0, sizeof(occ_buff));
    }
}

void kls_st_finish(bool sync_to_hdd)
{
    if (sync_to_hdd)
        flush_occ_buff(1);
    if (ht)
    {
        kls_ht_dump_stats(ht);
        if (sync_to_hdd)
            kls_ht_write(ht, words_file0, words_file1);
        kls_ht_destroy(ht);
        free(ht);
    }
    if (files_ptr0)
        fclose(files_ptr0);
    if (files_ptr1)
        fclose(files_ptr1);
    if (nested_ptr)
        fclose(nested_ptr);
}

char* kls_st_get_base_dir()
{
    return base_dir;
}

void kls_st_add_file(const char* file, bool is_binary)
{
    const char* relative_file = file + base_dir_len + 1;

    t_file_id ds = strlen(relative_file) + 1;
    
    fwrite(&files_ptr1_written, sizeof(files_ptr1_written),
           1, files_ptr0);
    char bb = (char)is_binary;
    fwrite(&bb, 1, 1, files_ptr0);
    
    fwrite(relative_file, ds, 1, files_ptr1);
    files_ptr1_written += ds;
}

void kls_st_file_done()
{
    fwrite(&total_occ_pos, sizeof(total_occ_pos), 1, files_ptr0);
}

void kls_st_add_word(const char* word)
{
    t_occ_id prev_occ_pos;
    kls_ht_put(ht, word, total_occ_pos, &prev_occ_pos);

    occ_buff[occ_buff_count++] = prev_occ_pos;
    flush_occ_buff(0);
    total_occ_pos++;
}

void kls_st_nested_ignored(const char* fname)
{
    fwrite(fname, 1, strlen(fname), nested_ptr);
    static const char newline = '\n';
    fwrite(&newline, 1, 1, nested_ptr);
}

void dump_nested_for(const char* word)
{
    FILE* f = fopen(nested_file, "r");

    char buff[FNAME_LEN + 1];
    memset(buff, 0, FNAME_LEN + 1);
    int buff_pos = 0;

    while (1)
    {
        char c;
        fread(&c, 1, 1, f);

        if (feof(f))
            break;

        KLS_CHECK(c, INAPPROPRIATE_FILE_OBJECT, "bad character in %s",
                  nested_file)

        if (c == '\n')
        {
            if (buff_pos > 0)
            {
                buff[buff_pos] = 0;
                char cmd_buff[FNAME_LEN * 2];
                snprintf(cmd_buff, FNAME_LEN, 
                         "cd %s && kaseklis get %s", 
                         buff, word);
                buff_pos = 0;
            }
        }
        else
        {
            buff[buff_pos++] = c;
            KLS_CHECK(buff_pos < FNAME_LEN, 
                      KLS_LIMIT_EXCEEDED,
                      "nested folder name too long %s", buff);
        }
    }

    fclose(f);
}

void read_fd0(char* data,
              t_occ_id occ_pos, t_file_id* curr_file_pos,
              uint32_t* fname_offset, bool* is_binary)
{
    KLS_ASSERT(occ_pos > 0, "occ_pos should be > 0");
    while (1)
    {
        t_file_id pos = *curr_file_pos * FILES_FILE0_ITEM_SIZE;

        t_occ_id min_occ_id;
        if (*curr_file_pos == 0)
            min_occ_id = 0;
        else
            min_occ_id = *((t_occ_id*)(data + pos - sizeof(t_occ_id))); 
        if (min_occ_id < occ_pos)
        {
            *fname_offset = *((uint32_t*)(data + pos));
            *is_binary = *((char*)(data + pos + sizeof(t_file_id)));
            break;
        }
        else
            (*curr_file_pos)--;
    }
}

void print_line(char* data, uint64_t curr_index, uint64_t line_start, 
                char* fname, uint32_t line_number)
{
    uint64_t line_len = curr_index - line_start - 1;
    if (line_len > MAX_DISPLAYABLE_LINE_LENGTH)
    {
        printf("LONG %s:%u\n", fname, line_number);
    }
    else
    {
        char* line = data + line_start + 1;
        char ll[MAX_DISPLAYABLE_LINE_LENGTH + 1];
        strncpy(ll, line, line_len);
        ll[line_len] = 0;
        printf("%s:%u %s\n", fname, line_number, ll);
    }
}

void find_word_in(char* fname, char* word)
{
    uint64_t size;
    char* data = kls_ut_load_file(fname, &size);

    int wlen = strlen(word);
    // 0: searching for word
    // 1: word found, looking for line end
    int state = 0;
    int good_char_count = 0;
    uint32_t line_number = 1;
    uint64_t line_start = 0;
    uint64_t i;

    for (i = 0; i < size; i++)
    {
        char c = data[i];
        switch (state)
        {
        case 0:
            {
                if (c == word[good_char_count])
                {
                    good_char_count++;

                    if (good_char_count == wlen)
                        state = 1;
                } 
                else
                {
                    good_char_count = 0;
                    if (c == '\n') 
                    {
                        line_start = i;
                        line_number++;
                    }
                }
                break;
            }
        case 1:
            {
                if (c == '\n')
                {
                    print_line(data, i, line_start, fname, line_number);
                    line_start = i;
                    line_number++;
                    good_char_count = 0;
                    state = 0;
                }
            
                break;
            }
        }
    }

    if (state == 1)
        print_line(data, i, line_start, fname, line_number);

    free(data);
}


void kls_st_dump_index_for(const char* word0)
{
    char word[MAX_WORD_LEN + 1];
    memset(word, 0, MAX_WORD_LEN + 1);
    strncpy(word, word0, MAX_WORD_LEN);
    dump_nested_for(word);

    t_occ_id occ_pos;
    if (!kls_ht_get_occ_id(word, words_file0, words_file1, &occ_pos))
        return;
    bool has_occ_file;
    t_occ_file_id curr_occ_file_index;

    uint64_t ff0_size, tmp;
    char* files_data0 = kls_ut_load_file(files_file0, &ff0_size);
    char* files_data1 = kls_ut_load_file(files_file1, &tmp);

    if (ff0_size == 0)
        return;

    KLS_CHECK(ff0_size % FILES_FILE0_ITEM_SIZE == 0, 
              INAPPROPRIATE_FILE_OBJECT,
              "bad file size %s", files_file0);
    t_file_id curr_file_pos = ff0_size / FILES_FILE0_ITEM_SIZE - 1;

    while (occ_pos > 0)
    {
        // displaying data for current pos
        {
            bool is_binary;
            uint32_t fname_offset;
            read_fd0(files_data0, 
                     occ_pos, &curr_file_pos,
                     &fname_offset, &is_binary);
            char* fname = files_data1 + fname_offset;

            if (is_binary)
                printf("Binary file %s matches\n", fname);
            else
                find_word_in(fname, word);
        }
        
        // moving to next pos
        {
            t_occ_file_id occ_file_index = occ_pos / OCC_FILE_ITEM_COUNT;
            t_occ_id occ_file_offset = occ_pos % OCC_FILE_ITEM_COUNT;

            if (!has_occ_file || occ_file_index != curr_occ_file_index)
            {
                char fbuff[FNAME_LEN];
                get_occ_fname(fbuff, occ_file_index);

                FILE* f = fopen(fbuff, "r");
                KLS_IO_CHECK(f, "couldn't open %s", fbuff);
                KLS_IO_CHECK(fread((char*)occ_buff, 
                                   sizeof(t_occ_id) * 
                                   OCC_FILE_ITEM_COUNT, 
                                   1, f) == 1, 
                             "couldn't read %s", fbuff);

                curr_occ_file_index = occ_file_index;
                has_occ_file = 1;
            }
            occ_pos = occ_buff[occ_file_offset];
        }
    }

    free(files_data0);
    free(files_data1);
}


