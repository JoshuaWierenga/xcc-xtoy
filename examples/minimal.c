#include <stdint.h>

int test(void) {
  return -1;
}

int add_redundant(int a) {
  return a + 0;
}

int add_halfword(int a) {
  return a + 1;
}

int add_word(int a) {
  return a + 256;
}

int main(void) {
  add_redundant(1);
  add_halfword(1);
  add_word(1);

  return test();
}
