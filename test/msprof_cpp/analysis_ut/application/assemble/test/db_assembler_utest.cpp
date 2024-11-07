/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : db_assembler_utest.cpp
 * Description        : db_assembler UT
 * Author             : msprof team
 * Creation Date      : 2024/8/30
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/database/db_assembler.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system//include/acc_pmu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system//include/aicore_freq_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system//include/ddr_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hccs_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/llc_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/pcie_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/soc_bandwidth_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sys_io_data.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Parser::Environment;
using IdPool = Analysis::Association::Credential::IdPool;

namespace {
const int DEPTH = 0;
const std::string DATA_DIR = "./db_assembler";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({DATA_DIR, MSPROF});
const std::string PROF = File::PathJoin({DATA_DIR, "PROF"});
}

class DBAssemblerUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(DATA_DIR));
        EXPECT_TRUE(File::CreateDir(PROF));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF, DEVICE_PREFIX + "0"})));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DATA_DIR, 0));
    }

    virtual void SetUp()
    {
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1715760307197379"},
            {"endCollectionTimeEnd", "1715760313397397"},
            {"startClockMonotonicRaw", "9691377159398230"},
            {"hostMonotonic", "9691377161797070"},
            {"platform_version", "5"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"DeviceInfo", {{{"hwts_frequency", "50"}, {"aic_frequency", "1650"}}}},
            {"devCntvct", "484576969200418"},
            {"hostCntvctDiff", "0"},
            {"hostname", "localhost"},
            {"hostCntvctDiff", "0"},
            {"pid", "1"},
            {"llc_profiling", "read"},
            {"ai_core_profiling_mode", "task-based"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
        IdPool::GetInstance().Clear();
    }

    virtual void TearDown()
    {
        IdPool::GetInstance().Clear();
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
        GlobalMockObject::verify();
    }
};

static std::vector<ApiData> GenerateApiData()
{
    std::vector<ApiData> res;
    ApiData data;
    data.apiName = "launch";
    data.connectionId = 2762; // connectionId 2762
    data.id = "0";
    data.start = 1717575960208020750; // start 1717575960208020750
    data.itemId = "hcom_broadcast_";
    data.level = MSPROF_REPORT_NODE_LEVEL;
    data.threadId = 87144; // threadId 87144
    data.end = 1717575960209010750; // end 1717575960209010750
    data.structType = "launch";
    res.push_back(data);
    return res;
}

static std::vector<CommunicationOpData> GenerateOpData()
{
    std::vector<CommunicationOpData> res;
    CommunicationOpData data;
    data.opName = "hcom_broadcast__674_0_1";
    data.groupName = "16898834563344171674";
    data.connectionId = 2762; // connectionId 2762
    data.opKey = "16898834563344171674-1-1-1-1";
    data.start = 1717575960213957957; // start 1717575960213957957
    data.end = 1717575960214957957; // end 1717575960214957957
    data.relay = 0; // relay 0
    data.retry = 0; // retry 0
    data.dataType = 1; // dataType 1
    data.algType = "MESH-RING";
    data.count = 5; // count 5
    data.opType = "hcom_broadcast_";
    data.modelId = 4294967295; // modelId 4294967295
    data.deviceId = 0; // device 0
    res.push_back(data);
    return res;
}

static std::vector<CommunicationTaskData> GenerateTaskData()
{
    std::vector<CommunicationTaskData> res;
    CommunicationTaskData data;
    data.planeId = 0; // planeId 0
    data.modelId = 4294967295; // modelId 4294967295
    data.streamId = 1; // streamId 1
    data.taskId = 1; // taskId 1
    data.contextId = 1; // contextId 1
    data.batchId = 1; // batchId 1
    data.srcRank = 0; // src 0
    data.dstRank = 1; // dst 1
    data.opKey = "16898834563344171674-1-1-1-1";
    data.deviceId = 0; // device 0
    data.opName = "hcom_broadcast__674_0_1";
    data.taskType = "Notify_Wait";
    data.groupName = "16898834563344171674";
    data.transportType = 2; // transport 2
    data.size = 3200; // size 3200
    data.dataType = UINT16_MAX;
    data.linkType = UINT16_MAX;
    data.notifyId = 456; // notifyId 456
    data.rdmaType = UINT16_MAX;
    data.start = 1717575960213957957; // start 1717575960213957957
    data.duration = 1000000.0; // dur 1000000.0
    data.duration_estimated = 20.0; // es_dur 20.0
    data.bandwidth = 0.0; // bw 0.0
    res.push_back(data);
    return res;
}

