#ifndef _ANALYSIS_DVVP_UT_IDE_COMMON_UTIL_STUB_H_
#define _ANALYSIS_DVVP_UT_IDE_COMMON_UTIL_STUB_H_

#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif
void *IdeXmalloc (int size);
void IdeXfree (void *ptr);
void *GetIdeDaemonHdcClient();
#ifdef __cplusplus
}
#endif
#endif

