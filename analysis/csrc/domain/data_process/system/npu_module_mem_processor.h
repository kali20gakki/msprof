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
#ifndef ANALYSIS_DOMAIN_NPU_MODULE_MEM_PROCESSOR_H
#define ANALYSIS_DOMAIN_NPU_MODULE_MEM_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_module_mem_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
// module_id, syscnt, total_size, device_type
using OriNpuModuleFormat = std::vector<std::tuple<uint32_t, double, uint64_t, std::string>>;

class NpuModuleMemProcessor : public DataProcessor {
public:
    NpuModuleMemProcessor() = default;
    explicit NpuModuleMemProcessor(const std::string& profPaths);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string& devicePath, LocaltimeContext& localtimeContext,
                             std::vector<NpuModuleMemData>& allProcessedData, SyscntConversionParams& params);
    OriNpuModuleFormat LoadData(const DBInfo& npuModuleMemDB);
    std::vector<NpuModuleMemData> FormatData(const OriNpuModuleFormat& oriData,
                                             const LocaltimeContext& localtimeContext,
                                             SyscntConversionParams& params);
};

} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_NPU_MODULE_MEM_PROCESSOR_H
