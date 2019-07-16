#include "string_set.h"

#include <string.h>
#include <stdlib.h>

struct StringSetEntry {
  char *string;
  void *element;
  StringSetEntry *next;
};

int string_set_init(StringSet *set) {
  set->first = 0;
  return 0;
}

int string_set_destroy(StringSet *set) {
  StringSetEntry *current = set->first;
  while(current) {
    StringSetEntry *next = current->next;
    free(current);
    current = next;
  }
  return 0;
}

int string_set_add(StringSet *set, char *string, void *element) {
  if(!set->first) {
    StringSetEntry *new = malloc(sizeof(StringSetEntry));
    new->string = string;
    new->element = element;
    set->first = new;
    return 0;
  }

  StringSetEntry *current = set->first;
  while(current) {
    if(!strcmp(string, current->string)) {
      // already in the set, do nothing
      return 1;
    }
    if(!current->next) {
      StringSetEntry *new = malloc(sizeof(StringSetEntry));
      new->string = string;
      new->element = element;
      current->next = new;
      return 0;
    }
    current = current->next;
  }

  return 1;
}

int string_set_remove(StringSet *set, char *string, void **element) {
  if(!set->first) {
    // empty
    return 1;
  }

  if(!strcmp(string, set->first->string)) {
    StringSetEntry *first = set->first;
    set->first = set->first->next;
    *element = first->element;
    free(first);
    return 0;
  }

  StringSetEntry *last = set->first;
  while(last->next) {
    StringSetEntry *current = last->next;
    if(!strcmp(string, current->string)) {
      // found element, remove
      last->next = current->next;
      *element = current->element;
      free(current);
      return 0;
    }
    last = current;
  }

  // element not found
  return 1;
}

int string_set_get(StringSet *set, char *string, void **element) {
  StringSetEntry *current = set->first;
  while(current) {
    if(!strcmp(string, current->string)) {
      // found
      *element = current->element;
      return 0;
    }
    current = current->next;
  }
  // not found
  return 1;
}

int string_set_size(StringSet *set) {
  int i = 0;
  StringSetEntry *current = set->first;
  while(current) {
    i++;
    current = current->next;
  }
  return i;
}
