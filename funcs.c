#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "htools.h"


char *stamp()
{
    static char s[256];
    time_t t;
    struct tm *tm;

    t = time(NULL);
    tm = localtime(&t);
    snprintf(s, sizeof(s), "%2.2d/%2.2d:%2.2d [%d]", tm->tm_mday, tm->tm_hour, tm->tm_min,
             getpid());

    return s;
}


char *double2money(double d)
{
    static char ns[256];
    char *p, *p2;

    snprintf(ns, sizeof(ns), "%6.6f", d);

    p = ns;
    while (*p)
        p++;

    if (*p == 0)
        p--;

    p2 = strchr(ns, '.');
    *p2 = ',';

    while (*p == '0' && p != p2 + 3) {
        *p = 0;
        p--;
    }

    return ns;
}


int is_space(char c)
{
    return isspace(c);
}

int is_space_logfile(char c)
{
    return isspace(c) || c == ':';
}

int is_semi(char c)
{
    return c == ';';
}

int is_minus(char c)
{
    return c == '-';
}

int is_32(char c)
{
    return c == ' ' || c == '\n';
}

int is_dot(char c)
{
    return c == '.';
}

int is_slashdot(char c)
{
    return c == '.' || c == '/';
}

int is_comma(char c)
{
    return c == ',';
}

int is_slash(char c)
{
    return c == '/';
}

int is_newline(char c)
{
    return c == '\n';
}

int is_plus(char c)
{
    return c == '+';
}

int is_pipe(char c)
{
    return c == '|';
}


char **split_quoted_csv(char *str, char **arg, int *cnt, int (*trunc)(char c))
{
    char *tmp = malloc(strlen(str) + 1);
    int i = 0, j = 0;

    if (arg == NULL) {
        arg = malloc(sizeof(char *));
        arg[0] = NULL;
    }

    while (*str) {
        while (*str && !(*trunc)(*str) && *str != '"')
            tmp[i++] = *str++;
        if (*str == '"') {
            str++;
            while (1) {
                if (*str == 0 || (*str == '"' && (*(str + 1) == ',' || *(str + 1) == 0)))
                    break;
                tmp[i++] = *str++;
            }
            if (*str)
                str++;
        }
        tmp[i] = 0;
        if (arg[j] == NULL)
            arg[j] = malloc(i + 1);
        else
            arg[j] = realloc(arg[j], i + 1);
        strcpy(arg[j++], tmp);
        i = 0;

        if (j > (*cnt)) {
            arg = realloc(arg, sizeof(char *) * (j + 1));
            arg[j] = NULL;
        }
        if (*str)
            str++;
    }
    if (j < (*cnt)) {
        for (i = j; i < (*cnt); i++) {
            free(arg[i]);
            arg[i] = NULL;
        }
        arg = realloc(arg, sizeof(char *) * (j + 1));
    }
    (*cnt) = j;
    free(tmp);

    return arg;
}

char **split_quoted_words(char *str, char **arg, int *cnt, int (*trunc)(char c))
{
    char *tmp = malloc(strlen(str) + 1);
    int i = 0, j = 0;

    if (arg == NULL) {
        arg = malloc(sizeof(char *));
        arg[0] = NULL;
    }

    while (*str) {
        while (*str && (*trunc)(*str) && *str != '"')
            str++;

        if (*str != 0) {
            while (*str && !(*trunc)(*str) && *str != '"')
                tmp[i++] = *str++;
            if (*str == '"') {
                str++;
                while (*str && *str != '"')
                    tmp[i++] = *str++;
                if (*str)
                    str++;
            }
            tmp[i] = 0;
            if (arg[j] == NULL)
                arg[j] = malloc(i + 1);
            else
                arg[j] = realloc(arg[j], i + 1);
            strcpy(arg[j++], tmp);
            i = 0;
        }

        if (j > (*cnt)) {
            arg = realloc(arg, sizeof(char *) * (j + 1));
            arg[j] = NULL;
        }
    }
    if (j < (*cnt)) {
        for (i = j; i < (*cnt); i++) {
            free(arg[i]);
            arg[i] = NULL;
        }
        arg = realloc(arg, sizeof(char *) * (j + 1));
    }
    (*cnt) = j;
    free(tmp);

    return arg;
}


/*
 * frees a char array (char **); last element must be NULL
 */

char **list_free(char **a)
{
    int i;

    if (a == NULL)
        return NULL;

    for (i = 0; a[i] != NULL; i++) {
        free(a[i]);
    }
    free(a);

    return NULL;
}


inline int isdigitstr(char *s)
{
    if (*s == 0)
        return 0;

    while (*s) {
        if (!isdigit(*s))
            return 0;
        s++;
    }

    return 1;
}


inline int isfloatstr(char *s)
{
    int dotcnt = 0;

    if (*s == 0)
        return 0;

    while (*s) {
        if (*s == '.') {
            if (++dotcnt > 1)
                return 0;
        } else if (!isdigit(*s))
            return 0;
        s++;
    }

    return 1;
}


int isIP(char *s)
{
    int i;
    int a_cnt = 0;
    char **arr = NULL;

    arr = split_quoted_words(s, arr, &a_cnt, is_dot);
    if (a_cnt < 4) {
        list_free(arr);
        return FALSE;
    }
    for (i = 0; i < a_cnt; i++) {
        if (!isdigitstr(arr[i]) || !strcmp(arr[i], "") || atoi(arr[i]) > 255 || atoi(arr[i]) < 0) {
            list_free(arr);
            return FALSE;
        }
    }
    list_free(arr);

    return TRUE;
}


