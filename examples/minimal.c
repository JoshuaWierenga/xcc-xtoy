#include <stdint.h>

int test_or(int a, int b) {
  return a | b;
}

int test_or_redundant(int a) {
  return a | 0;
}

int test_or_halfword(int a) {
  return a | 1;
}

int test_or_word(int a) {
  return a | 256;
}

int main(void) {
  test_or(1, 2);
  test_or_redundant(1);
  test_or_halfword(1);

  return test_or_word(1);
}
