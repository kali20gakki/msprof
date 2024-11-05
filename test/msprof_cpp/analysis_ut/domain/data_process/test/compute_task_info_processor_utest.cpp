/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : compute_task_info_processor_utest.cpp
 * Description        : compute_task_info_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/8/6
 * *****************************************************************************
 */
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/ai_task/compute_task_info_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using HashMap = std::unordered_map<std::string, std::string>;
namespace {
const int DEPTH = 0;
const uint16_t OP_NUM = 4;
const uint16_t STRING_NUM = 18;
const std::string COMPUTE_TASK_PATH = "./compute_task_path";
const std::string HOST_SUFFIX = "host";
const std::string DB_SUFFIX = "ge_info.db";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({COMPUTE_TASK_PATH,
                                                "PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string TABLE_NAME = "TaskInfo";

using GeInfoFormat = std::vector<std::tuple<uint32_t, std::string, int32_t, int32_t, uint32_t, uint32_t, std::string,
        std::string, std::string, int32_t, uint32_t, double, uint32_t, uint32_t,
        std::string, std::string, std::string, std::string, std::string,
        std::string, int32_t, uint32_t, std::string, std::string>>;

GeInfoFormat DATA_A{{4294967295, "aclnnMm_MatMulCommon_MatMulV2", 2, 1, 20, 40, "1", "MIX_AIC", "MatMulV2", -1,
                            3391981, 453148218443103, 0, 3, "FORMAT_ND;FORMAT_ND", "FLOAT16;FLOAT16",
                            "\"10000,10000;10000,10000\"", "FORMAT_ND", "FLOAT16", "\"1000,1000\"", 0, 0, "NO", "N/A"},
                    {4294967295, "trans_TransData_0", 2, 2, 35, 0, "1", "AI_CORE", "TransData", -1,
                            250512, 569402956566, 0, 2, "FORMAT_ND", "FLOAT",
                            "\"3072,768\"", "FRACTAL_NZ", "FLOAT", "\"48,192,16,16\"", 0, 4294967295, "NO", "N/A"}};
}

class ComputeTaskInfoProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(COMPUTE_TASK_PATH)) {
            File::RemoveDir(COMPUTE_TASK_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(COMPUTE_TASK_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE_SUFFIX})));
        CreateTaskInfo(File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_A);
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(COMPUTE_TASK_PATH, DEPTH));
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
    static void CreateTaskInfo(const std::string& dbPath, GeInfoFormat data)
    {
        std::shared_ptr<GEInfoDB> database;
        MAKE_SHARED0_RETURN_VOID(database, GEInfoDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

static void InitHashMap(DataInventory& dataInventory)
{
    HashMap hashMap;
    hashMap.emplace("111", "TEST");
    std::shared_ptr<std::unordered_map<std::string, std::string>> res;
    MAKE_SHARED_RETURN_VOID(res, HashMap, std::move(hashMap));
    dataInventory.Inject(res);
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    DataInventory dataInventory;
    InitHashMap(dataInventory);
    auto processor = ComputeTaskInfoProcessor(PROF_PATH_A);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO));
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    DataInventory dataInventory;
    InitHashMap(dataInventory);
    auto processor = ComputeTaskInfoProcessor(PROF_PATH_A);
    MOCKER_CPP(&DBRunner::CheckTableExists).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO));
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenGeHashNotExists)
{
    DataInventory dataInventory;
    auto processor = ComputeTaskInfoProcessor(PROF_PATH_A);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO));
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processor = ComputeTaskInfoProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&Analysis::Utils::File::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO));
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}
