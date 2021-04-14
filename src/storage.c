#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "htable.h"
#include "exit_codes.h"


// KLS03005
void get_occ_fname(char* buff, t_occ_file_id num, char* kls_dir)
{
    int rc = snprintf(buff, FNAME_LEN, "%s/occ.%u.dat", kls_dir, 
                      num);
    PATH_POSTPRINT_CHECK_U(buff, num, rc);
}

int remove_occ_file(int num, char* kls_dir)
{
    char occ_file_name[FNAME_LEN];
    get_occ_fname(occ_file_name, num, kls_dir);
    return remove(occ_file_name);
}

void kls_st_init(struct t_storage_context* sc, const char* base_dir0, 
                 char* output_file_prefix, bool purge)
{
    memset((char*)sc, 0, sizeof(struct t_storage_context));

    KLS_IO_CHECK(getcwd(sc->prev_cwd, FNAME_LEN), 
        "cannot get current working directory");
    KLS_IO_CHECK(chdir(base_dir0) == 0, 
                 "could not chdir to %s", base_dir0);

    strcpy(sc->output_file_prefix, output_file_prefix);

    sc->base_dir = kls_ut_concat_fnames(base_dir0, "");
    sc->base_dir_len = strlen(sc->base_dir);
    sc->kls_dir = kls_ut_concat_fnames(sc->base_dir, kls_ut_subdir);

    sc->words_file0 = kls_ut_concat_fnames(sc->kls_dir, "/words0.dat");
    sc->words_file1 = kls_ut_concat_fnames(sc->kls_dir, "/words1.dat");

    sc->files_file0 = kls_ut_concat_fnames(sc->kls_dir, "/files0.dat");
    sc->files_file1 = kls_ut_concat_fnames(sc->kls_dir, "/files1.dat");

    sc->index_log_file = kls_ut_concat_fnames(sc->kls_dir, "/index.log");

    sc->nested_file = kls_ut_concat_fnames(sc->kls_dir, "/nested.txt");
    
    if (purge) 
    {
        LOGI("purge");
        remove(sc->words_file0);
        remove(sc->words_file1);
        remove(sc->files_file0);
        remove(sc->files_file1);
        remove(sc->index_log_file); // KLS07001
        remove(sc->nested_file);
        size_t i = 0;
        while (remove_occ_file(i++, sc->kls_dir) == 0);

        mkdir(sc->kls_dir, 0700);

        kls_ut_init_log_file(sc->index_log_file);

        sc->files_ptr0 = fopen(sc->files_file0, "w");
        KLS_IO_CHECK(sc->files_ptr0, "cannot open for write %s", 
                     sc->files_file0);
        sc->files_ptr1 = fopen(sc->files_file1, "w");
        KLS_IO_CHECK(sc->files_ptr1, "cannot open for write %s", 
                     sc->files_file1);
        sc->nested_ptr = fopen(sc->nested_file, "w");
        KLS_IO_CHECK(sc->nested_ptr, "cannot open for write %s", 
                     sc->nested_file);

        sc->ht = (struct t_kls_ht_context*)kls_ut_malloc(
                         sizeof(struct t_kls_ht_context));
        kls_ht_create(sc->ht, HT_SIZE);

        // reserved for chain ends
        sc->occ_buff[sc->occ_buff_count++] == 0xffffffff;
        sc->total_occ_count++;
        sc->first_occ_in_file = 1;
    }
}

// KLS04001, KLS05008
void flush_occ_buff(struct t_storage_context* sc, int force)
{
    if ((sc->occ_buff_count == OCC_FILE_ITEM_COUNT || force) && 
        sc->occ_buff_count > 0)
    {
        sc->occ_buff_count = 0;
        char buff[FNAME_LEN];
        get_occ_fname(buff, sc->occ_file_count, sc->kls_dir);
        FILE* f = fopen(buff, "w");
        KLS_IO_CHECK(fwrite(sc->occ_buff, sizeof(t_occ_id) * 
                            OCC_FILE_ITEM_COUNT, 
                            1, f) == 1, "couldn't write file %s", buff);

        fclose(f);
        sc->occ_file_count++;
        memset(sc->occ_buff, 0, sizeof(t_occ_id) * OCC_FILE_ITEM_COUNT);
    }
}


