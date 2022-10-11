/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: manage relationships of task info
 * Author: zcj
 * Create: 2020-09-26
 */
#ifndef ANALYSIS_DVVP_TASKHANDLE_TASK_RELATIONSHIP_MGR_H
#define ANALYSIS_DVVP_TASKHANDLE_TASK_RELATIONSHIP_MGR_H

#include <map>
#include <memory>
#include <set>
#include <vector>
#include "singleton/singleton.h"

namespace Analysis {
namespace Dvvp {
namespace TaskHandle {
class TaskRelationshipMgr : public analysis::dvvp::common::singleton::Singleton<TaskRelationshipMgr> {
public:
    // device id - host id
    int GetHostIdByDevId(int devId);
    void AddLocalFlushJobId(const std::string &jobId);
    int GetFlushSuffixDevId(const std::string &jobId, int indexId);

private:
    std::mutex hostIdMapMutex_;
    std::map<int, int> hostIdToDevId_;
    std::set<std::string> localFlushJobIds_;
};
}  // namespace TaskHandle
}  // namespace Dvvp
}  // namespace Analysis

#endif