static std::vector<AccPmuData> GenerateAccPmuData()
{
    std::vector<AccPmuData> res;
    AccPmuData data;
    data.accId = 128; // accID 128
    data.timestampNs = 236368325745670; // timestampNs 236368325745670
    res.push_back(data);
    return res;
}

static std::vector<AicoreFreqData> GenerateAicoreFreqData()
{
    std::vector<AicoreFreqData> res;
    AicoreFreqData data;
    data.deviceId = 1; // device_1
    data.timestamp = 236368325745670; // timestamp 236368325745670
    data.freq = 50.0; // freq 50.0
    res.push_back(data);
    return res;
}

static std::vector<DDRData> GenerateDDRData()
{
    std::vector<DDRData> res;
    DDRData data;
    data.deviceId = 2; // device_2
    data.timestamp = 236368325745670; // timestamp 236368325745670
    data.fluxRead = 100.0; // read 100.0
    data.fluxWrite = 50.0; // write 50.0
    res.push_back(data);
    return res;
}

static std::vector<HbmData> GenerateHbmData()
{
    std::vector<HbmData> res;
    HbmData data;
    data.deviceId = 0; // deviceId 0
    data.localTime = 236368325745670; // localTime 236368325745670
    data.bandWidth = 100.0; // bandWidth 100.0
    data.hbmId = 1; // hbmId 50.0
    res.push_back(data);
    return res;
}

static std::vector<HccsData> GenerateHccsData()
{
    std::vector<HccsData> res;
    HccsData data;
    data.deviceId = 2; // deviceId 2
    data.localTime = 236368325745660; // localTime 236368325745660
    data.txThroughput = 100; // txThroughput 100.0
    data.rxThroughput = 200; // rxThroughput 50.0
    res.push_back(data);
    return res;
}

static std::vector<LLcData> GenerateLlcData()
{
    std::vector<LLcData> res;
    LLcData data;
    data.deviceId = 2; // deviceId 2
    data.localTime = 236368325745555; // localTime 236368325745555
    data.llcID = 10; // llcID 10
    data.hitRate = 30.5; // hitRate 30.5
    data.throughput = 50.0; // throughput 50.0
    res.push_back(data);
    return res;
}

static std::vector<MsprofTxHostData> GenerateMsprofTxData()
{
    std::vector<MsprofTxHostData> res;
    MsprofTxHostData data;
    data.start = 236368325741111; // start 236368325741111
    data.end = 236368325742222; // end 236368325742222
    data.payloadValue = 10; // payloadValue 10
    data.category = 1962761985; // category 1962761985
    data.connectionId = UINT32_MAX; // connectionId 4294967295
    res.push_back(data);
    return res;
}

static std::vector<NpuMemData> GenerateNpuMemData()
{
    std::vector<NpuMemData> res;
    NpuMemData data;
    data.deviceId = 8; // deviceId 8
    data.event = "event"; // event "event"
    data.ddr = 10000000000; // ddr 10000000000
    data.hbm = 20000000000; // hbm 20000000000
    data.memory = 30000000000; // memory 30000000000
    data.localTime = 236368325745555; // localTime 236368325745555
    res.push_back(data);
    return res;
}

