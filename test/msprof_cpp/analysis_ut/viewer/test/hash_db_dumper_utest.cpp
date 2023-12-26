/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hash_db_dumper_utest.cpp
 * Description        : HashDBDumper UT
 * Author             : msprof team
 * Creation Date      : 2023/11/18
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "hash_db_dumper.h"
#include "utils.h"
#include "error_code.h"

using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database::Drafts;
using namespace Analysis::Viewer::Database;
using HashData = std::unordered_map<uint64_t, std::string>;
const std::string TEST_DB_FILE_PATH = "./sqlite";
const uint32_t HASH_TABLE_SIZE = 4;
class HashDBDumperUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        File::CreateDir(TEST_DB_FILE_PATH);
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
        File::RemoveDir(TEST_DB_FILE_PATH, 0);
    }
};

TEST_F(HashDBDumperUtest, TestDumpHashDataWithCompletedHashDataShouldInsertSuccessAndCanBeQueried)
{
    HashDBDumper hashDbDumper(".");
    HashData hashData{{0, "a"}, {1, "b"}, {2, "c"}, {3, "d"}};
    auto res = hashDbDumper.DumpData(hashData);
    EXPECT_TRUE(res);
    HashDB hashDb;
    std::string runtimeDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, hashDb.GetDBName()});
    DBRunner runtimeDBRunner(runtimeDBPath);
    std::vector<std::tuple<std::string, std::string>> data;
    runtimeDBRunner.QueryData("select * from GeHashInfo", data);
    EXPECT_EQ(data.size(), HASH_TABLE_SIZE);
}


TEST_F(HashDBDumperUtest, TestHashDBDumperShouldReturnFalseWhenDBNotCreated)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    HashDBDumper hashDbDumper(".");
    HashData hashData{{0, "a"}, {1, "b"}, {2, "c"}, {3, "d"}};
    auto res = hashDbDumper.DumpData(hashData);
    ASSERT_FALSE(res);
}
TEST_F(HashDBDumperUtest, TestHashDBDumperShouldReturnFalseWhenCannotInsertTable)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(true));
    HashDBDumper hashDbDumper(".");
    HashData hashData{{0, "a"}, {1, "b"}, {2, "c"}, {3, "d"}};
    auto res = hashDbDumper.DumpData(hashData);
    ASSERT_FALSE(res);
}