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
    uint32_t level = 0;
    uint32_t type = 0;
    uint64_t totalAllocateMemory = 0;
    uint64_t totalReserveMemory = 0;
    int64_t size = 0;
    double timestamp = 0.0;
    std::string operatorName;
    std::string addr;
    std::string device_type;
};
const std::string COMPONENT = "GE"; // 命令行侧写死GE
}

NpuOpMemProcessor::NpuOpMemProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool NpuOpMemProcessor::Run()
{
    INFO("NpuOpMemProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_NPU_OP_MEM);
    return flag;
}

NpuOpMemProcessor::OriDataFormat NpuOpMemProcessor::GetData(DBInfo &npuOpMemDB)
{
    OriDataFormat oriData;
    std::string sql{"SELECT operator, addr, size, timestamp, thread_id, total_allocate_memory, total_reserve_memory, "
                    "level, type, device_type FROM " + npuOpMemDB.tableName};
    if (!npuOpMemDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", npuOpMemDB.tableName);
    }
    return oriData;
}

NpuOpMemProcessor::ProcessedDataFormat NpuOpMemProcessor::FormatData(const OriDataFormat &oriData,
                                                                     Utils::ProfTimeRecord &timeRecord,
                                                                     Utils::SyscntConversionParams &params,
                                                                     GeHashMap &hashMap, uint32_t profId)
{
    ProcessedDataFormat processedData;
    NpuOpMemData data;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for NpuOpMem data failed.");
        return processedData;
    }
    for (auto &row: oriData) {
        std::tie(data.operatorName, data.addr, data.size, data.timestamp, data.threadId, data.totalAllocateMemory,
                 data.totalReserveMemory, data.level, std::ignore, data.device_type) = row;
        HPFloat timestamp{GetTimeFromSyscnt(data.timestamp, params)};
        if (data.size < 0) {
            data.size = std::abs(data.size);
            data.type = MEMORY_TABLE.at("release"); // key写死在MEMORY_TABLE常量map中，不涉及外部输入，不做校验
        } else {
            data.type = MEMORY_TABLE.at("allocate");
        }
        uint64_t addr = UINT64_MAX;
        if (Utils::StrToU64(addr, data.addr) == ANALYSIS_OK) { // 转uint64失败默认值为UINT64_MAX
            WARN("Converting string(addr) to integer failed.");
        }
        processedData.emplace_back(
            IdPool::GetInstance().GetUint64Id(hashMap[data.operatorName]), addr, data.type, data.size,
            GetLocalTime(timestamp, timeRecord).Str(), Utils::Contact(profId, data.threadId),
            data.totalAllocateMemory / BYTE_SIZE, data.totalReserveMemory / BYTE_SIZE,
            IdPool::GetInstance().GetUint64Id(COMPONENT), GetDeviceId(data.device_type));
    }
    return processedData;
}

bool NpuOpMemProcessor::Process(const std::string &fileDir)
{
    INFO("Start to process %.", fileDir);
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
    auto profId = IdPool::GetInstance().GetUint32Id(fileDir);
    GeHashMap hashMap;
    if (!GetGeHashMap(hashMap, fileDir)) {
        return false; // GetGeHashMap方法内有日志输出，这里直接返回
    }
    auto processedData = FormatData(oriData, timeRecord, params, hashMap, profId);
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
        try {
            return std::stoi(numStr);
        } catch (std::invalid_argument const &e) {
            ERROR("Bad input: std::invalid_argument thrown");
        }
    }
    return UINT16_MAX; // 获取deviceId失败返回UINT16_MAX
}

}
}
}