char **add_to_list(char **list, char *str)
{
    int i = 0;

    if (list == NULL) {
        list = malloc(sizeof(char *) * 2);
    } else {
        for (; list[i] != NULL; i++);
        list = realloc(list, sizeof(char *) * (i + 2));
    }
    list[i] = strdup(str);
    list[i + 1] = NULL;

    return list;
}

int is_in_list(char **list, char *search)
{
    int i;

    if (list == NULL || search == NULL)
        return FALSE;

    for (i = 0; list[i] != NULL; i++)
        if (!strcmp(list[i], search))
            return TRUE;   /* found */

    return FALSE;       /* not found */
}

int is_in_list_i(char **list, char *search)
{
    int i;

    if (list == NULL || search == NULL)
        return FALSE;

    for (i = 0; list[i] != NULL; i++)
        if (!strcasecmp(list[i], search))
            return TRUE;   /* found */

    return FALSE;       /* not found */
}


int find_in_list(char **list, char *search)
{
    int i;

    if (list == NULL || search == NULL)
        return -1;

    for (i = 0; list[i] != NULL; i++)
        if (!strcmp(list[i], search))
            return i;

    return -1;
}

int find_in_list_i(char **list, char *search)
{
    int i;

    if (list == NULL || search == NULL)
        return -1;

    for (i = 0; list[i] != NULL; i++)
        if (!strcasecmp(list[i], search))
            return i;

    return -1;
}


char **del_index_from_list(char **list, int i)
{
    if (list == NULL)
        return list;

    if (list[i] != NULL) {
        free(list[i]);
        do
            list[i] = list[i + 1];
        while (list[i++] != NULL);

        if (list[0] == NULL) {
            free(list);
            list = NULL;
        } else
            list = realloc(list, sizeof(char *) * (i + 1));
    }

    return list;
}

char **del_from_list(char **list, char *str)
{
    int i = 0;

    if (list == NULL || str == NULL)
        return list;

    for (; list[i] != NULL && strcmp(list[i], str); i++);

    return del_index_from_list(list, i);
}


inline int ccnt(char *s, int c)
{
    int cnt = 0;

    if (s == NULL)
        return 0;

    while (*s)
        if (*s++ == c)
            cnt++;

    return cnt;
}


int str2ipmask(char *ip, unsigned long *net, unsigned long *mask)
{
    char *smask, *net1, *net2, *net3, *net4, *tmp = strdup(ip), *s = ip;
    int maskbits, i;
    unsigned long new_mask;

    while (*s) {
        if (strchr("0123456789./", *s) == NULL) {
            free(tmp);
            return INVALID;
        }
        s++;
    }

    net1 = tmp;
    smask = strchr(tmp, '/');
    if (smask == NULL)
        return INVALID;
    if (strlen(smask) > 3 || strlen(smask) < 1 || ccnt(ip, '.') > 4 || ccnt(ip, '/') > 1) {
        free(tmp);
        return INVALID;
    }
    if (smask == NULL) {
        free(tmp);
        return INVALID;
    }
    *smask = 0;
    smask++;
    net2 = strchr(net1, '.');
    if (net2 != NULL) {
        *net2 = 0;
        net2++;
    } else
        net2 = "0";
    net3 = strchr(net2, '.');
    if (net3 != NULL) {
        *net3 = 0;
        net3++;
    } else
        net3 = "0";
    net4 = strchr(net3, '.');
    if (net4 != NULL) {
        *net4 = 0;
        net4++;
    } else
        net4 = "0";

    if (strlen(net1) > 3 || atol(net1) > 255 || atol(net1) < 0 || !isdigitstr(net1) ||
        strlen(net2) > 3 || atol(net2) > 255 || atol(net2) < 0 || !isdigitstr(net2) ||
        strlen(net3) > 3 || atol(net3) > 255 || atol(net3) < 0 || !isdigitstr(net3) ||
        strlen(net4) > 3 || atol(net4) > 255 || atol(net4) < 0 || !isdigitstr(net4)) {
        free(tmp);
        return INVALID;
    }

    *net = (atol(net1) << 24) + (atol(net2) << 16) + (atol(net3) << 8) + (atol(net4));
    new_mask = 0;
    maskbits = atoi(smask);
    if (maskbits < 0 || maskbits > 32) {
        free(tmp);
        return INVALID;
    }
    for (i = 0; i < maskbits; i++) {
        new_mask <<= 1;
        new_mask |= 1;
    }
    for (i = 0; i < 32 - maskbits; i++)
        new_mask <<= 1;
    free(tmp);

    *mask = new_mask;

    return TRUE;
}


inline int ccntn(char *s, int c, int len)
{
    int cnt = 0;

    if (s == NULL)
        return 0;

    while (len--)
        if (*s++ == c)
            cnt++;

    return cnt;
}


inline char *trim(char *s)
{
    char *p;

    while (isspace(*s))
        s++;

    if (!*s)
        return s;

    p = s;
    while (*p)
        p++;
    p--;
    while (isspace(*p))
        p--;
    *(p + 1) = 0;

    return s;
}


int bit_cnt(int byte)
{
    int bits = 0;

    if (byte >= 128) {
        bits++;
        byte -= 128;
    }

    if (byte >= 64) {
        bits++;
        byte -= 64;
    }

    if (byte >= 32) {
        bits++;
        byte -= 32;
    }

    if (byte >= 16) {
        bits++;
        byte -= 16;
    }

    if (byte >= 8) {
        bits++;
        byte -= 8;
    }

    if (byte >= 4) {
        bits++;
        byte -= 4;
    }

    if (byte >= 2) {
        bits++;
        byte -= 2;
    }

    if (byte >= 1) {
        bits++;
        byte -= 1;
    }

    return bits;
}

