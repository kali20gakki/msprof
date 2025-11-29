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

#ifndef ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_H
#define ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_H

#include <cstdint>
#include <functional>
#include <vector>
#include <memory>
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/context/include/context.h"

namespace Analysis {

namespace Infra {

class Process {
public:
    explicit Process() = default;
    virtual ~Process() = default;

    uint32_t Run(DataInventory& dataInventory, const Context& context)
    {
        PreDump(dataInventory, context);
        auto ret = ProcessEntry(dataInventory, context);
        PostDump(dataInventory, context);
        return ret;
    }

private:
    virtual uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) = 0;
    virtual void PreDump(const DataInventory&, const Context&) const {}
    virtual void PostDump(const DataInventory&, const Context&) const {}
};

}

}

#endif