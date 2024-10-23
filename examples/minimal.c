#include <stdint.h>

void output_int(int val) {
  __asm("str r1, FF");
}

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
  output_int(test_not(1));
  output_int(test_not_redundant());
  output_int(test_not_halfword());

  return test_not_word();
}