static std::vector<PCIeData> GeneratePCIeData()
{
    std::vector<PCIeData> res;
    PCIeData data;
    data.deviceId = 1; // deviceId 1
    data.timestamp = 236368325745555; // timestamp 236368325745555
    BandwidthData txPost;
    txPost.min = 0; // min 0
    txPost.avg = 67; // avg 67
    txPost.min = 104; // min 104
    data.txPost = txPost;
    res.push_back(data);
    return res;
}

static std::vector<SocBandwidthData> GenerateSocData()
{
    std::vector<SocBandwidthData> res;
    SocBandwidthData data;
    data.deviceId = 1; // deviceId 1
    data.timestamp = 236368325745555; // timestamp 236368325745555
    data.l2_buffer_bw_level = 2; // l2_buffer_bw_level 2
    data.mata_bw_level = 3; // mata_bw_level 3
    res.push_back(data);
    return res;
}

static std::vector<SysIOOriginalData> GenerateSysIOData()
{
    std::vector<SysIOOriginalData> res;
    SysIOOriginalData data;
    data.deviceId = 1; // deviceId 1
    data.localTime = 236368325745555; // timestamp 236368325745555
    data.bandwidth = 12345; // bandwidth 12345.0
    data.rxPacketRate = 1200.0; // rxPacketRate 1200.0
    data.rxByteRate = 1300.0; // rxByteRate 1300.0
    res.push_back(data);
    return res;
}

static void InjectHcclData(DataInventory& dataInventory)
{
    std::shared_ptr<std::vector<CommunicationTaskData>> dataTaskS;
    std::shared_ptr<std::vector<CommunicationOpData>> dataOpS;
    auto dataOp = GenerateOpData();
    auto dataTask = GenerateTaskData();
    MAKE_SHARED_NO_OPERATION(dataOpS, std::vector<CommunicationOpData>, dataOp);
    MAKE_SHARED_NO_OPERATION(dataTaskS, std::vector<CommunicationTaskData>, dataTask);
    dataInventory.Inject(dataOpS);
    dataInventory.Inject(dataTaskS);
}

static void CheckEnumValueByTableName(const std::shared_ptr<DBRunner> &dbRunner, const std::string &tableName,
                               const std::unordered_map<std::string, uint16_t> &enumTable)
{
    using EnumDataFormat = std::vector<std::tuple<uint32_t, std::string>>;
    EnumDataFormat checkData;
    std::string sqlStr = "SELECT id, name FROM " + tableName;
    const uint32_t ID_INDEX = 0;
    const uint32_t NAME_INDEX = 1;
    const uint16_t expectNum = enumTable.size();
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
    for (auto record : checkData) {
        EXPECT_EQ(std::get<ID_INDEX>(record), enumTable.find(std::get<NAME_INDEX>(record))->second);
    }
}

static void CheckStringId(std::vector<std::tuple<uint64_t, std::string>> data)
{
    const uint16_t stringIdIndex = 0;
    std::vector<uint64_t> stringIds;
    for (auto item : data) {
        stringIds.emplace_back(std::get<stringIdIndex>(item));
    }
    std::sort(stringIds.begin(), stringIds.end());
    for (int i = 0; i < 2; ++i) {
        EXPECT_EQ(stringIds[i], i);
    }
}

TEST_F(DBAssemblerUTest, TestRunApiDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    std::shared_ptr<std::vector<ApiData>> dataS;
    auto data = GenerateApiData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<ApiData>, data);
    auto dataInventory = DataInventory();
    dataInventory.Inject(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));

    // start, end, type, globalTid, connectionId, name
    using QueryDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t,
        uint64_t, uint64_t, uint64_t>>;
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    ASSERT_NE(dbRunner, nullptr);
    std::string sqlStr = "SELECT startNs, endNs, type, globalTid, connectionId, name FROM " + TABLE_NAME_CANN_API;
    QueryDataFormat checkData;
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(1, checkData.size());
    auto start = std::get<0>(checkData[0]);
    uint64_t expectStart = 1717575960208020750;
    EXPECT_EQ(expectStart, start);
}

