/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : JsonAssembler.cpp
 * Description        : json数据组合基类
 * Author             : msprof team
 * Creation Date      : 2024/8/24
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/json_assembler.h"

namespace Analysis {
namespace Application {
JsonAssembler::JsonAssembler(const std::string &name, std::unordered_map<std::string, FileCategory> &&files)
    : fileMap_(files), processorName_(name) {}

bool JsonAssembler::Run(DataInventory& dataInventory, const std::string& profPath)
{
    INFO("Begin to Assemble % data", processorName_);
    JsonWriter ostream;
    // 写多个json对象，需要使用数组包起来作为一个完整有效json
    ostream.StartArray();
    auto res = AssembleData(dataInventory, ostream, profPath);
    // 数组的结束中括号需要在业务处理完后再加上
    ostream.EndArray();
    if (ASSEMBLE_SUCCESS == res) {
        INFO("Assemble % data success, begin to export", processorName_);
        // 处理成功，将写入数据落盘
        FlushToFile(ostream, profPath);
        return true;
    } else if (DATA_NOT_EXIST == res) {
        WARN("No % data, don't export deliverable!");
        return true;
    } else {
        ERROR("Assemble % data failed, can't export deliverable!");
        return false;
    }
}

uint32_t JsonAssembler::GetFormatPid(uint32_t pid, uint32_t index, uint32_t deviceId)
{
    return (pid << HIGH_BIT_OFFSET) | (index << MIDDLE_BIT_OFFSET) | deviceId;
}
}
}