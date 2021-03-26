#include "walker.h"
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include "storage.h"
#include "file_proc.h"
#include "utils.h"

void kls_walk(const char *name)
{
    char path[FNAME_LEN];

    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name))) 
    {
        LOG("w: cannot open folder %s\n", name);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || 
                strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            if (!kls_is_storage_folder(path))
                kls_walk(path);
        } else {
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            kls_process_file(path);
        }
    }
    closedir(dir);
}


