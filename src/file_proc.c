#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utils.h"
#include "storage.h"

#define MAX_WORD_LEN 0xffff

struct KlsFileProcessor 
{
    char curr_word[MAX_WORD_LEN];
    int curr_word_len;
    uint64_t bin_chars;
    uint64_t total_chars;
    int binary_definitely;
    const char* curr_file;
};

void kls_create_file_processor(struct KlsFileProcessor* fp)
{
    memset(fp, 0, sizeof(struct KlsFileProcessor));
}

void kls_destroy_file_processor(struct KlsFileProcessor* fp)
{
    // nothing here
}

void kls_word_found(struct KlsFileProcessor* fp)
{
    if (fp->curr_word_len > 2) 
    {
        fp->curr_word[fp->curr_word_len] = 0;
        kls_add_word(fp->curr_word);
    }
}

void kls_process_char(struct KlsFileProcessor* fp, char c)
{
    fp->total_chars++;
    if (!c)
        fp->binary_definitely = 1;

    int is_letter = ((c >= 'a' && c <= 'z') || 
                     (c >= 'A' && c <= 'Z') || (c == '_')) ? 1 : 0;
    int is_number = (c >= '0' && c <= '9');
    int is_newline = c == '\n';
    
    if (fp->curr_word_len == 0)
    {
        if (is_letter)
            fp->curr_word[fp->curr_word_len++] = c;
    }
    else
    {
        if (is_letter || is_number)
            fp->curr_word[fp->curr_word_len++] = c;
        else
        {
            kls_word_found(fp);
            fp->curr_word_len = 0;
        }
    }

    if (is_newline)
        kls_next_line();

    if ((c < 32 || c > 126) && c != '\n' && c != '\t' && c != 10)
        fp->bin_chars++;

    if (fp->curr_word_len > MAX_WORD_LEN) 
    {
        LOG("w: word buff size exceeded in %s\n", fp->curr_file);
        fp->curr_word_len = 0;
    }
}

void kls_done_processing(struct KlsFileProcessor* fp)
{
    if (fp->curr_word_len == 0)
        kls_word_found(fp);
    if (fp->binary_definitely || fp->bin_chars > fp->total_chars / 10)
        kls_is_binary();
    kls_file_done();
}


void kls_process_file(const char* fname)
{
    kls_add_file(fname);
    char buff[1024];
    FILE* f = fopen(fname, "r");
    int c;
    struct KlsFileProcessor fp;
    kls_create_file_processor(&fp);
    fp.curr_file = fname;
    while (c = (fread(buff, 1, 1024, f)))
    {
        for (int i = 0; i < c; i++)
            kls_process_char(&fp, buff[i]);
    }
    kls_done_processing(&fp);
    fclose(f);
}

