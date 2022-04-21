
#ifndef __MOCKCPP_JMP_CODE_AARCH32_H__
#define __MOCKCPP_JMP_CODE_AARCH32_H__

#include <stdlib.h>

const unsigned char jmpCodeTemplate[] = { 0xEA, 0x00, 0x00, 0x00 };

// @TODO : make it's style be same as JmpCodeAarch64
#define SET_JMP_CODE(base, from, to) do { \
    int offset = (int)to - (int)from - 8; \
    offset = (offset >> 2) & 0x00FFFFFF; \
    int code = *(int*)(base) | offset; \
    *(int*)(base) = changeByteOrder(code); \
  } while(0)

#define FLUSH_CACHE(from, length) do { \
  ::system("echo 3 > /proc/sys/vm/drop_caches"); \
} while(0)

#endif
