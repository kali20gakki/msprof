/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memcpy_info_processor_utest.cpp
 * Description        : MemcpyInfoProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/12/12
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "analysis/csrc/viewer/database/finals/memcpy_info_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;

// struct_type, level, thread_id, data_len, timestamp, data_size, memcpy_direction, max_size, connection_id
using OriginalFormat = std::vector<std::tuple<std::string, std::string, uint32_t, uint32_t, uint64_t,
                                       uint64_t, uint16_t, uint64_t, int64_t>>;
// connection_id, data_size, memcpy_direction, max_size
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t>>;
// connection_id, data_size, memcpy_direction, max_size
using LoadDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t, uint64_t>>;

const std::string MEMCPY_INFO_DIR = "./memcpy_info";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({MEMCPY_INFO_DIR, MSPROF});
const std::string PROF0 = File::PathJoin({MEMCPY_INFO_DIR, "PROF_0"});
const std::string PROF1 = File::PathJoin({MEMCPY_INFO_DIR, "PROF_1"});
const std::string PROF2 = File::PathJoin({MEMCPY_INFO_DIR, "PROF_2"});
const std::string PROF3 = File::PathJoin({MEMCPY_INFO_DIR, "PROF_3"});
const std::string TABLE_NAME = "MemcpyInfo";
const std::set<std::string> PROF_PATHS = {PROF0, PROF1, PROF2, PROF3};

const OriginalFormat MEMCPY_INFO_DATA = {
    {"memcpy_info", "runtime", 1534794, 24, 22101334093972, 8, 1, 83886080, 46},
    {"memcpy_info", "runtime", 1534794, 24, 22101334108563, 32, 0, 83886080, 53}
};
const uint16_t LEVEL_INDEX = 2;
const uint16_t TID_INDEX = 3;
const uint16_t CONNECTION_ID_INDEX = 7;

class MemcpyInfoProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(MEMCPY_INFO_DIR));
        EXPECT_TRUE(File::CreateDir(PROF0));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF0, HOST})));
        EXPECT_TRUE(CreateRuntimeDB(File::PathJoin({PROF0, HOST, SQLITE}), MEMCPY_INFO_DATA));
        EXPECT_TRUE(File::CreateDir(PROF1));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF1, HOST})));
        EXPECT_TRUE(CreateRuntimeDB(File::PathJoin({PROF1, HOST, SQLITE}), MEMCPY_INFO_DATA));
        EXPECT_TRUE(File::CreateDir(PROF2));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF2, HOST})));
        EXPECT_TRUE(CreateRuntimeDB(File::PathJoin({PROF2, HOST, SQLITE}), MEMCPY_INFO_DATA));
        EXPECT_TRUE(File::CreateDir(PROF3));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF3, HOST})));
        EXPECT_TRUE(CreateRuntimeDB(File::PathJoin({PROF3, HOST, SQLITE}), MEMCPY_INFO_DATA));
    }

    static bool CreateRuntimeDB(const std::string& sqlitePath, OriginalFormat data)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<RuntimeDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, RuntimeDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(TABLE_NAME, data));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(MEMCPY_INFO_DIR, 0));
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
    }
};

TEST_F(MemcpyInfoProcessorUTest, TestRunShouldReturnTrueWhenMemcpyInfoProcessorRunSuccess)
{
    auto processor = MemcpyInfoProcessor(DB_PATH, {PROF_PATHS});
    EXPECT_TRUE(processor.Run());
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    ProcessedDataFormat checkData;
    uint16_t expectNum = MEMCPY_INFO_DATA.size() * PROF_PATHS.size();
    std::string sqlStr = "SELECT connectionId, dataSize, memcpyDirection, maxSize FROM " + TABLE_NAME_MEMCPY_INFO;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
}


TEST_F(MemcpyInfoProcessorUTest, TestRunShouldReturnFalseWhenMemcpyInfoProcessorFormatDataFail)
{
    auto processor = MemcpyInfoProcessor(DB_PATH, {PROF0});
    MOCKER_CPP(&TableProcessor::CheckPathAndTable)
    .stubs()
    .will(returnValue(NOT_EXIST));
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&TableProcessor::CheckPathAndTable).reset();

    MOCKER_CPP(&LoadDataFormat::empty)
    .stubs()
    .will(returnValue(true));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&LoadDataFormat::empty).reset();

    MOCKER_CPP(&ProcessedDataFormat::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&ProcessedDataFormat::reserve).reset();

    MOCKER_CPP(&ProcessedDataFormat::empty)
    .stubs()
    .will(returnValue(true));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&ProcessedDataFormat::empty).reset();
}
