#include <stdint.h>

int test(int a) {
  return a;
}

int main(void) {
  return test(sizeof(intmax_t));
}
