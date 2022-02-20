#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <pwd.h>
#include <sys/stat.h>
#include <utmp.h>
#include <fcntl.h>
#include "htools.h"


int pos(char *s, int c) {
    register int i = 0;

    while (*s) {
        if (*s == c)
            return i;
        i++;
        s++;
    }
    return -1;
}


void circle_slow(int xc, int yc, int r1, int r2)
{
    register int i;
    int x, y;

    printf("int cir[] = {");
    for (i = 1; i < 340; i++) {
        //x = cos((double)i/55)*38;
        x = cos((double) i / 55) * r1 / 2;
        y = sin((double) i / 55) * r2 / 2;
        gotoxy(xc + x, yc + y);
        printf("*");
        fflush(stdout);
    }
}


void online()
{
    struct utmp user;
    struct passwd *pwd;
    struct stat st;
    int f, i;
    char ttydev[16];
    char *p;

    f = open(UTMP_FILE, 0);
    puts("Name\t  Port\t  Erreichbar    Bemerkung");
    while (read(f, &user, sizeof(user))) {
        if (user.ut_type != USER_PROCESS)
            continue;
        strcpy(ttydev, "/dev/");
        strcat(ttydev, user.ut_line);
        stat(ttydev, &st);
        if (st.st_mode & 00002)
            p = "Ja  ";
        else
            p = "Nein";
        pwd = getpwnam(user.ut_name);
        if (pwd != NULL && (i = pos(pwd->pw_gecos, ',')) != -1)
            pwd->pw_gecos[i] = 0;
        printf("%-8s  %-8s%s  \t%s\n", user.ut_name, user.ut_line, p, (pwd == NULL ? "" : pwd->pw_gecos));
    }
    close(f);
}


char *input_test(void)
{
    return strdup("test ");
}


int main(int argc, char **argv)
{
    DI *d;
    int i;
    ARRAY_T *a = array_init(5, 1);
    char **arr = NULL;
    int arr_cnt = 0;
    char str[256];
    int ch;
    char *s, *p;
    FILE *f;
    char buf[102400];
    char **list = NULL;
    int list_cnt = 0;
    int total_cnt = 0;
    buf_t *b;
    int x, y;
    char **a_files = NULL, **a_types = NULL;

    init_tcap();
    clrscr();
    a = array_add(a, "bla");
    a = array_add(a, "xz");
    a = array_add(a, "list");
    a = array_add(a, "bla");
    a = array_add(a, "xz");
    a = array_add(a, "list");
    a = array_add(a, "bla");
    a = array_add(a, "xz");
    a = array_add(a, "list");
    a = array_add(a, "bla");
    a = array_add(a, "xz");
    a = array_add(a, "list");

    for (i = 0; i < a->count; i++)
        puts(a->list[i]);
    array_free(a);

    circle_slow(13, 15, 25, 14);
    b = buf_init();
    b = buf_add(b, "test ");
    b = buf_add_sprintf(b, "(%d)", 3);
    puts(b->buffer);
    while (!kbhit())
        ;


    strcpy(str, "Dies  Ist ein   TEST  ");
    arr = split_quoted_words(str, arr, &arr_cnt, is_space);
    for (i = 0; i < arr_cnt; i++)
        puts(arr[i]);

    strcpy(str, "CSV,test,bla,\"testq\",,,ende,");
    arr = split_quoted_csv(str, arr, &arr_cnt, is_comma);
    for (i = 0; i < arr_cnt; i++)
        puts(arr[i]);

    setty();
    while (!kbhit())
        ;
    ch = readkey();
    resetty();
    printf("%c\n", ch);

    ikey_add("\033OP", input_test);
    do {
        s = input("prompt>");
        if (s == NULL) {
            putchar('\n');
            break;
        }
        if (!strcmp(s, "quit"))
            break;
        if (!strcmp(s, "sql")) {
            f = fopen("v.csv", "r");
            while (fgets(buf, sizeof(buf), f))
                list = add_to_list(list, strip(buf, "\"\n"));
            fclose(f);
            p = sql_where_in(list);
            printf("UPDATE voucher set expires='0002-00-00 00:00:00',user_id=0,pass='273766393159759d' WHERE voucher_nr IN %s;\n",
                   p);
            free(p);
            list_free(list);
            list = NULL;
        }
        if (!strcmp(s, "mail")) {
            a_files = add_to_list(a_files, "/tmp/test.zip");
            a_types = add_to_list(a_types, "application/zip");
            sending_email("test@test.de", "hej@gis.de", "test-subject", "test-body", NULL, a_files, a_types);
        }
        if (!strncmp(s, "tr", 2)) {
            arr = split_quoted_words(s, arr, &arr_cnt, is_space);
            total_cnt = 0;
            f = fopen("tr.csv", "r");
            while (fgets(buf, sizeof(buf), f)) {
                list = split_quoted_csv(buf, list, &list_cnt, is_comma);
                if (list == NULL || list_cnt < 200)
                    continue;
                printf("%s,%s\n", list[0], list[atoi(arr[1])]);
                total_cnt++;
            }
            fclose(f);
            list_free(list);
            printf("total_cnt = %d\n", total_cnt);
            list = NULL;
            list_cnt = 0;
        }
        if (!strncmp(s, "dir", 3)) {
            arr = split_quoted_words(s, arr, &arr_cnt, is_space);
            d = getdir(arr[1] == NULL ? "." : arr[1]);
            if (d == NULL) {
                fprintf(stderr, "%s\n", strerror(errno));
                continue;
            }
            for (i = 0; i < d->no_of_files; i++) {
                printf("%-15.15s %8d\n", d->fl[i].name, d->fl[i].stat.st_size);
            }
            freedir(d);
        }
        if (!strcmp(s, "size")) {
            scrsize(&x, &y);
            printf("x=%d -- y=%d\n", x, y);
        }
        if (!strcmp(s, "on"))
            online();
        free(s);
    } while (3);
}