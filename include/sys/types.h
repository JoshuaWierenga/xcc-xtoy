#pragma once

#if defined(__SIP16__)
typedef int ssize_t;
#else
typedef long ssize_t;
#endif

typedef int pid_t;
#if defined(__SIP16__)
typedef int off_t;
#else
typedef long off_t;
#endif
