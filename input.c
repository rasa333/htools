#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <termio.h>
#include <string.h>
#include <sys/time.h>

#include "htools.h"

#define ipos     set_input.edit_buf_position
#define ilen     set_input.edit_buf_length
#define istr     set_input.edit_buf
#define ipst     set_input.prompt_buf
#define iCOLS    set_input.default_cols
#define ixscl    set_input.update_line.scroll_xpos
#define old_ipos set_input.update_line.old_ipos
#define old_ilen set_input.update_line.old_ilen
#define irefresh set_input.update_line.line_refresh_flag
#define imdc     set_input.update_line.edit_buf_max_display
#define ixbegin  set_input.update_line.begin_xpos
#define ih_head  set_input.history.ih_head
#define ih_tail  set_input.history.ih_tail
#define ih_now   set_input.history.ih_now
#define ih_cnt   set_input.history.ih_cnt
#define METAstr  set_input.keys.METAstr
#define ikeytab  set_input.keys.ikeytab
#define ikeycnt  set_input.keys.ikeycnt
#define ioffset  set_input.mem.ioffset




static char *c_home(void);
static char *c_lf(void);
static char *c_end(void);
static char *c_rt(void);
static char *c_tab(void);
static char *c_dn(void);
static char *c_up(void);
static char *c_next_word(void);
static char *c_prev_word(void);
static char *c_backspace(void);
static char *c_home(void);
static char *c_delete(void);
static char *c_clreol(void);
static char *c_refresh(void);
static char *c_enter(void);
static char *c_abort(void);
static char *c_overwrite(void);

iKEYS std_ikeytab[] = {
  "\01",       c_home,
  "\02",       c_lf,
  "\05",       c_end,
  "\04",       c_abort,
  "\06",       c_rt,
  "\011",      c_tab, 
  "\012",      c_enter,
  "\013",      c_clreol,
  "\014",      c_refresh,
  "\016",      c_dn,
  "\020",      c_up,
  "\026",      c_overwrite,
  "\030",      c_next_word,
  "\031",      c_prev_word,
  "\033",      c_abort,
  "\033[A",    c_up,
  "\033[B",    c_dn,
  "\033[C",    c_rt,
  "\033[D",    c_lf,
  "\033[8~",   c_end,
  "\033[3~",   c_delete,
  "\033[7~",   c_home,
  "\033[2",    c_overwrite,
  "\177",      c_backspace,
  NULL,   0
};

/* default settings for input */

static struct set_input set_input = {
  is_any_char,	/* pointer to func to check c is allowed */
  0,            /* maximal length of string; 0=no limit */
  50,           /* 0=no history;else number of lines to store */
  0,            /* length of edit buffer yet */
  0,	        /* position of cursor in buffer yet */
  NULL,         /* pointer to edit buffer */	
  NULL, 	/* pointer to prompt string */	
  0,            /* 0=insert-mode; 1=overwrite-mode */            
  80,           /* default value for column */
  /* update-line */
  0,            /* scroll column position (ixscl) */
  0,            /* last ilen value (old_ilen) */
  0,            /* last ipos value (old_ipos) */
  0,            /* 0=refresh part of line;1=whole line */
  80,           /* maximal chars to display (imdc) */
  0,            /* column begin position (ixbegin) */
  /* ikeytab */
  "\033[",
  NULL,
  0,
  /* history */
  NULL,
  NULL,
  NULL,
  0,
  /* ioffset */
  0
  };

void h_add(char *s)
{
  if (ih_head == NULL) {
    ih_tail = malloc(sizeof(iHist));
    ih_head = ih_tail;
    ih_head->next = NULL;
    ih_head->prev = NULL;
    ih_head->buf = strdup(s);
  } else {
    ih_tail->next = malloc(sizeof(iHist));
    ih_tail->next->buf = strdup(s);
    ih_tail->next->prev = ih_tail;
    ih_tail = ih_tail->next;
    ih_tail->next = NULL;
  }
}

void _h_del_node(iHist *x)
{
  if (x == ih_head) {
    ih_head = x->next;
    ih_head->prev = NULL;
  } else {
    if (x == ih_tail) {
      ih_tail = x->prev;
      ih_tail->next = NULL;
    } else {
      x->next->prev = x->prev;
      x->prev->next = x->next;
    }
  }
}

void h_del(iHist *x)
{
  _h_del_node(x);
  free(x);
}

void history(char *s)
{
  if (!ilen)
    return;
  if (ih_cnt < set_input.history_flag) {
    h_add(s);
    ih_cnt++;
  } else {
    h_del(ih_head);
    h_add(s);
  }
  ih_now = 0;
}

int _ikey(void)
{
  int c; 

  read(0, &c, 1);

  return c;
}