char *mask2str(unsigned long mask)
{
    char *s = malloc(5);
    int m1, m2, m3, m4;

    m1 = bit_cnt(((mask & 0xff000000) >> 24));
    m2 = bit_cnt(((mask & 0x00ff0000) >> 16));
    m3 = bit_cnt(((mask & 0x0000ff00) >> 8));
    m4 = bit_cnt(((mask & 0x000000ff)));

    snprintf(s, 5, "%d", m1 + m2 + m3 + m4);

    return s;
}

char *ipmask2str(unsigned long ip, unsigned long mask) {
    static char s[50];
    int m1, m2, m3, m4;

    m1 = bit_cnt(((mask & 0xff000000) >> 24));
    m2 = bit_cnt(((mask & 0x00ff0000) >> 16));
    m3 = bit_cnt(((mask & 0x0000ff00) >> 8));
    m4 = bit_cnt(((mask & 0x000000ff)));

    snprintf(s, sizeof(s), "%d.%d.%d.%d/%d",
             (ip & 0xff000000) >> 24,
             (ip & 0x00ff0000) >> 16,
             (ip & 0x0000ff00) >> 8,
             (ip & 0x000000ff), m1 + m2 + m3 + m4);

    return s;
}

char *ip2str(unsigned long ip)
{
    char *s = malloc(20);

    snprintf(s, 20, "%d.%d.%d.%d",
             (ip & 0xff000000) >> 24,
             (ip & 0x00ff0000) >> 16,
             (ip & 0x0000ff00) >> 8,
             (ip & 0x000000ff));

    return s;
}


unsigned long str2ip(char *str)
{
    unsigned long inet;
    char *net1, *net2, *net3, *net4, *tmp = strdup(str);

    net1 = tmp;

    net2 = strchr(net1, '.');
    if (net2 != NULL) {
        *net2 = 0;
        net2++;
    } else
        net2 = "0";

    net3 = strchr(net2, '.');
    if (net3 != NULL) {
        *net3 = 0;
        net3++;
    } else
        net3 = "0";

    net4 = strchr(net3, '.');
    if (net4 != NULL) {
        *net4 = 0;
        net4++;
    } else
        net4 = "0";

    inet = (atol(net1) << 24) + (atol(net2) << 16) + (atol(net3) << 8) + (atol(net4));

    free(tmp);

    return inet;
}


void convert2ipmask(char *ip, unsigned long *net, unsigned long *mask)
{
    char *smask, *net1, *net2, *net3, *net4, *tmp = strdup(ip);
    int maskbits, i;
    unsigned long new_mask;

    net1 = tmp;
    smask = strchr(tmp, '/');
    *smask = 0;
    smask++;
    net2 = strchr(net1, '.');
    if (net2 != NULL) {
        *net2 = 0;
        net2++;
    } else
        net2 = "0";
    net3 = strchr(net2, '.');
    if (net3 != NULL) {
        *net3 = 0;
        net3++;
    } else
        net3 = "0";
    net4 = strchr(net3, '.');
    if (net4 != NULL) {
        *net4 = 0;
        net4++;
    } else
        net4 = "0";
    (*net) = (atol(net1) << 24) + (atol(net2) << 16) + (atol(net3) << 8) + (atol(net4));
    new_mask = 0;
    maskbits = atoi(smask);
    for (i = 0; i < maskbits; i++) {
        new_mask <<= 1;
        new_mask |= 1;
    }
    for (i = 0; i < 32 - maskbits; i++)
        new_mask <<= 1;
    free(tmp);

    (*mask) = new_mask;
}


long list_cnt(char **arr)
{
    long i;

    if (arr == NULL)
        return 0;

    for (i = 0; arr[i] != NULL; i++)
        ;

    return i;
}


char **list_dup(char **arr)
{
    int i;
    char **narr;

    if (arr == NULL)
        return NULL;
    for (i = 0; arr[i] != NULL; i++);
    narr = malloc(sizeof(char *) * (i + 1));
    for (i = 0; arr[i] != NULL; i++)
        narr[i] = strdup(arr[i]);
    narr[i] = NULL;

    return narr;
}

char **list_dupn(char **arr, int n)
{
    int i;
    char **narr;

    if (arr == NULL)
        return NULL;
    narr = malloc(sizeof(char *) * (n + 1));
    for (i = 0; i < n; i++)
        narr[i] = strdup(arr[i]);
    narr[i] = NULL;

    return narr;
}


char **list_cat(char **arr, char **cat)
{
    int c;

    if (arr == NULL && cat == NULL)
        return NULL;
    if (cat == NULL)
        return arr;

    for (c = 0; cat[c] != NULL; c++)
        arr = add_to_list(arr, cat[c]);

    return arr;
}


int list_cmp(char **arr1, char **arr2)
{
    int i1, i2, i;

    if (arr1 == NULL && arr2 == NULL)
        return TRUE;

    if ((arr1 == NULL && arr2 != NULL) ||
        (arr1 != NULL && arr2 == NULL))
        return FALSE;

    for (i1 = 0; arr1[i1] != NULL; i1++)
        ;

    for (i2 = 0; arr2[i2] != NULL; i2++)
        ;

    if (i1 != i2)
        return FALSE;

    for (i = 0; i < i1; i++) {
        if (strcmp(arr1[i], arr2[i]))
            return FALSE;
    }

    return TRUE;
}


