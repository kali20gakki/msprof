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

#ifndef ANALYSIS_DVVP_PROF_DRV_EVENT_H
#define ANALYSIS_DVVP_PROF_DRV_EVENT_H
#include <string>
#include "collection_register.h"
#include "ai_drv_prof_api.h"
#include "ascend_hal.h"
#include "mmpa_api.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
struct TaskEventAttr {
    uint32_t deviceId;
    analysis::dvvp::driver::AI_DRV_CHANNEL channelId;
    enum ProfCollectionJobE jobTag;
    bool isChannelValid;
    bool isProcessRun;
    bool isExit;
    bool isThreadStart;
    Collector::Dvvp::Mmpa::MmThread handle;
    bool isAttachDevice;
    bool isWaitDevPid;
    std::string grpName;
};

class ProfDrvEvent {
public:
    ProfDrvEvent();
    ~ProfDrvEvent();
    int SubscribeEventThreadInit(struct TaskEventAttr *eventAttr) const;
    void SubscribeEventThreadUninit(uint32_t devId) const;
private:
    static void *EventThreadHandle(void *attr);
    static int QueryGroupId(uint32_t devId, uint32_t &grpId, std::string grpName);
    static int QueryDevPid(struct TaskEventAttr *eventAttr);
    static void WaitEvent(struct TaskEventAttr *eventAttr, uint32_t grpId);
};

}  // namespace JobWrapper
}  // namespace Dvvp
}  // namespace Analysis
#endif  // ANALYSIS_DVVP_PROF_DRV_EVENT_H