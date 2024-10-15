#include <stdint.h>

int test(void) {
  return sizeof(intmax_t);
}

int main(void) {
  return test();
}
