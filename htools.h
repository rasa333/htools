#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TRUE   1
#define FALSE  0
#define INVALID -1

#define DEBUG_MSG_FILE   "./htools.debug"
#define _DEBUG_MSG_ 1

#if _DEBUG_MSG_ == 0
#define DEBUG_MSG(x)        fprintf(stderr, "  %s\n", x)
#define DEBUG_MSG_INT(x, y) fprintf(stderr, "  %s: %d\n", x, y)
#define DEBUG_MSG_FLOAT(x, y) fprintf(stderr, "  %s: %f\n", x, y)
#define DEBUG_MSG_STR(x, y) fprintf(stderr, "  %s: \"%s\"\n", x, y)
#define DEBUG_MSG_ARRAY(x, y) {int I; fprintf(stderr, "  %s:\n", x); \
			       if (y == NULL) \
                                 fprintf(stderr, "    NULL\n"); \
			       else \
				 for (I = 0 ; y[I] != NULL ; I++) \
				   fprintf(stderr, "    %d: \"%s\"\n", I, y[I]); \
                               }
#endif

#if _DEBUG_MSG_ == 1
#define DEBUG_MSG(x)          {FILE *F;F = fopen(DEBUG_MSG_FILE, "a");fprintf(F, "[%s] %s\n", stamp(), x);fclose(F);}
#define DEBUG_MSG_INT(x, y)   {FILE *F;F = fopen(DEBUG_MSG_FILE, "a");fprintf(F, "[%s] %s: %d\n", stamp(), x, y);fclose(F);}
#define DEBUG_MSG_STR(x, y)   {FILE *F;F = fopen(DEBUG_MSG_FILE, "a");fprintf(F, "[%s] %s: \"%s\"\n", stamp(), x, y);fclose(F);}
#define DEBUG_MSG_FLOAT(x, y)   {FILE *F;F = fopen(DEBUG_MSG_FILE, "a");fprintf(F, "[%s] %s: \"%f\"\n", stamp(), x, y);fclose(F);}
#define DEBUG_MSG_ARRAY(x, y) {FILE *F;int I;F = fopen(DEBUG_MSG_FILE, "a");fprintf(F, "[%s] %s:\n", stamp(), x); \
			       if (y == NULL) \
                                 fprintf(F, "    NULL\n"); \
			       else \
				 for (I = 0 ; y[I] != NULL ; I++) \
				   fprintf(F, "    %d: \"%s\"\n", I, y[I]); \
                               fclose(F);}
#endif

#ifndef _DEBUG_MSG_
#define DEBUG_MSG(x)          {}
#define DEBUG_MSG_INT(x, y)   {}
#define DEBUG_MSG_STR(x, y)   {}
#define DEBUG_MSG_ARRAY(x, y) {}
#endif

#define max(a, b)   (a > b ? a : b)
#define min(a, b)   (a < b ? a : b)


typedef struct {
  char *scan;
  char *(*input_func)(void);
} iKEYS;


/* input-history linked-list */
struct _ihistory {
  char *buf;
  struct _ihistory *next;
  struct _ihistory *prev;
};

typedef struct _ihistory iHist;


                   /* input global settings and variables */

struct set_input {
  int (*compar_char)(int c);   /* pointer to func to check c is allowed */
  int edit_buf_maximal_length; /* max length of input buffer; 0=unlimited */
  int history_flag;            /* 0=no history;else number of lines to store */
  int edit_buf_length;         /* length of edit buffer yet (ilen) */
  int edit_buf_position;       /* position of cursor in buffer yet (ipos) */
  char *edit_buf;              /* pointer to edit buffer (istr) */
  char *prompt_buf;            /* pointer to prompt string (ipst) */
  int overwrite_flag;          /* 0=insert-mode; 1=overwrite-mode */
  int default_cols;            /* default value for column on screen */
  struct {
    int scroll_xpos;           /* scroll column position (ixscl) */
    int old_ilen;              /* last ilen value (old_ilen) */
    int old_ipos;              /* last ipos value (old_ipos) */
    int line_refresh_flag;     /* 0=refr. part of line;1=whole li (irefresh) */
    int edit_buf_max_display;  /* maximal chars to display (imdc) */
    int begin_xpos;            /* column begin position (ixbegin) */
  } update_line;
  struct {
    char *METAstr;             /* meta-string table */
    iKEYS *ikeytab;            /* key-string table */
    int ikeycnt;               /* number of elements in key-string table */
  } keys;
  struct {
    iHist *ih_head;            /* history double-linked-list first element */
    iHist *ih_tail;            /* history double-linked-list last element */
    iHist *ih_now;             /* actually pointer element */
    int ih_cnt;                /* number of elements in history */
  } history;
  struct {
    int ioffset;               /* memory offset for edit_buf */
  } mem;
};