inline char *list2str(char **arr, char *delim)
{
    int i, len = 0;
    char *new;

    if (arr == NULL)
        return NULL;

    for (i = 0; arr[i] != NULL; i++) {
        len += strlen(arr[i]);
        if (arr[i + 1] != NULL)
            len += strlen(delim);
    }

    new = malloc(len + 1);

    *new = 0;
    for (i = 0; arr[i] != NULL; i++) {
        strcat(new, arr[i]);
        if (arr[i + 1] != NULL)
            strcat(new, delim);
    }

    return new;
}


int int_list_cmp(int *a, int *b, int cnt)
{
    int i;

    for (i = 0; i < cnt; i++)
        if (a[i] != b[i])
            return FALSE;

    return TRUE;
}


inline char *dstrncpy(char *buf, char *new, int n)
{
    if (buf == NULL)
        buf = malloc(n + 1);
    else
        buf = realloc(buf, n + 1);

    strncpy(buf, new, n);

    return buf;
}


inline char *dstrcpy(char *buf, char *new)
{
    if (buf == NULL)
        buf = malloc(strlen(new) + 1);
    else
        buf = realloc(buf, strlen(new) + 1);

    strcpy(buf, new);

    return buf;
}


inline char *dstrncat(char *buf, char *add, int n)
{
    if (buf == NULL) {
        buf = malloc(n + 1);
        *buf = 0;
    } else
        buf = realloc(buf, strlen(buf) + n + 1);

    strncat(buf, add, n);

    return buf;
}

inline char *dstrcat(char *buf, char *add)
{
    if (add == NULL)
        return buf;

    if (buf == NULL) {
        buf = malloc(strlen(add) + 1);
        *buf = 0;
    } else
        buf = realloc(buf, strlen(buf) + strlen(add) + 1);

    strcat(buf, add);

    return buf;
}


inline int iswhitespace(char *s)
{
    char *p = s;

    while (*p) {
        if (isspace(*p))
            return 1;
        p++;
    }

    return 0;
}


#define    QUOTE    0200
#define    QMASK    (0377&~QUOTE)

static char *cclass();

int match(char *s, char *p) {
    int sc, pc;

    if (s == NULL || p == NULL)
        return (0);
    while ((pc = *p++ & 0377) != '\0') {
        sc = *s++ & QMASK;
        switch (pc) {
            case '[':
                if ((p = cclass(p, sc)) == NULL)
                    return (0);
                break;

            case '?':
                if (sc == 0)
                    return (0);
                break;

            case '*':
                s--;
                do {
                    if (*p == '\0' || match(s, p))
                        return (1);
                } while (*s++ != '\0');
                return (0);

            default:
                if (sc != (pc & ~QUOTE))
                    return (0);
        }
    }
    return (*s == 0);
}


int matchcase(char *s, char *p)
{
    int sc, pc;

    if (s == NULL || p == NULL)
        return (0);
    while ((pc = toupper(*p) & 0377) != '\0') {
        p++;
        sc = toupper(*s) & QMASK;
        s++;
        switch (pc) {
            case '[':
                if ((p = cclass(p, sc)) == NULL)
                    return (0);
                break;

            case '?':
                if (sc == 0)
                    return (0);
                break;

            case '*':
                s--;
                do {
                    if (*p == '\0' || match(s, p))
                        return (1);
                } while (*s++ != '\0');
                return (0);

            default:
                if (sc != (pc & ~QUOTE))
                    return (0);
        }
    }
    return (*s == 0);
}


static char *cclass(p, sub)
        register char *p;
        register int sub;
{
    int c, d, not, found;

    if ((not = *p == '!') != 0)
        p++;
    found = not;
    do {
        if (*p == '\0')
            return (NULL);
        c = *p & 0377;
        if (p[1] == '-' && p[2] != ']') {
            d = p[2] & 0377;
            p++;
        } else
            d = c;
        if (c == sub || c <= sub && sub <= d)
            found = !not;
    } while (*++p != ']');

    return (found ? p + 1 : NULL);
}

inline double fgrade(double val, double div)
{
    double x, y;

    x = val / div;
    y = fmod(val, div);
    if (y != 0)
        return (val - y) / div + 1;
    else
        return x;
}


inline char *x_itoa(int n)
{
    static char buf[20];

    snprintf(buf, sizeof(buf), "%d", n);

    return buf;
}


char *path2file(char *s)
{
    char *n, *r;

    if (*s == '/')
        s++;
    n = strdup(s);
    r = n;

    while (*n) {
        if (*n == '/')
            *n = '.';
        n++;
    }

    return r;
}


void daemonize(int verbose_flag)
{
    switch (fork()) {
        case -1:
            syslog(LOG_ERR, "Couldn't fork.");
            fprintf(stderr, "Couldn't fork.\n");
            exit(2);
        case 0:
            /* We are the child */
            break;
        default:
            /* We are the parent */
            sleep(1);
            _exit(0);
    }

    setsid();

    switch (fork()) {
        case -1:
            syslog(LOG_ERR, "Couldn't fork.");
            exit(2);
        case 0:
            /* We are the child */
            break;
        default:
            /* We are the parent */
            sleep(1);
            _exit(0);
    }

    if (verbose_flag == TRUE)
        fprintf(stderr, "* *  >> [process id %d]\n", getpid());

    chdir("/");

    umask(077);
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
}


char *double2valuta(double v)
{
    char *str = malloc(1024);
    char *p;

    snprintf(str, 1024, "%.2f", v);
    p = str;
    while (*p) {
        if (*p == '.')
            *p = ',';
        p++;
    }

    return str;
}

char *double2valuta4(double v)
{
    char *str = malloc(1024);
    char *p;

    snprintf(str, 1024, "%.4f", v);
    p = str;
    while (*p) {
        if (*p == '.')
            *p = ',';
        p++;
    }

    return str;
}