char *getseq(void)
{
  char *sequence = malloc(20);
  int i = 0;
  
  do 
    sequence[i++] = _ikey();
  while (kbhit() && i < 19);
  sequence[i] = 0;

  return sequence;
}

int is_any_char(int c)
{
  return 1;
}

int is_ger_char(int c)
{
  return ((c >= 32 && c <= 126) || 
      c == '\201' || c == '\204' || c == '\216' || c == '\224' ||
      c == '\231' || c == '\232' || c == '\341');
}

static void jump_xpos(int xpos)
{
  int tabs, bs;

  tabs = xpos / 8 + (xpos % 8 != 0);
  bs = tabs * 8 - xpos;
  putchar(13);
  while(tabs--)
    putchar(9);
  while(bs--)
    putchar(8);
  fputs(ipst,stdout);
}

static void update_line(int refresh_flag)
{
  int i;

  if (refresh_flag) {
    jump_xpos(ixbegin);
    ipos = ipos > ilen ? ilen : ipos;
    ixscl = ipos == ilen ? ilen < imdc - 1 ? 0 : ilen - imdc + 1 : ipos < imdc
      ? 0 : ipos;
    for (i = 0 ; 
         i < (ilen < imdc - 1 ? ilen : (ipos == ilen ? imdc - 1 : imdc)) ;
         i++)
      putchar(istr[ixscl + i]);
    if (old_ilen > ilen && ilen < ixscl + imdc) {
      for (i = 0 ; i < (old_ilen < imdc ? old_ilen : imdc) - ilen ; i++) 
        putchar(' ');
      while(i--)
        putchar(8);
    } 
    if (ilen != ipos)
      for (i = (ilen < imdc ? ilen : ixscl + imdc) ; i > ipos ; i--)
        putchar(8);
    else {
      putchar(' ');
      putchar(8);
    }
  } else
    if (ipos < ixscl || ipos > ixscl + imdc - 1) {
      ixscl = ipos > ixscl + imdc - 1 ? ipos - (imdc - 1) : ipos;
      jump_xpos(ixbegin);
      for (i = 0 ; i < (ipos != ilen ? imdc : imdc - 1) && 
           istr[ixscl + i] != '\0' ; i++)
        putchar(istr[ixscl + i]);
      if (ipos == ixscl)
        jump_xpos(ixbegin);
      else {
        if (ilen == ipos)
          putchar(' ');
        putchar(8);
      }
    } else {
      if (ipos != old_ipos) {
        if (ipos > old_ipos)
          for (i = old_ipos ; i < ipos ; i++) 
            putchar(istr[i]);
        if (ipos < old_ipos)
          for (i = old_ipos ; i > ipos ; i--)
            putchar(8);
      }
      if (ilen != old_ilen) {
        for (i = ipos ; i < (ilen < imdc + ixscl ? ilen : ixscl + imdc) ; i++)
          putchar(istr[i]);

        if (old_ilen > ilen) {
          for (i = 0 ; i < (old_ilen < ixscl + imdc ? old_ilen - ilen :
                            ipos - ixscl + imdc); i++) 
            putchar(' ');
          while(i--)
            putchar(8);
        } 
        if (ipos != ilen)
          for (i = (ilen < imdc || ixscl + imdc > ilen 
                    ? ilen 
                    : ixscl + imdc) ; i > ipos ; i--)
            putchar(8);
      }
    }
  old_ilen = ilen;
  old_ipos = ipos;
  fflush(stdout);
}

char *c_rt(void)
{
  ipos++;
  if (ipos > ilen)
    ipos = ilen;
  return NULL;
}

char *c_lf(void)
{
  ipos--;
  if (ipos < 0)
    ipos = 0;
  return NULL;
}

char *c_next_word(void)
{
  while(!isspace(istr[ipos]) && ipos < ilen)
    ipos++;
  while(isspace(istr[ipos]) && ipos < ilen)
    ipos++;
  return NULL;
}

char *c_prev_word(void)
{
  while(!isspace(istr[ipos]) && ipos > 0)
    ipos--;
  while(isspace(istr[ipos]) && ipos > 0)
    ipos--;
  while(!isspace(istr[ipos]) && ipos > 0)
    ipos--;
  while(isspace(istr[ipos]) && ipos < ilen)
    ipos++;
  return NULL;
}

char *c_home(void)
{
  ipos = 0;
  return NULL;
}

char *c_end()
{
  ipos = ilen;
  return NULL;
}

char *c_clreol(void)
{
  ilen = ipos;
  return NULL;
}


char *c_refresh()
{
  update_line(1);

  return NULL;
}

char *c_backspace(void)
{
  int i;

  if (ipos) {
    ipos--;
    ilen--;
    for( i = ipos ; i < ilen ; i++ )
      istr[i] = istr[i+1];
  }
  return NULL;
}

