/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_inject.h
 * Description        : Common definition of mstx inject func.
 * Author             : msprof team
 * Creation Date      : 2024/07/31
 * *****************************************************************************
*/
#ifndef MSTX_INJECT_H
#define MSTX_INJECT_H

#include <stdint.h>

#define MSTX_INVALID_RANGE_ID 0
#define MSTX_SUCCESS 0
#define MSTX_FAIL 1

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

typedef void* aclrtStream;
typedef void (*MstxFuncPointer)(void);
typedef MstxFuncPointer** MstxFuncTable;
typedef int (*MstxGetModuleFuncTableFunc)(MstxFuncModule module, MstxFuncTable *outTable, unsigned int *outSize);

namespace MsprofMstxApi {
void MstxMarkAFunc(const char* msg, aclrtStream stream);
uint64_t MstxRangeStartAFunc(const char* msg, aclrtStream stream);
void  MstxRangeEndFunc(uint64_t id);
}

#endif