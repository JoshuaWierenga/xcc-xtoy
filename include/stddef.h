#pragma once

#ifndef NULL
#define NULL  ((void*)0)
#endif

#define offsetof(S, mem)  ((size_t)&(((S *)0)->mem))

#if defined(__ILP32__)
typedef unsigned long size_t;
typedef long ptrdiff_t;
#elif defined(__SIP16__)
typedef unsigned int size_t;
// Fine on C89/90 and C23 as they require 16 or more bits but C99 through C17 require 17 or more
typedef int ptrdiff_t;
#endif

//

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
typedef unsigned int wchar_t;
#endif
