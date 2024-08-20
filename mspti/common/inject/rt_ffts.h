/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : rt_ffts.cpp
 * Description        : runtime ffts结构体
 * Author             : msprof team
 * Creation Date      : 2024/08/20
 * *****************************************************************************
*/
#ifndef MSPTI_PROJECT_RT_FFTS_H
#define MSPTI_PROJECT_RT_FFTS_H

#include <cstdint>
#include <cstddef>

#if defined(__cplusplus)
extern "C" {
#endif

#define RT_FFTS_MAX_SUB_TASK_NUM    32U
#define RT_FFTS_MAX_TICKET_CACHE_NUM    64U
#define RT_FFTS_MAX_MANUAL_THREAD_NUM   16U
#define RT_FFTS_MAX_TICKET_CACHE_PER_SUBTASK    8U
#define RT_FFTS_MANUAL_SRC_DEPEND_TBL_LEN    32U

/**
 * @ingroup rt_kernel
 * @brief op name
 */
typedef struct RtKernelLaunchNames {
    const char *soName;      // defined for so name
    const char *kernelName;  // defined for kernel type name
    const char *opName;      // defined for operator name
} RtKernelLaunchNamesT;

typedef enum TagFftsType {
    RT_FFTS_TYPE_AUTO_THREAD = 2,   // ffts auto thread mode, same as ffts define
    RT_FFTS_TYPE_MANUAL_THREAD = 3, // ffts manual thread mode, same as ffts define
} RtFftsTypeT;

typedef enum TagFftsSubTaskType {
    RT_FFTS_SUB_TASK_TYPE_AIC = 0,
    RT_FFTS_SUB_TASK_TYPE_AIV = 1,
    RT_FFTS_SUB_TASK_TYPE_NOP = 2,
    RT_FFTS_SUB_TASK_TYPE_NOTIFY_WAIT = 3,
    RT_FFTS_SUB_TASK_TYPE_NOTIFY_RECORD = 4,
    RT_FFTS_SUB_TASK_TYPE_WRITE_VALUE = 5,
    RT_FFTS_SUB_TASK_TYPE_MIX_AIC = 6,
    RT_FFTS_SUB_TASK_TYPE_MIX_AIV = 7,
    RT_FFTS_SUB_TASK_TYPE_SDMA = 8,
    RT_FFTS_SUB_TASK_TYPE_RESERVED = 9,
} RtFftsSubTaskTypeT;

typedef struct TagManualThreadDmuInfo {
    uint64_t dataAddr; // device mem
    uint16_t numOuter;
    uint16_t numInner;
    uint32_t strideOuter;
    uint32_t lenInner;
    uint32_t strideInner;
} RtManualThreadDmuInfoT;

typedef struct TagManualThreadDependency {
    uint8_t dependency[RT_FFTS_MANUAL_SRC_DEPEND_TBL_LEN];
} RtManualThreadDependencyT;

typedef struct TagManualThreadAicAivInfo {
    uint64_t taskParamAddr; // device mem
    uint16_t taskParamOffset;
    // when satMode=1 and FP16 computation with none INF inputs overflows/underflows, results will be +/-INF of FP16
    // when satMode=0 and FP16 computation with none INF inputs overflows/underflows,
    // results will be saturated to +/-MAX of FP16
    uint8_t satMode;
    uint8_t scheduleMode;   // 0:normal mode, 1:batch mode, 2:sync mode 3:reserved
    uint8_t iCachePrefetchCnt; // units is 2K
    uint8_t prefetchEnableBitmap; // 8 bit bitmap  1 0 1 0
    uint8_t prefetchOnceBitmap; // 8 bit bitmap  1 0 1 0
    uint16_t prefetchOnceDmuNum; // prefetch_once_dmu_descriptor_index in ffts
    // num: thread0_prefetch_dmu_descriptor_index – prefetch_once_dmu_descriptor_index
    uint16_t threadPrefetchDmuIdx[RT_FFTS_MAX_MANUAL_THREAD_NUM]; // max valid is threadDim
    uint16_t threadBlkDim[RT_FFTS_MAX_MANUAL_THREAD_NUM];
    const char *threadTaskFuncStub[RT_FFTS_MAX_MANUAL_THREAD_NUM];

    RtManualThreadDmuInfoT *prefetchList; // dmu desc 0-64k, length is the last threadPrefetchDmuIdx[threadDim-1]
    RtManualThreadDependencyT srcDepTbl[RT_FFTS_MAX_TICKET_CACHE_PER_SUBTASK];
} RtManualThreadAicAivInfoT;

typedef struct TagAutoThreadPrefetch {
    uint64_t dataAddr; // device mem
    uint32_t dataAddrOffset;
    uint32_t nonTailDataLen;
    uint32_t tailDataLen;
} RtAutoThreadPrefetchT;

typedef struct TagAutoThreadAicAivInfo {
    uint64_t taskParamAddr; // device mem
    uint16_t taskParamOffset;
    /*
     * when satMode=1 and FP16 computation with none INF inputs overflows/underflows, results will be +/-INF of FP16
     * when satMode=0 and FP16 computation with none INF inputs overflows/underflows, results will be saturated to
     *     +/-MAX of FP16
     */
    uint8_t satMode;
    uint8_t scheduleMode;   // 0:normal mode, 1:batch mode, 2:sync mode 3:reserved
    uint8_t iCachePrefetchCnt; // units is 2K
    uint8_t prefetchEnableBitmap;   // 8 bit bitmap
    uint8_t prefetchOnceBitmap;     // 8 bit bitmap

    uint16_t tailBlkDim;
    uint16_t nonTailBlkDim;

    const char *nonTailTaskFuncStub;
    const char *tailTaskFuncStub;

    // for prefetch, valid num is prefetchEnableBitmap bit count.
    // if prefetchEnableBitmap='00010011', need prefetch number is 3, srcPrefetch is only 0, 1, 2 is valid
    RtAutoThreadPrefetchT srcPrefetch[RT_FFTS_MAX_TICKET_CACHE_PER_SUBTASK];
} RtAutoThreadAicAivInfoT;

typedef struct TagAutoThreadCacheInfo {
    uint64_t dataAddr; // device mem
    uint32_t dataAddrOffset;
    uint32_t nonTailDataLen;
    uint32_t tailDataLen;
    uint16_t ticketCacheRefCnt;
} RtAutoThreadCacheInfoT;

typedef struct TagManualThreadCacheInfo {
    RtManualThreadDmuInfoT *dmuList;  // 0-64k
    uint16_t dmuNum;
    uint16_t sliceDmuIdx[RT_FFTS_MAX_MANUAL_THREAD_NUM];
    uint16_t ticketCacheRefCntTbl[RT_FFTS_MAX_MANUAL_THREAD_NUM];
} RtManualThreadCacheInfoT;

typedef enum TagCacheOp {
    RT_CACHE_OP_NONE = 0,
    RT_CACHE_OP_FLUSH = 1,
    RT_CACHE_OP_INVALIDATE = 2,
    RT_CACHE_OP_WRITE_BACK = 3,
} RtCacheOpT;

typedef struct TagTicketCache {
    RtCacheOpT cacheOption;
    uint8_t ticketCacheWindow;
    union {
        RtAutoThreadCacheInfoT autoThreadCache;
        RtManualThreadCacheInfoT manualThreadCache;
    } custom;
} RtTicketCacheT;

typedef struct TagManualThreadNopInfo {
    // depend srcTickCacheVldBitmap in RtFftsSubTaskInfoT
    RtManualThreadDependencyT srcDepTbl[RT_FFTS_MAX_TICKET_CACHE_PER_SUBTASK];
} RtManualThreadNopInfoT;

typedef struct TagFftsSubTaskInfo {
    RtFftsSubTaskTypeT subTaskType;
    uint16_t threadDim;
    uint8_t dstTickCacheVldBitmap;
    uint8_t srcTickCacheVldBitmap;
    uint8_t srcDataOutOfSubGraphBitmap;
    uint8_t dstTickCacheID[RT_FFTS_MAX_TICKET_CACHE_PER_SUBTASK];
    uint8_t srcTickCacheID[RT_FFTS_MAX_TICKET_CACHE_PER_SUBTASK];
    union {
        RtAutoThreadAicAivInfoT autoThreadAicAiv;
        RtManualThreadAicAivInfoT manualThreadAicAiv;
        RtManualThreadNopInfoT manualThreadNop;
    } custom;
} RtFftsSubTaskInfoT;

typedef struct TagFftsDescInfo {
    uint8_t tm; // thread subtask kickstart mode, 0:order, 1:disorder
    uint8_t di; // discard invalidate
    uint8_t dw; // discard write back
    uint8_t df; // discard flush
    uint8_t dataSplitUnit;  // split source or ticket cache by 2^dataSplitUnit MB
    uint8_t prefetchOstNum;
    uint8_t cacheMaintainOstNum;
    uint8_t aicPrefetchUpper;
    uint8_t aicPrefetchLower;
    uint8_t aivPrefetchUpper;
    uint8_t aivPrefetchLower;
} RtFftsDescInfoT;

typedef struct TagFftsTaskInfo {
    RtFftsTypeT fftsType;
    uint16_t subTaskNum;
    uint16_t tickCacheNum;
    RtFftsDescInfoT fftsDesc;
    // sub task desc, real num is subTaskNum
    RtFftsSubTaskInfoT subTask[RT_FFTS_MAX_SUB_TASK_NUM];

    // ticket cache, real number is tickCacheNum.
    RtTicketCacheT ticketCache[RT_FFTS_MAX_TICKET_CACHE_NUM];
} RtFftsTaskInfoT;

typedef struct TagStarsSqeHeader {
    // 0-8
    uint8_t type : 6;
    uint8_t l1Lock : 1;
    uint8_t l1Unlock : 1;
    // 8-15
    uint8_t ie : 2;
    uint8_t preP : 2;
    uint8_t postP : 2;
    uint8_t wrCqe : 1;
    uint8_t reserved : 1;
    // blockDim
    uint16_t blockDim;
    uint16_t rtStreamId;
    uint16_t taskId;
} RtStarsSqeHeaderT;

typedef struct TagFftsPlusSqe {
    // 0-7 bytes
    RtStarsSqeHeaderT sqeHeader; // use rtStarsFftsPlusHeader_t instead
    // 8-11 bytes
    uint16_t fftsType : 3;
    uint16_t cmo : 1;
    uint16_t scheduleDfxFlag : 1;
    uint16_t reserved1 : 7;
    uint16_t wrrRatio : 4;
    uint16_t dsaSqId : 11;
    uint16_t reserved2 : 5;
    // 12-15 bytes
    uint16_t sqeIndex;
    uint8_t  kernelCredit;
    uint8_t  subType;
    uint32_t stackPhyBaseL;
    uint32_t stackPhyBaseH;
    uint16_t  totalContextNum;
    uint16_t  readyContextNum;
    uint16_t  preloadContextNum;
    uint16_t  timeout;
    uint16_t  reserved6;
    uint16_t  prefetchOstNum : 5;
    uint16_t  reserved9 : 3;
    uint16_t  cmaintOstNum : 5;
    uint16_t  reserved10 : 3;
    uint16_t  aicPrefetchLower : 5;
    uint16_t  reserved11 : 3;
    uint16_t  aicPrefetchUpper : 5;
    uint16_t  reserved12 : 3;
    uint16_t  aivPrefetchLower : 5;
    uint16_t  reserved13 : 3;
    uint16_t  aivPrefetchUpper : 5;
    uint16_t  reserved14 : 3;
    // 40-47 bytes
    uint32_t contextAddressBaseL;
    uint32_t contextAddressBaseH : 17;
    uint32_t reserved15 : 15;
    // 48-63 bytes:48-51 use for pid
    uint32_t reserved16[4];
} RtFftsPlusSqeT;

typedef struct TagFftsPlusDumpInfo {
    const void *loadDumpInfo;
    const void *unloadDumpInfo;
    uint32_t loadDumpInfolen;
    uint32_t unloadDumpInfolen;
} RtFftsPlusDumpInfoT;

typedef struct TagFftsPlusTaskInfo {
    const RtFftsPlusSqeT *fftsPlusSqe;
    const void *descBuf;                   // include total context
    size_t      descBufLen;                // the length of descBuf
    RtFftsPlusDumpInfoT fftsPlusDumpInfo; // used only in the dynamic shape
    uint32_t descAddrType;                 // 0:host addr 1:device addr
    uint32_t argsHandleInfoNum;
    void **argsHandleInfoPtr;
} RtFftsPlusTaskInfoT;

#if defined(__cplusplus)
}
#endif

#endif // MSPTI_PROJECT_RT_FFTS_H
