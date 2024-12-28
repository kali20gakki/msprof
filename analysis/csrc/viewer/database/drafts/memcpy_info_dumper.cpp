/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memcpy_info_dumper.cpp
 * Description        : MemcpyInfo落盘
 * Author             : msprof team
 * Creation Date      : 2024/12/12
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/drafts/memcpy_info_dumper.h"
#include <unordered_map>
#include "analysis/csrc/viewer/database/drafts/number_mapping.h"
#include "analysis/csrc/parser/host/cann/hash_data.h"
#include "analysis/csrc/parser/host/cann/type_data.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
using HashData = Analysis::Parser::Host::Cann::HashData;
using TypeData = Analysis::Parser::Host::Cann::TypeData;
using RuntimeFormat = std::vector<std::tuple<uint64_t, int64_t>>;
using namespace Analysis::Utils;
const std::string HOST_TASK_TABLE = "HostTask";

bool CheckPathAndTableExists(const std::string &path, DBRunner& dbRunner, const std::string &tableName)
{
    if (!File::Exist(path)) {
        WARN("There is no %", path);
        return false;
    }
    if (!dbRunner.CheckTableExists(tableName)) {
        WARN("There is no %", tableName);
        return false;
    }
    return true;
}

MemcpyInfoDumper::MemcpyInfoDumper(const std::string &hostPath)
    :  BaseDumper<MemcpyInfoDumper>(hostPath, "MemcpyInfo")
{
    MAKE_SHARED0_NO_OPERATION(database_, RuntimeDB);
}

std::unordered_map<uint64_t, int64_t> MemcpyInfoDumper::GetConnectionId()
{
    std::unordered_map<uint64_t, int64_t> connectionIdMap;
    RuntimeDB runtimeDb;
    std::string runtimeDbPath = Utils::File::PathJoin({dbPath_, runtimeDb.GetDBName()});
    DBRunner hostRuntimeDBRunner(runtimeDbPath);
    if (!CheckPathAndTableExists(runtimeDbPath, hostRuntimeDBRunner, HOST_TASK_TABLE)) {
        WARN("% not existed", HOST_TASK_TABLE);
        return connectionIdMap;
    }
    std::string sql{"SELECT DISTINCT timestamp, connection_id FROM HostTask;"};
    RuntimeFormat result(0);
    auto rc = hostRuntimeDBRunner.QueryData(sql, result);
    if (!rc) {
        ERROR("Failed to obtain data from the % table.", runtimeDb.GetDBName());
        return connectionIdMap;
    }
    uint64_t timestamp;
    int64_t connectionId;
    for (const auto& row : result) {
        std::tie(timestamp, connectionId) = row;
        connectionIdMap[timestamp] = connectionId;
    }
    return connectionIdMap;
}

MemcpyInfoData MemcpyInfoDumper::GenerateData(const MemcpyInfos &memcpyInfos)
{
    MemcpyInfoData data;
    if (!Utils::Reserve(data, memcpyInfos.size())) {
        ERROR("MemcpyInfos reserve failed");
        return data;
    }
    auto connectionIdMap = GetConnectionId();
    int64_t connectionId;
    for (auto &memcpyInfo: memcpyInfos) {
        auto it = connectionIdMap.find(memcpyInfo->timeStamp);
        if (it != connectionIdMap.end()) {
            connectionId = it->second;
        } else {
            connectionId = -1;
        }
        data.emplace_back(
            TypeData::GetInstance().Get(MSPROF_REPORT_RUNTIME_LEVEL, memcpyInfo->type),
            NumberMapping::Get(NumberMapping::MappingType::LEVEL, memcpyInfo->level),
            memcpyInfo->threadId, memcpyInfo->dataLen, memcpyInfo->timeStamp, memcpyInfo->data.memcpyInfo.dataSize,
            memcpyInfo->data.memcpyInfo.memcpyDirection, memcpyInfo->data.memcpyInfo.maxSize, connectionId);
    }
    return data;
}
} // Drafts
} // Database
} // Viewer
} // Analysis
