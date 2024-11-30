/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : db_assembler.cpp
 * Description        : DB数据组合类
 * Author             : msprof team
 * Creation Date      : 2024/8/27
 * *****************************************************************************
 */


#include "analysis/csrc/application/database/db_assembler.h"
#include <functional>
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/acc_pmu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/aicore_freq_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ddr_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hccs_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/llc_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_op_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/pcie_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/soc_bandwidth_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sys_io_data.h"


namespace Analysis {
namespace Application {
using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Parser::Environment;
using IdPool = Analysis::Association::Credential::IdPool;

namespace {
bool SaveApiData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    uint32_t pid = Context::GetInstance().GetPidFromInfoJson(Parser::Environment::HOST_ID, profPath);
    auto apiData = dataInventory.GetPtr<std::vector<ApiData>>();
    if (apiData == nullptr) {
        WARN("Api data not exist.");
        return true;
    }
    std::vector<std::tuple<uint64_t, uint64_t, uint16_t, uint64_t, uint64_t, uint64_t>> res;
    if (!Reserve(res, apiData->size())) {
        ERROR("Reserved for api data failed.");
        return false;
    }
    for (const auto& item : *apiData) {
        uint64_t name = IdPool::GetInstance().GetUint64Id(item.apiName);
        uint64_t globalTid = Utils::Contact(pid, item.threadId);
        res.emplace_back(item.start, item.end, item.level, globalTid, item.connectionId, name);
    }
    return SaveData(res, TABLE_NAME_CANN_API, msprofDB);
}

bool SaveCommOpData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // 大算子数据
    // opName, start, end, connectionId, group_name, opId, relay, retry, data_type, alg_type, count, op_type
    using CommunicationOpDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        int32_t, int32_t, int32_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    auto opData = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
    if (opData == nullptr) {
        WARN("Communication op data not exist.");
        return true;
    }
    CommunicationOpDataFormat processedOpData;
    if (!Reserve(processedOpData, opData->size())) {
        ERROR("Reserved for communication op data failed.");
        return false;
    }
    for (const auto& item : *opData) {
        uint64_t groupName = IdPool::GetInstance().GetUint64Id(item.groupName);
        uint64_t opName = IdPool::GetInstance().GetUint64Id(item.opName);
        int32_t opId = IdPool::GetInstance().GetUint32Id(item.opKey);
        uint64_t algType = IdPool::GetInstance().GetUint64Id(item.algType);
        uint64_t opType = IdPool::GetInstance().GetUint64Id(item.opType);
        processedOpData.emplace_back(opName, item.start, item.end, item.connectionId, groupName, opId, item.relay,
                                     item.retry, item.dataType, algType, item.count, opType);
    }
    return SaveData(processedOpData, TABLE_NAME_COMMUNICATION_OP, msprofDB);
}

bool SaveCommTaskData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // 小算子数据
    // name, globalTaskId, taskType, planeId, groupName, notifyId, rdmaType, srcRank, dstRank, transportType,
    // size, dataType, linkType, opId
    using CommunicationTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t,
        uint64_t, uint64_t, uint32_t, uint32_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint32_t>>;
    auto taskData = dataInventory.GetPtr<std::vector<CommunicationTaskData>>();
    if (taskData == nullptr) {
        WARN("Communication task data not exist.");
        return true;
    }
    CommunicationTaskDataFormat processedTaskData;
    if (!Reserve(processedTaskData, taskData->size())) {
        ERROR("Reserved for communication task failed.");
        return false;
    }
    uint64_t notifyId;
    for (const auto& item : *taskData) {
        uint64_t groupName = IdPool::GetInstance().GetUint64Id(item.groupName);
        uint64_t opName = IdPool::GetInstance().GetUint64Id(item.opName);
        uint64_t taskType = IdPool::GetInstance().GetUint64Id(item.taskType);
        uint64_t globalTaskId = IdPool::GetInstance().GetId(
            std::make_tuple(item.deviceId, item.streamId, item.taskId, item.contextId, item.batchId));
        uint32_t opId = IdPool::GetInstance().GetUint32Id(item.opKey);
        if (StrToU64(notifyId, item.notifyId) != ANALYSIS_OK) {
            notifyId = UINT64_MAX;
        }
        processedTaskData.emplace_back(opName, globalTaskId, taskType, item.planeId,
                                       groupName, notifyId, item.rdmaType, item.srcRank,
                                       item.dstRank, item.transportType, item.size, item.dataType,
                                       item.linkType, opId);
    }
    return SaveData(processedTaskData, TABLE_NAME_COMMUNICATION_TASK_INFO, msprofDB);
}

