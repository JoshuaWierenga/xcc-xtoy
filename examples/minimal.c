#include <stdint.h>

int add(int a, int b) {
  return a + b;
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
  add(1, 2);
  add_redundant(1);
  add_halfword(1);

  return add_word(1);
}
