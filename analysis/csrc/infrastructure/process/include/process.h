/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process.h
 * Description        : 流程处理基类
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */

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