TEST_F(DBAssemblerUTest, TestRunApiDataShouldReturnFalseWhenReserveFailed)
{
    // start, end, type, globalTid, connectionId, name
    using SaveDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t,
        uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    std::shared_ptr<std::vector<ApiData>> dataS;
    auto data = GenerateApiData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<ApiData>, data);
    auto dataInventory = DataInventory();
    dataInventory.Inject(dataS);

    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunHcclDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    InjectHcclData(dataInventory);
    EXPECT_TRUE(assembler.Run(dataInventory));

    uint64_t expectName = IdPool::GetInstance().GetUint64Id("hcom_broadcast__674_0_1");
    // 小算子数据
    // name, globalTaskId, taskType, planeId, groupName, notifyId, rdmaType, srcRank, dstRank, transportType,
    // size, dataType, linkType, opId
    using CommunicationTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t,
        uint64_t, uint64_t, uint32_t, uint32_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint32_t>>;
    CommunicationTaskDataFormat taskResult;
    std::string sql{"SELECT * FROM " + TABLE_NAME_COMMUNICATION_TASK_INFO};
    std::shared_ptr<DBRunner> msprofDBRunner;
    MAKE_SHARED0_NO_OPERATION(msprofDBRunner, DBRunner, DB_PATH);
    msprofDBRunner->QueryData(sql, taskResult);
    uint64_t opName = std::get<0>(taskResult[0]);
    EXPECT_EQ(expectName, opName);

    // 大算子数据
    // opName, start, end, connectionId, group_name, opId, relay, retry, data_type, alg_type, count, op_type
    using CommunicationOpDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        int32_t, int32_t, int32_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    CommunicationOpDataFormat opResult;
    sql = "SELECT * FROM " + TABLE_NAME_COMMUNICATION_OP;
    msprofDBRunner->QueryData(sql, opResult);
    opName = std::get<0>(opResult[0]);
    EXPECT_EQ(expectName, opName);
}

TEST_F(DBAssemblerUTest, TestRunHcclDataShouldReturnFalseWhenReserveFailed)
{
    // 小算子数据
    // name, globalTaskId, taskType, planeId, groupName, notifyId, rdmaType, srcRank, dstRank, transportType,
    // size, dataType, linkType, opId
    using CommunicationTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t,
        uint64_t, uint64_t, uint32_t, uint32_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint32_t>>;
    // 大算子数据
    // opName, start, end, connectionId, group_name, opId, relay, retry, data_type, alg_type, count, op_type
    using CommunicationOpDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        int32_t, int32_t, int32_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    InjectHcclData(dataInventory);
    // Reserve CommunicationTaskDataFormat failed
    MOCKER_CPP(&CommunicationTaskDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&CommunicationTaskDataFormat::reserve).reset();
    // Reserve CommunicationOpDataFormat failed
    MOCKER_CPP(&CommunicationOpDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&CommunicationOpDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunAccPmuDataShouldReturnTrueWhenRunSuccess)
{
    // accId, readBwLevel, writeBwLevel, readOstLevel, writeOstLevel, timestampNs, deviceId
    using AccPmuDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateAccPmuData();
    std::shared_ptr<std::vector<AccPmuData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<AccPmuData>, data);
    dataInventory.Inject<std::vector<AccPmuData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunAccPmuDataShouldReturnFalseWhenReserveFailed)
{
    // accId, readBwLevel, writeBwLevel, readOstLevel, writeOstLevel, timestampNs, deviceId
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateAccPmuData();
    std::shared_ptr<std::vector<AccPmuData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<AccPmuData>, data);
    dataInventory.Inject<std::vector<AccPmuData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunAicoreFreqDataShouldReturnTrueWhenRunSuccess)
{
    // deviceId, timestampNs, freq
    using AicoreFreqDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateAicoreFreqData();
    std::shared_ptr<std::vector<AicoreFreqData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<AicoreFreqData>, data);
    dataInventory.Inject<std::vector<AicoreFreqData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunAicoreFreqDataShouldReturnFalseWhenReserveFailed)
{
    // deviceId, timestampNs, freq
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateAicoreFreqData();
    std::shared_ptr<std::vector<AicoreFreqData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<AicoreFreqData>, data);
    dataInventory.Inject<std::vector<AicoreFreqData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunDDRDataShouldReturnTrueWhenRunSuccess)
{
    // device_id, timestamp, flux_read, flux_write
    using DDRDataFormat = std::vector<std::tuple<uint32_t, double, double, double>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateDDRData();
    std::shared_ptr<std::vector<DDRData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<DDRData>, data);
    dataInventory.Inject<std::vector<DDRData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunDDRDataShouldReturnFalseWhenReserveFailed)
{
    // device_id, timestamp, flux_read, flux_write
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateDDRData();
    std::shared_ptr<std::vector<DDRData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<DDRData>, data);
    dataInventory.Inject<std::vector<DDRData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunEnumDataShouldReturnTrueWhenProcessorRunSuccess)
{
    // 执行enum processor
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));

    // 校验processor生成的若干表及内容
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    ASSERT_NE(dbRunner, nullptr);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_API_TYPE, API_LEVEL_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_MODULE, MODULE_NAME_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_HCCL_DATA_TYPE, HCCL_DATA_TYPE_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_HCCL_LINK_TYPE, HCCL_LINK_TYPE_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_HCCL_TRANSPORT_TYPE, HCCL_TRANSPORT_TYPE_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_ENUM_HCCL_RDMA_TYPE, HCCL_RDMA_TYPE_TABLE);
    CheckEnumValueByTableName(dbRunner, TABLE_NAME_MSTX_EVENT_TYPE, MSTX_EVENT_TYPE_TABLE);
}

