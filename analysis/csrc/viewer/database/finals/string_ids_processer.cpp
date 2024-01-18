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
#include "analysis/csrc/utils/thread_pool.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Association::Credential;

StringIdsProcesser::StringIdsProcesser(const std::string &reportDBPath)
    : TableProcesser(reportDBPath)
{
    reportDB_.tableName = "STRING_IDS";
}

bool StringIdsProcesser::Run()
{
    return Process();
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
    if (!SaveData(processedData)) {
        ERROR("Failed to generate the STRING_IDS table.");
        return false;
    }
    return true;
}

}
}
}