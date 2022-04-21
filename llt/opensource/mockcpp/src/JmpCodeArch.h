
#ifndef __MOCKCPP_JMP_CODE_ARCH_H__
#define __MOCKCPP_JMP_CODE_ARCH_H__

#include <mockcpp/mockcpp.h>

template<typename T>
inline T changeByteOrder(const T v) {
  enum { S = sizeof(T) };
  T rst = v;
  char *p = (char *)&rst;
  char tmp = 0;
  for (unsigned int i = 0; i < S / 2; ++i) {
    tmp = p[i];
    p[i] = p[S - i - 1];
    p[S - i - 1] = tmp;
  }

  return rst;
}

#if defined(BUILD_FOR_X64)
# include "src/JmpCodeX64.h"
#elif defined(BUILD_FOR_X86)
# include "src/JmpCodeX86.h"
#elif defined(BUILD_FOR_AARCH32)
# include "src/JmpCodeAARCH32.h"
#elif defined(BUILD_FOR_AARCH64)
# include "src/JmpCodeAARCH64.h"
#endif

#endif

