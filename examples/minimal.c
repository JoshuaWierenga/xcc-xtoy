#include <stdint.h>

void output_int(int val) {
  __asm("str r1, FF");
}

int input_int(void) {
  int res;
  __asm("lod r1, FF" : "=r" (res));
  return res;
}

int test_not(int a) {
  return ~a;
}

int main(void) {
  int val = input_int();

  return test_not(val);
}
