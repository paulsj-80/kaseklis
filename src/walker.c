#include "walker.h"
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include "storage.h"
#include "file_proc.h"
#include "utils.h"



void kaseklis_flags(const char *name, 
                    bool* is_kaseklis, 
                    bool* has_it, 
                    bool* is_ignored)
{
    *is_kaseklis = *has_it = *is_ignored = 0;

    {
        int zz = strlen(kls_ut_subdir);
        int zz2 = strlen(name);
        if (zz2 >= zz && strcmp(name + (zz2 - zz), kls_ut_subdir) == 0)
        {
            *is_kaseklis = 1;
            return;
        }
    }
    
    char path[FNAME_LEN];

    int rc = snprintf(path, FNAME_LEN, "%s%s", name, kls_ut_subdir);
    PATH_POSTPRINT_CHECK(path, name, rc);

    if (opendir(path))
    {
        *has_it = 1;
        rc = snprintf(path, FNAME_LEN, "%s%s%s", name, kls_ut_subdir,
                      ignored_flag);
        PATH_POSTPRINT_CHECK(path, name, rc);

        *is_ignored = opendir(path) ? 1 : 0;
    }
}

void kls_wr_walk(const char* name, bool is_root)
{
    DIR *dir = opendir(name);
    KLS_IO_CHECK(dir, "cannot open folder %s", name);

    bool is_kaseklis, has_kaseklis, is_ignored;
    kaseklis_flags(name, &is_kaseklis, &has_kaseklis, &is_ignored);

    if (is_kaseklis)
        return;

    if (has_kaseklis && !is_root)
    {
        kls_st_nested_ignored(name);
        if (!is_ignored)
            LOGI("ignoring %s as being indexed separately", name);
        return;
    }

    KLS_CHECK(!is_root || has_kaseklis, MISSING_FILE_OBJECT, 
              "root folder %s doesn't contain %s", name, kls_ut_subdir)
    KLS_CHECK(!is_root || !is_ignored, BAD_FILE_OBJECT, 
              "root folder %s contains ignored", name)

    char path[FNAME_LEN];
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (entry->d_type == DT_DIR) 
        {
            if (strcmp(entry->d_name, ".") == 0 || 
                strcmp(entry->d_name, "..") == 0)
                continue;
            int rc = snprintf(path, sizeof(path), "%s/%s", name, 
                              entry->d_name);
            PATH_POSTPRINT_CHECK(path, entry->d_name, rc);
            kls_wr_walk(path, 0);
        } 
        else 
        {
            int rc = snprintf(path, sizeof(path), "%s/%s", name, 
                              entry->d_name);
            PATH_POSTPRINT_CHECK(path, entry->d_name, rc);
            kls_fp_process(path);
        }
    }
    closedir(dir);
}