bool SaveCommunicationData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    bool saveFlag = SaveCommOpData(dataInventory, msprofDB, profPath);
    saveFlag = SaveCommTaskData(dataInventory, msprofDB, profPath) && saveFlag;
    return saveFlag;
}

bool SaveAccPmuData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // accId, readBwLevel, writeBwLevel, readOstLevel, writeOstLevel, timestampNs, deviceId
    using AccPmuDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint64_t, uint16_t>>;
    auto accPmuData = dataInventory.GetPtr<std::vector<AccPmuData>>();
    if (accPmuData == nullptr) {
        WARN("Acc pmu data not exist.");
        return true;
    }
    AccPmuDataFormat res;
    if (!Reserve(res, accPmuData->size())) {
        ERROR("Reserved for acc pmu data failed.");
        return false;
    }
    for (const auto& item : *accPmuData) {
        res.emplace_back(item.accId, item.readBwLevel, item.writeBwLevel, item.readOstLevel,
                         item.writeOstLevel, item.timestampNs, item.deviceId);
    }
    return SaveData(res, TABLE_NAME_ACC_PMU, msprofDB);
}

bool SaveAicoreFreqData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // deviceId, timestampNs, freq
    using AicoreFreqDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double>>;
    auto aicoreFreqData = dataInventory.GetPtr<std::vector<AicoreFreqData>>();
    if (aicoreFreqData == nullptr) {
        WARN("AIcore freq data not exist.");
        return true;
    }
    AicoreFreqDataFormat res;
    if (!Reserve(res, aicoreFreqData->size())) {
        ERROR("Reserved for AIcore freq data failed.");
        return false;
    }
    for (const auto& item : *aicoreFreqData) {
        res.emplace_back(item.deviceId, item.timestamp, item.freq);
    }
    return SaveData(res, TABLE_NAME_AICORE_FREQ, msprofDB);
}

bool SaveDDRData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // deviceId, timestamp, read, write
    using DDRDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t>>;
    auto ddrData = dataInventory.GetPtr<std::vector<DDRData>>();
    if (ddrData == nullptr) {
        WARN("DDR data not exist.");
        return true;
    }
    DDRDataFormat res;
    if (!Reserve(res, ddrData->size())) {
        ERROR("Reserved for DDR data failed.");
        return false;
    }
    for (const auto& item : *ddrData) {
        res.emplace_back(item.deviceId, item.timestamp, static_cast<uint64_t>(item.fluxRead),
                         static_cast<uint64_t>(item.fluxWrite));
    }
    return SaveData(res, TABLE_NAME_DDR, msprofDB);
}

bool SaveSingleEnumData(const std::string& tableName, DBInfo& msprofDB)
{
    // 枚举值, 枚举名
    using EnumDataFormat = std::vector<std::tuple<uint16_t, std::string>>;
    auto table = ENUM_TABLE.find(tableName);
    // Process内保证不出现表外的表名,省去end判定
    EnumDataFormat enumData;
    if (!Utils::Reserve(enumData, table->second.size())) {
        ERROR("Reserve for % data failed.", tableName);
        return false;
    }
    for (const auto& record : table->second) {
        enumData.emplace_back(record.second, record.first);
    }
    return SaveData(enumData, tableName, msprofDB);
}

bool SaveEnumData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    INFO("EnumProcessor Process.");
    bool flag = true;
    for (const auto& tableInfo : ENUM_TABLE) {
        flag = SaveSingleEnumData(tableInfo.first, msprofDB) && flag;
    }
    return flag;
}

bool SaveHbmData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // deviceId, timestamp, bandwidth, hbmId, type
    using HbmDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint8_t, uint64_t>>;
    auto hbmData = dataInventory.GetPtr<std::vector<HbmData>>();
    if (hbmData == nullptr) {
        WARN("HBM data not exist.");
        return true;
    }
    HbmDataFormat res;
    if (!Reserve(res, hbmData->size())) {
        ERROR("Reserved for HBM data failed.");
        return false;
    }
    for (const auto& item : *hbmData) {
        uint64_t type = IdPool::GetInstance().GetUint64Id(item.eventType);
        uint64_t bandwidth = static_cast<uint64_t>(item.bandWidth * BYTE_SIZE * BYTE_SIZE); // bandwidth MB/s -> B/s
        res.emplace_back(item.deviceId, item.localTime, bandwidth, item.hbmId, type);
    }
    return SaveData(res, TABLE_NAME_HBM, msprofDB);
}

