/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stringids_processer.cpp
 * Description        : stringids_processer，处理StringIds数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/string_ids_processer.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Association::Credential;

StringIdsProcesser::StringIdsProcesser(const std::string &reportDBPath)
    : TableProcesser(reportDBPath) {}

bool StringIdsProcesser::Run()
{
    INFO("StringIdsProcessor Run.");
    bool flag = Process();
    PrintProcessorResult(flag, TABLE_NAME_STRING_IDS);
    return flag;
}

StringIdsProcesser::ProcessedDataFormat StringIdsProcesser::FormatData(const OriDataFormat &oriData)
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

bool StringIdsProcesser::Process(const std::string &fileDir)
{
    OriDataFormat oriData = IdPool::GetInstance().GetAllUint64Ids();
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