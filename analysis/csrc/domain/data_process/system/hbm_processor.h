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
#ifndef ANALYSIS_DOMAIN_HBM_PROCESSOR_H
#define ANALYSIS_DOMAIN_HBM_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

// timestamp, bandwidth, hbmid, event_type
using OriHbmData = std::vector<std::tuple<double, double, uint32_t, std::string>>;
// 该类用于定义处理hbm.db中HBMbwData表
class HBMProcessor : public DataProcessor {
public:
    HBMProcessor() = default;
    explicit HBMProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath,
                             std::vector<HbmData> &allProcessedData, std::vector<HbmSummaryData> &allSummaryData);
    std::vector<HbmData> ProcessData(const DBInfo &hbmDB, LocaltimeContext &localtimeContext);
    std::vector<HbmSummaryData> ProcessSummaryData(const uint16_t &deviceId, const DBInfo &hbmDB);
    OriHbmData LoadData(const DBInfo &hbmDB);
    std::vector<HbmData> FormatData(const OriHbmData &oriData, const LocaltimeContext &localtimeContext);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_HBM_PROCESSOR_H