bool SaveHccsData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // deviceId, timestampNs, txThroughput, rxThroughput
    using HccsDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t>>;
    auto hccsData = dataInventory.GetPtr<std::vector<HccsData>>();
    if (hccsData == nullptr) {
        WARN("HCCS data not exist.");
        return true;
    }
    HccsDataFormat res;
    if (!Reserve(res, hccsData->size())) {
        ERROR("Reserved for HCCS data failed.");
        return false;
    }
    for (const auto& item : *hccsData) {
        uint64_t txThroughput = static_cast<uint64_t>(item.txThroughput * BYTE_SIZE * BYTE_SIZE); // MB/s -> B/s
        uint64_t rxThroughput = static_cast<uint64_t>(item.rxThroughput * BYTE_SIZE * BYTE_SIZE); // MB/s -> B/s
        res.emplace_back(item.deviceId, item.localTime, txThroughput, rxThroughput);
    }
    return SaveData(res, TABLE_NAME_HCCS, msprofDB);
}

bool SaveHostInfoData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    if (profPath.empty()) {
        ERROR("Prof path is empty.");
        return false;
    }
    // hostuid, hostname
    using HostInfoDataFormat = std::vector<std::tuple<std::string, std::string>>;
    HostInfoDataFormat hostInfoData;
    std::string hostUid = Context::GetInstance().GetHostUid(Parser::Environment::HOST_ID, profPath);
    std::string hostName = Context::GetInstance().GetHostName(Parser::Environment::HOST_ID, profPath);
    hostInfoData.emplace_back(hostUid, hostName);
    if (hostInfoData.empty()) {
        // 无host目录 无数据时，避免saveData因数据为空 导致error
        INFO("Host dir not exist.");
        return true;
    }
    return SaveData(hostInfoData, TABLE_NAME_HOST_INFO, msprofDB);
}

bool SaveLlcData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // deviceId, llcID, timestamp, hitRate, throughput, mode
    using LlcDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint64_t, double, uint64_t, uint64_t>>;
    auto llcData = dataInventory.GetPtr<std::vector<LLcData>>();
    if (llcData == nullptr) {
        WARN("LLC data not exist.");
        return true;
    }
    LlcDataFormat res;
    if (!Reserve(res, llcData->size())) {
        ERROR("Reserved for LLC data failed.");
        return false;
    }
    for (const auto& item : *llcData) {
        uint64_t throughput = static_cast<uint64_t>(item.throughput * BYTE_SIZE * BYTE_SIZE);
        uint64_t mode = IdPool::GetInstance().GetUint64Id(item.mode);
        res.emplace_back(item.deviceId, item.llcID, item.localTime, item.hitRate, throughput, mode);
    }
    return SaveData(res, TABLE_NAME_LLC, msprofDB);
}

bool SaveMetaData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    using DataFormat = std::vector<std::tuple<std::string, std::string>>;
    DataFormat metaData;
    if (!Utils::Reserve(metaData, META_DATA.size())) {
        ERROR("Reserve for meta data failed.");
        return false;
    }
    for (const auto& record : META_DATA) {
        metaData.emplace_back(record.first, record.second);
    }
    return SaveData(metaData, TABLE_NAME_META_DATA, msprofDB);
}

bool SaveMsprofTxData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // startNs, endNs, eventType, rangeId, category, message, globalTid, endGlobalTid, domainId, connectionId
    using MsprofTxDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t,
        uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint16_t, uint64_t>>;
    auto msprofTxData = dataInventory.GetPtr<std::vector<MsprofTxHostData>>();
    if (msprofTxData == nullptr) {
        WARN("MsprofTx data not exist.");
        return true;
    }
    MsprofTxDataFormat res;
    if (!Reserve(res, msprofTxData->size())) {
        ERROR("Reserved for MsprofTx data failed.");
        return false;
    }
    uint32_t pid = Context::GetInstance().GetPidFromInfoJson(Parser::Environment::HOST_ID, profPath);
    for (const auto& item : *msprofTxData) {
        uint64_t message = IdPool::GetInstance().GetUint64Id(item.message);
        uint64_t globalTid = Utils::Contact(pid, item.tid);
        res.emplace_back(item.start, item.end, item.eventType, UINT32_MAX, item.category, message,
                         globalTid, globalTid, UINT16_MAX, item.connectionId);
    }
    return SaveData(res, TABLE_NAME_MSTX, msprofDB);
}