typedef struct {
  char       *buffer;
  int         length;
  int         memory;
} buf_t;

typedef struct {
  int add_size;
  int size;
  int count;
  char **list;
} ARRAY_T;

typedef struct
{
	char *name;
	struct stat stat;
} FL;

typedef struct 
{
	FL *fl;
	int no_of_files;
} DI;

// dir.c

extern DI *getdir(char *cwd);
extern void freedir(DI *d);

// funcs.c

extern inline char *dstrcpy(char *buf, char *new);
extern inline char *dstrncpy(char *buf, char *new, int n);
extern inline char *dstrcat(char *buf, char *add);
extern inline char *dstrncat(char *buf, char *add, int n);
extern char *trim(char *s);
extern void replace_char(char *s, int old, int new);
extern char *replace_str(char *s, char *search, char *replace, int count);
extern char *get_quoted_str(char *s, int q1, int q2);
extern void delete_char(char *s, int c);
extern double fgrade(double, double);
extern int is_space(char c);
extern int is_pipe(char c);
extern int is_space_logfile(char c);
extern int is_comma(char c);
extern int is_semi(char c);
extern int is_newline(char c);
extern int is_minus(char c);
extern int is_32(char c);
extern int is_plus(char c);
extern int is_slash(char c);
extern double unit2quantity(int, double);
extern char **split_quoted_words(char *str, char **arg, int *cnt, int (*trunc)(char c));
extern char **split_quoted_csv(char *str, char **arg, int *cnt, int (*trunc)(char c));
extern char **list_free(char **a);
extern long list_cnt(char **arr);
extern int match(char *s, char *p);
extern int matchcase(char *s, char *p);
extern char **add_to_list(char **list, char *str);
extern int find_in_list(char **list, char *search);
extern int find_in_list_i(char **list, char *search);
extern char **del_from_list(char **list, char *str);
extern char **del_index_from_list(char **list, int i);
extern char *unit2longstr(int unit);
extern int unit_autodetect_bits(double data);
extern int unit_autodetect_avg(double *data, int cnt);
extern char *unit2str(int unit);
extern char *ip2str(unsigned long ip);
extern char *list2str(char **arr, char *delim);
extern int int_list_cmp(int *a, int *b, int cnt);
extern char **list_dup(char **arr);
extern char **list_dupn(char **arr, int n);
extern char *mask2str(unsigned long mask);
extern unsigned long str2ip(char *ip);
extern void convert2ipmask(char *ip, unsigned long *net, unsigned long *mask);
extern char *double2valuta(double v);
extern char *double2valuta4(double v);
extern int month_days(int month, int year);
extern int day_first(int day, int month, int year, int dt, int mt, int yt);
extern int day_next(int day, int month, int year);
extern int day_month();
extern time_t quartal_next(time_t t);
extern char *x_itoa(int n);
extern void daemonize(int verbose_flag);
extern char *path2file(char *s);
extern char *volume_str(double bytes);
extern char *packets_str(double packets);
extern inline char *wup(char *s);
extern inline char *wlower(char *s);
extern inline int testchars(char *s, char *char_list);
extern int month_first(int month_from, int year_from, int month_till, int year_till, int *m, int *y);
extern int month_next(int *m, int *y);
extern char *long2clock(long t);
extern char *format_bytes(double v, int flag);
extern inline unsigned long get_class_mask(unsigned long ip);
extern char *ascii2str(int ch);
extern char *control2str(char *cstr, int cnt);
extern char *mygetlogin();
extern inline int ccnt(char *s, int c);
extern inline int ccntn(char *s, int c, int len);
extern inline int isdigitstr(char *s);
extern inline int isfloatstr(char *s);
extern inline char *strnstr(char *string, char *substr, int n);
extern inline char *format_string(const char *fmt, ...);
extern inline char *strcasestr(char *haystack, char *needle);
extern inline char *substrdup(const char *begin, const char *end);
extern buf_t *buf_init();
extern buf_t *buf_add(buf_t *b, char *s);
extern buf_t *buf_addn(buf_t *b, char *s, int size);
extern buf_t *buf_exp(buf_t *b, size_t size_to_expand);
extern buf_t *buf_add_sprintf(buf_t *b, char *fmt, ...);
extern void buf_free(buf_t *b);
extern char *shorter_name(char *name, int max_len);
extern char *strvalstr(char *s);
extern char *strip(char *b, char *chars_to_strip);
extern char *stringReplace(char *search, char *replace, char *string);
extern char *make_random_str(int len, char *valid_chars);
extern int sending_email(char *from, char *to, char *subject, char *email_body, char *email_body_type_and_charset,
			 char **attach_files, char **attach_types);
