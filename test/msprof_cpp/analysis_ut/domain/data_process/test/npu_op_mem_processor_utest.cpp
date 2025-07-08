/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_op_mem_processor_utest.cpp
 * Description        : npu_op_mem_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/8/14
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "analysis/csrc/domain/data_process/system/npu_op_mem_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
namespace {
const int DEPTH = 0;
const std::string NPU_OP_MEM_DIR = "./npu_op_mem";
const std::string DB_SUFFIX = "task_memory.db";
const std::string PROF0 = File::PathJoin({NPU_OP_MEM_DIR, "./PROF_0"});
const std::string TABLE_NAME = "NpuOpMemRaw";
using OriDataFormat = std::vector<std::tuple<std::string, std::string, int64_t, double, uint32_t, uint64_t,
                                             uint64_t, uint32_t, uint32_t, std::string>>;
const OriDataFormat DATA_A{{"7891295173964629722", "20074680643584", 196608, 7686603804672, 2973237, 262144, 623706112,
                            10000, 6, "NPU:1"},
                           {"7891295173964629722", "20074680643584", -196608, 7686603809555, 2973237, 65536, 623706112,
                            10000, 6, "NPU:1"}};
}

class NpuOpMemProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::CreateDir(NPU_OP_MEM_DIR));
        EXPECT_TRUE(File::CreateDir(PROF0));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF0, HOST})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF0, HOST, SQLITE})));
        CreateNpuOpMem(File::PathJoin({PROF0, HOST, SQLITE, DB_SUFFIX}), DATA_A);
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"hostMonotonic", "36471130547330"},
            {"devMonotonic", "36471130547330"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(NPU_OP_MEM_DIR, DEPTH));
        GlobalMockObject::verify();
    }
    static void CreateNpuOpMem(const std::string &dbPath, OriDataFormat data)
    {
        std::shared_ptr<TaskMemoryDB> database;
        MAKE_SHARED0_RETURN_VOID(database, TaskMemoryDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(NpuOpMemProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = NpuOpMemProcessor(PROF0);
    DataInventory dataInventory;
    GeHashMap geHashMap = {{"7891295173964629722", "npu"}};
    std::shared_ptr<GeHashMap> geHashMapPtr;
    MAKE_SHARED0_NO_OPERATION(geHashMapPtr, GeHashMap, std::move(geHashMap));
    dataInventory.Inject(geHashMapPtr);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_OP_MEM));
}

TEST_F(NpuOpMemProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = NpuOpMemProcessor(PROF0);
    DataInventory dataInventory;
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_OP_MEM));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
        .stubs()
        .will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_OP_MEM));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
    MOCKER_CPP(&Context::GetSyscntConversionParams)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_OP_MEM));
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_OP_MEM));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&OriDataFormat::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_OP_MEM));
    MOCKER_CPP(&OriDataFormat::empty).reset();
    // 覆盖dataInventory里面没有GeHashMap，写日志并返回false的分支
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_OP_MEM));
}


TEST_F(NpuOpMemProcessorUTest, TestRunShouldReturnFalseWhenFormatDataFail)
{
    auto processor = NpuOpMemProcessor(PROF0);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&std::vector<NpuOpMemData>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_OP_MEM));
    MOCKER_CPP(&std::vector<NpuOpMemData>::reserve).reset();
}

TEST_F(NpuOpMemProcessorUTest, TestRunShouldReturnTrueWhenCheckPathAndTableSuccess)
{
    auto processor = NpuOpMemProcessor(PROF0);
    DataInventory dataInventory;

    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(NOT_EXIST));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_OP_MEM));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
}

TEST_F(NpuOpMemProcessorUTest, TestGetDeviceIdShouldReturnUINT16_MAXWhenGetDeviceIdFailed)
{
    auto processor = NpuOpMemProcessor(PROF0);
    EXPECT_EQ(processor.GetDeviceId(""), UINT16_MAX);
}