void UpdateNpuData(const std::string& profPath, const std::string& deviceDir,
                   std::vector<std::tuple<uint16_t, std::string>>& npuInfoData)
{
    uint16_t deviceId = Utils::GetDeviceIdByDevicePath(deviceDir);
    uint16_t chip = Context::GetInstance().GetPlatformVersion(deviceId, profPath);
    std::string chipName;
    auto it = CHIP_TABLE.find(chip);
    if (it == CHIP_TABLE.end()) {
        ERROR("Unknown chip type key: % in %", chip, deviceDir);
        chipName = "UNKNOWN";
    } else {
        chipName = it->second;
    }
    npuInfoData.emplace_back(deviceId, chipName);
}

bool SaveNpuInfoData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    if (profPath.empty()) {
        ERROR("Prof path is empty.");
        return false;
    }
    // gpu_chip_num, gpu_name
    using NpuInfoDataFormat = std::vector<std::tuple<uint16_t, std::string>>;
    auto deviceDirs = Utils::File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    NpuInfoDataFormat npuInfoData;
    for (const auto& deviceDir : deviceDirs) {
        UpdateNpuData(profPath, deviceDir, npuInfoData);
    }
    if (npuInfoData.empty()) {
        WARN("No device info in %.", profPath);
        return true;
    }
    return SaveData(npuInfoData, TABLE_NAME_NPU_INFO, msprofDB);
}

bool SaveNpuMemData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // type, ddr, hbm, timestamp, deviceId
    using NpuMemDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint16_t>>;
    auto npuMemData = dataInventory.GetPtr<std::vector<NpuMemData>>();
    if (npuMemData == nullptr) {
        WARN("NpuMem data not exist.");
        return true;
    }
    NpuMemDataFormat res;
    if (!Reserve(res, npuMemData->size())) {
        ERROR("Reserved for NpuMem data failed.");
        return false;
    }
    const std::string app = "app";
    const uint64_t appIndex = 0;
    const std::string device = "device";
    const uint64_t deviceIndex = 1;
    uint64_t stringAppId = IdPool::GetInstance().GetUint64Id(app);
    uint64_t stringDeviceId = IdPool::GetInstance().GetUint64Id(device);
    for (const auto& item : *npuMemData) {
        uint64_t type = UINT64_MAX;
        if (StrToU64(type, item.event) == ANALYSIS_ERROR) {
            ERROR("Converting string(event: %) to integer failed, deviceId is: %.", item.event, item.deviceId);
        }
        if (type == appIndex) {
            type = stringAppId;
        } else if (type == deviceIndex) {
            type = stringDeviceId;
        }
        res.emplace_back(type, item.ddr, item.hbm, item.localTime, item.deviceId);
    }
    return SaveData(res, TABLE_NAME_NPU_MEM, msprofDB);
}

bool SaveNpuOpMemData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // operatorName, addr, type, size, timestamp, globalTid, totalAllocate, totalReserve,  component, deviceId
    using NpuOpMemDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                                      uint64_t, uint64_t, uint64_t, uint16_t>>;
    auto npuOpMemData = dataInventory.GetPtr<std::vector<NpuOpMemData>>();
    if (npuOpMemData == nullptr) {
        WARN("NpuOpMem data not exist.");
        return true;
    }
    NpuOpMemDataFormat res;
    if (!Reserve(res, npuOpMemData->size())) {
        ERROR("Reserved for NpuOpMem data failed.");
        return false;
    }
    uint64_t stringGeId = IdPool::GetInstance().GetUint64Id("GE");
    uint32_t pid = Context::GetInstance().GetPidFromInfoJson(Parser::Environment::HOST_ID, profPath);
    uint64_t operatorNameId;
    uint64_t globalTid;
    for (const auto& item : *npuOpMemData) {
        operatorNameId = IdPool::GetInstance().GetUint64Id(item.operatorName);
        globalTid = Utils::Contact(pid, item.threadId);
        res.emplace_back(operatorNameId, item.addr, item.type, item.size, item.localTime, globalTid,
                         item.totalAllocateMemory, item.totalReserveMemory, stringGeId, item.deviceId);
    }
    return SaveData(res, TABLE_NAME_NPU_OP_MEM, msprofDB);
}

