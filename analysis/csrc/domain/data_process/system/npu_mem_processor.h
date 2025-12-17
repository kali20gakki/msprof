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
#ifndef ANALYSIS_DOMAIN_NPU_MEM_PROCESSOR_H
#define ANALYSIS_DOMAIN_NPU_MEM_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
// event, ddr, hbm, timestamp, memory
using OriNpuMemData = std::vector<std::tuple<std::string, uint64_t, uint64_t, double, uint64_t>>;

class NpuMemProcessor : public DataProcessor {
public:
    NpuMemProcessor() = default;
    explicit NpuMemProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<NpuMemData> &allProcessedData);
    OriNpuMemData LoadData(const DBInfo &npuMemDB);
    std::vector<NpuMemData> FormatData(const OriNpuMemData &oriData, const LocaltimeContext &localtimeContext);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_NPU_MEM_PROCESSOR_H
