#ifndef STRING_SET_H
#define STRING_SET_H

typedef struct StringSetEntry StringSetEntry;
typedef struct StringSet StringSet;

struct StringSet {
  StringSetEntry *first;
};

int string_set_init(StringSet *set);
int string_set_destroy(StringSet *set);
int string_set_add(StringSet *set, char *string, void *element);
int string_set_remove(StringSet *set, char *string, void **element);
int string_set_get(StringSet *set, char *string, void **element);
int string_set_size(StringSet *set);

#endif