bool SavePCIeData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // deviceId, timestampNs, txPostMin, txPostMax, txPostAvg, txNonpostMin, txNonpostMax, txNonpostAvg,
    // txCplMin, txCplMax, txCplAvg, txNonpostLatencyMin, txNonpostLatencyMax, txNonpostLatencyAvg,
    // rxPostMin, rxPostMax, rxPostAvg, rxNonpostMin, rxNonpostMax, rxNonpostAvg, rxCplMin, rxCplMax, rxCplAvg
    using PCIeDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    auto pcieMemData = dataInventory.GetPtr<std::vector<PCIeData>>();
    if (pcieMemData == nullptr) {
        WARN("PCIe data not exist.");
        return true;
    }
    PCIeDataFormat res;
    if (!Reserve(res, pcieMemData->size())) {
        ERROR("Reserved for PCIe data failed.");
        return false;
    }
    for (const auto& item : *pcieMemData) {
        res.emplace_back(item.deviceId, item.timestamp,
                         item.txPost.min, item.txPost.max, item.txPost.avg,
                         item.txNonpost.min, item.txNonpost.max, item.txNonpost.avg,
                         item.txCpl.min, item.txCpl.max, item.txCpl.avg,
                         item.txNonpostLatency.min, item.txNonpostLatency.max, item.txNonpostLatency.avg,
                         item.rxPost.min, item.rxPost.max, item.rxPost.avg,
                         item.rxNonpost.min, item.rxNonpost.max, item.rxNonpost.avg,
                         item.rxCpl.min, item.rxCpl.max, item.rxCpl.avg);
    }
    return SaveData(res, TABLE_NAME_PCIE, msprofDB);
}

bool SaveSessionTimeInfoData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // startTime, endTime
    using TimeDataFormat = std::vector<std::tuple<uint64_t, uint64_t>>;
    Utils::ProfTimeRecord tempRecord;
    if (!Context::GetInstance().GetProfTimeRecordInfo(tempRecord, profPath)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", profPath);
        return false;
    }
    if (tempRecord.endTimeNs == UINT64_MAX) {
        ERROR("No end_info.json, can't get session time info.");
        return false;
    }
    TimeDataFormat timeInfoData = {std::make_tuple(tempRecord.startTimeNs, tempRecord.endTimeNs)};
    return SaveData(timeInfoData, TABLE_NAME_SESSION_TIME_INFO, msprofDB);
}

bool SaveSocData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // l2_buffer_bw_level, mata_bw_level, timestamp, deviceId
    using SocDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint16_t>>;
    auto socMemData = dataInventory.GetPtr<std::vector<SocBandwidthData>>();
    if (socMemData == nullptr) {
        WARN("Soc data not exist.");
        return true;
    }
    SocDataFormat res;
    if (!Reserve(res, socMemData->size())) {
        ERROR("Reserved for Soc data failed.");
        return false;
    }
    for (const auto& item : *socMemData) {
        res.emplace_back(item.l2_buffer_bw_level, item.mata_bw_level, item.timestamp, item.deviceId);
    }
    return SaveData(res, TABLE_NAME_SOC, msprofDB);
}

