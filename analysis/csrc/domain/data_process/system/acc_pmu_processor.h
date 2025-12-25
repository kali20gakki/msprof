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

#ifndef ANALYSIS_APPLICATION_ACC_PMU_PROCESSOR_H
#define ANALYSIS_APPLICATION_ACC_PMU_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/acc_pmu_data.h"

namespace Analysis {
namespace Domain {
// acc_id, read_bandwidth, write_bandwidth, read_ost, write_ost, timestamp
using OriAccPmuData = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, double>>;

class AccPmuProcessor : public DataProcessor {
public:
    AccPmuProcessor() = default;
    explicit AccPmuProcessor(const std::string &profPath);
private:
    bool Process(DataInventory& dataInventory) override;
    OriAccPmuData LoadData(const DBInfo &accPmuDB, const std::string &dbPath);
    std::vector<AccPmuData> FormatData(const OriAccPmuData &oriData,
                                       const Utils::ProfTimeRecord &timeRecord,
                                       const uint16_t deviceId);
    bool ProcessOneDevice(const std::string& devicePath, std::vector<AccPmuData>& res);
};
}
}

#endif // ANALYSIS_APPLICATION_ACC_PMU_PROCESSOR_H
