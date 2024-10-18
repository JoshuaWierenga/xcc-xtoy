#include <stdint.h>

int test_xor(int a, int b) {
  return a ^ b;
}

int test_xor_redundant(int a) {
  return a ^ 0;
}

int test_xor_halfword(int a) {
  return a ^ 1;
}

int test_xor_word(int a) {
  return a ^ 256;
}

int main(void) {
  test_xor(1, 2);
  test_xor_redundant(1);
  test_xor_halfword(1);

  return test_xor_word(1);
}