bool SaveComputeTaskInfo(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // name, globalTaskId, block_dim, mixBlockDim, taskType, opType, inputFormats, inputDataTypes, inputShapes,
    // outputFormats, outputDataTypes, outputShapes, hashid
    using ComputeTaskInfoFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, uint32_t,
                                                       uint64_t, uint64_t, uint64_t, uint64_t,
                                                       uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    auto computeTaskInfo = dataInventory.GetPtr<std::vector<TaskInfoData>>();
    if (computeTaskInfo == nullptr) {
        WARN("ComputeTaskInfo data not exist.");
        return true;
    }
    ComputeTaskInfoFormat res;
    if (!Reserve(res, computeTaskInfo->size())) {
        ERROR("Reserved for ComputeTaskInfo data failed.");
        return false;
    }
    uint64_t opName;
    uint64_t globalTaskId;
    uint64_t taskType;
    uint64_t opType;
    uint64_t inputFormats;
    uint64_t inputDataTypes;
    uint64_t inputShapes;
    uint64_t outputFormats;
    uint64_t outputDataTypes;
    uint64_t outputShapes;
    uint64_t hashId;  // 对应attrInfo
    for (const auto& item : *computeTaskInfo) {
        opName = IdPool::GetInstance().GetUint64Id(item.opName);
        globalTaskId = IdPool::GetInstance().GetId(std::make_tuple(item.deviceId, item.streamId, item.taskId,
                                                                   item.contextId, item.batchId));
        taskType = IdPool::GetInstance().GetUint64Id(item.taskType);
        opType = IdPool::GetInstance().GetUint64Id(item.opType);
        inputFormats = IdPool::GetInstance().GetUint64Id(item.inputFormats);
        inputDataTypes = IdPool::GetInstance().GetUint64Id(item.inputDataTypes);
        inputShapes = IdPool::GetInstance().GetUint64Id(item.inputShapes);
        outputFormats = IdPool::GetInstance().GetUint64Id(item.outputFormats);
        outputDataTypes = IdPool::GetInstance().GetUint64Id(item.outputDataTypes);
        outputShapes = IdPool::GetInstance().GetUint64Id(item.outputShapes);
        hashId = IdPool::GetInstance().GetUint64Id(item.hashId);
        res.emplace_back(opName, globalTaskId, item.blockDim, item.mixBlockDim, taskType, opType, inputFormats,
                         inputDataTypes, inputShapes, outputFormats, outputDataTypes, outputShapes, hashId);
    }
    return SaveData(res, TABLE_NAME_COMPUTE_TASK_INFO, msprofDB);
}

bool SaveAscendTaskData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // start, end, deviceId, connectionId, globalTaskId, globalPid, taskType, contextId, streamId, taskId,
    // modelId
    using ascendTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, int64_t, uint64_t,
                                                       uint32_t, uint32_t, uint32_t, int32_t, uint32_t, uint32_t>>;
    auto ascendTaskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    if (ascendTaskData == nullptr) {
        WARN("AscendTaskData data not exist.");
        return true;
    }
    ascendTaskDataFormat res;
    if (!Reserve(res, ascendTaskData->size())) {
        ERROR("Reserved for AscendTaskData data failed.");
        return false;
    }
    uint64_t globalTaskId;
    uint32_t globalPid = Context::GetInstance().GetPidFromInfoJson(Parser::Environment::HOST_ID, profPath);
    uint64_t taskType;
    for (const auto& item : *ascendTaskData) {
        globalTaskId = IdPool::GetInstance().GetId(std::make_tuple(item.deviceId, item.streamId, item.taskId,
                                                                   item.contextId, item.batchId));
        taskType = IdPool::GetInstance().GetUint64Id(item.taskType);
        res.emplace_back(item.start, item.end, item.deviceId, item.connectionId, globalTaskId, globalPid,
                         taskType, item.contextId, item.streamId, item.taskId, item.taskId);
    }
    return SaveData(res, TABLE_NAME_TASK, msprofDB);
}

bool SaveStringIdsData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    using OriDataFormat = std::unordered_map<std::string, uint64_t>;
    using ProcessedDataFormat = std::vector<std::tuple<uint64_t, std::string>>;
    OriDataFormat oriData = IdPool::GetInstance().GetAllUint64Ids();
    if (oriData.empty()) {
        WARN("No StringIds data.");
        return true;
    }
    ProcessedDataFormat res;
    if (!Utils::Reserve(res, oriData.size())) {
        ERROR("Reserve for stringIds data failed.");
        return false;
    }
    for (const auto& pair : oriData) {
        res.emplace_back(pair.second, pair.first);
    }
    return SaveData(res, TABLE_NAME_STRING_IDS, msprofDB);
}

