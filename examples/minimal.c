#include <stdint.h>

int test_ls(int a, int b) {
  return a << b;
}

int test_ls_redundant(int a) {
  return a << 0;
}

int test_ls_halfword(int a) {
  return a << 1;
}

// int test_ls_word(int a) {
//   return a << 256;
// }

int main(void) {
  test_ls(1, 2);
  test_ls_redundant(1);

  return test_ls_halfword(1);
}