char *c_tab()
{
  DI *d;
  char *name, *exp, *path, *xpath, *basen, *dirn;
  int i, diff;
  int found = 0;
  int *idx, cnt_equ, flag = 1;
  int name_len;

  for (i = ipos ; i != 0 && !isspace(istr[i]) ; i--)
    ;
  i += (i != 0); 
  path = malloc(ipos - i + 1);
  strncpy(path,&istr[i],ipos - i);
  path[ipos - i] = 0;

  xpath = strenv(path);
  dirn = dirname(xpath);
  basen = basename(xpath);
  d = getdir(dirn);
  if (d == NULL) {
    free(xpath);
    free(path);
    free(dirn);
    return NULL;
  }
  name = malloc(strlen(basen) + 1);
  strcpy(name, basen);
  name_len = strlen(name);
  free(path);
  free(xpath);
  free(dirn);

  if (d->no_of_files == 2) {
    freedir(d);
    free(name);
    return NULL;
  }
  idx = malloc(sizeof(int) * d->no_of_files);

  if (d->no_of_files == 3 && !name_len) {
    exp = malloc(strlen(d->fl[2].name) + 2);
    strcpy(exp, d->fl[2].name);
    found = 1; 
    idx[0] = 2;
  } else {
    for (i = 0 ; i < d->no_of_files ; i++) {
      if (!strncmp(name,d->fl[i].name, name_len)) {
        if (!found) {
          diff = strlen(d->fl[i].name) - name_len;
          exp = malloc(diff + 2);
          strncpy(exp, &d->fl[i].name[name_len], diff);
          exp[diff] = 0;
        }
        idx[found++] = i;
      }
    }
  }
  if (found > 1) {
    cnt_equ = name_len;
    while(flag) {
      for (i = 1 ; i < found && flag ; i++) 
        flag = d->fl[idx[i]].name[cnt_equ] == d->fl[idx[0]].name[cnt_equ] &&
          d->fl[idx[i]].name[cnt_equ] && d->fl[idx[0]].name[cnt_equ];
      cnt_equ += flag; 
    }
    diff = cnt_equ - name_len;
    if (diff) {
      exp = realloc(exp, diff + 1);
      strncpy(exp,&d->fl[idx[0]].name[name_len], diff);
      exp[diff] = 0;
    } else {
      free(exp);
      found = 0;
    }
  }
  if (found == 1 && (d->fl[idx[0]].stat.st_mode & S_IFMT) == S_IFDIR) 
    strcat(exp, "/");
  
  free(name);
  free(idx);
  freedir(d);
  
  if (!found)
    return NULL;

  return exp;
}

char *c_delete(void)
{
  int i;

  if ( ipos != ilen ) {
    ilen--;
    for( i = ipos ; i < ilen ; i++ )
      istr[i] = istr[i+1];
  }
  return NULL;
}

char *c_up(void)
{
  if (!ih_head || ih_now == ih_head)
    return NULL;
  ih_now = ih_now ? ih_now->prev : ih_tail;
  ipos = ilen = strlen(ih_now->buf);
  ioffset = ilen + 64;
  istr = realloc(istr, ioffset);
  strcpy(istr, ih_now->buf);
  irefresh = 1;
  return NULL;
}


char *c_dn(void)
{
  if (!ih_head || ih_now == 0)
    return NULL;
  ih_now = ih_now->next;
  if (ih_now == NULL) {
    ioffset = 64;
    istr = realloc(istr, ioffset);
    *istr = ilen = ipos = 0;
  } else {
    ipos = ilen = strlen(ih_now->buf);
    ioffset = ilen + 64;
    istr = realloc(istr, ioffset);
    strcpy(istr, ih_now->buf);
  }
  irefresh = 1;
  return NULL;
}

char *c_overwrite(void)
{
  set_input.overwrite_flag =! set_input.overwrite_flag;
  return NULL;
}

char *c_enter(void)
{
  istr[ilen] = 0;
  if (set_input.history_flag)
    history(istr);
  putchar('\n');
  return NULL;
}

char *c_abort(void)
{
  return NULL;
}

static int ikey_cmp(iKEYS *a, iKEYS *b)
{
  return strcmp(a->scan, b->scan);
}




static void ikey_sort(void)
{
  if (!ikeytab)
    ikey_init(std_ikeytab);

  qsort(ikeytab, ikeycnt, sizeof(iKEYS), (void *)ikey_cmp);
}


void ikey_init(iKEYS *tab)
{
  int i;

  if (ikeytab != 0) {
    while(ikeycnt--) {
      free(ikeytab[ikeycnt].scan);
      ikeytab[ikeycnt].input_func = 0;
    }
    free(ikeytab);
  }
  ikeycnt = 0;
  while(tab[ikeycnt].scan)
    ikeycnt++;
  ikeytab = (iKEYS *) malloc(ikeycnt * sizeof(iKEYS));
  for (i = 0 ; i < ikeycnt ; i++) {
    ikeytab[i].scan = (char *) malloc(strlen(tab[i].scan) + 1);
    strcpy(ikeytab[i].scan,tab[i].scan);
    ikeytab[i].input_func = tab[i].input_func;
  }
  ikey_sort();
}

