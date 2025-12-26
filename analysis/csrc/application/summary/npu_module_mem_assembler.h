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
#ifndef ANALYSIS_APPLICATION_SUMMARY_NPU_MODULE_MEM_ASSEMBLER_H
#define ANALYSIS_APPLICATION_SUMMARY_NPU_MODULE_MEM_ASSEMBLER_H

#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"

namespace Analysis {
namespace Application {

class NpuModuleMemAssembler : public SummaryAssembler {
public:
    NpuModuleMemAssembler() = default;
    NpuModuleMemAssembler(const std::string &name, const std::string &profPath);
private:
    std::unordered_map<uint16_t, std::string> moduleMap_;

    uint8_t AssembleData(DataInventory &dataInventory);
};
}
}
#endif // ANALYSIS_APPLICATION_SUMMARY_NPU_MODULE_MEM_ASSEMBLER_H