static int days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int month_days(int month, int year)
{
    int days;

    if (month < 1 || year < 1 || month > 12)
        return 0;
    days = days_per_month[month - 1];
    if (year % 4 == 0 && month == 2)
        days++;

    return days;
}


static int _day = 0;
static int _month = 0;
static int _year = 0;

int day_first(int day_from, int month_from, int year_from, int day_till, int month_till, int year_till)
{
    _day = day_from;
    _month = month_from;
    _year = year_from;

    if (_month > 12 || _month < 1)
        return 0;

    if (_day > month_days(_month, _year) || _day < 1)
        return 0;

    if (month_till > 12 || month_till < 1)
        return 0;

    if (day_till > month_days(month_till, year_till) || day_till < 1)
        return 0;

    return _day;
}

int day_next(int day, int month, int year)
{
    _day++;

    if (_day == day + 1 && _month == month && _year == year)
        return 0;

    if (_day > month_days(_month, _year)) {
        _month++;
        _day = 1;
    }
    if (_month > 12) {
        _year++;
        _month = 1;
    }

    return _day;
}

int day_month()
{
    return _month;
}

int day_year()
{
    return _year;
}


static int _month_from = 0;
static int _year_from = 0;
static int _month_till = 0;
static int _year_till = 0;


int month_first(int month_from, int year_from, int month_till, int year_till, int *m, int *y)
{
    _month_from = month_from;
    _year_from = year_from;
    _month_till = month_till;
    _year_till = year_till;

    *(m) = month_from;
    *(y) = year_from;

    return TRUE;
}


int month_next(int *m, int *y)
{
    if (_month_till == INVALID)
        return FALSE;

    _month_from++;

    if (_month_from > 12) {
        _year_from++;
        _month_from = 1;
    }

    if (_year_from > _year_till || (_year_from == _year_till && _month_from > _month_till))
        return FALSE;

    *(m) = _month_from;
    *(y) = _year_from;

    return TRUE;
}

time_t quartal_next(time_t t)
{
    struct tm *tm;

    tm = localtime(&t);
    switch (tm->tm_mon) {
        case 0:
        case 1:
        case 2:
            tm->tm_mon = 3;
            tm->tm_isdst = 1;
            break;
        case 3:
        case 4:
        case 5:
            tm->tm_mon = 6;
            tm->tm_isdst = 1;
            break;
        case 6:
        case 7:
        case 8:
            tm->tm_mon = 9;
            tm->tm_isdst = 1;
            break;
        case 9:
        case 10:
        case 11:
            tm->tm_mon = 0;
            tm->tm_year++;
            tm->tm_isdst = 0;
            break;
    }
    tm->tm_sec = 0;
    tm->tm_min = 0;
    tm->tm_hour = 0;
    tm->tm_mday = 1;
    tm->tm_wday = 0;
    tm->tm_yday = 0;

    return mktime(tm);
}


int posn(char *s, int c, int len)
{
    int cnt = 0;

    while (len--) {
        if (*s == c)
            return cnt;
        s++;
        cnt++;
    }

    return -1;
}

void replace_char(char *s, int old, int new)
{
    while (*s) {
        if (*s == old)
            *s = new;
        s++;
    }
}


char *replace_str(char *s, char *search, char *replace, int count)
{
    char *p = strdup(s);
    char *n = NULL;
    char *x;
    int len;

    do {
        x = strstr(p, search);
        if (x == NULL)
            break;
        if (n == NULL)
            n = malloc(strlen(p) + strlen(replace) + 1);
        else
            n = realloc(n, strlen(p) + strlen(replace) + 1);
        len = strlen(p) - strlen(x);
        strncpy(n, p, len);
        n[len] = 0;
        strcat(n, replace);
        strcat(n, x + strlen(search));
        p = realloc(p, strlen(n) + 1);
        strcpy(p, n);
    } while (--count);
    if (n != NULL)
        free(n);

    return p;
}


char *get_quoted_str(char *s, int q1, int q2)
{
    char *n = malloc(strlen(s) + 1);
    char *p = n;

    while (*s) {
        if (*s == q1) {
            s++;
            while (*s && *s != q2)
                *p++ = *s++;
            *p = 0;
            return n;
        }
        s++;
    }

    return NULL;
}


void delete_char(char *s, int c)
{
    char tmp[strlen(s) + 1], *t, *q;

    t = tmp;
    q = s;
    while (*s) {
        if (*s == c) {
            s++;
            continue;
        }
        *t++ = *s++;
    }
    *t = 0;
    strcpy(q, tmp);
}

char *volume_str_bytes(double bytes)
{
    static char str[20], *p;

    str[0] = 0;
    if (bytes > 1024. * 1024 * 1024 * 1024 * 1024)
        snprintf(str, sizeof(str), "%4.2fP", bytes / 1024 / 1024 / 1024 / 1024 / 1024);

    if (str[0] == 0 && bytes > 1024. * 1024 * 1024 * 1024)
        snprintf(str, sizeof(str), "%4.2fT", bytes / 1024 / 1024 / 1024 / 1024);

    if (str[0] == 0 && bytes > 1024. * 1024 * 1024)
        snprintf(str, sizeof(str), "%4.2fG", bytes / 1024 / 1024 / 1024);

    if (str[0] == 0 && bytes > 1024. * 1024)
        snprintf(str, sizeof(str), "%4.2fM", bytes / 1024 / 1024);

    if (str[0] == 0 && bytes > 1024.)
        snprintf(str, sizeof(str), "%4.2fK", bytes / 1024);

    if (str[0] == 0)
        snprintf(str, sizeof(str), "%4.2fB", bytes);

    p = strchr(str, '.');
    if (*(p + 1) == '0' && *(p + 2) == '0') {
        *p = *(p + 3);
        *(p + 1) = 0;
    }

    return str;
}


