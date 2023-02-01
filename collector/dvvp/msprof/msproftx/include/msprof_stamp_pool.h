/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: msprof tx stamp pool
 * Author:
 * Create: 2021-11-22
 */
#ifndef PROF_STAMP_POOL_H
#define PROF_STAMP_POOL_H

#include <map>
#include <mutex>
#include <vector>
#include "prof_common.h"
#include "prof_callback.h"

namespace Msprof {
namespace MsprofTx {

// Specification
constexpr int MAX_STAMP_SIZE = 10000;
constexpr int CURRENT_STAMP_SIZE = 100;

struct MsprofStampInstance {
    ReporterData report;
    MsprofStampInfo stampInfo;
    int id;
    struct MsprofStampInstance* next;
    struct MsprofStampInstance* prev;
    char stampTagName[MSPROF_ENGINE_MAX_TAG_LEN + 1];
};

struct MsprofStampCtrlHandle {
    struct MsprofStampInstance* memPool;
    struct MsprofStampInstance* freelist;
    struct MsprofStampInstance* usedlist;
    uint32_t freeCnt;
    uint32_t usedCnt;
    uint32_t instanceSize;
};

class MsprofStampPool {
public:
    MsprofStampPool();
    virtual ~MsprofStampPool();

public:
    // create stamp memory pool
    int Init(int size);
    // destroy memory pool
    int UnInit() const;

    // get stamp from memory pool
    MsprofStampInstance* CreateStamp();
    // destroy stamp
    void DestroyStamp(MsprofStampInstance* stamp);

    // push/pob
    int MsprofStampPush(MsprofStampInstance* stamp);
    MsprofStampInstance* MsprofStampPop();

    MsprofStampInstance* GetStampById(int id);
    int GetIdByStamp(MsprofStampInstance* stamp) const;

private:
    std::vector<MsprofStampInstance *> singleTStack_;
    std::mutex singleTStackMtx_;
    std::mutex memoryListMtx_;
};

}
}

#endif // PROF_STAMP_POOL_H
