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
#include "analysis/csrc/domain/entities/viewer_data/system//include/acc_pmu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system//include/aicore_freq_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system//include/ddr_data.h"

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

TEST_F(DBAssemblerUTest, TestRunApiDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(DB_PATH, PROF);
    std::shared_ptr<std::vector<ApiData>> dataS;
    auto data = GenerateApiData();
    MAKE_SHARED_NO_OPERATION(dataS, std::vector<ApiData>, data);
    auto dataInventory = DataInventory();
    dataInventory.Inject(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory, PROCESSOR_NAME_API));

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

TEST_F(DBAssemblerUTest, TestRunApiDataShouldReturnFalseWhenRservedFaild)
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
    EXPECT_FALSE(assembler.Run(dataInventory, PROCESSOR_NAME_API));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunHcclDataShouldReturnTrueWhenRunSuccess)
{
    auto assembler = DBAssembler(DB_PATH);
    auto dataInventory = DataInventory();
    InjectHcclData(dataInventory);
    EXPECT_TRUE(assembler.Run(dataInventory, PROCESSOR_NAME_COMMUNICATION));

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

TEST_F(DBAssemblerUTest, TestRunHcclDataShouldReturnFalseWhenRservedFaild)
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
    auto assembler = DBAssembler(DB_PATH);
    auto dataInventory = DataInventory();
    InjectHcclData(dataInventory);
    // Reserve CommunicationTaskDataFormat failed
    MOCKER_CPP(&CommunicationTaskDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory, PROCESSOR_NAME_COMMUNICATION));
    MOCKER_CPP(&CommunicationTaskDataFormat::reserve).reset();
    // Reserve CommunicationOpDataFormat failed
    MOCKER_CPP(&CommunicationOpDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory, PROCESSOR_NAME_COMMUNICATION));
    MOCKER_CPP(&CommunicationOpDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunAccPmuDataShouldReturnTrueWhenRunSuccess)
{
    // accId, readBwLevel, writeBwLevel, readOstLevel, writeOstLevel, timestampNs, deviceId
    using AccPmuDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(DB_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateAccPmuData();
    std::shared_ptr<std::vector<AccPmuData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<AccPmuData>, data);
    dataInventory.Inject<std::vector<AccPmuData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory, PROCESSOR_NAME_ACC_PMU));
}

TEST_F(DBAssemblerUTest, TestRunAccPmuDataShouldReturnFalseWhenRservedFaild)
{
    // accId, readBwLevel, writeBwLevel, readOstLevel, writeOstLevel, timestampNs, deviceId
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint64_t, uint16_t>>;
    auto assembler = DBAssembler(DB_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateAccPmuData();
    std::shared_ptr<std::vector<AccPmuData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<AccPmuData>, data);
    dataInventory.Inject<std::vector<AccPmuData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory, PROCESSOR_NAME_ACC_PMU));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunAicoreFreqDataShouldReturnTrueWhenRunSuccess)
{
    // deviceId, timestampNs, freq
    using AicoreFreqDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double>>;
    auto assembler = DBAssembler(DB_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateAicoreFreqData();
    std::shared_ptr<std::vector<AicoreFreqData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<AicoreFreqData>, data);
    dataInventory.Inject<std::vector<AicoreFreqData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory, PROCESSOR_NAME_AICORE_FREQ));
}

TEST_F(DBAssemblerUTest, TestRunAicoreFreqDataShouldReturnFalseWhenRservedFaild)
{
    // deviceId, timestampNs, freq
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double>>;
    auto assembler = DBAssembler(DB_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateAicoreFreqData();
    std::shared_ptr<std::vector<AicoreFreqData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<AicoreFreqData>, data);
    dataInventory.Inject<std::vector<AicoreFreqData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory, PROCESSOR_NAME_AICORE_FREQ));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}

TEST_F(DBAssemblerUTest, TestRunDDRDataShouldReturnTrueWhenRunSuccess)
{
    // device_id, timestamp, flux_read, flux_write
    using DDRDataFormat = std::vector<std::tuple<uint32_t, double, double, double>>;
    auto assembler = DBAssembler(DB_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateDDRData();
    std::shared_ptr<std::vector<DDRData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<DDRData>, data);
    dataInventory.Inject<std::vector<DDRData>>(dataS);
    EXPECT_TRUE(assembler.Run(dataInventory, PROCESSOR_NAME_DDR));
}

TEST_F(DBAssemblerUTest, TestRunDDRDataShouldReturnFalseWhenRservedFaild)
{
    // device_id, timestamp, flux_read, flux_write
    using SaveDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t>>;
    auto assembler = DBAssembler(DB_PATH);
    auto dataInventory = DataInventory();
    auto data = GenerateDDRData();
    std::shared_ptr<std::vector<DDRData>> dataS;
    MAKE_SHARED0_NO_OPERATION(dataS, std::vector<DDRData>, data);
    dataInventory.Inject<std::vector<DDRData>>(dataS);
    // Reserve failed
    MOCKER_CPP(&SaveDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(assembler.Run(dataInventory, PROCESSOR_NAME_DDR));
    MOCKER_CPP(&SaveDataFormat::reserve).reset();
}
