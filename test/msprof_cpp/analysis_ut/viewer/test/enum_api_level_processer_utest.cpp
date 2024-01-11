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

const std::string ENUM_API_LEVEL_DIR = "./enum_api_level";
const std::string REPORT = "report.db";


using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;

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
        if (File::Exist(File::PathJoin({ENUM_API_LEVEL_DIR, REPORT}))) {
            EXPECT_TRUE(File::DeleteFile(File::PathJoin({ENUM_API_LEVEL_DIR, REPORT})));
        }
    }
};

TEST_F(EnumApiLevelProcesserUTest, TestRunShouldReturnTrueWhenProcesserRunSuccess)
{
    auto processer = EnumApiLevelProcesser(File::PathJoin({ENUM_API_LEVEL_DIR, REPORT}), {""});
    EXPECT_TRUE(processer.Run());
}

TEST_F(EnumApiLevelProcesserUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint16_t, std::string>;
    MOCKER_CPP(&std::vector<TempT>::reserve).stubs().will(throws(std::bad_alloc()));
    auto processer = EnumApiLevelProcesser(File::PathJoin({ENUM_API_LEVEL_DIR, REPORT}), {""});
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}

TEST_F(EnumApiLevelProcesserUTest, TestRunShouldReturnFalseWhenMakeSharedFailedThenPtrIsNullptr)
{
    MOCKER_CPP(&std::make_shared<ReportDB>).stubs().will(throws(std::bad_alloc()));
    auto processer = EnumApiLevelProcesser(File::PathJoin({ENUM_API_LEVEL_DIR, REPORT}), {""});
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&std::make_shared<ReportDB>).reset();
}

TEST_F(EnumApiLevelProcesserUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto processer = EnumApiLevelProcesser(File::PathJoin({ENUM_API_LEVEL_DIR, REPORT}), {""});
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&DBRunner::CreateTable).reset();
}
