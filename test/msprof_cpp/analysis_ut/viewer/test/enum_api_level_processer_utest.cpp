/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : enum_api_level_processer.cpp
 * Description        : EnumApiLevelProcesser UT
 * Author             : msprof team
 * Creation Date      : 2024/01/10
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/enum_api_level_processer.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using EnumApiLevelDataFormat = std::vector<std::tuple<uint32_t, std::string>>;

const std::string ENUM_API_LEVEL_DIR = "./enum_api_level";
const std::string REPORT = "report.db";
const std::string DB_PATH = File::PathJoin({ENUM_API_LEVEL_DIR, REPORT});

class EnumApiLevelProcesserUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(ENUM_API_LEVEL_DIR));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(ENUM_API_LEVEL_DIR, 0));
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
    }
};

TEST_F(EnumApiLevelProcesserUTest, TestRunShouldReturnTrueWhenProcesserRunSuccess)
{
    auto processer = EnumApiLevelProcesser(DB_PATH, {""});
    EXPECT_TRUE(processer.Run());

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    EnumApiLevelDataFormat checkData;
    std::string sqlStr = "SELECT id, name FROM " + TABLE_NAME_ENUM_API_LEVEL;
    const uint32_t ID_INDEX = 0;
    const uint32_t NAME_INDEX = 1;
    const uint16_t expectNum = 7;
    if (dbRunner != nullptr) {
        EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
        EXPECT_EQ(expectNum, checkData.size());
        for (auto record : checkData) {
            EXPECT_EQ(std::get<ID_INDEX>(record), API_LEVEL_TABLE.find(std::get<NAME_INDEX>(record))->second);
        }
    }
}

TEST_F(EnumApiLevelProcesserUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint16_t, std::string>;
    MOCKER_CPP(&std::vector<TempT>::reserve).stubs().will(throws(std::bad_alloc()));
    auto processer = EnumApiLevelProcesser(DB_PATH, {""});
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}

TEST_F(EnumApiLevelProcesserUTest, TestRunShouldReturnFalseWhenMakeSharedFailedThenPtrIsNullptr)
{
    MOCKER_CPP(&std::make_shared<ReportDB>).stubs().will(throws(std::bad_alloc()));
    auto processer = EnumApiLevelProcesser(DB_PATH, {""});
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&std::make_shared<ReportDB>).reset();
}

TEST_F(EnumApiLevelProcesserUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto processer = EnumApiLevelProcesser(DB_PATH, {""});
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&DBRunner::CreateTable).reset();
}
