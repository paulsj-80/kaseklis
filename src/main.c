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
#include "error_codes.h"


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

void kls_do_index()
{
    kls_walk(kls_get_base_dir());
    kls_finish_storage(1);
}

void kls_do_get(const char* w)
{
    kls_dump_index_for(w);
    kls_finish_storage(0);
}

int main(int argc, char** arg)
{
    char* kls_dir = 0;
    int write_mode = 0;
    enum KlsCommand cmd = KLS_COMMAND_UNKNOWN;
    char* word = 0;
    {
        if (argc < 2)
        {
            kls_print_welcome();
            exit(EX_USAGE);
        }

        if (strcmp(arg[1], "index") == 0)
        {
            if (argc > 2) 
            {
                kls_print_welcome();
                exit(EX_USAGE);
            }
            write_mode = 1;
            cmd = KLS_COMMAND_INDEX;
        }
        else if (strcmp(arg[1], "get") == 0)
        {
            if (argc != 3) 
            {
                kls_print_welcome();
                exit(EX_USAGE);
            }
            cmd = KLS_COMMAND_GET;
            word = arg[2];
        }
        else
        {
            kls_print_welcome();
            exit(EX_USAGE);
        }
    }

    {
        char base_dir[1024];
        char* bd = getcwd(base_dir, 1024);

        int rr = kls_init_storage(base_dir, write_mode);
        if (rr)
        {
            LOG("e: couldn't init storage, ec = %d\n", rr);
            exit(EX_IOERR);
        }
    }

    {
        switch (cmd)
        {
        case KLS_COMMAND_UNKNOWN:
            LOG("e: unknown command\n");
            break;
        case KLS_COMMAND_INDEX:
            kls_do_index();
            break;
        case KLS_COMMAND_GET:
            kls_do_get(word);
            break;
        }
    }

    if (kls_dir)
        free(kls_dir);

    return 0;
}
