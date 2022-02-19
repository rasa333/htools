#define HASHSIZE 100001

struct hash_t {
  struct hash_t  *next;
  char           *str;
};

extern struct hash_t *hash_lookup(char *str);
extern void hash_install(char *str);
extern void hash_free();
