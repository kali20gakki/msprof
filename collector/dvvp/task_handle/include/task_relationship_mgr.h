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