char *volume_str(double vol)
{
    static char str[20];

    str[0] = 0;
    if (str[0] == 0 && vol > 1000. * 1000 * 1000)
        snprintf(str, sizeof(str), "%.1fB", vol / 1000 / 1000 / 1000);

    if (str[0] == 0 && vol > 1000. * 1000)
        snprintf(str, sizeof(str), "%.1fM", vol / 1000 / 1000);

    if (str[0] == 0 && vol >= 10000.)
        snprintf(str, sizeof(str), "%.1fT", vol / 1000);

    if (str[0] == 0)
        snprintf(str, sizeof(str), "%.0f", vol);

    /*
    p = strchr(str, '.');
    if (p != NULL) {
      if (*(p + 1) == '0') {
        *p = *(p + 2);
        *(p + 1) = 0;
      }
      }*/

    return str;
}


char *packets_str(double packets)
{
    static char str[20], *p;

    str[0] = 0;
    if (packets > 1000. * 1000 * 1000)
        snprintf(str, sizeof(str), "%4.2f>", packets / 1000 / 1000 / 1000);

    if (str[0] == 0 && packets > 1000. * 1000)
        snprintf(str, sizeof(str), "%4.2fM", packets / 1000 / 1000);

    if (str[0] == 0) {
        snprintf(str, sizeof(str), "%6.0f ", packets);
        return str;
    }

    p = strchr(str, '.');
    if (*(p + 1) == '0' && *(p + 2) == '0') {
        *p = *(p + 3);
        *(p + 1) = 0;
    }

    return str;
}


inline char *wup(char *s)
{
    char *t = s;

    if (t == NULL)
        return NULL;

    do
        *s = toupper(*s);
    while (*s++);

    return t;
}

inline char *wlower(char *s)
{
    char *t = s;

    do
        *s = tolower(*s);
    while (*s++);

    return t;
}


inline int testchars(char *s, char *char_list)
{
    while (*s) {
        if (strchr(char_list, *s) != NULL)
            return TRUE;
        s++;
    }

    return FALSE;
}


char *long2clock(long t)
{
    long a = 0, b = 0, c = 0;
    static char str[256];

    if (t > 3600) {
        a = t / 3600;
        t = t % 3600;
    }

    if (t > 60)
        b = t / 60;

    c = t % 60;


    sprintf(str, "%2.2d:%2.2d:%2.2d", a, b, c);

    return str;
}


char *format_bytes(double v, int flag)
{
    int len, j;
    char *s = NULL;
    char *tmp;

    if (v == 0.0)
        return strdup("0");

    len = (int) log10(v) + 1;
    if (flag == FALSE || len < 4) {
        s = malloc(len + 1);
        snprintf(s, len + 1, "%.0f", v);
    } else {
        if ((len % 3) == 0)
            len += len / 3 - 1;
        else
            len += len / 3;
        s = malloc(len + 1);
        tmp = malloc(len + 1);
        snprintf(tmp, len + 1, "%.0f", v);
        while (*tmp)
            tmp++;
        j = 0;
        s[len] = 0;
        while (len) {
            if (j++ == 3) {
                j = 0;
                if (len - 1 != 0)
                    s[--len] = '.';
                continue;
            }
            s[--len] = *--tmp;
        }
        free(tmp);
    }

    return s;
}


inline unsigned long get_class_mask(unsigned long ip)
{
    if (ip >= 0UL && ip <= 2147483647UL)
        return 4278190080UL;
    if (ip >= 214783648UL && ip <= 3221225471UL)
        return 4294901760UL;
    if (ip >= 3221225462UL && ip <= 3758096383UL)
        return 4294967040UL;

    return 4294967295UL;
}


char *ascii2str(int ch)
{
    char *t, *r;

    t = malloc(3);
    r = t;

    if (ch < 0x20 && ch >= 0x00) {
        *r++ = '^';
        *r++ = ch + 0x40;
    } else if (ch < 0x00) {
        *r++ = '.';
    } else {
        *r++ = ch++;
    }
    *r = 0;

    return t;
}


char *control2str(char *cstr, int cnt)
{
    char *s, *t;
    int n = cnt;

    s = malloc(cnt * 3 + 1);
    *s = 0;
    while (n--) {
        t = ascii2str(*cstr++);
        strcat(s, t);
        free(t);
    }

    return s;
}


int check_ctrl_chars(char *buf)
{
    char *s = buf;

    while (*s) {
        if (*s < 32 && *s != 9 && *s != 10 && *s != 13)
            return TRUE;
        s++;
    }

    return FALSE;
}


inline char *strnstr(char *string, char *substr, int n)
{
    char head_string;
    char head_substr;

    if (string == NULL || substr == NULL)
        return NULL;

    head_substr = *substr++;
    while (n--) {
        head_string = *string++;
        if (head_string == head_substr) {
            char *tail_string = string;
            char *tail_substr = substr;
            do {
                if (*tail_substr == '\0')
                    return string - 1;
            } while (*tail_string++ == *tail_substr++);
        }
    }

    return NULL;
}


inline char *format_string(const char *fmt, ...)
{
    /* Guess we need no more than 1024 bytes. */
    int n, size = 1024;
    char *p;
    va_list ap;

    p = malloc(size);
    while (1) {
        /* Try to print in the allocated space. */
        va_start(ap, fmt);
        n = vsnprintf(p, size, fmt, ap);
        va_end(ap);
        /* If that worked, return the string. */
        if (n > -1 && n < size)
            return p;
        /* Else try again with more space. */
        if (n > -1)     /* glibc 2.1 */
            size = n + 1; /* precisely what is needed */
        else            /* glibc 2.0 */
            size *= 2;    /* twice the old size */
        p = realloc(p, size);
    }

    return p;
}


