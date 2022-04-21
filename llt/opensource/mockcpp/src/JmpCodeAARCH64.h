
#ifndef __MOCKCPP_JMP_CODE_AARCH64_H__
#define __MOCKCPP_JMP_CODE_AARCH64_H__

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <asm/unistd.h>
#include <sys/ioctl.h>
#include <unistd.h>

MOCKCPP_NS_START

struct l2cache_addr_range
{
  uintptr_t start;
  uintptr_t end;
};

MOCKCPP_NS_END

#define ADDR_ALIGN_UP(addr) ((((addr)+((4096)-1))&(~((4096)-1)))&0x00000000ffffffff)
#define ADDR_ALIGN_DOWN(addr) (((addr)&(~((4096)-1)))&0x00000000ffffffff)
#define OUTER_CACHE_INV_RANGE _IOWR('S', 0x00, struct l2cache_addr_range)
#define OUTER_CACHE_CLEAN_RANGE _IOWR('S', 0x01, struct l2cache_addr_range)
#define OUTER_CACHE_FLUSH_RANGE _IOWR('S', 0x02, struct l2cache_addr_range)
#define L1_INV_I_CACHE _IOWR('S', 0x03, struct l2cache_addr_range)
#define D_TO_I_CACHEFLUSH_RANGE _IOWR('S', 0x04, struct l2cache_addr_range)

const unsigned char jmpCodeTemplate[] = { 0x00, 0x00, 0x00, 0x00 };

#define SET_JMP_CODE(base, from, to) do { \
  uintptr_t offset = (uintptr_t)to - (uintptr_t)from; \
  offset = ((offset >> 2) & 0x03FFFFFF) | 0x14000000; \
  *(uintptr_t*)(base) = changeByteOrder(offset); \
} while(0)

//#define FLUSH_CACHE(from, length) do { \
//  int fd = open("/dev/cache_ops", O_RDWR); \
//  MOCKCPP_ASSERT_TRUE_MESSAGE( \
//    "Open /dev/cache_ops failed! Make sure the cache_ops.ko had been loaded!", \
//    0 <= fd); \
//  struct l2cache_addr_range usr_data; \
//  usr_data.start = ADDR_ALIGN_DOWN((unsigned long long)from); \
//  usr_data.end = ADDR_ALIGN_UP((unsigned long long)from) + length; \
//  printf("from = %lld, length = %lld, usr_data.start = %lld, usr_data.end = %lld\n", from, length, usr_data.start, usr_data.end); \
//  int err = ioctl(fd, OUTER_CACHE_FLUSH_RANGE, &usr_data); \
//  MOCKCPP_ASSERT_TRUE_MESSAGE("Cache flush failed!", 0 == err); \
//  close(fd); \
//} while(0)

#define FLUSH_CACHE(from, length) do { \
  struct l2cache_addr_range usr_data; \
  usr_data.start = ADDR_ALIGN_DOWN((unsigned long long)from); \
  usr_data.end = ADDR_ALIGN_UP((unsigned long long)from) + length; \
  __clear_cache((void*)usr_data.start, (void*)usr_data.end); \
  /* printf("from = %lld, length = %lld, usr_data.start = %lld, usr_data.end = %lld\n", from, length, usr_data.start, usr_data.end); */ \
} while(0)


#endif