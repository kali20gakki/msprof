/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stringids_processor.cpp
 * Description        : stringids_processor，处理StringIds数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/string_ids_processor.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Association::Credential;

StringIdsProcessor::StringIdsProcessor(const std::string &msprofDBPath)
    : TableProcessor(msprofDBPath) {}

bool StringIdsProcessor::Run()
{
    INFO("StringIdsProcessor Run.");
    bool flag = Process();
    PrintProcessorResult(flag, PROCESSOR_NAME_STRING_IDS);
    // 清空上一个PROF的string id信息，避免影响后面的PROF数据存入
    IdPool::GetInstance().Clear();
    return flag;
}

StringIdsProcessor::ProcessedDataFormat StringIdsProcessor::FormatData(const OriDataFormat &oriData)
{
    ProcessedDataFormat processedData;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for stringIds data failed.");
        return processedData;
    }
    for (const auto &pair: oriData) {
        processedData.emplace_back(pair.second, pair.first);
    }
    return processedData;
}

bool StringIdsProcessor::Process(const std::string &fileDir)
{
    OriDataFormat oriData = IdPool::GetInstance().GetAllUint64Ids();
    if (oriData.empty()) {
        WARN("No StringIds data.");
        return true;
    }
    auto processedData = FormatData(oriData);
    if (!SaveData(processedData, TABLE_NAME_STRING_IDS)) {
        ERROR("Failed to generate the STRING_IDS table.");
        return false;
    }
    return true;
}

}
}
}