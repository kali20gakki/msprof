/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_task_info_processor_utest.cpp
 * Description        : communication_task_info_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/1/4
 * *****************************************************************************
 */
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/communication_info_processor.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Association::Credential;
using namespace Analysis::Utils;
using namespace Analysis::Parser;
namespace {
const int DEPTH = 0;
const uint16_t OP_NUM = 4;
const uint16_t OP_NAME_NUM = 4;
const uint16_t CONNECTION_ID_NUM = 4;
const uint16_t OP_ID_NUM = 4;
const uint16_t OP_DATATYPE_NUM = 4;
const uint16_t OP_TYPE_NUM = 3;
const std::string COMMUNICATION_TASK_PATH = "./task_path";
const std::string DB_PATH = File::PathJoin({COMMUNICATION_TASK_PATH, "msprof.db"});
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "hccl_single_device.db";
const std::string PROF_PATH_A = File::PathJoin({COMMUNICATION_TASK_PATH,
                                                "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({COMMUNICATION_TASK_PATH,
                                                "./PROF_000001_20231125090304037_012333MBJNQLKJ"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TASK_TABLE_NAME = "HCCLTaskSingleDevice";
const std::string OP_TABLE_NAME = "HCCLOpSingleDevice";
const std::string KFC_TASK_TABLE_NAME = "KfcTask";
const std::string KFC_OP_TABLE_NAME = "KfcOP";

using HcclTaskSingleDeviceFormat = std::vector<std::tuple<uint32_t, int32_t, std::string, uint32_t, std::string,
                                                      std::string, double, int32_t, double, double, double,
                                                      std::string, std::string, uint64_t, int32_t, uint64_t, uint32_t,
                                                      double, uint32_t, uint32_t, std::string, uint64_t, std::string,
                                                      std::string, double, uint32_t, uint64_t, uint32_t, std::string>>;

using HcclOpSingleDeviceFormat = std::vector<std::tuple<uint32_t, std::string, std::string, std::string,
                                                        double, int32_t, int32_t, std::string, std::string,
                                                        uint64_t, std::string, uint32_t>>;
using KfcTaskFormat = std::vector<std::tuple<uint32_t, uint32_t, std::string, double, double, uint32_t, std::string,
                                             std::string, uint32_t, double, double, uint32_t, uint32_t, uint32_t,
                                             uint32_t, uint32_t, std::string, uint32_t, std::string, std::string,
                                             double, uint32_t, std::string, uint32_t, std::string>>;

using KfcOpFormat = std::vector<std::tuple<uint32_t, uint32_t, std::string, double, double,
                                           std::string, uint32_t, std::string>>;

const HcclTaskSingleDeviceFormat DATA_A{{4294967295, -1, "hcom_allReduce__360_0_1", 0, "Memcpy", "10652832407468360",
                                     78180470736653, 0, 781687236999151, 2994.875, 1, "HCCL", "hcom_allReduce_", 125,
                                     1, 11, 1, 14.1825906735751, 0, 0, "SDMA", 262144, "INVALID_TYPE",
                                     "ON_CHIP", 87.530865228098, 4294967295, 4294967296, 1, "INVALID_TYPE"},
                                    {4294967295, -1, "hcom_allReduce__360_0_1", 0, "Reduce23", "10652832407468360",
                                     78180470736653, 0, 781687236999152, 2994.875, 1, "HCCL", "hcom_allReduce_", 126,
                                     1, 11, 2, 14.1825906735751, 0, 0, "SDMA", 262144, "FP16",
                                     "HCCS", 87.530865228098, 4294967295, 8, 1, "INVALID_TYPE"}};
const HcclOpSingleDeviceFormat DATA_OP_A {{4294967295, "hcom_allReduce_", "HCCL", "hcom_allReduce_",
                                           821026362976, 0, 1, "INT16",	"HD-NB", 3021, "10652832407468360", 125}};
const KfcTaskFormat DATA_KFC_A {{4294967295, -1, "MatmulAllReduceAicpu_724_2_1", 229762691053930, 162334520, 0,
                                 "Memcpy", "7939241045454381724", 0, 229762691262190, 134380, 50, 1, 0, 0, 0,
                                 "SDMA", 0, "INT8", "INVALID_TYPE", 590.71, 4294967295, "0", 0, "INVALID_TYPE"}};
const KfcOpFormat DATA_KFC_OP_A {{4294967295, -1, "AllGatherMatmulAicpu_903_0_1", 157145504371260, 4764660,
                                  "12713090737648878903", 67270, "AllGatherMatmulAicpu"}};
const HcclTaskSingleDeviceFormat DATA_B{{4294967295, -1, "hcom_allReduce__233_0_2", 0, "Memcpy23", "10653832407468233",
                                     78180470736653, 0, 781687236999153, 2994.875, 3, "HCCL", "hcom_allReduce_", 125,
                                     1, 11, 3, 14.1825906735751, 1, 4, "SDMA", 262144, "INVALID_TYPE",
                                     "ON_CHIP", 87.530865228098, 4294967295, 4294967296, 1, "INVALID_TYPE"},
                                    {4294967295, -1, "hcom_allReduce__832_0_1", 0, "Memcpy", "10652853832407468832",
                                     78180470736653, 0, 781687236999154, 2994.875, 1, "HCCL", "hcom_allReduce_", 126,
                                     1, 21, 4, 14.1825906735751, 4, 2, "SDMA", 262144, "FP32",
                                     "HCCS", 87.530865228098, 4294967295, 8, 2, "INVALID_TYPE"}};
const HcclOpSingleDeviceFormat DATA_OP_B {{4294967295, "hcom_allReduce_", "HCCL", "hcom_allReduce_",
                                           821026362976, 1, 1, "INT32",	"HD-NHR", 4921, "10652853832407468832", 126}};
}

class CommunicationInfoProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        IdPool::GetInstance().Clear();
        if (File::Exist(COMMUNICATION_TASK_PATH)) {
            File::RemoveDir(COMMUNICATION_TASK_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(COMMUNICATION_TASK_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE})));
        CreateHcclTaskSingleDevice(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_A);
        CreateHcclTaskSingleDevice(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_B);
        CreateHcclOpSingleDevice(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_OP_A);
        CreateKfcTask(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_KFC_A);
        CreateKfcOp(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_KFC_OP_A);
        CreateHcclOpSingleDevice(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_OP_B);
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
            .stubs()
            .will(returnValue(true));
        MOCKER_CPP(&Analysis::Viewer::Database::TableProcessor::GetGeHashMap)
            .stubs()
            .will(returnValue(true));
    }
    virtual void TearDown()
    {
        IdPool::GetInstance().Clear();
        EXPECT_TRUE(File::RemoveDir(COMMUNICATION_TASK_PATH, DEPTH));
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo).reset();
    }
    static void CreateHcclTaskSingleDevice(const std::string& dbPath, HcclTaskSingleDeviceFormat data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TASK_TABLE_NAME);
        dbRunner->CreateTable(TASK_TABLE_NAME, cols);
        dbRunner->InsertData(TASK_TABLE_NAME, data);
    }
    static void CreateHcclOpSingleDevice(const std::string& dbPath, HcclOpSingleDeviceFormat data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(OP_TABLE_NAME);
        dbRunner->CreateTable(OP_TABLE_NAME, cols);
        dbRunner->InsertData(OP_TABLE_NAME, data);
    }
    static void CreateKfcTask(const std::string& dbPath, KfcTaskFormat data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(KFC_TASK_TABLE_NAME);
        dbRunner->CreateTable(KFC_TASK_TABLE_NAME, cols);
        dbRunner->InsertData(KFC_TASK_TABLE_NAME, data);
    }
    static void CreateKfcOp(const std::string& dbPath, KfcOpFormat data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(KFC_OP_TABLE_NAME);
        dbRunner->CreateTable(KFC_OP_TABLE_NAME, cols);
        dbRunner->InsertData(KFC_OP_TABLE_NAME, data);
    }
};

