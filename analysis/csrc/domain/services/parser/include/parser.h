/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : parser.h
 * Description        : 解析二进制数据
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_PARSER_H
#define MSPROF_ANALYSIS_PARSER_H

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/entities/hal/include/hal.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"

namespace Analysis {
namespace Domain {

class Parser : public Infra::Process {
public:
    std::unique_ptr<uint8_t[]> binaryData;
    uint64_t binaryDataSize;
private:
    uint32_t ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context) override;

    uint32_t GetFileSize(const char *filePath);

    uint64_t GetFilesSize(const std::vector<std::string> &path);

    uint32_t ReadData(const std::vector<std::string> &files, size_t firstFileOffset);

    uint32_t ReadDataEntry(const DeviceContext &deviceContext);

    // 获取文件路径
    std::string GetFilePath(const DeviceContext &deviceContext);

    virtual std::string GetFilePattern() = 0;

    virtual uint32_t GetTrunkSize() = 0;

    virtual uint32_t ParseData(Infra::DataInventory &dataInventory) = 0;

    // 获取没有文件返回码
    virtual uint32_t GetNoFileCode();
};
}
}
#endif // MSPROF_ANALYSIS_PARSER_H