void kls_st_finish(struct t_storage_context* sc, bool sync_to_hdd)
{
    if (sync_to_hdd)
        flush_occ_buff(sc, 1);
    if (sc->ht)
    {
        kls_ht_dump_stats(sc->ht);
        LOGI("peak dynamic memory usage: %lu", peak_allocated);
        if (sync_to_hdd)
            kls_ht_write(sc->ht, sc->words_file0, sc->words_file1);
        kls_ht_destroy(sc->ht);
        kls_ut_free(sc->ht, sizeof(struct t_kls_ht_context));
    }
    if (sc->files_ptr0)
        fclose(sc->files_ptr0);
    if (sc->files_ptr1)
        fclose(sc->files_ptr1);
    if (sc->nested_ptr)
        fclose(sc->nested_ptr);

#define DO_FREE(_name) if (sc->_name) free(sc->_name);

    DO_FREE(base_dir);
    DO_FREE(kls_dir);
    DO_FREE(words_file0);
    DO_FREE(words_file1);
    DO_FREE(files_file0);
    DO_FREE(files_file1);
    DO_FREE(index_log_file);
    DO_FREE(nested_file);

    KLS_IO_CHECK(chdir(sc->prev_cwd) == 0, 
                 "could not chdir to %s", sc->prev_cwd);
}

char* kls_st_get_base_dir(struct t_storage_context* sc)
{
    return sc->base_dir;
}

void kls_st_add_file(struct t_storage_context* sc, 
                     const char* file, bool is_binary)
{
    // writing happens when file is done, as it could be empty
    // (empty files are omitted)
    const char* relative_file = file + sc->base_dir_len + 1;
    strcpy(sc->fname_to_write, relative_file);
    sc->is_binary_to_write = is_binary;
}

void kls_st_file_done(struct t_storage_context* sc)
{
    if (sc->total_occ_count > sc->first_occ_in_file)
    {
        // file had some word in it

        t_file_id ds = strlen(sc->fname_to_write) + 1;
        KLS_IO_CHECK(fwrite(sc->fname_to_write, ds, 1, 
                            sc->files_ptr1) == 1,
                     "couldn't write %s", sc->files_file1);

        // KLS04006
        KLS_IO_CHECK(fwrite(&sc->files_ptr1_written, 
                            sizeof(sc->files_ptr1_written),
                            1, sc->files_ptr0) == 1,
                     "couldn't write %s", sc->files_file0);
        sc->files_ptr1_written += ds;
        
        char bb = (char)sc->is_binary_to_write;
        KLS_IO_CHECK(fwrite(&bb, 1, 1, sc->files_ptr0) == 1,
                     "couldn't write %s", sc->files_file0);

        // KLS04005
        t_occ_id tmp = sc->total_occ_count - 1;
        KLS_IO_CHECK(fwrite(&tmp, sizeof(tmp), 1, sc->files_ptr0) == 1, 
                     "couldn't write %s", sc->files_file0);
        sc->first_occ_in_file = sc->total_occ_count;
    }
}

void kls_st_add_word(struct t_storage_context* sc, const char* word)
{
    t_occ_id prev_occ_pos;
    // KLS04002
    if (kls_ht_put(sc->ht, word, sc->total_occ_count, &prev_occ_pos, 
                   sc->first_occ_in_file))
    {
        // KLS04002
        sc->occ_buff[sc->occ_buff_count++] = prev_occ_pos;
        flush_occ_buff(sc, 0);
        sc->total_occ_count++;
    }
}

void kls_st_nested_ignored(struct t_storage_context* sc, 
                           const char* fname)
{
    // KLS03011, KLS03013
    fwrite(fname, 1, strlen(fname), sc->nested_ptr);

    static const char separator = '/';
    fwrite(&separator, 1, 1, sc->nested_ptr);
    static const char newline = '\n';
    fwrite(&newline, 1, 1, sc->nested_ptr);
}

