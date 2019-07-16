#include <stdio.h>
#include <assert.h>

#include "string_set.h"


void test_0() {
  StringSet set;
  assert(string_set_init(&set) == 0);
  assert(string_set_size(&set) == 0);
}

void test_1() {
  StringSet set;
  assert(string_set_init(&set) == 0);

  const char *astr = "ich bin ein a";
  assert(string_set_add(&set, "a", (void *)astr) == 0);

  assert(string_set_size(&set) == 1);

  char *astr_ret;
  assert(string_set_get(&set, "a", (void **)(&astr_ret)) == 0);
  assert(astr_ret == astr);
  astr_ret = 0;

  assert(string_set_remove(&set, "a", (void **)(&astr_ret)) == 0);
  assert(astr_ret == astr);

  assert(string_set_size(&set) == 0);

  assert(string_set_destroy(&set) == 0);
}

void test_2() {
  StringSet set;
  assert(string_set_init(&set) == 0);

  const char *astr = "ich bin ein a";
  assert(string_set_add(&set, "a", (void *)astr) == 0);

  assert(string_set_size(&set) == 1);

  const char *bstr = "ich bin ein b";
  assert(string_set_add(&set, "b", (void *)bstr) == 0);

  assert(string_set_size(&set) == 2);

  char *astr_ret;
  assert(string_set_get(&set, "a", (void **)(&astr_ret)) == 0);
  assert(astr_ret == astr);
  astr_ret = 0;

  char *bstr_ret;
  assert(string_set_get(&set, "b", (void **)(&bstr_ret)) == 0);
  assert(bstr_ret == bstr);
  bstr_ret = 0;

  assert(string_set_size(&set) == 2);

  assert(string_set_remove(&set, "a", (void **)(&astr_ret)) == 0);
  assert(astr_ret == astr);
  astr_ret = 0;

  assert(string_set_size(&set) == 1);

  assert(string_set_destroy(&set) == 0);
}

int main() {
  test_0();
  test_1();
  test_2();
  printf("tests OK\n");
}