inline char *strcasestr(char *haystack, char *needle)
{
    char *p, *startn = 0, *np = 0;

    for (p = haystack; *p; p++) {
        if (np) {
            if (toupper(*p) == toupper(*np)) {
                if (!*++np)
                    return startn;
            } else
                np = 0;
        } else if (toupper(*p) == toupper(*needle)) {
            np = needle + 1;
            startn = p;
        }
    }

    return NULL;
}


inline char *substrdup(const char *begin, const char *end)
{
    size_t len;
    char *p;

    if (end)
        len = end - begin;
    else
        len = strlen(begin);

    p = malloc(len + 1);
    memcpy(p, begin, len);
    p[len] = 0;

    return p;
}


#define BUF_CHUNK_SIZE 10240

buf_t *buf_init()
{
    buf_t *b;

    b = malloc(sizeof(buf_t));
    b->buffer = malloc(BUF_CHUNK_SIZE);
    b->memory = BUF_CHUNK_SIZE;
    b->length = 0;
    *b->buffer = 0;

    return b;
}

buf_t *buf_add(buf_t *b, char *s)
{
    int len = strlen(s);

    while (b->length + len > b->memory) {
        b->memory += BUF_CHUNK_SIZE;
        b->buffer = realloc(b->buffer, b->memory);
    }
    strcat(b->buffer, s);
    b->length += len;

    return b;
}

buf_t *buf_addn(buf_t *b, char *s, int size)
{
    while (b->length + size > b->memory) {
        b->memory += BUF_CHUNK_SIZE;
        b->buffer = realloc(b->buffer, b->memory);
    }
    strncat(b->buffer, s, size);
    b->length += size;

    return b;
}

buf_t *buf_exp(buf_t *b, size_t size_to_expand)
{
    b->memory += size_to_expand;
    b->buffer = realloc(b->buffer, b->memory);

    return b;
}

buf_t *buf_add_sprintf(buf_t *b, char *fmt, ...)
{
    char *tmp;
    va_list ap;
    int n, size = 1024;

    tmp = malloc(size);
    while (1) {
        va_start(ap, fmt);
        n = vsnprintf(tmp, size, fmt, ap);
        va_end(ap);
        if (n > -1 && n < size)
            break;
        if (n > -1)     /* glibc 2.1 */
            size = n + 1; /* precisely what is needed */
        else            /* glibc 2.0 */
            size *= 2;    /* twice the old size */
        tmp = realloc(tmp, size);
    }
    b = buf_add(b, tmp);
    free(tmp);

    return b;
}


void buf_free(buf_t *b)
{
    if (b != NULL) {
        if (b->buffer != NULL)
            free(b->buffer);
        free(b);
    }
}

int date_init(char *s, int *m, int *y)
{
    int v;
    char *o, *x, *month = NULL, *year = NULL;
    time_t ti;
    struct tm *tm;

    if (s == NULL || !strcasecmp(s, "_now_")) {
        time(&ti);
        tm = localtime(&ti);
        (*m) = tm->tm_mon + 1;
        (*y) = 1900 + tm->tm_year;
        return 1;
    }

    if (!strcasecmp(s, "_last_")) {
        time(&ti);
        tm = localtime(&ti);
        if (tm->tm_mon + 1 == 1) {
            (*m) = 12;
            (*y) = (1900 + tm->tm_year) - 1;
        } else {
            (*m) = (tm->tm_mon + 1) - 1;
            (*y) = 1900 + tm->tm_year;
        }
        return 1;
    }

    o = strdup(s);
    x = strdup(s);

    year = strchr(x, '/');
    if (year == NULL) {
        (*m) = -1;
        (*y) = -1;
        return 0;
    }
    *year = 0;
    ++year;
    v = atoi(year);
    if (strlen(year) != 4 || v < 0 || isdigitstr(year) == 0) {
        (*m) = -1;
        (*y) = -1;
        return 0;
    }

    (*y) = v;

    month = x;
    v = atoi(month);

    if (strlen(month) != 2 || v < 0 || v > 12 || isdigitstr(month) == 0) {
        (*m) = -1;
        (*y) = -1;
        return 0;
    }
    free(o);
    free(x);
    (*m) = v;

    return 1;
}

int isZeit(char *s)
{
    if (ccnt(s, ':') == 2 && strlen(s) == 8)
        return TRUE;

    return FALSE;
}


char *shorter_name(char *name, int max_len)
{
    char *str;
    char *p;

    if (name == NULL)
        return NULL;
    str = malloc(strlen(name) + 1);
    strcpy(str, name);
    if (strlen(str) > max_len)
        str[max_len + 1] = 0;
    p = strchr(str, ' ');
    if (p == NULL)
        return str;
    *p = 0;

    return str;
}


char *mygetlogin()
{
    struct passwd *pw;

    pw = getpwuid(getuid());

    return pw == NULL ? NULL : pw->pw_name;
}


char *strvalstr(char *s)
{
    char *n = malloc(strlen(s) + 1);
    char *r = n;

    *r = 0;
    while (*s && !strchr("0123456789.,:", *s))
        s++;
    while (*s && strchr("0123456789.:,", *s))
        *r++ = *s++;
    *r = 0;
    if (*n == 0 || *n == ':' || *n == ',') {
        free(n);
        return NULL;
    }

    return n;
}

