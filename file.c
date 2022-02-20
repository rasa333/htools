#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "htools.h"



// return file-size or -1 in case of error

inline off_t getfilesize(char *name)
{
  struct stat st;
	    
  return stat(name, &st) ? (off_t)-1 : (off_t)st.st_size;
}


// return TRUE if file exists
// return FALSE if not

inline off_t exists(char *file)
{
  struct stat st;

  return stat(file, &st) ? FALSE : TRUE;
}




// creates directories

int mkdirs(char *dname, int mode)
{
  char buf[1024];
  char **args = NULL;
  int i = 0;
  int arg_cnt = 0;

  if (*dname == '/')
    strcpy(buf, "/");
  else
    buf[0] = 0;
  args = split_quoted_words(dname, args, &arg_cnt, is_slash);

  for (i = 0 ; i < arg_cnt ; i++) {
    if (i != 0)
      strcat(buf, "/");
    strcat(buf, args[i]);
    if (!exists(buf)) { 
      if (mkdir(buf, mode) == -1 && errno != EEXIST) {
        return 0;
      }
    }
  }
  list_free(args);

  return 1; 
}


int fget_logline(char *buf, size_t size, FILE *f, int follow_flag)
{
  int chc, ch;

  buf[0] = 0;
  chc = 0;
  do {
    ch = getc(f);
    if (ch == EOF) {
      if (!follow_flag)
	return EOF;
      sleep(1);
      clearerr(f);
    } else
      if (isascii(ch))
	buf[chc++] = ch;
  } while(chc < (size-1) && ch != '\n');
  buf[chc] = 0;

  return chc;
}


int fget_logline_return(char *buf, size_t size, FILE *f, int follow_flag)
{
  int chc, ch;

  buf[0] = 0;
  chc = 0;
  do {
    ch = getc(f);
    if (ch == EOF) {
      if (!follow_flag)
	return EOF;
      sleep(1);
      clearerr(f);
      if (chc == 0)
	return 0;
    } else
      if (isascii(ch))
	buf[chc++] = ch;
  } while(chc < (size-1) && ch != '\n');
  buf[chc] = 0;

  return chc;
}

inline char *basename(char *name)
{
  char *base;

  base = strrchr(name, '/');
  return base ? base + 1 : name;
}

char *dirname(char *path)
{
  char *newpath;
  char *slash;

  slash = strrchr(path, '/');
  if (slash == NULL)
    return strdup(".");

  newpath = malloc(strlen(path) + 1);
  if (newpath == NULL)
    return NULL;
  strcpy(newpath, path);
  slash += newpath - path;
  /* Remove any trailing slashes and final element. */
  while (slash > newpath && *slash == '/')
    --slash;
  slash[1] = 0;

  return newpath;
}
