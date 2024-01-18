/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : string_ids_processer_utest.cpp
 * Description        : string_ids_processer UT
 * Author             : msprof team
 * Creation Date      : 2023/12/11
 * *****************************************************************************
 */
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/string_ids_processer.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/utils/file.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Association::Credential;
using namespace Analysis::Utils;
namespace {
const int DEPTH = 0;
const std::string STRING_IDS_PATH = "./string_ids_path";
const std::string DB_PATH = File::PathJoin({STRING_IDS_PATH, "report.db"});
const uint16_t STRING_NUM = 2;
const std::string TARGET_TABLE_NAME = "STRING_IDS";
using PROCESSED_DATA_FORMAT = std::vector<std::tuple<uint64_t, std::string>>;
}

class StringIdsProcesserUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        IdPool::GetInstance().Clear();
        if (File::Check(STRING_IDS_PATH)) {
            File::RemoveDir(STRING_IDS_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(STRING_IDS_PATH));
        IdPool::GetInstance().GetUint64Id("Conv2d");
        IdPool::GetInstance().GetUint64Id("pool");
    }
    virtual void TearDown()
    {
        IdPool::GetInstance().Clear();
        EXPECT_TRUE(File::RemoveDir(STRING_IDS_PATH, DEPTH));
    }
};

void CheckStringId(PROCESSED_DATA_FORMAT data)
{
    const uint16_t stringIdIndex = 0;
    std::vector<uint64_t> stringIds;
    for (auto item : data) {
        stringIds.emplace_back(std::get<stringIdIndex>(item));
    }
    std::sort(stringIds.begin(), stringIds.end());
    for (int i = 0; i < STRING_NUM; ++i) {
        EXPECT_EQ(stringIds[i], i);
    }
}

TEST_F(StringIdsProcesserUTest, TestRunShouldReturnTrueWhenProcesserRunSuccess)
{
    std::shared_ptr<DBRunner> ReportDBRunner;
    PROCESSED_DATA_FORMAT result;
    MAKE_SHARED0_NO_OPERATION(ReportDBRunner, DBRunner, DB_PATH);
    auto processer = StringIdsProcesser(DB_PATH);
    IdPool::GetInstance().GetUint64Id("pool");
    EXPECT_TRUE(processer.Run());
    std::string sql{"SELECT * FROM " + TARGET_TABLE_NAME};
    ReportDBRunner->QueryData(sql, result);
    CheckStringId(result);
}

TEST_F(StringIdsProcesserUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    auto processer = StringIdsProcesser(DB_PATH);
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(StringIdsProcesserUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    auto id{TableColumn("Id", "INTEGER")};
    std::vector<TableColumn> cols{id};
    auto processer = StringIdsProcesser(DB_PATH);
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols)
    .stubs()
    .will(returnValue(cols));
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).reset();
}

TEST_F(StringIdsProcesserUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint64_t, std::string>;
    MOCKER_CPP(&std::vector<TempT>::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    auto processer = StringIdsProcesser(DB_PATH);
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}
