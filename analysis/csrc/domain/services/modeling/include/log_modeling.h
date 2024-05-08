/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : log_modeling.h
 * Description        : log modeling 处理流程类
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_LOG_MODELING_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_LOG_MODELING_H

#include <map>
#include <vector>
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"

namespace Analysis {

namespace Domain {

class LogModeling final : public Infra::Process {
private:
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;

    std::map<uint16_t, std::vector<HalTrackData*>> SplitLogGroups(std::vector<HalLogData>& logData,
        std::shared_ptr<std::vector<HalTrackData>>& flipTrack);
    void OutputLogCounts(const std::vector<HalLogData>& logData) const;
    void AddToDeviceTask(std::map<uint16_t, std::vector<HalLogData*>>& startTask,
                         std::map<uint16_t, std::vector<HalLogData*>>& endTask,
                         std::map<uint16_t, std::vector<HalTrackData*>>& flipGroups,
                         std::map<TaskId, std::vector<DeviceTask>>& deviceTaskMap,
                         std::function<void(Domain::DeviceTask&, const HalLogData&, const HalLogData&)> mergeFunc);

private:
    std::map<uint16_t, std::vector<HalLogData*>> acsqStart_, acsqEnd_, fftsStart_, fftsEnd_;
};

}

}

#endif