int ikey_add(char *scan, char *(*input_func)(void))
{
  int i;

  if (!ikeytab)
    ikey_init(std_ikeytab);
  for (i = 0 ; i < ikeycnt && strcmp(ikeytab[i].scan, scan) ; i++)
    ;
  if (i != ikeycnt)
    return -1;
  ikeytab = realloc(ikeytab, (ikeycnt+2) * sizeof(iKEYS));
  ikeytab[ikeycnt].scan = malloc(strlen(scan) + 1);
  strcpy(ikeytab[ikeycnt].scan, scan);
  ikeytab[ikeycnt].input_func = input_func;
  ikeycnt++;
  ikey_sort();

  return 0;
}

int ikey_del(char *scan)
{
  int i;

  if (!ikeytab)
    ikey_init(std_ikeytab);
  for (i = 0 ; i < ikeycnt && strcmp(ikeytab[i].scan, scan) ; i++)
    ;
  if (i == ikeycnt)
    return -1;
  for ( ; i < ikeycnt ; i++ ) 
    ikeytab[i] = ikeytab[i+1];
  ikeycnt--;
  free(ikeytab[ikeycnt].scan);
  ikeytab = realloc(ikeytab, (ikeycnt+1) * sizeof(iKEYS));

  return 0;
}
 
int ikey_replace(char *scan1, char *scan2)
{
  int i;

  if (!ikeytab)
    ikey_init(std_ikeytab);
  for (i = 0 ; i < ikeycnt && strcmp(ikeytab[i].scan, scan1) ; i++)
    ;
  if (i == ikeycnt)
    return -1;
  ikeytab[i].scan = realloc(ikeytab[i].scan, strlen(scan2) + 1);
  strcpy(ikeytab[i].scan, scan2);
  ikey_sort();

  return 0;
}

char *input(char *prompt)
{
  int i, y;
  iKEYS k, *f;
  char *tmp, *insert;

  if (!ikeytab)
    ikey_init(std_ikeytab);
  scrsize(&iCOLS, &y);
  ipst = prompt;
  i = strlen(ipst);
  if (imdc + i > iCOLS - 1) 
    imdc = iCOLS - i - 1;
  if (imdc <= 1)
    return NULL;
  setty(3);
  if (istr == NULL) {
    ioffset = 64;
    istr = malloc(ioffset);
    ilen = ipos = 0;
  } else {
    ilen = strlen(istr);
    ioffset = ilen + 64;
    istr = realloc(istr, ioffset);
  }
  irefresh = 1;
  for(;;) {
    update_line(irefresh);
    irefresh = 0;
    k.scan = getseq();
    f = bsearch(&k, (iKEYS *)ikeytab, ikeycnt, sizeof(iKEYS), (void *)ikey_cmp);
    insert = f ? f->input_func() : k.scan;
    if (insert) {
      i = strlen(insert);
      if (set_input.overwrite_flag && 
	  (!set_input.edit_buf_maximal_length ||
	   set_input.edit_buf_maximal_length >= ipos + i)) {
	int j = i;
	ilen = ipos + i > ilen ? ipos + i : ilen;
	if (ilen + 1 > ioffset) {
	  ioffset = ilen + 64;
	  istr = realloc(istr, ioffset);
	}
	while(j--)
	  istr[ipos + j] = insert[j];
	ipos += i;
      }
      if (!set_input.overwrite_flag && 
	  (!set_input.edit_buf_maximal_length ||
	   set_input.edit_buf_maximal_length >= ilen + i)) {
	tmp = malloc(ilen - ipos + 1);
	strncpy(tmp, &istr[ipos], ilen - ipos);
	if (ilen + i + 1 > ioffset) {
	  ioffset = ilen + i + 64;
	  istr = realloc(istr, ioffset);
	}
	istr[ipos] = 0;
	strcat(istr, insert);
	strcat(istr, tmp);
	free(tmp);
	ilen += i;
	ipos += i;
      }
      free(insert);
    }
    istr[ilen] = 0;
    
    if (f) {
      if (f->input_func == c_enter) {
	resetty();
	tmp = strdup(istr);
	free(istr);
	istr = NULL;
	return tmp;
      }
      
      if (f->input_func == c_abort) {
	resetty();
	free(istr);
	istr = NULL;
	return NULL;
      }
    }
  }
}

char *inputs(char *p, char *s)
{
  istr = strdup(s);
  ipos = strlen(istr);
  return input(p);
}
