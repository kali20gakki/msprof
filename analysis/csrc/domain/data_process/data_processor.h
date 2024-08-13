/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : data_processor.h
 * Description        : 数据处理父类
 * Author             : msprof team
 * Creation Date      : 2024/07/19
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_DATA_PROCESSOR_H
#define ANALYSIS_DOMAIN_DATA_PROCESSOR_H

#include <set>
#include "analysis/csrc/infrastructure/db/include/db_info.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

namespace Analysis {
namespace Domain {
const uint8_t CHECK_SUCCESS = 0;
const uint8_t NOT_EXIST = 1;
const uint8_t CHECK_FAILED = 2;
using namespace Analysis::Infra;
// 该类用于定义处理父类
// 主要包括以下特性：用于规范各db处理流程
// 1、Run拉起processor整体流程
// 2、以PROF文件为粒度，分别拉起Process
// (不强制要求 Run, Process完成上述工作,具体根据子类的实际情况进行实现,比如不感知PROF文件的情况)
class DataProcessor {
public:
    DataProcessor() = default;
    explicit DataProcessor(const std::string &profPath);
    bool Run(DataInventory&, const std::string& processorName);
    virtual ~DataProcessor() = default;

protected:
    static uint8_t CheckPathAndTable(const std::string& path, const DBInfo& dbInfo);
    static uint16_t GetEnumTypeValue(const std::string &key, const std::string &tableName,
                                     const std::unordered_map<std::string, uint16_t> &enumTable);
    template<typename Tp>
    bool SaveToDataInventory(std::vector<Tp>&& data, DataInventory& dataInventory, const std::string& processorName);

protected:
    std::string profPath_;

private:
    virtual bool Process(DataInventory& dataInventory) = 0;
};

template<typename Tp>
bool DataProcessor::SaveToDataInventory(std::vector<Tp>&& data, DataInventory& dataInventory,
                                        const std::string& processorName)
{
    INFO("Save % data into dataInventory", processorName);
    if (data.empty()) {
        WARN("% no data to inject to dataInventory", processorName);
        return true;
    }
    std::shared_ptr<std::vector<Tp>> oneSharedData;
    MAKE_SHARED_RETURN_VALUE(oneSharedData, std::vector<Tp>, false, std::move(data));
    dataInventory.Inject(oneSharedData);
    return true;
}
}
}

#endif // ANALYSIS_DOMAIN_DATA_PROCESSOR_H