TEST_F(DBAssemblerUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using SaveDataFormat = std::tuple<uint16_t, std::string>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    MOCKER_CPP(&std::vector<SaveDataFormat>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&std::vector<SaveDataFormat>::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunHbmDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateHbmData();
    std::shared_ptr<std::vector<HbmData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<HbmData>, data);
    dataInventory.Inject<std::vector<HbmData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunHbmDataShouldReturnFalseWhenReserveFailed)
{
    // device_id, timestamp, flux_read, flux_write
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint8_t, uint64_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateHbmData();
    std::shared_ptr<std::vector<HbmData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<HbmData>, data);
    dataInventory.Inject<std::vector<HbmData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunHostInfoShouldReturnTrueWhenProcessorRunSuccess)
{
    using HostInfoDataFormat = std::vector<std::tuple<std::string, std::string>>;
    std::vector<std::string> hostDirs = {"host"};
    std::string hostUid = "123456789";
    std::string hostName = "localhost";
    MOCKER_CPP(&File::GetFilesWithPrefix).stubs().will(returnValue(hostDirs));
    MOCKER_CPP(&Context::GetHostUid).stubs().will(returnValue(hostUid));
    MOCKER_CPP(&Context::GetHostName).stubs().will(returnValue(hostName));

    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
    MOCKER_CPP(&File::GetFilesWithPrefix).reset();
    MOCKER_CPP(&Context::GetHostUid).reset();
    MOCKER_CPP(&Context::GetHostName).reset();
    std::shared_ptr<DBRunner> dbRunner;

    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    HostInfoDataFormat checkData;
    HostInfoDataFormat expectData = {
        {hostUid, hostName}
    };
    std::string sqlStr = "SELECT hostUid, hostName FROM " + TABLE_NAME_HOST_INFO;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectData, checkData);
}

TEST_F(DBAssemblerUTest, TestRunHostInfoShouldReturnTrueWhenNoHost)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
}


TEST_F(DBAssemblerUTest, TestRunHccsDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateHccsData();
    std::shared_ptr<std::vector<HccsData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<HccsData>, data);
    dataInventory.Inject<std::vector<HccsData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunHccsDataShouldReturnFalseWhenReserveFailed)
{
    // deviceId, timestampNs, txThroughput, rxThroughput
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateHccsData();
    std::shared_ptr<std::vector<HccsData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<HccsData>, data);
    dataInventory.Inject<std::vector<HccsData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}


