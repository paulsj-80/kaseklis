#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utils.h"
#include "storage.h"



struct t_file_processor
{
    char curr_word[MAX_WORD_LEN + 1];
    int curr_word_len;
    const char* curr_file;
    char* data;
    uint64_t size;
};

bool is_binary(struct t_file_processor* fp)
{
    char* p = fp->data;
    char* p_end = p + fp->size;

    bool res = 0;
    uint64_t bin_char_count = 0;

    while (p != p_end)
    {
        char c = *p;
        if (c == 0)
        {
            res = 1;
            break;
        }
        if ((c != 10 && c != 13 && c < 32) || c > 126)
            bin_char_count++;
        p++;
    }
    if (bin_char_count > fp->size / 10)
        res = 1;
    return res;
}

void init_file_processor(struct t_file_processor* fp, 
                         const char* fname)
{
    memset(fp, 0, sizeof(struct t_file_processor));
    fp->curr_file = fname;

    fp->data = kls_ut_load_file(fname, &fp->size);
    
    kls_st_add_file(fname, is_binary(fp));
}

void word_found(struct t_file_processor* fp)
{
    if (fp->curr_word_len > 2) 
    {
        // last byte is always zero, it is not overwritten
        if (fp->curr_word_len < MAX_WORD_LEN)
            fp->curr_word[fp->curr_word_len] = 0;
        kls_st_add_word(fp->curr_word);
    }
}

void process_char(struct t_file_processor* fp, char c)
{
    // TODO: ignore 30+ chars; remove is_newline

    bool is_letter = ((c >= 'a' && c <= 'z') || 
                      (c >= 'A' && c <= 'Z') || (c == '_')) ? 1 : 0;
    bool is_number = (c >= '0' && c <= '9');
    bool word_started = fp->curr_word_len > 0;
    
    if (!word_started)
    {
        if (is_letter)
            fp->curr_word[fp->curr_word_len++] = c;
    }
    else
    {
        bool word_ended = !(is_letter || is_number);
        bool word_just_filled = !word_ended && 
            (fp->curr_word_len + 1 == MAX_WORD_LEN);
        bool word_notified_already = fp->curr_word_len >= MAX_WORD_LEN;
        bool word_can_add = fp->curr_word_len < MAX_WORD_LEN;

        if (word_ended) 
        {
            if (!word_notified_already)
                word_found(fp);
            fp->curr_word_len = 0;
        } 
        else
        {
            if (word_can_add)
                fp->curr_word[fp->curr_word_len] = c;
            if (word_just_filled)
                word_found(fp);
            fp->curr_word_len++;
        }
    }
}

void process(struct t_file_processor* fp)
{
    char* p = fp->data;
    char* p_end = p + fp->size;

    while (p != p_end)
    {
        process_char(fp, *p);
        p++;
    }
}

void finish_file_processor(struct t_file_processor* fp)
{
    bool word_started = fp->curr_word_len > 0;
    bool word_notified_already = fp->curr_word_len >= MAX_WORD_LEN;

    if (word_started && !word_notified_already)
        word_found(fp);
    free(fp->data);
    kls_st_file_done();
}

void kls_fp_process(const char* fname)
{
    if (kls_ut_file_size(fname) > MAX_INDEXABLE_FILE_SIZE)
    {
        LOGI("too big %s", fname);
        return;
    }

    struct t_file_processor fp;
    init_file_processor(&fp, fname);
    process(&fp);
    finish_file_processor(&fp);
}


