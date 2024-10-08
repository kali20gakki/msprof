/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_info_processor.cpp
 * Description        : 落盘Host侧数据
 * Author             : msprof team
 * Creation Date      : 2024/04/11
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/host_info_processor.h"

#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using Context = Parser::Environment::Context;

HostInfoProcessor::HostInfoProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

bool HostInfoProcessor::Run()
{
    INFO("HostInfoProcessor Run.");
    // profPaths_已在外部校验, 保证传到这里的值不为空
    const auto& fileDir = *(profPaths_.begin());
    bool flag = Process(fileDir);
    // 无host目录 无数据时，避免saveData因数据为空 导致error
    if (flag && hostInfoData_.empty()) {
        return true;
    }
    flag &= SaveData(hostInfoData_, TABLE_NAME_HOST_INFO);
    PrintProcessorResult(flag, PROCESSOR_NAME_HOST_INFO);
    return flag;
}

bool HostInfoProcessor::Process(const std::string &fileDir)
{
    INFO("HostInfoProcessor Process, dir is %", fileDir);
    auto hostDirs = Utils::File::GetFilesWithPrefix(fileDir, HOST);
    for (const auto& hostDir : hostDirs) {
        UpdateHostData(fileDir, hostDir, hostInfoData_);
    }
    return true;
}

void HostInfoProcessor::UpdateHostData(const std::string &fileDir, const std::string &hostDir,
                                       HostInfoDataFormat &hostInfoData)
{
    INFO("HostInfoProcessor UpdateHostData, dir is %", fileDir);
    std::string hostUid = Context::GetInstance().GetHostUid(Parser::Environment::HOST_ID, fileDir);
    std::string hostName = Context::GetInstance().GetHostName(Parser::Environment::HOST_ID, fileDir);
    hostInfoData.emplace_back(hostUid, hostName);
}

} // Database
} // Viewer
} // Analysis
