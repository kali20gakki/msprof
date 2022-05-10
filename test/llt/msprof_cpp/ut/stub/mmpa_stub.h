#ifndef _ANALYSIS_DVVP_UT_MMPA_STUB_H_
#define _ANALYSIS_DVVP_UT_MMPA_STUB_H_

#include "mmpa_api.h"
#include <string>
extern INT32 mmCreateTaskWithThreadAttrStub(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
                                         const mmThreadAttr *threadAttr);
extern INT32 mmCreateTaskWithThreadAttrNormalStub(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
                                         const mmThreadAttr *threadAttr);
std::string GetAdxWorkPath();                                         
#endif