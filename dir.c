#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>

#include "htools.h"


static int _fl_name_cmp(FL *a, FL *b)
{
    return (strcmp(a->name, b->name));
}

DI *getdir(char *cwd)
{
    DIR *dirp;
    struct dirent *dp;
    char tmp[PATH_MAX];
    DI *d;

    dirp = opendir(cwd);
    if (dirp == NULL)
        return NULL;

    d = malloc(sizeof(DI));
    d->fl = malloc(sizeof(FL) * (d->no_of_files + 64));
    d->no_of_files = 0;
    while (dp = readdir(dirp)) {
        if (d->no_of_files != 0 && (d->no_of_files % 64) == 0)
            d->fl = realloc(d->fl, sizeof(FL) * (d->no_of_files + 64));
        d->fl[d->no_of_files].name = strdup(dp->d_name);
        strcpy(tmp, cwd);
        strcat(tmp, "/");
        strcat(tmp, dp->d_name);
        lstat(tmp, &d->fl[d->no_of_files++].stat);
    }
    closedir(dirp);
    qsort(d->fl, d->no_of_files, sizeof(FL), (void *) _fl_name_cmp);

    return d;
}

void freedir(DI *d)
{
    while (d->no_of_files--)
        free(d->fl[d->no_of_files].name);
    free(d->fl);
    free(d);
}