TEST_F(DBAssemblerUTest, TestRunLLcDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateLlcData();
    std::shared_ptr<std::vector<LLcData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<LLcData>, data);
    dataInventory.Inject<std::vector<LLcData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunLLcDataShouldReturnFalseWhenReserveFailed)
{
    // deviceId, llcID, timestamp, hitRate, throughput, mode
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint64_t, double, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateLlcData();
    std::shared_ptr<std::vector<LLcData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<LLcData>, data);
    dataInventory.Inject<std::vector<LLcData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunMsprofTxDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateMsprofTxData();
    std::shared_ptr<std::vector<MsprofTxHostData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<MsprofTxHostData>, data);
    dataInventory.Inject<std::vector<MsprofTxHostData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunMsprofTxDataShouldReturnFalseWhenReserveFailed)
{
    // startNs, endNs, eventType, rangeId, category, message, globalTid, endGlobalTid, domainId, connectionId
    using SaveDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t,
        uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint16_t, uint64_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateMsprofTxData();
    std::shared_ptr<std::vector<MsprofTxHostData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<MsprofTxHostData>, data);
    dataInventory.Inject<std::vector<MsprofTxHostData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunNpuInfoDataShouldReturnTrueWhenProcessorRunSuccess)
{
    using NpuInfoDataFormat = std::vector<std::tuple<uint32_t, std::string>>;
    std::vector<std::string> deviceDirs = {"device_0", "device_1", "device_2", "device_3", "device_4", "device_5"};
    uint16_t chip0 = 0;
    uint16_t chip1 = 1;
    uint16_t chip4 = 4;
    uint16_t chip5 = 5;
    uint16_t chip7 = 7;
    uint16_t chipX = 20;
    MOCKER_CPP(&File::GetFilesWithPrefix).stubs().will(returnValue(deviceDirs));
    MOCKER_CPP(&Context::GetPlatformVersion)
        .stubs()
        .will(returnValue(chip0))
        .then(returnValue(chip1))
        .then(returnValue(chip4))
        .then(returnValue(chip5))
        .then(returnValue(chip7))
        .then(returnValue(chipX));
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
    MOCKER_CPP(&File::GetFilesWithPrefix).reset();
    MOCKER_CPP(&Context::GetPlatformVersion).reset();
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    NpuInfoDataFormat checkData;
    NpuInfoDataFormat expectData = {
        {0, "Ascend310"},
        {1, "Ascend910A"},
        {2, "Ascend310P"},
        {3, "Ascend910B"},
        {4, "Ascend310B"},
        {5, "UNKNOWN"},
    };
    std::string sqlStr = "SELECT id, name FROM " + TABLE_NAME_NPU_INFO;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectData, checkData);
}

