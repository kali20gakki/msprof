/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sys_io_assembler.h
 * Description        : 组合NIC RoCE层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_SYS_IO_ASSEMBLER_H
#define ANALYSIS_APPLICATION_SYS_IO_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"

namespace Analysis {
namespace Application {
class SysIOAssembler : public JsonAssembler {
public:
    SysIOAssembler(const std::string &processorName);
private:
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
};

// 处理NIC数据
class NicAssembler : public SysIOAssembler {
public:
    NicAssembler();
};

// 处理RoCE数据
class RoCEAssembler : public SysIOAssembler {
public:
    RoCEAssembler();
};
}
}
#endif // ANALYSIS_APPLICATION_SYS_IO_ASSEMBLER_H
