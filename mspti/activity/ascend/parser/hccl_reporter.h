/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_repoter.h
 * Description        : 上报记录hccl数据
 * Author             : msprof team
 * Creation Date      : 2024/11/18
 * *****************************************************************************
*/

#ifndef MSPTI_PROJECT_HCCL_REPORTER_H
#define MSPTI_PROJECT_HCCL_REPORTER_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include "external/mspti_activity.h"
#include "common/inject/hccl_inject.h"
#include "activity/ascend/entity/hccl_op_desc.h"

namespace Mspti {
namespace Parser {
class HcclReporter {
public:
    msptiResult RecordHcclMarker(const msptiActivityMarker *markActivity);
    msptiResult RecordHcclOp(uint32_t markId, std::shared_ptr<HcclOpDesc> opDesc);
    msptiResult ReportHcclActivity(std::shared_ptr<HcclOpDesc> markActivity);
    static HcclReporter* GetInstance();
private:
    msptiResult RecordStartMarker(const msptiActivityMarker *markActivity);
    msptiResult ReportHcclData(const msptiActivityMarker *markActivity);
    const char* GetSharedHcclName(std::string& hcclName);
private:
    std::mutex markMutex_;
    std::mutex nameMutex_;
    std::unordered_map<std::string, std::shared_ptr<std::string>> commNameCache_;   // 缓存通信域名称，用于延长生命周期
    std::unordered_map<uint64_t, std::shared_ptr<HcclOpDesc>> markId2HcclOp_;
};
}
}

#endif // MSPTI_PROJECT_HCCL_REPORTER_H
