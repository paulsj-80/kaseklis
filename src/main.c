#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "utils.h"
#include "walker.h"
#include "storage.h"
#include "exit_codes.h"

void kls_print_welcome()
{
    printf("kaseklis <command>\n");
    printf("command: \n");
    printf("    index\n");
    printf("    get <word>\n");
}

enum KlsCommand 
{
    KLS_COMMAND_UNKNOWN=0,
    KLS_COMMAND_INDEX,
    KLS_COMMAND_GET
};

void cmd_index()
{
    kls_wr_walk(kls_st_get_base_dir(), 1);
    kls_st_finish(1);
}

void cmd_get(const char* w)
{
    kls_st_dump_index_for(w);
    kls_st_finish(0);
}

void exit_bad_usage()
{
    kls_print_welcome();
    exit(EX_USAGE);
}

int main(int argc, char** arg)
{
    char* kls_dir = 0;
    int write_mode = 0;
    enum KlsCommand cmd = KLS_COMMAND_UNKNOWN;
    char* word = 0;
    {
        if (argc < 2)
            exit_bad_usage();

        if (strcmp(arg[1], "index") == 0)
        {
            if (argc > 2) 
                exit_bad_usage();
            write_mode = 1;
            cmd = KLS_COMMAND_INDEX;
        }
        else if (strcmp(arg[1], "get") == 0)
        {
            if (argc != 3) 
                exit_bad_usage();
            cmd = KLS_COMMAND_GET;
            word = arg[2];
            KLS_CHECK(strlen(word) > 1, EX_USAGE, 
                      "word length should exceed 1");
        }
        else
            exit_bad_usage();
    }

    {
        char base_dir[FNAME_LEN];
        char* bd = getcwd(base_dir, FNAME_LEN);

        KLS_IO_CHECK(bd, "cannot get current working directory");
        kls_st_init(base_dir, write_mode);
    }

    {
        switch (cmd)
        {
        case KLS_COMMAND_UNKNOWN:
            LOGE("unknown command");
            exit(EX_USAGE);
            break;
        case KLS_COMMAND_INDEX:
            cmd_index();
            break;
        case KLS_COMMAND_GET:
            cmd_get(word);
            break;
        }
    }

    if (kls_dir)
        free(kls_dir);

    return 0;
}
