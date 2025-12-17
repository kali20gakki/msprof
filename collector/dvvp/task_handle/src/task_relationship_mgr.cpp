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
#include "task_relationship_mgr.h"

#include "errno/error_code.h"
#include "msprof_dlog.h"


namespace Analysis {
namespace Dvvp {
namespace TaskHandle {

int TaskRelationshipMgr::GetHostIdByDevId(int devId)
{
    std::lock_guard<std::mutex> lock(hostIdMapMutex_);
    for (auto iter = hostIdToDevId_.begin(); iter != hostIdToDevId_.end(); iter++) {
        if (iter->second == devId) {
            return iter->first;
        }
    }
    return devId;
}

void TaskRelationshipMgr::AddLocalFlushJobId(const std::string &jobId)
{
    MSPROF_LOGI("Job %s should flush locally", jobId.c_str());
    localFlushJobIds_.insert(jobId);
}

int TaskRelationshipMgr::GetFlushSuffixDevId(const std::string &jobId, int indexId)
{
    if (localFlushJobIds_.find(jobId) != localFlushJobIds_.end()) {
        return indexId;
    } else {
        return GetHostIdByDevId(indexId);
    }
}
}  // namespace TaskHandle
}  // namespace Dvvp
}  // namespace Analysis
