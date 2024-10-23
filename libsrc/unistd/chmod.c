#include "sys/stat.h"  // mode_t
#include "_syscall.h"

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if defined(__NR_chmod)
int chmod(const char *pathname, mode_t mode) {
  int ret;
  SYSCALL_RET(__NR_chmod, ret);
  return ret;
}

#elif defined(__NR_fchmodat)
#include "fcntl.h"  // AT_FDCWD

int chmod(const char *pathname, mode_t mode) {
  return fchmodat(AT_FDCWD, pathname, mode, 0);
}
#endif
