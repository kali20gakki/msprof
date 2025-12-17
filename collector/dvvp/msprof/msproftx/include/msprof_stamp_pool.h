/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
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
    MsprofTxInfo txInfo;
    int id;
    struct MsprofStampInstance* next;
    struct MsprofStampInstance* prev;
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
