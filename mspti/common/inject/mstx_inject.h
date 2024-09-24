/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_inject.h
 * Description        : Injection of mstx.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_INJECT_MSPROFTX_INJECT_H
#define MSPTI_COMMON_INJECT_MSPROFTX_INJECT_H

#include "common/inject/inject_base.h"
#include "external/mspti_result.h"

typedef enum {
    MSTX_FUNC_START = 0,
    MSTX_FUNC_MARKA = 1,
    MSTX_FUNC_RANGE_STARTA = 2,
    MSTX_FUNC_RANGE_END = 3,
    MSTX_FUNC_END
} MstxFuncSeq;
 
typedef enum {
    MSTX_API_MODULE_INVALID                 = 0,
    MSTX_API_MODULE_CORE                    = 1,
    MSTX_API_MODULE_SIZE,                   // end of the enum, new enum items must be added before this
    MSTX_API_MODULE_FORCE_INT               = 0x7fffffff
} MstxFuncModule;

typedef enum  {
    MSTX_API_CORE_INVALID                   =  0,
    MSTX_API_CORE_MARK_A                    =  1,
    MSTX_API_CORE_RANGE_START_A             =  2,
    MSTX_API_CORE_RANGE_END                 =  3,
    MSTX_API_CORE_SIZE,                     // end of the enum, new enum items must be added before this
    MSTX_API_CORE_FORCE_INT                 = 0x7fffffff
} MstxImplCoreFuncId;

typedef void (*MstxFuncPointer)(void);
typedef MstxFuncPointer** MstxFuncTable;
typedef int (*MstxGetModuleFuncTableFunc)(MstxFuncModule module, MstxFuncTable *outTable, unsigned int *outSize);
 
void MstxMarkAFunc(const char* msg, RtStreamT stream);
uint64_t MstxRangeStartAFunc(const char* msg, RtStreamT stream);
void MstxRangeEndFunc(uint64_t rangeId);

#if defined(__cplusplus)
extern "C" {
#endif

MSPTI_API int InitInjectionMstx(MstxGetModuleFuncTableFunc getFuncTable);

#if defined(__cplusplus)
}
#endif

#endif
