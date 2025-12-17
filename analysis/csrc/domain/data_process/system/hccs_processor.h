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
#ifndef ANALYSIS_DOMAIN_HCCS_PROCESSOR_H
#define ANALYSIS_DOMAIN_HCCS_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hccs_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
// timestamp, txthroughput, rxthroughput
using OriHccsData = std::vector<std::tuple<double, double, double>>;

// 该类用于定义处理hccs.db中HCCSEventsData表
class HCCSProcessor : public DataProcessor {
public:
    HCCSProcessor() = default;
    explicit HCCSProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath,
                             std::vector<HccsData> &allProcessedData, std::vector<HccsSummaryData> &allSummaryData);
    std::vector<HccsData> ProcessData(const DBInfo &hccsDB, LocaltimeContext &localtimeContext);
    HccsSummaryData ProcessSummaryData(const uint16_t &deviceId, const DBInfo &hccsDB);
    OriHccsData LoadData(const DBInfo &hccsDB);
    std::vector<HccsData> FormatData(const OriHccsData &oriData, const LocaltimeContext &localtimeContext);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_HCCS_PROCESSOR_H
