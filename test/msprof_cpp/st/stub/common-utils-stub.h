#ifndef _ANALYSIS_DVVP_UT_COMMON_UTILS_STUB_H
#define _ANALYSIS_DVVP_UT_COMMON_UTILS_STUB_H
#include <string.h>
#include <stdio.h>
#include <string>
#include "mmpa_api.h"

extern "C" int snprintf_s(char* strDest, size_t destMax, size_t count, const char* format, ...);
std::string GetAdxWorkPath();
#endif

