#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "htools.h"

ARRAY_T *array_init(int init_size, int add_size)
{
  ARRAY_T *a = malloc(sizeof(ARRAY_T));

  a->size = init_size;
  a->add_size = add_size;
  a->count = 0;
  a->list = malloc(sizeof(char *) * a->size + 1);
  a->list[0] = NULL;

  return a;
}

ARRAY_T *array_add(ARRAY_T *a, char *s)
{
  if (s == NULL)
    return a;
  if (a == NULL)
    a = array_init(100, 50);
  a->count++;
  if (a->count > a->size) {
    a->size += a->add_size;
    a->list = realloc(a->list, sizeof(char *) * a->size + 1);
  }
  a->list[a->count-1] = strdup(s);
  a->list[a->count] = NULL;

  return a;
}

void array_free(ARRAY_T *a)
{
  int i;
  
  if (a == NULL)
    return;
  
  for (i = 0 ; i < a->count ; i++)
    free(a->list[i]);
  free(a->list);
  free(a);
}


