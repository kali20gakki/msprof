/**
* @file prof_drv_event.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

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