char *strip(char *s, char *chars_to_strip)
{
    char *p, *q;

    if (s == NULL)
        return NULL;

    p = malloc(strlen(s) + 1);
    q = p;
    while (*s) {
        if (strchr(chars_to_strip, *s) != NULL) {
            s++;
            continue;
        }
        *q++ = *s++;
    }
    *q = 0;

    return p;
}


char *stringReplace(char *search, char *replace, char *string)
{
    char *tempString, *searchStart;
    int len = 0;

    // preuefe ob Such-String vorhanden ist
    searchStart = strstr(string, search);
    if (searchStart == NULL) {
        return string;
    }

    // Speicher reservieren
    tempString = (char *) malloc(strlen(string) * sizeof(char));
    if (tempString == NULL) {
        return NULL;
    }

    // temporaere Kopie anlegen
    strcpy(tempString, string);

    // ersten Abschnitt in String setzen
    len = searchStart - string;
    string[len] = '\0';

    // zweiten Abschnitt anhaengen
    strcat(string, replace);

    // dritten Abschnitt anhaengen
    len += strlen(search);
    strcat(string, (char *) tempString + len);

    // Speicher freigeben
    free(tempString);

    return string;
}

char *strenv(char *s)
{
    int len;
    char *envstr, *str;
    char *tmp;
    int j;

    if (!*s)
        return NULL;
    len = strlen(s);
    envstr = malloc(len + 1);
    str = malloc(len + 1);
    j = 0;
    while (*s) {
        if (*s == '$' || *s == '~') {
            s++;
            str[j] = 0;
            j = 0;
            if (*(s - 1) == '$') {
                while (*s && !isspace(*s) && !strchr("/:.(){}-+", *s))
                    envstr[j++] = *s++;
                envstr[j] = '\0';
            } else {
                strcpy(envstr, "HOME");
            }
            tmp = getenv(envstr);

            if (tmp == NULL) {
                str = realloc(str, strlen(str) + strlen(envstr) + 2 + len);
                strcat(str, "$");
                strcat(str, envstr);
            } else {
                str = realloc(str, strlen(str) + strlen(tmp) + 1 + len);
                strcat(str, tmp);
            }
            j = strlen(str);
        } else {
            str[j++] = *s++;
        }
    }
    free(envstr);
    str[j] = 0;

    return str;
}


static int _irandom(int f, int t)
{
    static int _random_seed = 0;

    if (_random_seed == 0) {
        srand(time(NULL));
        _random_seed = 1;
    }
    return rand() % (t + 1 - f) + f;
}

char *make_random_str(int len, char *valid_chars)
{
    char *t = malloc(len + 1);
    char *p = t;
    int slen;

    if (valid_chars == NULL)
        valid_chars = "abcdefghijklmnopqrstuvwxyz1234567890";

    slen = strlen(valid_chars) - 1;
    while (p - t < len)
        *p++ = valid_chars[_irandom(0, slen)];
    *p = 0;

    return t;
}

int sending_email(char *from, char *to, char *subject, char *email_body, char *email_body_type_and_charset, char **attach_files, char **attach_types)
{
    char *random_str;
    char exec_str[1024];
    int i;
    FILE *f;

    if (attach_files != NULL)
        for (i = 0; attach_files[i] != NULL; i++)
            if (access(attach_files[i], R_OK) != 0)
                return -1;

    snprintf(exec_str, sizeof(exec_str), "/usr/sbin/sendmail -f %s %s", from, to);
    f = popen(exec_str, "w");
    if (f == NULL)
        return -1;

    if (email_body_type_and_charset == NULL)
        email_body_type_and_charset = "text/plain; charset=ISO-8859-15";

    random_str = make_random_str(32, "abcdef0123456789");
    fprintf(f, "From: <%s>\nMIME-Version: 1.0\r\n", from);
    if (attach_files == NULL) {
        fprintf(f, "Content-Transfer-Encoding: quoted-printable\r\nContent-Type: %s\r\n", email_body_type_and_charset);
    } else {
        fprintf(f, "Content-Type: multipart/mixed; boundary=%s\r\nContent-Transfer-Encoding: 8bit\r\n", random_str);
    }
    fprintf(f, "To: %s\r\nSubject: %s\r\n\r\n", to, subject);
    if (attach_files != NULL)
        fprintf(f, "--%s\r\nContent-Transfer-Encoding: quoted-printable\r\nContent-Type: %s\r\n\r\n", random_str,
                email_body_type_and_charset);
    fprintf(f, "%s\r\n\r\n", email_body);
    if (attach_files != NULL) {
        for (i = 0; attach_files[i] != NULL; i++) {
            fprintf(f,
                    "--%s\r\nContent-Transfer-Encoding: base64\r\nContent-Type: %s\r\nContent-Disposition: attachment; filename=\"%s\"\r\n",
                    random_str, attach_types[i], basename(attach_files[i]));
            fprintf(f, "Content-Description: \r\n\r\n");
            base64_encode_file2fp(attach_files[i], f);
        }
    }
    free(random_str);

    return (pclose(f) >> 8) & 0xFF;
}


void de_encrypt_data(FILE *input_file, FILE *output_file, char *key)
{
    int key_count = 0; // Used to restart key if strlen(key) < strlen(encrypt)
    int encrypt_byte;

    while ((encrypt_byte = fgetc(input_file)) != EOF) { // Loop through each byte of file until EOF
        // XOR the data and write it to a file
        fputc(encrypt_byte ^ key[key_count], output_file);
        // Increment key_count and start over if necessary
        key_count++;
        if (key_count == strlen(key))
            key_count = 0;
    }
}