extern void de_encrypt_data(FILE *input_file, FILE *output_file, char *key);

// array.c

extern ARRAY_T *array_init(int init_size, int add_size);
extern ARRAY_T *array_add(ARRAY_T *a, char *s);
extern void array_free(ARRAY_T *a);

// tcap.c

extern int init_tcap();
extern int scrsize(int *x, int *y);
extern void clrscr();
extern void gotoxy(int x, int y);
extern void clrtobot();
extern void clrtoeol();
extern void insertln();
extern void insert_n_lines(int y, int n);
extern void deleteln();
extern void delete_n_lines(int y, int n);
extern void standout();
extern void standend();
extern void setty();
extern void resetty();
extern int kbhit();
extern int readkey();
extern char *strenv(char *s);

// file.c

extern inline off_t getfilesize(char *name);
extern inline off_t exists(char *file);
extern int mkdirs(char *dname, int mode);
extern int fget_logline(char *buf, size_t size, FILE *f, int follow_flag);
extern int fget_logline_return(char *buf, size_t size, FILE *f, int follow_flag);
extern inline char *basename(char *name);
extern char *dirname(char *path);

// input.c

extern void ikey_init(iKEYS *);
extern int ikey_add(char *scan, char *(*input_func)(void));
extern int ikey_del(char *scan);
extern int ikey_replace(char *scan1, char *scan2);
extern char *input(char *prompt);
extern char *inputs(char *p, char *s);
extern int is_any_char(int);
extern int is_ger_char(int);

// postgres.c

extern int sql_postgres_connect(char *database, char *host, char *port, char *username, char *password);
extern int sql_postgres_is_connected();
extern void sql_postgres_disconnect();
extern void sql_postgres_begin();
extern void sql_postgres_end();
extern char **sql_postgres_query_first(char *fmt, ...);
extern char **sql_postgres_query_next();
extern char **sql_postgres_execute(char *fmt, ...);
extern char *sql_where_in(char **list);
extern char *sql_postgres_escape_string(char *s);

// base64.c

extern inline int base64_decode(char *input);
extern inline char *base64_encode(char *binStr, int len);
extern inline char *base64_encode_file(char *file);
extern inline void base64_encode_file2fp(char *file, FILE *p);

// broker.c

extern int broker(char *bindtohost, int port, void (*func)(int fd), int blocking_flag, int max_childs);

// signal.c

extern char *signal_text(int sig);
extern void signal_hdl__exit(int sig);
extern void signal_hdl__pipe(int sig);
extern pid_t *signal_childs_died(pid_t pid);
extern int child_cnt(int state);
extern void signal_hdl__child(int sig);
extern void signal_init();
extern void signal_block_all();
extern void signal_unblock_all();
extern void signal_block_reset();
extern void signal_block(int sig);
extern void signal_unblock(int sig);
extern void signal_action(int sig, void (*handler)(int));
extern void signal_action_flags(int sig, void (*handler)(int), int flags);

// lock.c

extern int lock_make(char *file, int mode);
extern void lock_unmake(int lockfd);
extern int lock_test(char *file);
extern void lock_verify();
extern FILE *lock_fopen(char *file, char *mode);
extern FILE *lock_fopen_exit(char *file, char *mode);
extern int lock_fclose(FILE *f, char *file);
/*extern gzFile lock_gzopen(char *file, char *mode);
extern gzFile lock_gzopen_exit(char *file, char *mode);
extern int lock_gzclose(gzFile f, char *file);
*/
