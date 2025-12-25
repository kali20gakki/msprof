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
#ifndef ANALYSIS_DOMAIN_DDR_PROCESSOR_H
#define ANALYSIS_DOMAIN_DDR_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ddr_data.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
// device_id, timestamp, flux_read, flux_write
using OriDataFormat = std::vector<std::tuple<uint32_t, double, double, double>>;

// 该类用于定义处理ddr.db中DDRMetricData表
class DDRProcessor : public DataProcessor {
public:
    DDRProcessor() = default;
    explicit DDRProcessor(const std::string& profPaths);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessOneDevice(const std::string& devicePath, std::vector<DDRData>& res);
    OriDataFormat LoadData(const DBInfo& DDRDB);
    std::vector<DDRData> FormatData(const OriDataFormat& oriData, const LocaltimeContext& localtimeContext);
};
} // namespace Domain
} // namespace Analysis

#endif // ANALYSIS_DOMAIN_DDR_PROCESSOR_H
