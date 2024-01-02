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
#include "type_info_db_dumper.h"
#include "utils.h"
#include "error_code.h"

using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Viewer::Database::Drafts;
using TypeInfoData = std::unordered_map<uint16_t, std::unordered_map<uint64_t, std::string>>;
using HashData = std::unordered_map<uint64_t, std::string>;
const std::string TEST_DB_FILE_PATH = "./sqlite";
const std::string TYPE_VALUE = "123";

class TypeInfoDumperUtest : public testing::Test {
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

TEST_F(TypeInfoDumperUtest, TestTypeInfoDumperWithCompletedHashDataShouldInsertSuccessAndCanBeQueried)
{
    TypeInfoDBDumper TypeInfoDBDumper(".");
    TypeInfoData typeInfoData{
            {1, {{1, TYPE_VALUE}}}
    };
    auto res = TypeInfoDBDumper.DumpData(typeInfoData);
    EXPECT_TRUE(res);

    HashDB hashDb;
    std::string runtimeDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, hashDb.GetDBName()});
    DBRunner runtimeDBRunner(runtimeDBPath);
    std::vector<std::tuple<std::string, std::string>> data;
    runtimeDBRunner.QueryData("select * from TypeHashInfo", data);
    EXPECT_EQ(data.size(), ANALYSIS_ERROR);
}

TEST_F(TypeInfoDumperUtest, TestTypeInfoDumperShouldReturnFalseWhenDBNotCreated)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    TypeInfoDBDumper TypeInfoDBDumper(".");
    TypeInfoData typeInfoData{
            {1, {{1, TYPE_VALUE}}}
    };
    auto res = TypeInfoDBDumper.DumpData(typeInfoData);
    EXPECT_FALSE(res);
}

TEST_F(TypeInfoDumperUtest, TestTypeInfoDumperShouldReturnFalseWhenCannotInsertTable)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(true));
    TypeInfoDBDumper TypeInfoDBDumper(".");
    TypeInfoData typeInfoData{
            {1, {{1, TYPE_VALUE}}}
    };
    auto res = TypeInfoDBDumper.DumpData(typeInfoData);
    EXPECT_FALSE(res);
}