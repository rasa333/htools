#include <stdlib.h>
#include <string.h>

#include "hash.h"

static struct hash_t **hash_table = NULL;


static unsigned hash_make(char *str)
{
  unsigned hashval;

  for (hashval = 0 ; *str != 0 ; str++) 
    hashval = *str + 31 * hashval;
  
  return hashval % HASHSIZE;
}


static void hash_table_init()
{
  int i;

  hash_table = malloc(sizeof(struct hash_t) * HASHSIZE);

  for (i = 0 ; i < HASHSIZE ; i++)
    hash_table[i] = NULL;
}


struct hash_t *hash_lookup(char *str)
{
  struct hash_t *h;

  if (hash_table == NULL)
    hash_table_init();

  for (h = hash_table[hash_make(str)] ; h != NULL ; h = h->next)
    if (!strcmp(str, h->str))
      return h;

  return NULL;
}


void hash_install(char *str)
{
  struct hash_t *h;
  unsigned int hashval;

  if (hash_table == NULL)
    hash_table_init();

  h                   = malloc(sizeof(*h));
  h->str              = strdup(str);
  hashval             = hash_make(str);
  h->next             = hash_table[hashval];
  hash_table[hashval] = h;
}

void hash_free()
{
  unsigned long i;
  struct hash_t *data;

  if (hash_table == NULL)
    return;

  for (i = 0 ; i < HASHSIZE ; i++) {
    if (hash_table[i] == NULL)
      continue;
    for (data = hash_table[i] ; data != NULL ; ) { 
      hash_table[i] = data->next;
      free(data->str);
      free(data);
      data = hash_table[i];
    }
  }
  free(hash_table);
}