TEST_F(DBAssemblerUTest, TestRunNpuInfoDataShouldReturnTrueWhenNoDevice)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunNpuMemDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateNpuMemData();
    std::shared_ptr<std::vector<NpuMemData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<NpuMemData>, data);
    dataInventory.Inject<std::vector<NpuMemData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunNpuMemDataShouldReturnFalseWhenReserveFailed)
{
    // type, ddr, hbm, timestamp, deviceId
    using SaveDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateNpuMemData();
    std::shared_ptr<std::vector<NpuMemData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<NpuMemData>, data);
    dataInventory.Inject<std::vector<NpuMemData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunPCIeDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GeneratePCIeData();
    std::shared_ptr<std::vector<PCIeData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<PCIeData>, data);
    dataInventory.Inject<std::vector<PCIeData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunPCIeDataShouldReturnFalseWhenReserveFailed)
{
    // deviceId, timestampNs, txPostMin, txPostMax, txPostAvg, txNonpostMin, txNonpostMax, txNonpostAvg,
    // txCplMin, txCplMax, txCplAvg, txNonpostLatencyMin, txNonpostLatencyMax, txNonpostLatencyAvg,
    // rxPostMin, rxPostMax, rxPostAvg, rxNonpostMin, rxNonpostMax, rxNonpostAvg, rxCplMin, rxCplMax, rxCplAvg
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GeneratePCIeData();
    std::shared_ptr<std::vector<PCIeData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<PCIeData>, data);
    dataInventory.Inject<std::vector<PCIeData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunSessionTimeInfoShouldReturnTrueWhenProcessorRunSuccess)
{
    using TimeDataFormat = std::vector<std::tuple<uint64_t, uint64_t>>;
    Utils::ProfTimeRecord expectRecord{1715760307197379000, 1715760313397397000, UINT64_MAX};
    auto dataInventory = DataInventory();
    auto assembler = DBAssembler(DB_PATH, PROF);
    EXPECT_TRUE(assembler.Run(dataInventory));

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    TimeDataFormat checkData;
    TimeDataFormat expectData = {{expectRecord.startTimeNs, expectRecord.endTimeNs}};
    std::string sqlStr = "SELECT startTimeNs, endTimeNs FROM " + TABLE_NAME_SESSION_TIME_INFO;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectData, checkData);
}

TEST_F(DBAssemblerUTest, TestRunSessionTimeInfoShouldReturnFalseWhenGetTimeFailed)
{
    using TimeDataFormat = std::vector<std::tuple<uint64_t, uint64_t>>;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    auto dataInventory = DataInventory();
    auto assembler = DBAssembler(DB_PATH, PROF);
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}

TEST_F(DBAssemblerUTest, TestRunSocShouldReturnTrueWhenProcessorRunSuccess)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateSocData();
    std::shared_ptr<std::vector<SocBandwidthData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<SocBandwidthData>, data);
    dataInventory.Inject<std::vector<SocBandwidthData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

TEST_F(DBAssemblerUTest, TestRunSocShouldReturnFalseWhenGetTimeFailed)
{
    // l2_buffer_bw_level, mata_bw_level, timestamp, deviceId
    using SaveDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    auto data = GenerateSocData();
    std::shared_ptr<std::vector<SocBandwidthData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<SocBandwidthData>, data);
    dataInventory.Inject<std::vector<SocBandwidthData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunStringIdsShouldReturnTrueWhenProcessorRunSuccess)
{
    using IDS_DATA_FORMAT = std::vector<std::tuple<uint64_t, std::string>>;
    IDS_DATA_FORMAT result;
    auto assembler = DBAssembler(DB_PATH, PROF);
    auto dataInventory = DataInventory();
    IdPool::GetInstance().GetUint64Id("pool");
    IdPool::GetInstance().GetUint64Id("Conv2d");
    EXPECT_TRUE(assembler.Run(dataInventory));

    std::string sql{"SELECT * FROM STRING_IDS"};
    std::shared_ptr<DBRunner> MsprofDBRunner;
    MAKE_SHARED0_NO_OPERATION(MsprofDBRunner, DBRunner, DB_PATH);
    MsprofDBRunner->QueryData(sql, result);
    CheckStringId(result);
}

TEST_F(DBAssemblerUTest, TestRunStringIdsShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint64_t, std::string>;
    using ProcessedDataFormat = std::vector<std::tuple<uint64_t, std::string>>;
    MOCKER_CPP(&ProcessedDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    IdPool::GetInstance().GetUint64Id("pool");
    IdPool::GetInstance().GetUint64Id("Conv2d");
    auto dataInventory = DataInventory();
    auto assembler = DBAssembler(DB_PATH, PROF);
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&ProcessedDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunShouldReturnTrueWhenDataIsEmpty)
{
    IdPool::GetInstance().Clear();
    auto dataInventory = DataInventory();
    auto assembler = DBAssembler(DB_PATH, PROF);
    EXPECT_TRUE(assembler.Run(dataInventory));
}