void CheckGlobalTaskId(CommunicationInfoProcessor::CommunicationTaskDataFormat data)
{
    const uint16_t globalTaskIdIndex = 1;
    std::vector<uint64_t> globalTaskIds;
    for (auto item : data) {
        globalTaskIds.emplace_back(std::get<globalTaskIdIndex>(item));
    }
    std::sort(globalTaskIds.begin(), globalTaskIds.end());
    for (int i = 0; i < OP_NUM; ++i) {
        EXPECT_EQ(globalTaskIds[i], i);
    }
}

void CheckStringId(CommunicationInfoProcessor::CommunicationTaskDataFormat data)
{
    const uint16_t nameIndex = 0;
    const std::set<uint64_t> nameSet = {IdPool::GetInstance().GetUint64Id("hcom_allReduce__360_0_1"),
                                        IdPool::GetInstance().GetUint64Id("hcom_allReduce__233_0_2"),
                                        IdPool::GetInstance().GetUint64Id("hcom_allReduce__832_0_1"),
                                        IdPool::GetInstance().GetUint64Id("MatmulAllReduceAicpu_724_2_1")};
    const uint16_t taskTypeIndex = 2;
    const std::set<uint64_t> taskTypeSet = {IdPool::GetInstance().GetUint64Id("Memcpy"),
                                            IdPool::GetInstance().GetUint64Id("Memcpy23"),
                                            IdPool::GetInstance().GetUint64Id("Reduce23")};
    const uint16_t rdmaTypeIndex = 6;
    const uint64_t rdmaTypeHashId = HCCL_RDMA_TYPE_TABLE.find("INVALID_TYPE")->second;
    const uint16_t srcRankIndex = 7;
    const std::set<uint64_t> srcRankSet = {0, 1, 4};
    const uint16_t dstRankIndex = 8;
    const std::set<uint64_t> dstRankSet = {0, 2, 4};
    const uint16_t transportTypeIndex = 9;
    const uint64_t transportTypeHashId = HCCL_TRANSPORT_TYPE_TABLE.find("SDMA")->second;
    const uint16_t dataTypeIndex = 11;
    const std::set<uint64_t> dataTypeSet = {HCCL_DATA_TYPE_TABLE.find("FP16")->second,
                                            HCCL_DATA_TYPE_TABLE.find("FP32")->second,
                                            HCCL_DATA_TYPE_TABLE.find("INVALID_TYPE")->second,
                                            HCCL_DATA_TYPE_TABLE.find("INT8")->second};
    const uint16_t linkTypeIndex = 12;
    const std::set<uint64_t> linkTypeSet = {HCCL_LINK_TYPE_TABLE.find("HCCS")->second,
                                            HCCL_LINK_TYPE_TABLE.find("ON_CHIP")->second,
                                            HCCL_LINK_TYPE_TABLE.find("INVALID_TYPE")->second};
    std::set<uint64_t> stringIdsSet;
    std::vector<uint64_t> stringIds;
    for (auto item : data) {
        EXPECT_NE(nameSet.find(std::get<nameIndex>(item)), nameSet.end());
        EXPECT_NE(taskTypeSet.find(std::get<taskTypeIndex>(item)), taskTypeSet.end());
        EXPECT_EQ(std::get<rdmaTypeIndex>(item), rdmaTypeHashId);
        EXPECT_NE(srcRankSet.find(std::get<srcRankIndex>(item)), srcRankSet.end());
        EXPECT_NE(dstRankSet.find(std::get<dstRankIndex>(item)), dstRankSet.end());
        EXPECT_EQ(std::get<transportTypeIndex>(item), transportTypeHashId);
        EXPECT_NE(dataTypeSet.find(std::get<dataTypeIndex>(item)), dataTypeSet.end());
        EXPECT_NE(linkTypeSet.find(std::get<linkTypeIndex>(item)), linkTypeSet.end());
    }
}

