/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_mem_processor_utest.cpp
 * Description        : npu_mem_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/8/14
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "analysis/csrc/domain/data_process/system/npu_mem_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"

using namespace Analysis::Domain;
using namespace Parser::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
namespace {
const int DEPTH = 0;
const std::string NPU_MEM_DIR = "./npu_mem";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "npu_mem.db";
const std::string PROF0 = File::PathJoin({NPU_MEM_DIR, "./PROF_0"});
const std::string TABLE_NAME = "NpuMem";
using OriDataFormat = std::vector<std::tuple<std::string, uint64_t, uint64_t, double, uint64_t>>;
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint16_t>>;
const OriDataFormat DATA_0{{"0", 0, 47243669504, 4, 47243669504},
                           {"1", 0, 47414005760, 212, 47414005760}};
}

class NpuMemProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(NPU_MEM_DIR));
        EXPECT_TRUE(File::CreateDir(PROF0));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF0, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF0, DEVICE_SUFFIX, SQLITE})));
        CreateNpuMem(File::PathJoin({PROF0, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_0);
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
        EXPECT_TRUE(File::RemoveDir(NPU_MEM_DIR, DEPTH));
        GlobalMockObject::verify();
    }
    static void CreateNpuMem(const std::string &dbPath, OriDataFormat data)
    {
        std::shared_ptr<NpuMemDB> database;
        MAKE_SHARED0_RETURN_VOID(database, NpuMemDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(NpuMemProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = NpuMemProcessor(PROF0);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
}

TEST_F(NpuMemProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = NpuMemProcessor(PROF0);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Context::GetClockMonotonicRaw)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).reset();
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
        .stubs()
        .will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
}

TEST_F(NpuMemProcessorUTest, TestRunShouldReturnFalseWhenFormatDataFail)
{
    auto processor = NpuMemProcessor(PROF0);
    DataInventory dataInventory;
    MOCKER_CPP(&std::vector<NpuMemData>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&std::vector<NpuMemData>::reserve).reset();
}