#include <stdint.h>

int test_and(int a, int b) {
  return a & b;
}

int test_and_redundant(int a) {
  return a & 0;
}

int test_and_halfword(int a) {
  return a & 1;
}

int test_and_word(int a) {
  return a & 256;
}

int main(void) {
  test_and(1, 2);
  test_and_redundant(1);
  test_and_halfword(1);

  return test_and_word(1);
}