template<typename T>
bool SaveSysIOData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& tableName)
{
    // deviceId, timestamp, bandwidth, rxPacketRate, rxByteRate, rxPackets, rxBytes, rxErrors, rxDropped
    // txPacketRate, txByteRate, txPackets, txBytes, txErrors, txDropped, funcId
    using SysIODataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, double, double, uint32_t,
        uint32_t, uint32_t, uint32_t, double, double, uint32_t, uint32_t, uint32_t, uint32_t, uint16_t>>;
    auto sysIOMemData = dataInventory.GetPtr<std::vector<T>>();
    if (sysIOMemData == nullptr) {
        WARN("SysIO % data not exist.", tableName);
        return true;
    }
    SysIODataFormat res;
    // sysIOMemData不为nullptr时，一定有一个元素
    auto sysIOOriginalData = sysIOMemData->back().sysIOOriginalData;
    if (!Reserve(res, sysIOOriginalData.size())) {
        ERROR("Reserved for SysIO % data failed.", tableName);
        return false;
    }
    for (const auto& item : sysIOOriginalData) {
        res.emplace_back(item.deviceId,
                         item.localTime,
                         item.bandwidth, // MB/s -> B/s
                         item.rxPacketRate, item.rxByteRate, item.rxPackets,
                         item.rxBytes, item.rxErrors, item.rxDropped,
                         item.txPacketRate, item.txByteRate, item.txPackets, item.txBytes,
                         item.txErrors, item.txDropped, item.funcId);
    }
    SaveData(res, tableName, msprofDB);
}

bool SaveNicData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    return SaveSysIOData<NicOriginalData>(dataInventory, msprofDB, TABLE_NAME_NIC);
}

bool SaveRoCEData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    return SaveSysIOData<RoceOriginalData>(dataInventory, msprofDB, TABLE_NAME_ROCE);
}

// 创建 SaveData 的函数类型
using SaveDataFunc = std::function<bool(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)>;
const std::unordered_map<std::string, SaveDataFunc> DATA_SAVER = {
    {Viewer::Database::PROCESSOR_NAME_API,               SaveApiData},
    {Viewer::Database::PROCESSOR_NAME_COMMUNICATION,     SaveCommunicationData},
    {Viewer::Database::PROCESSOR_NAME_ACC_PMU,           SaveAccPmuData},
    {Viewer::Database::PROCESSOR_NAME_AICORE_FREQ,       SaveAicoreFreqData},
    {Viewer::Database::PROCESSOR_NAME_DDR,               SaveDDRData},
    {Viewer::Database::PROCESSOR_NAME_ENUM,              SaveEnumData},
    {Viewer::Database::PROCESSOR_NAME_HBM,               SaveHbmData},
    {Viewer::Database::PROCESSOR_NAME_HOST_INFO,         SaveHostInfoData},
    {Viewer::Database::PROCESSOR_NAME_HCCS,              SaveHccsData},
    {Viewer::Database::PROCESSOR_NAME_LLC,               SaveLlcData},
    {Viewer::Database::PROCESSOR_NAME_META_DATA,         SaveMetaData},
    {Viewer::Database::PROCESSOR_NAME_MSTX,              SaveMsprofTxData},
    {Viewer::Database::PROCESSOR_NAME_NPU_INFO,          SaveNpuInfoData},
    {Viewer::Database::PROCESSOR_NAME_NPU_MEM,           SaveNpuMemData},
    {Viewer::Database::PROCESSOR_NAME_NPU_OP_MEM,        SaveNpuOpMemData},
    {Viewer::Database::PROCESSOR_NAME_PCIE,              SavePCIeData},
    {Viewer::Database::PROCESSOR_NAME_SESSION_TIME_INFO, SaveSessionTimeInfoData},
    {Viewer::Database::PROCESSOR_NAME_SOC,               SaveSocData},
    {Viewer::Database::PROCESSOR_NAME_NIC,               SaveNicData},
    {Viewer::Database::PROCESSOR_NAME_ROCE,              SaveRoCEData},
    {Viewer::Database::PROCESSOR_NAME_TASK,              SaveAscendTaskData},
    {Viewer::Database::PROCESSOR_NAME_COMPUTE_TASK_INFO, SaveComputeTaskInfo},
};
}


DBAssembler::DBAssembler(const std::string& msprofDBPath, const std::string& profPath)
    : msprofDBPath_(msprofDBPath), profPath_(profPath)
{
    MAKE_SHARED0_NO_OPERATION(msprofDB_.database, MsprofDB);
    msprofDB_.ConstructDBRunner(msprofDBPath_);
}

bool DBAssembler::Run(DataInventory& dataInventory)
{
    bool flag = true;
    for (const auto& saveFunc : DATA_SAVER) {
        INFO("Begin to save % data.", saveFunc.first);
        flag = saveFunc.second(dataInventory, msprofDB_, profPath_) && flag;
        if (!flag) {
            ERROR("Save % data failed.", saveFunc.first);
        }
    }
    flag = SaveStringIdsData(dataInventory, msprofDB_, profPath_) && flag;
    return flag;
}

}
}