void CheckOpInfo(CommunicationInfoProcessor::CommunicationOpDataFormat data)
{
    const uint16_t opNameIndex = 0;
    const uint16_t connectionIdIndex = 3;
    const uint16_t opIdIndex = 5;
    const uint16_t opDataTypeIndex = 8;
    const uint16_t opTypeIndex = 11;
    std::set<uint64_t> opNameSet;
    std::set<uint64_t> connectionIdSet;
    std::set<uint32_t> opIdSet;
    std::set<uint64_t> opDataTypeSet;
    std::set<uint64_t> opTypeSet;
    for (auto item : data) {
        opNameSet.insert(std::get<opNameIndex>(item));
        connectionIdSet.insert(std::get<connectionIdIndex>(item));
        opIdSet.insert(std::get<opIdIndex>(item));
        opDataTypeSet.insert(std::get<opDataTypeIndex>(item));
        opTypeSet.insert(std::get<opTypeIndex>(item));
    }
    EXPECT_EQ(opNameSet.size(), OP_NAME_NUM);
    EXPECT_EQ(connectionIdSet.size(), CONNECTION_ID_NUM);
    EXPECT_EQ(opIdSet.size(), OP_ID_NUM);
    EXPECT_EQ(opDataTypeSet.size(), OP_DATATYPE_NUM);
    EXPECT_EQ(opTypeSet.size(), OP_TYPE_NUM);
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    std::shared_ptr<DBRunner> msprofDBRunner;
    CommunicationInfoProcessor::CommunicationTaskDataFormat taskResult;
    CommunicationInfoProcessor::CommunicationOpDataFormat opResult;
    std::string sql{"SELECT * FROM " + TABLE_NAME_COMMUNICATION_TASK_INFO};
    MAKE_SHARED0_NO_OPERATION(msprofDBRunner, DBRunner, DB_PATH);
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processor.Run());
    msprofDBRunner->QueryData(sql, taskResult);
    CheckGlobalTaskId(taskResult);
    CheckStringId(taskResult);
    sql = "SELECT * FROM " + TABLE_NAME_COMMUNICATION_OP;
    msprofDBRunner->QueryData(sql, opResult);
    CheckOpInfo(opResult);
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenCreateIndexFailed)
{
    MOCKER_CPP(&DBRunner::CreateIndex).stubs().will(returnValue(false));
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&DBRunner::CreateIndex).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TASK_TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TASK_TABLE_NAME);
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processor.Run());
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Utils::File::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    auto id{TableColumn("Id", "INTEGER")};
    auto name{TableColumn("Name", "INTEGER")};
    std::vector<TableColumn> cols{id, name};
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols)
    .stubs()
    .will(returnValue(cols));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t, uint64_t,
                             uint64_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t,
                             uint64_t, uint32_t>;
    MOCKER_CPP(&std::vector<TempT>::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({COMMUNICATION_TASK_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix)
    .stubs()
    .will(returnValue(deviceList));
    auto processor = CommunicationInfoProcessor(DB_PATH, {File::PathJoin({COMMUNICATION_TASK_PATH, "test"})});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}
