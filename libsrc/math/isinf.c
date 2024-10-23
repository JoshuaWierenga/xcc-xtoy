#include "math.h"
#include "stdint.h"  // int64_t
#include "_ieee.h"

#ifndef __NO_FLONUM
extern inline int isinf(double x);

int __isinf(double x) {
  union { double d; int64_t q; } u;
  u.d = x;
  return (u.q & NAN_MASK) == EXPO_MASK;
}
#endif