void dump_nested_for(struct t_storage_context* sc, const char* word)
{
    FILE* f = fopen(sc->nested_file, "r");

    char buff[FNAME_LEN + 1];
    memset(buff, 0, FNAME_LEN + 1);
    int buff_pos = 0;

    while (1)
    {
        char c;
        size_t cr = fread(&c, 1, 1, f);
        KLS_CHECK(cr || !buff_pos, 
                  BAD_FILE_OBJECT,
                  "file is not properly formed %s", sc->nested_file);
        if (feof(f))
            break;

        KLS_IO_CHECK(cr, "cannot read %s", sc->nested_file);
        KLS_CHECK(c, BAD_FILE_OBJECT, "bad character in %s",
                  sc->nested_file)

        if (c == '\n')
        {
            if (buff_pos > 0)
            {
                buff[buff_pos] = 0;
                // KLS03010
                struct t_storage_context sc2;
                kls_st_init(&sc2, buff, buff + sc->base_dir_len + 1, 0);
                kls_st_dump_index_for(&sc2, word);
                kls_st_finish(&sc2, 0);

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
        {
            min_occ_id = *((t_occ_id*)(data + pos - sizeof(t_occ_id))); 
        }
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

// KLS06001
uint64_t print_line(char* data, uint64_t curr_index, 
                    uint64_t line_start, 
                    char* fname, uint32_t line_number,
                    char* prefix)
{
    uint64_t res = 0;
    uint64_t line_len = curr_index - line_start;
    if (line_len > MAX_DISPLAYABLE_LINE_LENGTH)
    {
        // KLS06002
        printf("LONG %s/%s:%u\n", prefix, fname, line_number);
        res++;
    }
    else
    {
        char* line = data + line_start;
        char ll[MAX_DISPLAYABLE_LINE_LENGTH + 1];
        strncpy(ll, line, line_len);
        ll[line_len] = 0;
        if (line_len > 1)
            res++;
        printf("%s%s:%u %s\n", prefix, fname, line_number, ll);
    }
    return res;
}

void find_word_in(struct t_storage_context* sc,
                  char* fname, const char* word)
{
    uint64_t size;
    char* data = kls_ut_load_file(fname, &size);

    if (size == 0)
        return;

    int wlen = strlen(word);
    // 0: searching for word
    // 1: word found, looking for line end
    int state = 0;
    int good_char_count = 0;
    uint32_t line_number = 1;
    uint64_t line_start = 0;
    uint64_t i;
    uint64_t results_printed = 0;
    bool prev_was_letter = 0;

    for (i = 0; i < size; i++)
    {
        char c = data[i];
        switch (state)
        {
        case 0:
            {                
                if ((!prev_was_letter || good_char_count > 0) && 
                    c == word[good_char_count])
                {
                    good_char_count++;

                    if (good_char_count == wlen &&
                        (i == size - 1 ||
                         (!kls_ut_is_letter(data[i + 1]) && 
                         !kls_ut_is_number(data[i + 1]))))
                        state = 1;
                } 
                else
                {
                    good_char_count = 0;

                    // KLS02006
                    if (c == '\n') 
                    {
                        line_start = i + 1;
                        line_number++;
                    }
                }

                prev_was_letter = kls_ut_is_letter(c) || 
                    (good_char_count > 0 && kls_ut_is_number(c));
                break;
            }
        case 1:
            {
                // KLS02006
                if (c == '\n')
                {
                    results_printed += print_line(data, i, line_start, 
                                                  fname, line_number,
                                                  sc->output_file_prefix);
                    line_start = i + 1;
                    line_number++;
                    good_char_count = 0;
                    state = 0;
                    prev_was_letter = 0;
                }
            
                break;
            }
        }
    }

    if (state == 1)
        results_printed += print_line(data, i, line_start, fname, 
                                      line_number, 
                                      sc->output_file_prefix);

    if (!results_printed)
        LOGW("EMPTY %s", fname);

    free(data);
}


void kls_st_dump_index_for(struct t_storage_context* sc, 
                           const char* word0)
{
    char word[MAX_WORD_LEN + 1];
    memset(word, 0, MAX_WORD_LEN + 1);
    strncpy(word, word0, MAX_WORD_LEN);
    dump_nested_for(sc, word);

    t_occ_id occ_pos;
    if (!kls_ht_get_occ_id(word, sc->words_file0, sc->words_file1, 
                           &occ_pos, HT_SIZE))
        return;
    bool has_occ_file = 0;
    t_occ_file_id curr_occ_file_index;

    uint64_t ff0_size, tmp;
    char* files_data0 = kls_ut_load_file(sc->files_file0, &ff0_size);
    char* files_data1 = kls_ut_load_file(sc->files_file1, &tmp);

    if (ff0_size == 0)
        return;

    KLS_CHECK(ff0_size % FILES_FILE0_ITEM_SIZE == 0, 
              BAD_FILE_OBJECT,
              "bad file size %s", sc->files_file0);
    t_file_id curr_file_pos = ff0_size / FILES_FILE0_ITEM_SIZE - 1;

    // KLS04003
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
                // KLS01005, KLS06000
                printf("Binary file %s matches\n", fname);
            else
            {
                find_word_in(sc, fname, word0);
            }
        }
        
        // moving to next pos
        {
            t_occ_file_id occ_file_index = occ_pos / OCC_FILE_ITEM_COUNT;
            t_occ_id occ_file_offset = occ_pos % OCC_FILE_ITEM_COUNT;

            if (!has_occ_file || occ_file_index != curr_occ_file_index)
            {
                char fbuff[FNAME_LEN];
                get_occ_fname(fbuff, occ_file_index, sc->kls_dir);

                FILE* f = fopen(fbuff, "r");
                KLS_IO_CHECK(f, "couldn't open %s", fbuff);
                KLS_IO_CHECK(fread((char*)sc->occ_buff, 
                                   sizeof(t_occ_id) * 
                                   OCC_FILE_ITEM_COUNT, 
                                   1, f) == 1, 
                             "couldn't read %s", fbuff);
                fclose(f);
                curr_occ_file_index = occ_file_index;
                has_occ_file = 1;
            }
            occ_pos = sc->occ_buff[occ_file_offset];
        }
    }

    free(files_data0);
    free(files_data1);
}


