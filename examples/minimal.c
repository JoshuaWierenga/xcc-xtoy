#include <stdint.h>

int test_not(int a) {
  return ~a;
}

int test_not_redundant(void) {
  return ~0xFFFF;
}

int test_not_halfword(void) {
  return ~1;
}

int test_not_word(void) {
  return ~256;
}

int main(void) {
  test_not(1);
  test_not_redundant();
  test_not_halfword();

  return test_not_word();
}
