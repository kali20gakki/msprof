/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : JsonAssembler.h
 * Description        : json数据组合基类
 * Author             : msprof team
 * Creation Date      : 2024/8/24
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_JSON_ASSEMBLER_H
#define ANALYSIS_APPLICATION_JSON_ASSEMBLER_H

#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/dump_tools/json_tool/include/json_writer.h"
#include "analysis/csrc/domain/entities/json_trace/include/meta_data_event.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/infrastructure/dump_tools/include/dump_tool.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Domain;
using namespace Analysis::Infra;

class JsonAssembler {
public:
    JsonAssembler() = default;
    JsonAssembler(const std::string &name, std::unordered_map<std::string, FileCategory> &&files);
    virtual ~JsonAssembler() = default;
    bool Run(DataInventory &dataInventory, const std::string &profPath);
    static uint32_t GetFormatPid(uint32_t pid, uint32_t index, uint32_t deviceId = HOST_PID);
protected:
    uint32_t GetDevicePid(std::unordered_map<uint16_t, uint32_t> &pidMap, uint16_t deviceId,
                          const std::string &profPath, uint32_t index);
    void GenerateHWMetaData(const std::unordered_map<uint16_t, uint32_t> &pidMap, const struct LayerInfo &layerInfo,
                          std::vector<std::shared_ptr<TraceEvent>> &res);
protected:
    std::unordered_map<std::string, FileCategory> fileMap_;
private:
    bool FlushToFile(JsonWriter &ostream, const std::string &profPath);
    virtual uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) = 0;
private:
    std::string processorName_;
};
}
}
#endif // ANALYSIS_APPLICATION_JSON_ASSEMBLER_H
