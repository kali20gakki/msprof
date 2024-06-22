/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_op_mem_processor.cpp
 * Description        : npu_op_mem_processor，处理NpuOpMemRaw表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/21
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/npu_op_mem_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/dfx/error_code.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Association::Credential;
namespace {
struct NpuOpMemData {
    uint32_t threadId = 0;
    uint64_t type = 0;
    uint64_t totalAllocateMemory = 0;
    uint64_t totalReserveMemory = 0;
    int64_t size = 0;
    double timestamp = 0.0;
    std::string operatorName;
    std::string addr;
    std::string device_type;
};
const std::string COMPONENT = "GE"; // 命令行侧写死GE
const std::string RELEASE = "release";
const std::string ALLOCATE = "allocate";
}

NpuOpMemProcessor::NpuOpMemProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

bool NpuOpMemProcessor::Run()
{
    INFO("NpuOpMemProcessor Run.");
    stringGeId_ = IdPool::GetInstance().GetUint64Id(COMPONENT);
    stringReleaseId_ = IdPool::GetInstance().GetUint64Id(RELEASE);
    stringAllocateId_ = IdPool::GetInstance().GetUint64Id(ALLOCATE);
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_NPU_OP_MEM);
    return flag;
}

NpuOpMemProcessor::OriDataFormat NpuOpMemProcessor::GetData(DBInfo &npuOpMemDB)
{
    OriDataFormat oriData;
    std::string sql{"SELECT operator, addr, size, timestamp, thread_id, total_allocate_memory, total_reserve_memory, "
                    "device_type FROM " + npuOpMemDB.tableName};
    if (!npuOpMemDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", npuOpMemDB.tableName);
    }
    return oriData;
}

NpuOpMemProcessor::ProcessedDataFormat NpuOpMemProcessor::FormatData(const OriDataFormat &oriData,
                                                                     const Utils::ProfTimeRecord &timeRecord,
                                                                     Utils::SyscntConversionParams &params,
                                                                     GeHashMap &hashMap, uint32_t pid) const
{
    ProcessedDataFormat processedData;
    NpuOpMemData data;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for NpuOpMem data failed.");
        return processedData;
    }
    for (auto &row: oriData) {
        std::tie(data.operatorName, data.addr, data.size, data.timestamp, data.threadId, data.totalAllocateMemory,
                 data.totalReserveMemory, data.device_type) = row;
        HPFloat timestamp{GetTimeFromSyscnt(data.timestamp, params)};
        if (data.size < 0) {
            data.size = std::abs(data.size);
            data.type = stringReleaseId_;
        } else {
            data.type = stringAllocateId_;
        }
        uint64_t addr = UINT64_MAX;
        if (Utils::StrToU64(addr, data.addr) == ANALYSIS_ERROR) { // 转uint64失败默认值为UINT64_MAX
            WARN("Converting string(addr) to integer failed.");
        }
        // hash 可能为空，不能直接使用。如果有数据才取用
        if (hashMap.find(data.operatorName) != hashMap.end()) {
            data.operatorName = hashMap[data.operatorName];
        }
        processedData.emplace_back(
            IdPool::GetInstance().GetUint64Id(data.operatorName), addr, data.type, data.size,
            GetLocalTime(timestamp, timeRecord).Uint64(), Utils::Contact(pid, data.threadId),
            data.totalAllocateMemory, data.totalReserveMemory, stringGeId_, GetDeviceId(data.device_type));
    }
    return processedData;
}

bool NpuOpMemProcessor::Process(const std::string &fileDir)
{
    INFO("Start to process %.", fileDir);
    uint32_t pid = Context::GetInstance().GetPidFromInfoJson(Parser::Environment::HOST_ID, fileDir);
    Utils::ProfTimeRecord timeRecord;
    Utils::SyscntConversionParams params;
    DBInfo npuOpMemDB("task_memory.db", "NpuOpMemRaw");
    MAKE_SHARED0_NO_OPERATION(npuOpMemDB.database, NpuModuleMemDB);
    std::string dbPath = Utils::File::PathJoin({fileDir, HOST, SQLITE, npuOpMemDB.dbName});
    // 并不是所有场景都有NpuOpMemRaw数据
    auto status = CheckPath(dbPath);
    if (status != CHECK_SUCCESS) {
        if (status == CHECK_FAILED) {
            return false;
        }
        return true;
    }
    if (!Context::GetInstance().GetSyscntConversionParams(params, HOST_ID, fileDir)) {
        ERROR("GetSyscntConversionParams failed, profPath is %.", fileDir);
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir)) {
        ERROR("Failed to obtain the time in start_info and end_info.");
        return false;
    }
    MAKE_SHARED_RETURN_VALUE(npuOpMemDB.dbRunner, DBRunner, false, dbPath);
    auto oriData = GetData(npuOpMemDB);
    if (oriData.empty()) {
        ERROR("Get % data failed in %.", npuOpMemDB.tableName, dbPath);
        return false;
    }
    GeHashMap hashMap;
    if (!GetGeHashMap(hashMap, fileDir)) {
        ERROR("Can't get hash data.");
        return false;
    }
    auto processedData = FormatData(oriData, timeRecord, params, hashMap, pid);
    if (!SaveData(processedData, TABLE_NAME_NPU_OP_MEM)) {
        ERROR("Save data failed, %.", dbPath);
        return false;
    }
    INFO("process % ends.", fileDir);
    return true;
}

uint16_t NpuOpMemProcessor::GetDeviceId(const std::string& deviceType)
{
    std::size_t pos = deviceType.find(':');
    if (pos != std::string::npos) {
        std::string numStr = deviceType.substr(pos + 1);
        uint16_t deviceId;
        if (StrToU16(deviceId, numStr) == ANALYSIS_OK) {
            return deviceId;
        }
        ERROR("Bad input: std::invalid_argument thrown");
    }
    return UINT16_MAX; // 获取deviceId失败返回UINT16_MAX
}

}
}
}